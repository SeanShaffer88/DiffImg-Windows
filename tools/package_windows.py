"""Create an app-local portable folder; keep developer paths out of its manifest."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
NAME='DiffImg-2.6.0-win-x64'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-root',type=Path,required=True)
    parser.add_argument('--qt-root',type=Path,required=True)
    parser.add_argument('--crt-root',type=Path,required=True,help='Microsoft.VC143.CRT x64 directory')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--qt-source',type=Path,required=True,help='Verified Qtbase 6.12.0 source tar.xz')
    parser.add_argument("--sign-thumbprint",help="Optional existing Windows user-store code-signing certificate; private keys are never exported")
    args=parser.parse_args()
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    if hashlib.sha256(args.qt_source.read_bytes()).hexdigest()!=lock['qt']['source_sha256']:
        raise RuntimeError('Qt source does not match dependencies.lock.json')
    build=args.build_root.resolve();qt=args.qt_root.resolve();output=args.output.resolve()
    package=output/NAME
    package.mkdir(parents=True,exist_ok=False)
    for executable in ('diffimg.exe','diffimg-cli.exe'):
        shutil.copy2(build/'app'/executable,package/executable)
    shutil.copy2(build/'lib/qwt.dll',package/'qwt.dll')
    env=dict(os.environ)
    env['PATH']=str(qt/'bin')+os.pathsep+env.get('PATH','')
    # windeployqt misses QtPrintSupport because it is imported by Qwt; request it explicitly.
    subprocess.run([str(qt/'bin/windeployqt.exe'),'--release','--no-translations',
                    '--no-opengl-sw','--no-compiler-runtime','--printsupport',str(package/'diffimg.exe')],env=env,check=True)
    # Qtbase-only codecs keep source/license scope explicit. TIFF/WebP/etc are not shipped.
    for plugin in (package/'imageformats').glob('*.dll'):
        if plugin.name.lower() not in {'qjpeg.dll','qgif.dll','qico.dll'}:
            plugin.unlink()
    shutil.copy2(qt/'plugins/platforms/qoffscreen.dll',package/'platforms/qoffscreen.dll')
    for file in args.crt_root.glob('*.dll'):
        shutil.copy2(file,package/file.name)
    (package/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n',encoding='ascii')
    notices=package/'licenses';notices.mkdir()
    for file in ('LICENSE.txt','COPYING.txt','AUTHORS.txt','CREDITS.txt'):
        shutil.copy2(ROOT/file,notices/('DiffImg-'+file))
    shutil.copy2(build/'qwt-source/COPYING',notices/'Qwt-COPYING.txt')
    # Source archive contains Qt and third-party notices without copying unrelated installed files.
    import tarfile
    with tarfile.open(args.qt_source,'r:xz') as archive:
        for member in archive.getmembers():
            leaf=Path(member.name).name
            if member.isfile() and '/LICENSES/' in member.name:
                destination=notices/'Qt-LICENSES'/leaf
                destination.parent.mkdir(exist_ok=True)
                destination.write_bytes(archive.extractfile(member).read())
            if member.isfile() and leaf in {'LICENSE.GPL3','LICENSE.GPL2','LICENSE.LGPL3','LGPL_EXCEPTION.txt','LICENSE.FDL'} and member.name.count('/')==1:
                (notices/('Qt-'+leaf+'.txt')).write_bytes(archive.extractfile(member).read())
        for name in ('src/3rdparty/libpng/LICENSE','src/3rdparty/libjpeg/LICENSE.md','src/3rdparty/zlib/LICENSE',
                     'src/3rdparty/harfbuzz-ng/COPYING','src/3rdparty/freetype/LICENSE.TXT','src/3rdparty/double-conversion/LICENSE'):
            matches=[m for m in archive.getmembers() if m.name.endswith('/'+name) and m.isfile()]
            if matches:
                (notices/('Qt-thirdparty-'+name.split('/')[2]+'.txt')).write_bytes(archive.extractfile(matches[0]).read())
        for member in archive.getmembers():
            if member.isfile() and '/src/3rdparty/' in member.name and Path(member.name).name.upper().startswith(('LICENSE','COPYING','FTL.','GPL.','LGPL.','NOTICE')):
                relative=Path(member.name.split('/src/3rdparty/',1)[1])
                if '..' in relative.parts or relative.is_absolute():
                    raise RuntimeError('Unsafe source notice path')
                target=notices/'Qt-thirdparty'/relative
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(archive.extractfile(member).read())
    if not (notices/'Qt-LICENSES/LGPL-3.0-only.txt').exists():
        raise RuntimeError('Required Qt license texts missing')
    # Existing source files hold the full BSD notices for bundled QtPropertyBrowser and qimageblitz-derived code.
    for src,dst in [('3rdparty/qtpropertybrowser/qtpropertybrowser.cpp','QtPropertyBrowser-notice.txt'),
                    ('src/metrics/BaseMetric.cpp','qimageblitz-derived-notice.txt'),
                    ('3rdparty/perceptualdiff/Metric.cpp','PerceptualDiff-notice.txt')]:
        text=(ROOT/src).read_text(encoding='utf-8')
        # Preserve the whole source as a notice copy so attribution isn't truncated accidentally.
        (notices/dst).write_text(text,encoding='utf-8')
    for source,target in [('README-WINDOWS.md','README.txt'),('THIRD-PARTY.md','THIRD-PARTY.txt')]:
        shutil.copy2(ROOT/source,package/target)
    shutil.copy2(ROOT/'VALIDATION.md',package/'VALIDATION.md')
    shutil.copy2(ROOT/'dependencies.lock.json',package/'dependencies.lock.json')
    import sys
    sys.path.insert(0,str(ROOT/'tests'))
    from cli_tests import png
    examples=package/'examples';examples.mkdir()
    left=[220,220,220,255]*(400*300)
    right=left.copy()
    for y in range(100,180):
        for x in range(130,250):
            at=(y*400+x)*4
            right[at:at+4]=[40,90,180,255]
    png(examples/'baseline.png',400,300,left)
    png(examples/'current.png',400,300,right)
    for label,pixels in [('baseline',left),('current',right)]:
        directory=examples/label;directory.mkdir()
        png(directory/'screen.png',400,300,pixels)
        png(directory/'unchanged.png',400,300,left)
    (examples/'default-profile.json').write_text(json.dumps({'schema':1,'name':'Screenshot regression','metric':'PerChannelMetric','allowedPixels':0,'maxError':255,'meanError':255,'parameters':{},'masks':{}},indent=2)+'\n')
    (package/'Try project review.bat').write_text('@echo off\r\ncd /d "%~dp0"\r\nstart "" "%~dp0diffimg.exe" --project-review "%~dp0examples\\baseline" "%~dp0examples\\current"\r\n',encoding='ascii')
    (package/'Try sample.bat').write_text('@echo off\r\ncd /d "%~dp0"\r\nstart "" "%~dp0diffimg.exe" --portable "%~dp0examples\\baseline.png" "%~dp0examples\\current.png"\r\n',encoding='ascii')
    (package/'Start DiffImg.bat').write_text('@echo off\r\ncd /d "%~dp0"\r\nstart "" "%~dp0diffimg.exe" --portable %*\r\n',encoding='ascii')
    if args.sign_thumbprint:
        from sign_windows import sign
        sign(package,args.sign_thumbprint)
    manifest={p.relative_to(package).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(package.rglob('*')) if p.is_file()}
    (package/'SHA256.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('Portable folder prepared:',package)
    print('Run relocated checks before archiving or publishing.')

if __name__=='__main__':
    main()
