"""Stream only selected complete pairs from SignalTrain 1.1; never store the 20.7 GB archive."""
import argparse, hashlib, json, pathlib, tarfile, urllib.request
p=argparse.ArgumentParser();p.add_argument('directory',type=pathlib.Path);p.add_argument('--ids',nargs='+',default=['235','256','259']);a=p.parse_args()
a.directory.mkdir(parents=True,exist_ok=True)
meta=json.load(urllib.request.urlopen('https://zenodo.org/api/records/3824876'))
(a.directory/'zenodo_metadata.json').write_text(json.dumps(meta,indent=2))
url=meta['files'][0]['links']['self'];done=set();manifest=[]
with urllib.request.urlopen(url,timeout=120) as remote,tarfile.open(fileobj=remote,mode='r|gz') as archive:
 for m in archive:
  name=pathlib.Path(m.name).name
  if not m.isfile() or not any(name.startswith('input_'+i+'_') or name.startswith('target_'+i+'_') for i in a.ids):continue
  dest=a.directory/name
  if not dest.exists() or dest.stat().st_size!=m.size:
   part=dest.with_suffix('.partial')
   with archive.extractfile(m) as source,part.open('wb') as out:
    while chunk:=source.read(4*1024*1024):out.write(chunk)
   part.replace(dest)
  digest=hashlib.sha256(dest.read_bytes()).hexdigest()
  manifest.append({'file':name,'bytes':m.size,'sha256':digest,'archive_path':m.name})
  print(name,digest,flush=True);done.add(name)
  if all(any(n.startswith('input_'+i+'_') for n in done) and any(n.startswith('target_'+i+'_') for n in done) for i in a.ids):break
(a.directory/'manifest.json').write_text(json.dumps({'source':url,'record':'3824876','subset':manifest},indent=2))
