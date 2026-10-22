"""Download the hash-pinned official Qt SDK and corresponding source.

Requires py7zr (also installed by aqtinstall 3.3.0). No global installation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request
import py7zr

ROOT=Path(__file__).resolve().parents[1]

def download(url,path,digest):
    if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        with urllib.request.urlopen(url,timeout=120) as response:
            data=response.read()
        if hashlib.sha256(data).hexdigest()!=digest:
            raise RuntimeError('Upstream archive hash mismatch: '+path.name)
        path.write_bytes(data)
    print('Verified:',path.name,flush=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())['qt']
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    cache=output/'archives';cache.mkdir(exist_ok=True)
    qt=output/lock['version']/'msvc2022_64'
    if qt.exists():raise RuntimeError('SDK output already exists; use a fresh output directory')
    qt.mkdir(parents=True)
    for name,digest in lock['archives'].items():
        path=cache/name
        url=lock['repository']+lock['package']+'/'+lock['package_version']+name
        download(url,path,digest)
        with py7zr.SevenZipFile(path) as archive:
            names=archive.getnames()
            if any(Path(n).is_absolute() or '..' in Path(n).parts for n in names):
                raise RuntimeError('Unsafe SDK archive path')
            archive.extractall(qt)
    # Qt relocatable tools honor qt.conf; avoid modifying vendor executable bytes.
    (qt/'bin/qt.conf').write_text('[Paths]\nPrefix=..\n',encoding='ascii')
    source=output/Path(lock['source_url']).name
    download(lock['source_url'],source,lock['source_sha256'])
    print('SDK:',qt)

if __name__=='__main__':main()
