"""Independent PNG/CLI integration cases for project review (no approval writes in CI mode)."""
import argparse,json,os,subprocess,tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from cli_tests import png

def verify(executable,output=None):
    executable=Path(executable).resolve();results=[];env=dict(os.environ,QT_QPA_PLATFORM='offscreen')
    with tempfile.TemporaryDirectory(prefix='DiffImg Folder Ω ') as temporary:
        root=Path(temporary);a=root/'baseline';b=root/'current';a.mkdir();b.mkdir()
        for folder in (a,b):png(folder/'same.png',2,2,[0,0,0,255]*4)
        png(a/'changed.png',2,2,[0,0,0,255]*4);png(b/'changed.png',2,2,[255,255,255,255]*4)
        def case(name,args,expected):
            p=subprocess.run([str(executable),*map(str,args)],cwd=root,env=env,capture_output=True,timeout=20)
            assert p.returncode==expected,(name,p.returncode,p.stderr.decode(errors='replace'))
            results.append({'name':name,'exit_code':p.returncode});return p
        base=['--folder-batch','--baseline',a,'--current',b]
        case('strict changed folders',base,1)
        report=root/'report.html';case('standalone HTML',base+['--report',report],1)
        text=report.read_text(encoding='utf-8');assert 'data:image/png;base64,' in text and 'changed.png' in text
        jr=root/'results.json'; xr=root/'results.xml'
        case('structured reports',base+['--json-report',jr,'--junit-report',xr],1)
        data=json.loads(jr.read_text());assert data['tests']==2 and data['failures']==1 and data['errors']==0
        suite=ET.parse(xr).getroot();assert suite.attrib['failures']=='1' and len(suite.findall('testcase'))==2
        assert str(a) not in jr.read_text()
        case('existing JSON refused',base+['--json-report',jr],2)
        case('report inside input refused',base+['--junit-report',a/'bad.xml'],2)
        case('existing report refused' ,base+['--report',report],2)
        case('same folder refused',['--folder-batch','--baseline',a,'--current',a],2)
        case('unknown folder option',base+['--unknown'],2)
        case('no auto approval option',base+['--approve'],2)
        case('missing option value',['--folder-batch','--baseline'],2)
        case('missing current option',['--folder-batch','--baseline',a],2)
        p={'schema':1,'name':'Independent profile','metric':'PerChannelMetric','allowedPixels':4,'maxError':255,'meanError':255,'parameters':{},'masks':{}}
        profile=root/'profile.json';profile.write_text(json.dumps(p));case('threshold acceptance',base+['--profile',profile],0)
        p['allowedPixels']=0;p['masks']={'*':[[0,0,2,2]]};profile.write_text(json.dumps(p));case('global ignore region',base+['--profile',profile],0)
        p['masks']={'changed.png':[[0,0,2,2]]};profile.write_text(json.dumps(p));case('image-specific ignore region',base+['--profile',profile],0)
        p['masks']={};p['allowedPixels']=-1;profile.write_text(json.dumps(p));case('negative profile threshold rejected',base+['--profile',profile],2)
        p['allowedPixels']=0;p['masks']={'../escape.png':[[0,0,1,1]]};profile.write_text(json.dumps(p));case('traversal mask rejected',base+['--profile',profile],2)
        profile.write_text('not JSON');case('malformed profile rejected',base+['--profile',profile],2)
        (b/'changed.png').write_bytes(b'bad image');case('decoder failure exit code',base,2)
        (b/'changed.png').unlink();case('missing current image',base,1)
        (a/'changed.png').unlink();case('identical folders',base,0)
        png(b/'new.png',1,1,[1,2,3,255]);case('missing baseline image',base,1)
        case('missing folder',['--folder-batch','--baseline',a,'--current',root/'absent'],2)
        empty=root/'empty';empty.mkdir();other=root/'other';other.mkdir();case('empty folders rejected',['--folder-batch','--baseline',empty,'--current',other],2)
    if output:Path(output).write_text(json.dumps(results,indent=2)+'\n')
    print('PASS:',len(results),'folder CLI cases');return results
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('executable');p.add_argument('--output');a=p.parse_args();verify(a.executable,a.output)
