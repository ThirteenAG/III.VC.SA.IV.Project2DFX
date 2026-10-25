"""Run with Python and the built 2DFXDataGrabber executable as the first argument."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


exe = Path(sys.argv[1]).resolve()


def chunk(kind, data):
    return struct.pack('<III', kind, len(data), 0x1803FFFF) + data


def model(payload_size=80, flash=0):
    light = bytearray(payload_size)
    struct.pack_into('<4B4f', light, 0, 120, 140, 160, 200, 300, 18, 1, 8)
    light[20] = flash
    light[21] = 1
    light[23] = 40
    light[24] = 66
    light[25:25+6] = b'corona'
    light[49:49+8] = b'shad_exp'
    light[73] = 23
    effect = struct.pack('<I3fII', 1, 1, 2, 3, 0, payload_size) + light
    return chunk(0x10, chunk(0x1A, chunk(1, struct.pack('<I', 1)) +
                 chunk(0xF, chunk(3, chunk(0x253F2F8, effect)))))


with tempfile.TemporaryDirectory(prefix='grabber-') as directory:
    root = Path(directory)
    loose = root / '2dfx'
    loose.mkdir()
    output = root / 'out.dat'
    checks = 0

    def run(*args, success=True):
        global checks
        proc = subprocess.run([str(exe), '--game', 'sa', '--root', str(root),
                               '--output', str(output), *args], capture_output=True, text=True)
        assert (proc.returncode == 0) == success, proc.stdout + proc.stderr
        checks += 1
        return proc

    for size in [76, 80]:
        (loose / 'lamp.dff').write_bytes(model(size))
        run()
        rows = [line.split() for line in output.read_text().splitlines()
                if line and line[0] not in '#%']
        assert len(rows) == 1
        assert rows[0][12:] == ['shad_exp', '8', '40', '23', '120', '140', '160']
        checks += 1

    for flash, size, blink in [(3, '1', '2'), (7, '0.449999988', '0'), (8, '0.100000001', '0')]:
        (loose / 'lamp.dff').write_bytes(model(flash=flash))
        run()
        row = next(line.split() for line in output.read_text().splitlines()
                   if line and line[0] not in '#%')
        assert row[7] == size and row[9] == blink
        assert row[12:] == ['shad_exp', '8', '40', '23', '120', '140', '160']
        checks += 1
    (loose / 'lamp.dff').write_bytes(model())

    baseline = root / 'baseline.dat'
    original = (b'# Custom comment\r\n%lamp\r\n'
                b'001 002 003 255 1.0 2.0 3.0 0.70 123.0 03 2 1\r\n'
                b'%additional_coronas\r\n255 000 000 255 4 5 6 1 300 00 2 0\r\n'
                b'%broken_custom\r\n255 0 0\r\n')
    baseline.write_bytes(original)
    run('--baseline', str(baseline))
    rows = output.read_bytes().splitlines(keepends=True)
    before = original.splitlines(keepends=True)
    assert rows[2].startswith(before[2][:-2] + b' ')
    assert rows[:2] == before[:2] and rows[3:] == before[3:]
    checks += 1
    baseline.write_bytes(output.read_bytes())
    previous = output.read_bytes()
    run('--baseline', str(baseline))
    assert output.read_bytes() == previous
    checks += 1

    # Releases preserve full-precision pools even when copied text inputs are rounded.
    curated = previous.replace(b'shad_exp 8 40 23', b'shad_exp 8.00000095 40 23')
    baseline.write_bytes(curated)
    run('--baseline', str(baseline), '--keep-pools')
    assert output.read_bytes() == curated
    checks += 1
    run('--baseline', str(baseline))
    assert output.read_bytes() == previous
    checks += 1

    # Valid IMG v1/v2 entries and their bounds; no external extractor is involved.
    (loose / 'lamp.dff').unlink()
    (root / 'models').mkdir()
    dff = model()
    for version in [1, 2]:
        entry = struct.pack('<II24s', 1, 1, b'lamp.dff')
        prefix = b'VER2' + struct.pack('<I', 1) + entry if version == 2 else b''
        (root / 'models' / 'gta3.img').write_bytes(prefix.ljust(2048, b'\0') + dff.ljust(2048, b'\0'))
        (root / 'models' / 'gta3.dir').write_bytes(entry)
        run()
        assert 'shad_exp 8 40 23 120 140 160' in output.read_text()
        checks += 1

    # Rejected inputs must leave an existing output untouched.
    archive = root / 'models' / 'gta3.img'
    archive.write_bytes(b'VER2' + struct.pack('<I', 0xFFFFFFFF))
    output.write_bytes(b'keep')
    run(success=False)
    assert output.read_bytes() == b'keep'
    archive.unlink()
    for invalid in [b'', model()[:11], model()[:-1],
                    chunk(0x10, chunk(3, chunk(0x253F2F8, struct.pack('<I', 0xFFFFFFFF))))]:
        (loose / 'lamp.dff').write_bytes(invalid)
        run(success=False)
        assert output.read_bytes() == b'keep'
        checks += 1

    # IDE section handling, quoted textures, commas, and blank/comment lines.
    (root / 'gta3.dat').write_text('# manifest\nIDE test.ide\nIPL test.ipl\n')
    (root / 'test.ide').write_text('objs\n100, lamppost1, generic, 1, 100, 0\nend\n\n2dfx\n'
        '100, 1, 2, 3, 120, 140, 160, 200, 0, "coronastar", "shad_exp", 300, 18, 1, 8, 40, 1, 0, 0, 0\nend\n')
    (root / 'test.ipl').write_text('inst\n100, lamppost1, 1, 2, 3, 1, 1, 1, 0, 0, 0, 1\nend\n')
    proc = subprocess.run([str(exe), '--root', str(root), '--output', str(output)], capture_output=True, text=True)
    assert proc.returncode == 0, proc.stderr
    assert '%lamppost1' in output.read_text()
    assert 'shad_exp 8 40 15 120 140 160' in output.read_text()
    checks += 1
    (root / 'test.ide').write_text((root / 'test.ide').read_text().replace('300, 18', 'nan, 18'))
    output.write_bytes(b'keep')
    proc = subprocess.run([str(exe), '--root', str(root), '--output', str(output)], capture_output=True)
    assert proc.returncode != 0 and output.read_bytes() == b'keep'
    checks += 1
    # Portable detection precedence, IV bone offsets, and both Stories outputs.
    (root / 'test.ide').write_text((root / 'test.ide').read_text().replace('nan, 18', '300, 18'))
    manifest = (root / 'gta3.dat').read_bytes()
    for name in ['gta_lcs.dat', 'gta_vcs.dat', 'gta_vc.dat']:
        (root / name).write_bytes(manifest)
    (root / 'gta4.dat').touch()
    (root / 'iv').mkdir()
    (root / 'iv' / 'lamp.light').write_text('Flags 2\nTypeFlags f\nPosition 1 2 3\n'
        'Color 10 20 30 255\nCoronaSize 2\n_f28 123\nType Omni\nBoneID 7\n')
    (root / 'iv' / 'lamp.skel').write_text('Id 7\nWorldOffset 10 20 30\n')
    for game, name in [('IV', 'gta4.dat'), ('III', 'gta3.dat'), ('LCS', 'gta_lcs.dat'),
                       ('VCS', 'gta_vcs.dat'), ('VC', 'gta_vc.dat')]:
        destination = root / (game + 'LodLights.dat')
        destination.unlink(missing_ok=True)
        proc = subprocess.run([str(exe)], cwd=root, capture_output=True)
        assert proc.returncode == 0, proc.stderr
        assert destination.exists()
        if game == 'IV':
            fields = next(line.split() for line in destination.read_text().splitlines()
                          if line and line[0] not in '#%')
            assert fields[:4] == ['010', '020', '030', '128']
            assert [float(value) for value in fields[4:7]] == [11, 22, 33]
            assert fields[9] == '01'
        if game in ['LCS', 'VCS']:
            assert (root / ('ADD' + game + 'LodLights.dat')).read_text().startswith('{ ')
        (root / name).unlink()
        checks += 1
    (loose / 'lamp.dff').write_bytes(model())
    proc = subprocess.run([str(exe)], cwd=root, capture_output=True)
    assert proc.returncode == 0, proc.stderr
    assert 'shad_exp 8 40 23 120 140 160' in (root / 'SALodLights.dat').read_text()
    assert '%additional_coronas' in (root / 'SALodLights.dat').read_text()
    checks += 1
    (loose / 'lamp.dff').unlink()
    (loose / 'lamp.txt').write_text('2dfxType LIGHT\nPosition 1 2 3\nColor 120 140 160 255\n'
        'CoronaFarClip 300\nCoronaSize 1\nCoronaShowMode DEFAULT\nShadowSize 8\n'
        'ShadowColorMP 40\nShadowZDistance 23\nShadowTexName shad_exp\n')
    proc = subprocess.run([str(exe)], cwd=root, capture_output=True)
    assert proc.returncode == 0, proc.stderr
    assert 'shad_exp 8 40 23 120 140 160' in (root / 'SALodLights.dat').read_text()
    checks += 1
    print(f'{checks} parser, preservation and portable compatibility checks passed')
