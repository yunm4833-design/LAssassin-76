"""Regression check for the Live scanner ordinal-345 load failure. No dependencies."""
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def check(path):
    data = Path(path).read_bytes()
    def u16(at): return struct.unpack_from('<H', data, at)[0]
    def u32(at): return struct.unpack_from('<I', data, at)[0]
    pe = u32(0x3c)
    assert data[pe:pe + 4] == b'PE\0\0', 'Not a PE image'
    assert u16(pe + 4) == 0x8664, 'Expected AMD64'
    optional = pe + 24
    assert u16(optional) == 0x20b, 'Expected PE32+'
    sections = []
    for i in range(u16(pe + 6)):
        at = optional + u16(pe + 20) + 40 * i
        size, rva, raw_size, raw = struct.unpack_from('<IIII', data, at + 8)
        sections.append((rva, max(size, raw_size), raw))
    def offset(rva):
        for start, size, raw in sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise AssertionError('RVA outside sections')
    resource_rva = u32(optional + 112 + 16)
    assert resource_rva, 'Missing resources'
    root = offset(resource_rva)
    def entries(relative):
        start = root + relative
        count = u16(start + 12) + u16(start + 14)
        return [struct.unpack_from('<II', data, start + 16 + i * 8) for i in range(count)]
    def child(relative, identifier):
        for name, target in entries(relative):
            if name == identifier:
                assert target & 0x80000000, 'Expected resource directory'
                return target & 0x7fffffff
        raise AssertionError('Missing resource ID ' + str(identifier))
    manifests = child(0, 24)
    languages = child(manifests, 2)  # DLL isolation-aware manifest, not EXE ID 1
    _, leaf = entries(languages)[0]
    content_rva, size = struct.unpack_from('<II', data, root + leaf)
    at = offset(content_rva)
    xml = ET.fromstring(data[at:at + size])
    ns = '{urn:schemas-microsoft-com:asm.v1}'
    dependencies = xml.findall(ns + 'dependency/' + ns + 'dependentAssembly/' + ns + 'assemblyIdentity')
    assert any(x.get('name') == 'Microsoft.Windows.Common-Controls' and
               x.get('version') == '6.0.0.0' for x in dependencies), 'Missing Common Controls v6 dependency'
    # In addition to the manifest, dialogs are delay-loaded to protect host scanners.
    imports_rva = u32(optional + 112 + 8)
    cursor = offset(imports_rva)
    static_imports = []
    while any(data[cursor:cursor + 20]):
        name_at = offset(u32(cursor + 12))
        end = data.index(b'\0', name_at)
        static_imports.append(data[name_at:end].decode('ascii').lower())
        cursor += 20
    assert 'comctl32.dll' not in static_imports, 'Common Controls must be delay-loaded'
    print('PASS: Windows x64; DLL manifest ID 2; Common Controls v6; no mandatory COMCTL32 import')


if __name__ == '__main__':
    try:
        check(sys.argv[1])
    except (AssertionError, IndexError, ValueError) as error:
        print('FAIL:', error)
        sys.exit(1)
