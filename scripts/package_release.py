"""Package built plugins and their corresponding license/source information."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(r'project\(LAssassin76 VERSION ([0-9.]+)', (ROOT / 'CMakeLists.txt').read_text())[1]
parser = argparse.ArgumentParser()
parser.add_argument('--print-version', action='store_true')
parser.add_argument('--platform', choices=['windows', 'macos'])
parser.add_argument('--artefacts', type=Path)
parser.add_argument('--out', type=Path, default=ROOT / 'dist')
args = parser.parse_args()
if args.print_version:
    print(VERSION)
    raise SystemExit(0)
if not args.platform or not args.artefacts:
    parser.error('--platform and --artefacts are required')
name = 'Windows-x64' if args.platform == 'windows' else 'macOS-Universal'
args.out.mkdir(parents=True, exist_ok=True)
stage = args.out / ('LAssassin-76-' + name)
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir()
formats = ['VST3'] if args.platform == 'windows' else ['VST3', 'AU']
for kind in formats:
    extension = 'vst3' if kind == 'VST3' else 'component'
    source = args.artefacts / kind / ('LAssassin-76.' + extension)
    if not source.is_dir():
        raise SystemExit('Missing bundle: ' + str(source))
    shutil.copytree(source, stage / source.name, symlinks=True)
for filename in ['LICENSE', 'THIRD_PARTY_NOTICES.md', 'README.md']:
    shutil.copy2(ROOT / filename, stage / filename)
shutil.copytree(ROOT / 'Assets/Fonts', stage / 'Font-licenses', ignore=shutil.ignore_patterns('*.ttf'))
shutil.copytree(ROOT / 'ThirdParty', stage / 'ThirdParty')
shutil.copy2(ROOT / 'Documentation/INSTALL.md', stage / 'INSTALL.md')
(stage / 'SOURCE.txt').write_text(
    f'Corresponding source for {VERSION}:\nhttps://github.com/yunm4833-design/LAssassin-76/tree/v{VERSION}\n'
    'JUCE 8.0.6: https://github.com/juce-framework/JUCE/tree/8.0.6\n', encoding='utf-8')
archive = args.out / f'LAssassin-76-v{VERSION}-{name}.zip'
if archive.exists():
    archive.unlink()
if args.platform == 'macos':
    subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(stage), str(archive)], check=True)
else:
    with ZipFile(archive, 'w', ZIP_DEFLATED) as target:
        for file in stage.rglob('*'):
            if file.is_file():
                target.write(file, Path(stage.name) / file.relative_to(stage))
    with ZipFile(archive) as target:
        if target.testzip():
            raise SystemExit('Corrupt ZIP')
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(digest + '  ' + archive.name + '\n', encoding='ascii')
print(archive)
