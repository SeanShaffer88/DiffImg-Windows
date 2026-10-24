"""Build and test the pinned Windows x64 Release baseline (Python standard library)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

QWT_COMMIT='4ddf2f3878d0835f678da0f2aba1180f2ba208f6'
ROOT=Path(__file__).resolve().parents[1]

def run(args,cwd,env,log=None):
    args=list(map(str,args))
    args[0]=shutil.which(args[0],path=env.get('PATH')) or args[0]
    print('Running:',Path(args[0]).name,*args[1:],flush=True)
    result=subprocess.run(args,cwd=cwd,env=env,stdout=log,stderr=subprocess.STDOUT if log else None)
    if result.returncode:
        raise RuntimeError(f'{Path(args[0]).name} failed with exit {result.returncode}; see build log if supplied.')

def environment(vc,sdk,sdk_version,qt,tools,qwt_lib):
    env={k.upper():v for k,v in os.environ.items()}
    includes=[vc/'include',sdk/'Include'/sdk_version/'ucrt',sdk/'Include'/sdk_version/'shared',sdk/'Include'/sdk_version/'um',sdk/'Include'/sdk_version/'winrt']
    libs=[vc/'lib/x64',sdk/'Lib'/sdk_version/'ucrt/x64',sdk/'Lib'/sdk_version/'um/x64']
    for p in includes+libs:
        if not p.is_dir(): raise RuntimeError(f'Toolchain directory not found: {p}')
    env['INCLUDE']=';'.join(map(str,includes))
    env['LIB']=';'.join(map(str,libs))
    env['PATH']=';'.join(map(str,[qt/'bin',qwt_lib,tools,vc/'bin/Hostx64/x64',sdk/'bin'/sdk_version/'x64']))+';'+env['PATH']
    env['VCTOOLSINSTALLDIR']=str(vc)+'\\'
    env['WINDOWSSDKDIR']=str(sdk)+'\\'
    env['WINDOWSSDKVERSION']=sdk_version+'\\'
    env['QT_QPA_PLATFORM']='windows'
    # Never inherit a different Qt installation's plugin search path.
    env.pop('QT_PLUGIN_PATH',None)
    env.pop('QT_QPA_PLATFORM_PLUGIN_PATH',None)
    return env

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qt-root',type=Path,required=True)
    parser.add_argument('--qwt-source',type=Path,required=True)
    parser.add_argument('--msvc-root',type=Path,required=True,help='VC/Tools/MSVC/<version> directory')
    parser.add_argument('--sdk-root',type=Path,default=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Windows Kits/10')
    parser.add_argument('--sdk-version',default='10.0.26100.0')
    parser.add_argument('--tools-dir',type=Path,default=Path(sys.executable).parent,help='Directory containing cmake.exe and ninja.exe')
    parser.add_argument('--build-root',type=Path,default=ROOT/'build-windows')
    args=parser.parse_args()
    qt=args.qt_root.resolve(); source=args.qwt_source.resolve(); build=args.build_root.resolve()
    if subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()!=QWT_COMMIT:
        raise RuntimeError('Qwt source must be exactly the pinned v6.3.0 commit.')
    if subprocess.run(['git','diff','--quiet','HEAD','--'],cwd=source).returncode:
        raise RuntimeError('Qwt source has local modifications; supply a clean pinned checkout.')
    build.mkdir(parents=True,exist_ok=True)
    qwt_copy=build/'qwt-source'
    if not qwt_copy.exists():
        shutil.copytree(source,qwt_copy,ignore=shutil.ignore_patterns('.git'))
    config=(source/'qwtconfig.pri').read_text()
    for feature in ('QwtSvg','QwtOpenGL','QwtDesigner','QwtExamples','QwtPlayground','QwtTests'):
        import re
        config=re.sub(r'^QWT_CONFIG\s*\+=\s*'+feature+r'\s*$', '# Disabled by DiffImg minimal build: '+feature,config,flags=re.M)
    (qwt_copy/'qwtconfig.pri').write_text(config)
    qwt_build=build/'qwt-build'; qwt_build.mkdir(exist_ok=True)
    app_build=build/'app'; app_build.mkdir(exist_ok=True)
    env=environment(args.msvc_root.resolve(),args.sdk_root.resolve(),args.sdk_version,qt,args.tools_dir.resolve(),build/'lib')
    version=subprocess.check_output([str(qt/'bin/qmake.exe'),'-query','QT_VERSION'],env=env,text=True).strip()
    if version!='6.12.0': raise RuntimeError('This baseline requires Qt 6.12.0, MSVC x64 shared binaries.')
    with (build/'qwt-build.log').open('w',encoding='utf-8') as log:
        run([qt/'bin/qmake.exe',qwt_copy/'src/src.pro','-after','CONFIG-=debug_and_release','CONFIG-=build_all','CONFIG+=release','QMAKE_CXXFLAGS+=/Brepro /MP8','QMAKE_LFLAGS+=/Brepro'],qwt_build,env,log)
        run(['nmake.exe','/nologo'],qwt_build,env,log)
    with (build/'app-build.log').open('w',encoding='utf-8') as log:
        run(['cmake.exe','-S',ROOT,'-B',app_build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
             '-DCMAKE_PREFIX_PATH='+str(qt),'-DQWT_INCLUDE_DIR='+str(qwt_copy/'src'),'-DQWT_LIBRARY='+str(build/'lib/qwt.lib')],build,env,log)
        run(['cmake.exe','--build',app_build,'--parallel','8'],build,env,log)
    run(['ctest.exe','--test-dir',app_build,'--output-on-failure'],build,env)
    run([sys.executable,ROOT/'tests/cli_tests.py',app_build/'diffimg-cli.exe','--output',build/'cli-results.json'],build,env)
    run([sys.executable,ROOT/'tests/robustness_tests.py',app_build/'diffimg-cli.exe','--output',build/'robustness-results.json'],build,env)
    run([sys.executable,ROOT/'tests/folder_tests.py',app_build/'diffimg-cli.exe','--output',build/'folder-results.json'],build,env)
    run([app_build/'comparison-tests.exe','-platform','windows','guiComparisonSmoke','-o',build/'native-gui-results.xml,xml'],app_build,env)
    manifests={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (app_build/'diffimg.exe',app_build/'diffimg-cli.exe',build/'lib/qwt.dll')}
    (build/'binary-hashes.json').write_text(json.dumps(manifests,indent=2)+'\n')
    print('Build and tests passed. SHA256:',json.dumps(manifests))

if __name__=='__main__':
    main()
