param([Parameter(Mandatory=$true)][string]$Folder,[Parameter(Mandatory=$true)][string]$Thumbprint)
$ErrorActionPreference = 'Stop'
$certificate = Get-Item -LiteralPath "Cert:/CurrentUser/My/$Thumbprint"
if (-not $certificate.HasPrivateKey) { throw 'Signing certificate has no private key' }
if ($certificate.NotAfter -lt (Get-Date)) { throw 'Signing certificate expired' }
foreach ($name in @('diffimg.exe','diffimg-cli.exe')) {
    $path = Join-Path $Folder $name
    $signature = Set-AuthenticodeSignature -LiteralPath $path -Certificate $certificate -HashAlgorithm SHA256
    if ($signature.SignerCertificate.Thumbprint -ne $Thumbprint) { throw "Signature missing for $name" }
    # A self-signed certificate is expected to have an untrusted chain on other computers.
    $verify = Get-AuthenticodeSignature -LiteralPath $path
    if ($verify.Status -eq 'HashMismatch' -or -not $verify.SignerCertificate) { throw "Signature integrity failed: $name" }
}
Export-Certificate -Cert $certificate -FilePath (Join-Path $Folder 'DiffImg-publisher.cer') -Type CERT | Out-Null
$kind = if ($certificate.Subject -eq $certificate.Issuer) { 'self-signed; no public Windows publisher trust' } else { 'certificate-chain trust must be verified on the target computer' }
@"
DiffImg application executables are signed with SHA256 Authenticode.
Certificate: $($certificate.Subject)
SHA1 certificate thumbprint (identity only): $Thumbprint
Trust: $kind
No certificate is installed into a trusted certificate store by this package.
The adjacent .cer contains only the public certificate, never its private key.
Signatures identify distribution; source/dependency licenses remain in force.
"@ | Set-Content -LiteralPath (Join-Path $Folder 'SIGNING.txt') -Encoding utf8
