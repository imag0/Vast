"""Inspect the final Vast APK without requiring Android SDK build tools."""

import hashlib
import struct
import sys
import zipfile
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding


ROOT = Path(__file__).resolve().parents[1]
APK = ROOT / "dist" / "Vast-v3.2.18.apk"
BASELINE = ROOT / "artifacts" / "baseline" / "Vast-v3.2.7-baseline.apk"
BASELINE_SIZE = 173264
CERT = ROOT / "build" / "v2-cert.der"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def u64(data, offset):
    return struct.unpack_from("<Q", data, offset)[0]


def read_length_prefixed(data, offset):
    length = u32(data, offset)
    offset += 4
    end = offset + length
    if end > len(data):
        raise ValueError("truncated length-prefixed APK signature value")
    return data[offset:end], end


def read_string_pool(chunk):
    count = u32(chunk, 8)
    flags = u32(chunk, 16)
    strings_start = u32(chunk, 20)
    if flags & 0x100:
        raise ValueError("UTF-8 Android string pools are not used by this build")
    offsets = [u32(chunk, 28 + index * 4) for index in range(count)]
    values = []
    for offset in offsets:
        cursor = strings_start + offset
        length = u16(chunk, cursor)
        cursor += 2
        if length & 0x8000:
            length = ((length & 0x7FFF) << 16) | u16(chunk, cursor)
            cursor += 2
        values.append(chunk[cursor:cursor + length * 2].decode("utf-16le"))
    return values


def parse_manifest(data):
    if u16(data, 0) != 0x0003 or u32(data, 4) != len(data):
        raise ValueError("invalid binary Android manifest")
    strings = None
    tags = []
    offset = u16(data, 2)
    while offset < len(data):
        kind = u16(data, offset)
        header_size = u16(data, offset + 2)
        size = u32(data, offset + 4)
        if size < header_size or offset + size > len(data):
            raise ValueError("invalid binary Android manifest chunk")
        chunk = data[offset:offset + size]
        if kind == 0x0001:
            strings = read_string_pool(chunk)
        elif kind == 0x0102:
            if strings is None:
                raise ValueError("manifest element precedes string pool")
            name = strings[u32(chunk, 20)]
            attribute_start = u16(chunk, 24)
            attribute_size = u16(chunk, 26)
            attribute_count = u16(chunk, 28)
            attrs = {}
            cursor = 16 + attribute_start
            for _ in range(attribute_count):
                name_index = u32(chunk, cursor + 4)
                raw_index = u32(chunk, cursor + 8)
                value_type = chunk[cursor + 15]
                value_data = u32(chunk, cursor + 16)
                attr_name = strings[name_index]
                if raw_index != 0xFFFFFFFF:
                    value = strings[raw_index]
                elif value_type == 0x03:
                    value = strings[value_data]
                elif value_type == 0x12:
                    value = bool(value_data)
                else:
                    value = value_data
                attrs[attr_name] = value
                cursor += attribute_size
            tags.append((name, attrs))
        offset += size
    return tags


def content_digest(sections):
    chunk_digests = []
    chunk_bytes = 1024 * 1024
    for section in sections:
        for offset in range(0, len(section), chunk_bytes):
            chunk = section[offset:offset + chunk_bytes]
            chunk_digests.append(
                hashlib.sha256(b"\xa5" + struct.pack("<I", len(chunk)) + chunk).digest()
            )
    return hashlib.sha256(
        b"\x5a" + struct.pack("<I", len(chunk_digests)) + b"".join(chunk_digests)
    ).digest()


def verify_v2_signature(apk_bytes):
    eocd_offset = apk_bytes.rfind(b"PK\x05\x06")
    if eocd_offset < 0:
        raise ValueError("ZIP EOCD not found")
    central_offset = u32(apk_bytes, eocd_offset + 16)
    if apk_bytes[central_offset - 16:central_offset] != b"APK Sig Block 42":
        raise ValueError("APK Signature Scheme block not found")
    block_size = u64(apk_bytes, central_offset - 24)
    block_start = central_offset - (block_size + 8)
    if u64(apk_bytes, block_start) != block_size:
        raise ValueError("APK signing block size mismatch")
    cursor = block_start + 8
    pair_length = u64(apk_bytes, cursor)
    cursor += 8
    pair_id = u32(apk_bytes, cursor)
    cursor += 4
    if pair_id != 0x7109871A:
        raise ValueError("APK v2 signer pair missing")
    value = apk_bytes[cursor:cursor + pair_length - 4]
    signers, _ = read_length_prefixed(value, 0)
    signer, _ = read_length_prefixed(signers, 0)
    cursor = 0
    signed_data, cursor = read_length_prefixed(signer, cursor)
    signatures, cursor = read_length_prefixed(signer, cursor)
    public_key, cursor = read_length_prefixed(signer, cursor)
    signature_record, _ = read_length_prefixed(signatures, 0)
    algorithm = u32(signature_record, 0)
    signature, _ = read_length_prefixed(signature_record, 4)
    if algorithm != 0x0103:
        raise ValueError(f"unexpected APK signature algorithm {algorithm:#x}")

    cursor = 0
    digests, cursor = read_length_prefixed(signed_data, cursor)
    certificates, cursor = read_length_prefixed(signed_data, cursor)
    _, cursor = read_length_prefixed(signed_data, cursor)
    digest_record, _ = read_length_prefixed(digests, 0)
    digest_algorithm = u32(digest_record, 0)
    stored_digest, _ = read_length_prefixed(digest_record, 4)
    certificate_der, _ = read_length_prefixed(certificates, 0)
    if digest_algorithm != algorithm:
        raise ValueError("APK signature/digest algorithm mismatch")
    certificate = x509.load_der_x509_certificate(certificate_der)
    certificate.public_key().verify(
        signature, signed_data, padding.PKCS1v15(), hashes.SHA256()
    )
    # SubjectPublicKeyInfo equality catches a certificate/signing-key mismatch.
    from cryptography.hazmat.primitives import serialization
    cert_public_key = certificate.public_key().public_bytes(
        serialization.Encoding.DER,
        serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    if cert_public_key != public_key:
        raise ValueError("APK certificate and signer public key differ")

    central_directory = apk_bytes[central_offset:eocd_offset]
    eocd = bytearray(apk_bytes[eocd_offset:])
    struct.pack_into("<I", eocd, 16, block_start)
    calculated = content_digest(
        [apk_bytes[:block_start], central_directory, bytes(eocd)]
    )
    if calculated != stored_digest:
        raise ValueError("APK v2 content digest mismatch")
    return certificate_der, block_size + 8


def main():
    apk_bytes = APK.read_bytes()
    certificate_der, signing_block_bytes = verify_v2_signature(apk_bytes)
    expected_certificate = CERT.read_bytes()
    if certificate_der != expected_certificate:
        raise ValueError("APK signer certificate differs from persisted release certificate")
    baseline_signer_status = "SKIPPED"
    if BASELINE.is_file():
        if BASELINE.stat().st_size != BASELINE_SIZE:
            raise ValueError("update-compatible baseline APK size differs from the pinned release")
        baseline_certificate, _ = verify_v2_signature(BASELINE.read_bytes())
        if certificate_der != baseline_certificate:
            raise ValueError("APK signer certificate differs from the update-compatible baseline")
        baseline_signer_status = "PASS"

    with zipfile.ZipFile(APK) as archive:
        entries = archive.infolist()
        names = [entry.filename for entry in entries]
        if archive.testzip() is not None:
            raise ValueError("APK contains a corrupt ZIP member")
        if len(names) != len(set(names)):
            raise ValueError("APK contains duplicate ZIP names")
        contents = {name: archive.read(name) for name in names}

    tags = parse_manifest(contents["AndroidManifest.xml"])
    manifest = next(attrs for name, attrs in tags if name == "manifest")
    sdk = next(attrs for name, attrs in tags if name == "uses-sdk")
    permissions = [attrs["name"] for name, attrs in tags if name == "uses-permission"]
    application = next(attrs for name, attrs in tags if name == "application")

    expected = {
        "package": "com.ayomi.infinitecanvas",
        "versionCode": 53,
        "versionName": "3.2.18",
        "minSdkVersion": 26,
        "targetSdkVersion": 36,
    }
    actual = {
        "package": manifest["package"],
        "versionCode": manifest["versionCode"],
        "versionName": manifest["versionName"],
        "minSdkVersion": sdk["minSdkVersion"],
        "targetSdkVersion": sdk["targetSdkVersion"],
    }
    if actual != expected:
        raise ValueError(f"manifest mismatch: {actual!r}")
    required_permissions = {"android.permission.INTERNET", "android.permission.REQUEST_INSTALL_PACKAGES"}
    if not required_permissions.issubset(permissions):
        raise ValueError("APK is missing its updater permissions")
    forbidden_permissions = {"android.permission.ACCESS_NETWORK_STATE"}
    if forbidden_permissions.intersection(permissions):
        raise ValueError("APK unexpectedly declares a network permission")
    if application.get("hasCode") is not True or "classes.dex" not in names:
        raise ValueError("APK is missing its Android View adapter")
    if application.get("extractNativeLibs") is not True:
        raise ValueError("APK must extract its native libraries for NativeActivity")
    provider = next((attrs for name, attrs in tags if name == "provider"), None)
    if not provider or provider.get("name") != "com.ayomi.infinitecanvas.VastUpdateProvider" or provider.get("authorities") != "com.ayomi.infinitecanvas.updates" or provider.get("exported") is not False or provider.get("grantUriPermissions") is not True:
        raise ValueError("APK update provider is missing or unsafe")

    abis = sorted({name.split("/")[1] for name in names if name.startswith("lib/")})
    if abis != ["arm64-v8a"]:
        raise ValueError(f"unexpected packaged ABIs: {abis}")
    required = {
        "classes.dex": ROOT / "build" / "android-dex" / "classes.dex",
        "lib/arm64-v8a/libcanvas.so": ROOT / "build" / "libcanvas.so",
        "lib/arm64-v8a/libonnxruntime.so": ROOT / "third_party" / "onnxruntime" / "lib" / "arm64-v8a" / "libonnxruntime.so",
        "assets/ocr/ppocrv5_mobile_rec/inference.onnx": ROOT / "assets" / "ocr" / "ppocrv5_mobile_rec" / "inference.onnx",
        "assets/ocr/ppocrv5_mobile_rec/inference.yml": ROOT / "assets" / "ocr" / "ppocrv5_mobile_rec" / "inference.yml",
        "assets/ocr/ppocrv5_mobile_rec/ppocrv5_dict.txt": ROOT / "assets" / "ocr" / "ppocrv5_mobile_rec" / "ppocrv5_dict.txt",
        "assets/ocr/ppocrv5_mobile_rec/model-metadata.json": ROOT / "assets" / "ocr" / "ppocrv5_mobile_rec" / "model-metadata.json",
        "assets/licenses/THIRD_PARTY_NOTICES.md": ROOT / "THIRD_PARTY_NOTICES.md",
        "assets/licenses/onnxruntime-LICENSE": ROOT / "third_party" / "onnxruntime" / "LICENSE",
        "assets/licenses/onnxruntime-ThirdPartyNotices.txt": ROOT / "third_party" / "onnxruntime" / "ThirdPartyNotices.txt",
        "assets/licenses/paddleocr-LICENSE": ROOT / "third_party" / "paddleocr" / "LICENSE",
        "assets/licenses/paddle2onnx-LICENSE": ROOT / "third_party" / "paddle2onnx" / "LICENSE",
    }
    expected_names = {"AndroidManifest.xml", *required}
    missing = sorted(expected_names.difference(names))
    unexpected = sorted(set(names).difference(expected_names))
    if missing or unexpected:
        raise ValueError(
            f"APK member set mismatch: missing={missing!r} unexpected={unexpected!r}"
        )
    for archive_name, source in required.items():
        if archive_name not in contents:
            raise ValueError(f"required APK member missing: {archive_name}")
        if contents[archive_name] != source.read_bytes():
            raise ValueError(f"packaged member differs from source: {archive_name}")
    forbidden_names = ("firebase", "mlkit", "play-services", "google-services")
    if any(any(term in name.lower() for term in forbidden_names) for name in names):
        raise ValueError("APK unexpectedly contains a forbidden cloud/Google dependency")

    cert = x509.load_der_x509_certificate(certificate_der)
    apk_size = len(apk_bytes)
    baseline_size = BASELINE_SIZE
    print("APK RELEASE INSPECTION: PASS")
    print(
        "manifest "
        f"package={actual['package']} versionCode={actual['versionCode']} "
        f"versionName={actual['versionName']} minSdk={actual['minSdkVersion']} "
        f"targetSdk={actual['targetSdkVersion']} hasCode={application['hasCode']} "
        f"extractNativeLibs={application['extractNativeLibs']}"
    )
    print(f"permissions={','.join(permissions)}")
    print("network_permissions=INTERNET updater_only; access_network_state=absent")
    print(f"abis={','.join(abis)} entries={len(entries)} unique={len(set(names))} zip_test=PASS")
    print(f"apk_bytes={apk_size} apk_sha256={sha256(apk_bytes)}")
    print(
        f"baseline_bytes={baseline_size} increase_bytes={apk_size - baseline_size}"
    )
    print(
        f"v2_signature=PASS signing_block_bytes={signing_block_bytes} "
        f"certificate_sha256={sha256(certificate_der)}"
    )
    if baseline_signer_status == "PASS":
        print("baseline_signer_match=PASS")
    else:
        print("baseline_signer_match=SKIPPED reason=baseline_apk_unavailable")
    for archive_name, source in required.items():
        data = contents[archive_name]
        print(
            f"member={archive_name} bytes={len(data)} sha256={sha256(data)} "
            f"source_match=PASS"
        )
    print("forbidden_cloud_dependencies=absent")
    print("entries:")
    for entry in entries:
        print(
            f"  {entry.filename} bytes={entry.file_size} compressed={entry.compress_size}"
        )


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"APK RELEASE INSPECTION: FAIL: {error}", file=sys.stderr)
        raise
