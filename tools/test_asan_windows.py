"""Instrument DiffImg and bundled code; Qt/Qwt vendor libraries remain uninstrumented."""
import argparse
from pathlib import Path
import sys
from build_windows import environment,run,ROOT

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--qt-root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True)
    p.add_argument('--msvc-root',type=Path,required=True);p.add_argument('--sdk-root',type=Path,required=True)
    p.add_argument('--sdk-version',required=True);p.add_argument('--tools-dir',type=Path,required=True)
    a=p.parse_args();build=a.build_root.resolve();qt=a.qt_root.resolve();vc=a.msvc_root.resolve()
    if not (vc/'lib/x64/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib').exists():
        raise RuntimeError('Install the matching MSVC x64 AddressSanitizer component before this check')
    e=environment(vc,a.sdk_root.resolve(),a.sdk_version,qt,a.tools_dir.resolve(),build/'lib')
    e['ASAN_OPTIONS']='halt_on_error=1:alloc_dealloc_mismatch=1'
    target=build/'asan';target.mkdir(exist_ok=True)
    run(['cmake.exe','-S',ROOT,'-B',target,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DDIFFIMG_ASAN=ON',
         '-DCMAKE_PREFIX_PATH='+str(qt),'-DQWT_INCLUDE_DIR='+str(build/'qwt-source/src'),'-DQWT_LIBRARY='+str(build/'lib/qwt.lib')],build,e)
    run(['cmake.exe','--build',target,'--parallel','4'],build,e)
    run(['ctest.exe','--test-dir',target,'--output-on-failure'],build,e)
    run([sys.executable,ROOT/'tests/cli_tests.py',target/'diffimg-cli.exe','--output',build/'asan-cli-results.json'],build,e)
    run([sys.executable,ROOT/'tests/robustness_tests.py',target/'diffimg-cli.exe','--output',build/'asan-robustness-results.json'],build,e)

if __name__=='__main__':main()
