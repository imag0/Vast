import struct, zipfile, hashlib, datetime
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import hashes, serialization
from cryptography import x509
from cryptography.x509.oid import NameOID

ROOT=Path(__file__).resolve().parents[1]; B=ROOT/'build'; OUT=ROOT/'dist'; OUT.mkdir(exist_ok=True)
VERSION_CODE=55; VERSION_NAME='3.3.0'
FIXED_ZIP_TIME=(2026,9,23,0,0,0)
OCR=ROOT/'assets'/'ocr'/'ppocrv5_mobile_rec'
ORT=ROOT/'third_party'/'onnxruntime'

# Pin the externally sourced runtime/model inputs. A packaging run must fail
# instead of silently shipping a different model, dictionary, or native ABI.
PACKAGE_FILES=[
    (OCR/'inference.onnx','assets/ocr/ppocrv5_mobile_rec/inference.onnx','dc7de8ee31d9246783cf346f3b207b4cb871e11824b1cfab2ff20aa1f1f8e67b'),
    (OCR/'inference.yml','assets/ocr/ppocrv5_mobile_rec/inference.yml','5dfeb2777f6d0db8177d8128a8acfcf6e6276dc4ac73ea3bf0dc06d6a5e85d8e'),
    (OCR/'ppocrv5_dict.txt','assets/ocr/ppocrv5_mobile_rec/ppocrv5_dict.txt','d1979e9f794c464c0d2e0b70a7fe14dd978e9dc644c0e71f14158cdf8342af1b'),
    (OCR/'model-metadata.json','assets/ocr/ppocrv5_mobile_rec/model-metadata.json','af23ea00d7a8067bd8ce093adab559776e9d6323da80bc8f50ab146ad7145e86'),
    (ORT/'lib'/'arm64-v8a'/'libonnxruntime.so','lib/arm64-v8a/libonnxruntime.so','f826d8efb03adf0a84f10e7ba408f9d4cd11b0a2ccd8d08aeb0f7451fb50cacc'),
    (ROOT/'THIRD_PARTY_NOTICES.md','assets/licenses/THIRD_PARTY_NOTICES.md','426ab80e080228ab3e1d9b07c72f88a56351a69a83b619ba252ede20c7f7852c'),
    (ORT/'LICENSE','assets/licenses/onnxruntime-LICENSE','2f07c72751aed99790b8a4869cf2311df85a860b22ded05fa22803587a48922c'),
    (ORT/'ThirdPartyNotices.txt','assets/licenses/onnxruntime-ThirdPartyNotices.txt','0e07b95f3a8d6230037707c5c4a2b554d12c4cb67369669ac255635528ffcee2'),
    (ROOT/'third_party'/'paddleocr'/'LICENSE','assets/licenses/paddleocr-LICENSE','3840c5c0c61c294264d2dd77b8777be6ddd90121ef4e0e64abcd22edea581d6e'),
    (ROOT/'third_party'/'paddle2onnx'/'LICENSE','assets/licenses/paddle2onnx-LICENSE','0b21a30d4bc50f58a5db6e8edb5a85cc8d7eef29ce1c8507c7676969a6823160'),
]

for path,_,expected_sha256 in PACKAGE_FILES:
    if not path.is_file(): raise FileNotFoundError(f'required APK input missing: {path}')
    if expected_sha256:
        actual=hashlib.sha256(path.read_bytes()).hexdigest()
        if actual!=expected_sha256: raise RuntimeError(f'SHA-256 mismatch for {path}: {actual}')

# Refuse to package a native library that predates any of its build inputs.
# This guards the custom non-Gradle workflow against accidentally shipping a
# previously linked canvas after a source or header edit.
NATIVE=B/'libcanvas.so'
if not NATIVE.is_file(): raise FileNotFoundError(f'native library missing: {NATIVE}')
NATIVE_INPUTS=[ROOT/'build-native.sh']
for folder in (ROOT/'src', ROOT/'stubs', ORT/'include'):
    NATIVE_INPUTS.extend(path for path in folder.rglob('*') if path.is_file())
stale=[path for path in NATIVE_INPUTS
       if path.stat().st_mtime_ns > NATIVE.stat().st_mtime_ns]
if stale:
    names=', '.join(str(path.relative_to(ROOT)) for path in stale[:8])
    if len(stale)>8: names+=f', and {len(stale)-8} more'
    raise RuntimeError(f'build/libcanvas.so is stale; rebuild after: {names}')
NO=0xffffffff; NS='http://schemas.android.com/apk/res/android'
RES={
'versionCode':0x0101021b,'versionName':0x0101021c,'minSdkVersion':0x0101020c,'targetSdkVersion':0x01010270,
'label':0x01010001,'icon':0x01010002,'hasCode':0x0101000c,'name':0x01010003,'exported':0x01010010,'value':0x01010024,'extractNativeLibs':0x010104ea,
'authorities':0x01010018,'grantUriPermissions':0x0101001b,
}
strings=list(RES.keys())
for s in ['', 'android',NS,'manifest','package','com.ayomi.infinitecanvas',VERSION_NAME,'uses-sdk','uses-permission','android.permission.READ_MEDIA_IMAGES','android.permission.READ_MEDIA_VISUAL_USER_SELECTED','android.permission.READ_EXTERNAL_STORAGE','android.permission.INTERNET','android.permission.REQUEST_INSTALL_PACKAGES','application','Vast','activity','android.app.NativeActivity','com.ayomi.infinitecanvas.VastFileActivity','provider','com.ayomi.infinitecanvas.VastUpdateProvider','com.ayomi.infinitecanvas.updates','meta-data','android.app.lib_name','canvas','intent-filter','action','android.intent.action.MAIN','category','android.intent.category.LAUNCHER']:
    if s not in strings: strings.append(s)
idx={s:i for i,s in enumerate(strings)}
u16=lambda v:struct.pack('<H',v&0xffff); u32=lambda v:struct.pack('<I',v&0xffffffff); u64=lambda v:struct.pack('<Q',v)
def hdr(t,hs,sz):return u16(t)+u16(hs)+u32(sz)
def sp():
    enc=[]
    for s in strings:
        b=s.encode('utf-16le');enc.append(u16(len(b)//2)+b+b'\0\0')
    hs=0x1c; ss=hs+4*len(strings); offs=[];o=0
    for e in enc:offs.append(u32(o));o+=len(e)
    body=b''.join(offs)+b''.join(enc);sz=hs+len(body);pad=(-sz)%4;sz+=pad
    return hdr(1,hs,sz)+u32(len(strings))+u32(0)+u32(0)+u32(ss)+u32(0)+body+b'\0'*pad
def rmap():
    vals=[RES[s] for s in RES]; return hdr(0x0180,8,8+4*len(vals))+b''.join(u32(v) for v in vals)
def nsstart(line=1):return hdr(0x0100,0x10,24)+u32(line)+u32(NO)+u32(idx['android'])+u32(idx[NS])
def nsend(line=20):return hdr(0x0101,0x10,24)+u32(line)+u32(NO)+u32(idx['android'])+u32(idx[NS])
def sval(x):return('s',x)
def ival(x):return('i',x)
def bval(x):return('b',x)
def rval(x):return('r',x)
def attr(ns,name,v):
    ni=NO if ns is None else idx[ns]; kind,val=v; namei=idx[name]
    if kind=='s':vi=idx[val];return u32(ni)+u32(namei)+u32(vi)+u16(8)+b'\0\x03'+u32(vi)
    if kind=='i':return u32(ni)+u32(namei)+u32(NO)+u16(8)+b'\0\x10'+u32(val)
    if kind=='b':return u32(ni)+u32(namei)+u32(NO)+u16(8)+b'\0\x12'+u32(NO if val else 0)
    if kind=='r':return u32(ni)+u32(namei)+u32(NO)+u16(8)+b'\0\x01'+u32(val)
def start(name,attrs=(),line=1):
    ab=b''.join(attr(*a) for a in attrs);sz=36+len(ab)
    return hdr(0x0102,0x10,sz)+u32(line)+u32(NO)+u32(NO)+u32(idx[name])+u16(0x14)+u16(0x14)+u16(len(attrs))+u16(0)+u16(0)+u16(0)+ab
def end(name,line=1):return hdr(0x0103,0x10,24)+u32(line)+u32(NO)+u32(NO)+u32(idx[name])

c=[];c.append(nsstart())
c.append(start('manifest',[(NS,'versionCode',ival(VERSION_CODE)),(NS,'versionName',sval(VERSION_NAME)),(None,'package',sval('com.ayomi.infinitecanvas'))],1))
c.append(start('uses-sdk',[(NS,'minSdkVersion',ival(26)),(NS,'targetSdkVersion',ival(36))],2));c.append(end('uses-sdk',2))
c.append(start('uses-permission',[(NS,'name',sval('android.permission.READ_MEDIA_IMAGES'))],3));c.append(end('uses-permission',3))
c.append(start('uses-permission',[(NS,'name',sval('android.permission.READ_MEDIA_VISUAL_USER_SELECTED'))],4));c.append(end('uses-permission',4))
c.append(start('uses-permission',[(NS,'name',sval('android.permission.READ_EXTERNAL_STORAGE'))],5));c.append(end('uses-permission',5))
c.append(start('uses-permission',[(NS,'name',sval('android.permission.INTERNET'))],6));c.append(end('uses-permission',6))
c.append(start('uses-permission',[(NS,'name',sval('android.permission.REQUEST_INSTALL_PACKAGES'))],7));c.append(end('uses-permission',7))
c.append(start('application',[(NS,'label',sval('Vast')),(NS,'icon',rval(0x0108003e)),(NS,'hasCode',bval(True)),(NS,'extractNativeLibs',bval(True))],3))
c.append(start('activity',[(NS,'name',sval('android.app.NativeActivity')),(NS,'label',sval('Vast')),(NS,'exported',bval(True))],4))
c.append(start('meta-data',[(NS,'name',sval('android.app.lib_name')),(NS,'value',sval('canvas'))],5));c.append(end('meta-data',5))
c.append(start('intent-filter',[],6));c.append(start('action',[(NS,'name',sval('android.intent.action.MAIN'))],7));c.append(end('action',7));c.append(start('category',[(NS,'name',sval('android.intent.category.LAUNCHER'))],8));c.append(end('category',8));c.append(end('intent-filter',9));c.append(end('activity',10))
c.append(start('activity',[(NS,'name',sval('com.ayomi.infinitecanvas.VastFileActivity')),(NS,'exported',bval(False))],11));c.append(end('activity',11))
c.append(start('provider',[(NS,'name',sval('com.ayomi.infinitecanvas.VastUpdateProvider')),(NS,'authorities',sval('com.ayomi.infinitecanvas.updates')),(NS,'exported',bval(False)),(NS,'grantUriPermissions',bval(True))],12));c.append(end('provider',12))
c.append(end('application',13));c.append(end('manifest',14));c.append(nsend(14))
body=sp()+rmap()+b''.join(c); manifest=hdr(3,8,8+len(body))+body
(B/'AndroidManifest-v2.xml').write_bytes(manifest)

def apk_zip_info(name,mode=0o644):
    info=zipfile.ZipInfo(name,FIXED_ZIP_TIME)
    info.compress_type=zipfile.ZIP_DEFLATED
    info.create_system=3
    info.external_attr=(0o100000|mode)<<16
    return info

def apk_write(archive,name,data,mode=0o644):
    # Stable timestamps and modes keep the unsigned payload, v2 digest and
    # final signed APK reproducible for the same inputs and signing identity.
    archive.writestr(apk_zip_info(name,mode),data)

unsigned=OUT/f'Vast-v{VERSION_NAME}-unsigned.apk'; signed=OUT/f'Vast-v{VERSION_NAME}.apk'
DEX=B/'android-dex'/'classes.dex'
if not DEX.is_file(): raise RuntimeError('Build Android Views with tools/build_android_views.py first')
if any(p.stat().st_mtime>DEX.stat().st_mtime for p in (ROOT/'src'/'java').rglob('*.java')):
    raise RuntimeError('Android View bytecode is stale; run tools/build_android_views.py')
with zipfile.ZipFile(unsigned,'w') as z:
    apk_write(z,'AndroidManifest.xml',manifest)
    apk_write(z,'classes.dex',DEX.read_bytes())
    apk_write(z,'lib/arm64-v8a/libcanvas.so',NATIVE.read_bytes(),0o755)
    for path,archive_name,_ in PACKAGE_FILES:
        apk_write(z,archive_name,path.read_bytes(),0o755 if archive_name.endswith('.so') else 0o644)

# Generate/persist an RSA key + self-signed X.509 cert specifically for APK Signature Scheme v2.
key_path=B/'v2-key.pem'; cert_path=B/'v2-cert.der'
if key_path.exists() != cert_path.exists():
    raise RuntimeError('refusing to rotate signing identity: v2 key/certificate pair is incomplete')
if key_path.exists() and cert_path.exists():
    key=serialization.load_pem_private_key(key_path.read_bytes(), password=None)
    cert_der=cert_path.read_bytes(); cert=x509.load_der_x509_certificate(cert_der)
    if key.public_key().public_numbers() != cert.public_key().public_numbers():
        raise RuntimeError('persisted v2 signing key does not match certificate')
else:
    key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
    key_path.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
    subject=issuer=x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME,'Infinite Canvas'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,'Local Build'),
        x509.NameAttribute(NameOID.COUNTRY_NAME,'PL'),
    ])
    now=datetime.datetime.now(datetime.timezone.utc)
    cert=(x509.CertificateBuilder().subject_name(subject).issuer_name(issuer)
          .public_key(key.public_key()).serial_number(x509.random_serial_number())
          .not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=3650))
          .sign(key,hashes.SHA256()))
    cert_der=cert.public_bytes(serialization.Encoding.DER); cert_path.write_bytes(cert_der)

pub_der=key.public_key().public_bytes(serialization.Encoding.DER,serialization.PublicFormat.SubjectPublicKeyInfo)
raw=unsigned.read_bytes()
# Locate EOCD (non-ZIP64, sufficient for this tiny APK)
eosig=b'PK\x05\x06'; eocd_off=raw.rfind(eosig)
if eocd_off<0: raise RuntimeError('EOCD not found')
comment_len=struct.unpack_from('<H',raw,eocd_off+20)[0]
if eocd_off+22+comment_len != len(raw): raise RuntimeError('unexpected trailing data')
cd_off=struct.unpack_from('<I',raw,eocd_off+16)[0]
cd_size=struct.unpack_from('<I',raw,eocd_off+12)[0]
if cd_off+cd_size != eocd_off: raise RuntimeError('central directory layout unexpected')
sec1=raw[:cd_off]; sec3=raw[cd_off:eocd_off]; sec4=bytearray(raw[eocd_off:])
# For digest calculation, EOCD's CD offset is treated as signing-block offset = original cd_off.
struct.pack_into('<I',sec4,16,cd_off)

def content_digest(sections):
    cds=[]; CH=1024*1024
    for sec in sections:
        for i in range(0,len(sec),CH):
            ch=sec[i:i+CH]
            cds.append(hashlib.sha256(b'\xa5'+u32(len(ch))+ch).digest())
    return hashlib.sha256(b'\x5a'+u32(len(cds))+b''.join(cds)).digest()

def lp(b): return u32(len(b))+b
ALG=0x0103
apk_digest=content_digest([sec1,sec3,bytes(sec4)])
digest_record=u32(ALG)+lp(apk_digest)
digests_seq=lp(digest_record)
certs_seq=lp(cert_der)
attrs_seq=b''
signed_data=lp(digests_seq)+lp(certs_seq)+lp(attrs_seq)
sig=key.sign(signed_data,padding.PKCS1v15(),hashes.SHA256())
sig_record=u32(ALG)+lp(sig)
sigs_seq=lp(sig_record)
signer=lp(signed_data)+lp(sigs_seq)+lp(pub_der)
v2_value=lp(lp(signer))
PAIR_ID=0x7109871a
pair=u64(4+len(v2_value))+u32(PAIR_ID)+v2_value
block_size=len(pair)+24
sig_block=u64(block_size)+pair+u64(block_size)+b'APK Sig Block 42'

# Build final APK and patch EOCD's central-directory offset to account for inserted signing block.
new_eocd=bytearray(raw[eocd_off:])
struct.pack_into('<I',new_eocd,16,cd_off+len(sig_block))
final=sec1+sig_block+sec3+bytes(new_eocd)
signed.write_bytes(final)

# Self-verification of the v2 signature block, signer signature, certificate/public key, and APK digest.
def read_lp(buf, pos):
    n=struct.unpack_from('<I',buf,pos)[0]; pos+=4
    return buf[pos:pos+n], pos+n
# Locate final EOCD/CD/signing block.
f=final; eo=f.rfind(eosig); cdo=struct.unpack_from('<I',f,eo+16)[0]
magic=f[cdo-16:cdo]
assert magic==b'APK Sig Block 42'
bs=struct.unpack_from('<Q',f,cdo-24)[0]
start=cdo-(bs+8)
assert struct.unpack_from('<Q',f,start)[0]==bs
p=start+8
plen=struct.unpack_from('<Q',f,p)[0]; p+=8
pid=struct.unpack_from('<I',f,p)[0]; p+=4
assert pid==PAIR_ID
vv=f[p:p+plen-4]
signers,_=read_lp(vv,0)
signer_blob,_=read_lp(signers,0)
pos=0; sd,pos=read_lp(signer_blob,pos); ss,pos=read_lp(signer_blob,pos); pk,pos=read_lp(signer_blob,pos)
assert pk==pub_der
# signature record
rec,_=read_lp(ss,0); alg=struct.unpack_from('<I',rec,0)[0]; sval,_=read_lp(rec,4)
assert alg==ALG
key.public_key().verify(sval,sd,padding.PKCS1v15(),hashes.SHA256())
# digest record from signed data
pos=0; dseq,pos=read_lp(sd,pos); cseq,pos=read_lp(sd,pos); aseq,pos=read_lp(sd,pos)
drec,_=read_lp(dseq,0); dalg=struct.unpack_from('<I',drec,0)[0]; stored,_=read_lp(drec,4)
assert dalg==ALG
cert0,_=read_lp(cseq,0); assert cert0==cert_der
cert_pub=x509.load_der_x509_certificate(cert0).public_key().public_bytes(serialization.Encoding.DER,serialization.PublicFormat.SubjectPublicKeyInfo)
assert cert_pub==pub_der
# Recompute digest from final APK sections, patching final EOCD CD offset to signing-block start.
cd=f[cdo:eo]; e=bytearray(f[eo:]); struct.pack_into('<I',e,16,start)
recalc=content_digest([f[:start],cd,bytes(e)])
assert recalc==stored==apk_digest
print(signed, signed.stat().st_size)
print('v2 self-verification: OK; targetSdk=36; signingBlockBytes=',len(sig_block))
