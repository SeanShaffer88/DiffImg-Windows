"""Sign only DiffImg executables using an existing Windows user-store certificate.

Uses the Windows SDK SignTool. No policy, trust-store or private-key changes.
"""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path

def signature(path):
    env={key:value for key,value in os.environ.items() if key.upper()!='PSMODULEPATH'}
    env['DIFFIMG_SIGN_PATH']=str(path)
    modern=Path(os.environ.get('ProgramFiles','C:/Program Files'))/'PowerShell/7/pwsh.exe'
    shell=str(modern) if modern.exists() else str(Path(os.environ['SystemRoot'])/'System32/WindowsPowerShell/v1.0/powershell.exe')
    command="$s=Get-AuthenticodeSignature -LiteralPath $env:DIFFIMG_SIGN_PATH; [pscustomobject]@{status=$s.Status.ToString();subject=$s.SignerCertificate.Subject;issuer=$s.SignerCertificate.Issuer;thumbprint=$s.SignerCertificate.Thumbprint}|ConvertTo-Json -Compress"
    return json.loads(subprocess.check_output([shell,'-NoProfile','-NonInteractive','-Command',command],env=env,text=True))

def sign(folder,thumbprint,signtool=None):
    if not re.fullmatch('[0-9a-fA-F]{40}',thumbprint):raise RuntimeError('Expected certificate SHA1 thumbprint (identity lookup, signatures use SHA256)')
    if not signtool:
        sdk=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Windows Kits/10/bin'
        matches=sorted(sdk.glob('10.*/x64/signtool.exe'),reverse=True)
        if not matches:raise RuntimeError('Windows SDK x64 SignTool is required')
        signtool=matches[0]
    records={}
    for name in ('diffimg.exe','diffimg-cli.exe'):
        path=folder/name
        subprocess.run([str(signtool),'sign','/sha1',thumbprint,'/s','My','/fd','SHA256',str(path)],check=True)
        result=signature(path)
        if result['thumbprint'].casefold()!=thumbprint.casefold() or result['status'] not in ('Valid','NotTrusted'):
            raise RuntimeError('Signature validation failed: '+name+' ('+result['status']+')')
        records[name]=result
    public=folder/'DiffImg-publisher.cer'
    subprocess.run(['certutil.exe','-user','-store','My',thumbprint,str(public)],check=True,capture_output=True)
    record=records['diffimg.exe'];self_signed=record['subject']==record['issuer']
    (folder/'signatures.json').write_text(json.dumps({'algorithm':'SHA256','self_signed':self_signed,'public_certificate_sha256':hashlib.sha256(public.read_bytes()).hexdigest(),'files':records},indent=2)+'\n')
    (folder/'SIGNING.txt').write_text('DiffImg executables use SHA256 Authenticode.\nPublisher: '+record['subject']+'\nCertificate identity thumbprint: '+thumbprint+'\nTrust: '+('self-signed; no public Windows publisher trust or SmartScreen reputation is implied.' if self_signed else 'certificate chain must be checked on the target computer.')+'\nOnly the public certificate is included. No private key or trust-store changes are included. Original licenses remain in force.\n')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--folder',type=Path,required=True);p.add_argument('--thumbprint',required=True);p.add_argument('--signtool',type=Path);a=p.parse_args();sign(a.folder.resolve(),a.thumbprint,a.signtool)
