"""Add a training-only fixed-gain control, to expose mere loudness matching."""
import ctypes,json,pathlib,sys,hashlib,platform,subprocess
import numpy as np
from scipy.io import wavfile
root=pathlib.Path(__file__).resolve().parents[2];out=root/'Documentation/Calibration';data=json.loads((out/'results.json').read_text());directory=pathlib.Path(sys.argv[1]);lib=ctypes.CDLL('/tmp/lassassin-optical.so')
ptr=np.ctypeslib.ndpointer(dtype=np.float32,flags='C_CONTIGUOUS');lib.render_optical.argtypes=[ptr,ptr,ptr,ctypes.c_int,ctypes.c_double,*([ctypes.c_float]*4)];lib.render_optical.restype=None
for r in data['results']:
 fs,x=wavfile.read(directory/f'input_{r["id"]}_.wav',mmap=True);_,y=wavfile.read(directory/r['target'],mmap=True)
 x=np.ascontiguousarray(x[:240*fs]);y=y[:len(x)];z=np.empty_like(x);gr=np.empty_like(x)
 lib.render_optical(x,z,gr,len(x),fs,r['peak'],r['mode'],0,0)
 def envelope(v):return 20*np.log10(np.maximum(np.sqrt(np.mean(v[:len(v)//882*882].astype(float).reshape(-1,882)**2,axis=1)),1e-9))
 yz=envelope(y);zz=envelope(z);t=np.arange(len(yz))*.02;m=np.logical_or.reduce([(t>=a)&(t<b) for a,b in data['fit_ranges_s']])&(yz>-55)
 trim=float(np.mean(yz[m]-zz[m]));g=10**(trim/20);b=r['validation']['baseline']
 env=np.loadtxt(out/f'{r["id"]}-envelope.csv',delimiter=',',skiprows=1)
 spec=np.loadtxt(out/f'{r["id"]}-spectrum.csv',delimiter=',',skiprows=1)
 # Gain-only ESR follows exactly from baseline energy ratio and cross term.
 energy=10**(b['level_error_db']/10);cross=(energy+1-b['esr'])/2
 metrics={'training_only_trim_db':trim,'envelope_rmse_db':float(np.sqrt(np.mean((env[:,3]+trim-env[:,2])**2))), 'envelope_mae_db':float(np.mean(abs(env[:,3]+trim-env[:,2]))),'esr':float(g*g*energy+1-2*g*cross),'level_error_db':b['level_error_db']+trim}
 r['validation']['gain_only_control']=metrics
 print(r['id'],metrics)
data['provenance']={'baseline_tag':'v0.4.0','baseline_commit':subprocess.check_output(['git','rev-parse','v0.4.0^{}'],cwd=root,text=True).strip(),'python':platform.python_version(),'numpy':np.__version__,'core_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'Source/DSP/Dynamics.h',root/'Source/DSP/OpticalMemory.h',root/'Source/DSP/PhysicalStages.h']}}
(out/'results.json').write_text(json.dumps(data,indent=2))
