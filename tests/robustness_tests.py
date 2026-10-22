"""Bounded deterministic malformed-image smoke corpus; not exhaustive fuzzing."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
from cli_tests import png

def verify(executable,output=None):
    executable=Path(executable).resolve();randomizer=random.Random(0xD1FF2026)
    env=dict(os.environ,QT_QPA_PLATFORM='offscreen')
    results=[]
    with tempfile.TemporaryDirectory(prefix='DiffImg Corpus ') as temporary:
        root=Path(temporary);baseline=root/'baseline.png'
        png(baseline,7,5,[12,34,56,255]*35)
        original=baseline.read_bytes()
        samples=[('empty.png',b'',True),('random.png',randomizer.randbytes(512),True)]
        for cut in [1,7,8,16,24,32,40,len(original)//2]:
            samples.append((f'truncated-{cut}.png',original[:cut],True))
        for suffix,magic in [('bmp',b'BM'),('jpg',b'\xff\xd8\xff'),('gif',b'GIF89a'),('ico',b'\0\0\1\0')]:
            for length in [0,8,32,128]:samples.append((f'invalid-{length}.{suffix}',magic+randomizer.randbytes(length),True))
        for i in range(100):
            data=bytearray(original)
            for _ in range(1+randomizer.randrange(6)):
                at=randomizer.randrange(len(data));data[at]^=1<<randomizer.randrange(8)
            samples.append((f'mutation-{i}.png',bytes(data),False))
        for name,data,must_reject in samples:
            sample=root/name;sample.write_bytes(data)
            digest=hashlib.sha256(data).hexdigest()
            result=subprocess.run([str(executable),'--batch',str(baseline),str(sample)],cwd=root,env=env,capture_output=True,timeout=10)
            allowed={2} if must_reject else {0,1,2}
            assert result.returncode in allowed,(name,result.returncode,result.stderr.decode(errors='replace'))
            assert hashlib.sha256(sample.read_bytes()).hexdigest()==digest
            results.append({'sample':name,'sha256':digest,'exit_code':result.returncode})
        assert baseline.read_bytes()==original
    report={'seed':'0xD1FF2026','cases':len(results),'results':results,'scope':'bounded parser robustness smoke tests; no exhaustive fuzz/security claim'}
    if output:Path(output).write_text(json.dumps(report,indent=2)+'\n')
    print('PASS:',len(results),'malformed/mutated image cases')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('executable');p.add_argument('--output');a=p.parse_args();verify(a.executable,a.output)
