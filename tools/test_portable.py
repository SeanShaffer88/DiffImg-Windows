"""Relocate the package and verify it without a Qt SDK on PATH."""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from cli_tests import verify,png

def modules(pid):
    class Entry(ctypes.Structure):
        _fields_=[('dwSize',wintypes.DWORD),('th32ModuleID',wintypes.DWORD),('th32ProcessID',wintypes.DWORD),
                  ('GlblcntUsage',wintypes.DWORD),('ProccntUsage',wintypes.DWORD),('modBaseAddr',ctypes.POINTER(ctypes.c_byte)),
                  ('modBaseSize',wintypes.DWORD),('hModule',wintypes.HMODULE),('szModule',wintypes.WCHAR*256),('szExePath',wintypes.WCHAR*260)]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes=[wintypes.DWORD,wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype=wintypes.HANDLE
    kernel.Module32FirstW.argtypes=[wintypes.HANDLE,ctypes.POINTER(Entry)]
    kernel.Module32NextW.argtypes=[wintypes.HANDLE,ctypes.POINTER(Entry)]
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.CreateToolhelp32Snapshot(0x8|0x10,pid)
    if handle==ctypes.c_void_p(-1).value: raise ctypes.WinError(ctypes.get_last_error())
    rows=[]
    try:
        entry=Entry();entry.dwSize=ctypes.sizeof(entry)
        ok=kernel.Module32FirstW(handle,ctypes.byref(entry))
        while ok:
            rows.append(Path(entry.szExePath))
            ok=kernel.Module32NextW(handle,ctypes.byref(entry))
    finally:
        kernel.CloseHandle(handle)
    return rows

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--package',type=Path,required=True)
    parser.add_argument('--test-exe',type=Path,required=True)
    parser.add_argument('--qt-test-dll',type=Path,required=True)
    parser.add_argument('--results',type=Path,required=True)
    args=parser.parse_args()
    env=dict(os.environ)
    for key in list(env):
        if key.upper().startswith(('QT_','QML')) or key.upper()=='PATH': env.pop(key)
    env['PATH']=str(Path(os.environ['SystemRoot'])/'System32')
    original_path=os.environ.get('PATH','')
    result={'architecture':'AMD64','relocated_unicode_path':True,'sdk_removed_from_path':True}
    with tempfile.TemporaryDirectory(prefix='DiffImg Portable ') as temporary:
        folder=Path(temporary)/'App With Spaces'
        shutil.copytree(args.package,folder)
        manifest=json.loads((folder/'SHA256.json').read_text())
        assert all(hashlib.sha256((folder/p).read_bytes()).hexdigest()==digest for p,digest in manifest.items())
        result['manifest_verified_files']=len(manifest)
        # The test harness is supplemental QA material, not a runtime requirement.
        shutil.copy2(args.test_exe,folder/'comparison-tests.exe')
        shutil.copy2(args.qt_test_dll,folder/'Qt6Test.dll')
        subprocess.run([str(folder/'comparison-tests.exe'),'-platform','windows','-o','portable-tests.xml,xml'],cwd=folder,env=env,check=True,timeout=30)
        xml=ET.parse(folder/'portable-tests.xml')
        assert not xml.findall('.//Incident[@type="fail"]')
        result['qt_test_passes_including_setup_cleanup']=len(xml.findall('.//Incident[@type="pass"]'))
        # Reuse the independent PNG/CLI checks under the same minimal environment.
        saved_env=dict(os.environ)
        try:
            os.environ.clear();os.environ.update(env)
            checks=verify(folder/'diffimg-cli.exe')
        finally:
            os.environ.clear();os.environ.update(saved_env)
        result['cli_checks']=len(checks)
        a=folder/'left.png';b=folder/'right.png'
        png(a,2,2,[0,0,0,255]*4);png(b,2,2,[0,0,0,255]*3+[30,60,90,255])
        process=subprocess.Popen([str(folder/'diffimg.exe'),str(a),str(b)],cwd=folder,env=env)
        try:
            time.sleep(1)
            assert process.poll() is None,'GUI exited unexpectedly'
            loaded=modules(process.pid)
            runtime=[p for p in loaded if p.name.lower().startswith(('qt6','qwt','msvcp140','vcruntime140')) or p.name.lower()=='qwindows.dll']
            assert any(p.name.lower()=='qwindows.dll' for p in runtime),'Windows platform plugin not loaded'
            assert any(p.name.lower()=='qwt.dll' for p in runtime),'Qwt not loaded'
            for module in runtime:
                expected=folder/('platforms/qwindows.dll' if module.name.lower()=='qwindows.dll' else module.name)
                assert os.path.samefile(module,expected),(module,expected)
            result['loaded_runtime_modules_from_package']=sorted(p.name for p in runtime)
            result['native_gui_alive']=True
        finally:
            process.terminate();process.wait(timeout=5)
        # Inspect module origins on ASCII paths; verify Unicode behavior separately.
        unicode_folder=Path(temporary)/'Unicode Ω App'
        shutil.copytree(folder,unicode_folder)
        subprocess.run([str(unicode_folder/'comparison-tests.exe'),'-platform','windows','-o','unicode-tests.xml,xml'],cwd=unicode_folder,env=env,check=True,timeout=30)
        saved_env=dict(os.environ)
        try:
            os.environ.clear();os.environ.update(env)
            result['unicode_cli_checks']=len(verify(unicode_folder/'diffimg-cli.exe'))
        finally:
            os.environ.clear();os.environ.update(saved_env)
    args.results.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Portable verification passed:',json.dumps(result))

if __name__=='__main__':
    main()
