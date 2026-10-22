"""Independent PNG fixtures and real CLI exit-code/output integration checks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

def png(path,width,height,pixels):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+bytes(pixels[y*width*4:(y+1)*width*4]) for y in range(height))
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))

def verify(executable, output=None):
    executable=Path(executable).resolve()
    results=[]
    env=dict(os.environ,QT_QPA_PLATFORM='offscreen')
    with tempfile.TemporaryDirectory(prefix='DiffImg Unicode Ω ') as temp:
        root=Path(temp)
        a=root/'left.png'; b=root/'right.png'; mismatch=root/'size.png'; invalid=root/'invalid.png'
        png(a,2,2,[0,0,0,255]*4)
        png(b,2,2,[0,0,0,255]*3+[30,60,90,255])
        png(mismatch,1,1,[0,0,0,255]); invalid.write_bytes(b'bad PNG')
        def case(name,args,expected):
            result=subprocess.run([str(executable),*map(str,args)],env=env,cwd=root,capture_output=True,timeout=20)
            assert result.returncode==expected,(name,result.returncode,result.stdout.decode(errors='replace'),result.stderr.decode(errors='replace'))
            results.append({'name':name,'exit_code':result.returncode,'passed':True})
            return result
        base=['--batch']
        before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (a,b)}
        case('identical',base+[a,a],0)
        stats=case('strict difference',base+[a,b],1)
        assert b'MeanError = 15' in stats.stdout,stats.stdout
        case('threshold boundary',base+['--threshold','ErrorNum=1',a,b],0)
        case('threshold exceeded',base+['--threshold','ErrorNum=0',a,b],1)
        case('multiple thresholds all pass',base+['--threshold','ErrorNum=1','--threshold','MaxError=90',a,b],0)
        case('multiple thresholds one fails',base+['--threshold','ErrorNum=1','--threshold','MaxError=89',a,b],1)
        case('size mismatch',base+[a,mismatch],2)
        case('invalid image',base+[a,invalid],2)
        case('missing input',base+[a,root/'missing.png'],2)
        case('unknown metric',base+['--metric','missing',a,b],2)
        for option in ('--metric','--in','--threshold','--output'):
            case('missing option value '+option,base+[option],2)
        for value in ('Unknown=1','ErrorNum=-1','ErrorNum=0.5','ErrorNum=NaN','ErrorNum=inf','ErrorNum=2147483648'):
            case('invalid threshold '+value,base+['--threshold',value,a,b],2)
        case('invalid gamma',base+['--metric','PerceptualMetric','--in','Gamma=0',a,b],2)
        case('unsupported downsample',base+['--metric','PerceptualMetric','--in','Downsample=1',a,b],2)
        case('unknown option',base+['--unknown',a,b],2)
        case('new diff output',base+['--output',root/'diff.png',a,b],1)
        assert (root/'diff.png').is_file()
        case('identical output is written',base+['--output',root/'zero.png',a,a],0)
        assert (root/'zero.png').is_file()
        case('existing output refused',base+['--output',root/'diff.png',a,b],2)
        case('input overwrite refused',base+['--output',a,a,b],2)
        case('output failure',base+['--output',root/'absent'/'out.png',a,b],2)
        case('help', ['--help'],0)
        case('metric list', ['--list'],0)
        for p in (a,b):
            assert hashlib.sha256(p.read_bytes()).hexdigest()==before[p.name]
        results.append({'name':'inputs unchanged','passed':True})
    if output:
        Path(output).write_text(json.dumps(results,indent=2),encoding='utf-8')
    print(f'PASS: {len(results)} CLI checks')
    return results

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('executable')
    parser.add_argument('--output')
    args=parser.parse_args()
    verify(args.executable,args.output)
