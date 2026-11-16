"""Fit a bounded offline operating-point profile and report held-out errors.
Requires numpy scipy matplotlib; compile render_optical.cpp as described in README.
No input/target WAV files are redistributed.
"""
import argparse,ctypes,json,hashlib,pathlib,subprocess,os
os.environ.setdefault('MPLCONFIGDIR','/tmp/lassassin-mpl')
import numpy as np
from scipy.io import wavfile
from scipy import signal,optimize
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('data',type=pathlib.Path);p.add_argument('--library',default='/tmp/lassassin-optical.so');p.add_argument('--out',type=pathlib.Path,default=pathlib.Path('Documentation/Calibration'));a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
lib=ctypes.CDLL(a.library);ptr=np.ctypeslib.ndpointer(dtype=np.float32,flags='C_CONTIGUOUS')
lib.render_optical.argtypes=[ptr,ptr,ptr,ctypes.c_int,ctypes.c_double,*([ctypes.c_float]*4)]
lib.render_optical.restype=None

def render(x,fs,params,mode):
 x=np.ascontiguousarray(x,dtype=np.float32);out=np.empty_like(x);gr=np.empty_like(x)
 peak,ingain,outgain=params
 lib.render_optical(x,out,gr,len(x),fs,peak,mode,ingain,outgain)
 return out,gr

def rms(x,fs):
 size=round(fs*.020);x=x[:len(x)//size*size].astype(float).reshape(-1,size)
 return np.sqrt(np.mean(x*x,axis=1)+1e-14)
def db(x):return 20*np.log10(np.maximum(x,1e-9))
def mask_times(n,rate,ranges):
 t=np.arange(n)/rate;return np.logical_or.reduce([(t>=l)&(t<r) for l,r in ranges])
# The source archive calls these files Test. We explicitly repurpose disjoint
# time ranges as development data; these are NOT untouched official test scores.
train=[(30,60),(120,150),(210,240)]
validation=[(330,390),(510,570),(690,750),(810,870)]
results=[];manifest=[]
for ident in ['235','256','259']:
 inputs=a.data/f'input_{ident}_.wav';targets=list(a.data.glob(f'target_{ident}_*.wav'))
 if not inputs.exists() or not targets:raise RuntimeError('Missing complete pair '+ident)
 target=targets[0];mode=int(target.stem.split('__')[-2]);peak=float(target.stem.split('__')[-1])
 fs,x=wavfile.read(inputs,mmap=True);fy,y=wavfile.read(target,mmap=True)
 assert fs==fy==44100 and len(x)==len(y)==39690000,(ident,fs,len(x),len(y))
 for path in [inputs,target]:
  manifest.append({'file':path.name,'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
 # Check lags at several separated locations, without fitting a validation offset.
 lags=[];correlations=[]
 for sec in [1,100,350,700]:
  xx=np.asarray(x[sec*fs:(sec+2)*fs]);yy=np.asarray(y[sec*fs:(sec+2)*fs]);c=signal.correlate(yy,xx,method='fft');j=np.argmax(abs(c));lags.append(int(j-len(xx)+1));correlations.append(float(c[j]/np.linalg.norm(xx)/np.linalg.norm(yy)))
 assert all(lag==0 for lag in lags),'Unresolved alignment; do not silently normalize it'
 print('Fitting',ident,'mode',mode,'PR',peak,'lags',lags,flush=True)
 fit_fs=11025
 # Render from t=0 to preserve the complete preceding envelope history.
 xf=signal.resample_poly(x[:240*fs],1,4).astype(np.float32)
 yf=signal.resample_poly(y[:240*fs],1,4).astype(np.float32)
 yr=rms(yf,fit_fs);mask=mask_times(len(yr),fit_fs/round(fit_fs*.020),train)&(yr>10**(-55/20))
 # A 10 dB signal / noise mask is not assumed: this is simply an absolute floor.
 def residual(params):
  z,_=render(xf,fit_fs,params,mode)
  return (db(rms(z,fit_fs))[mask]-db(yr)[mask])
 initial=np.array([peak,0.,0.]);base_loss=float(np.sqrt(np.mean(residual(initial)**2)))
 # Keep input drive fixed: otherwise a fit can replace optical compression
 # with excess tube saturation, giving a misleadingly good envelope score.
 def objective(q): return residual(np.array([q[0],0.,q[1]]))
 def jacobian(q):
  base=objective(q);columns=[]
  for j in range(2):
   shifted=q.copy();shifted[j]+=.1
   columns.append((objective(shifted)-base)/.1)
  return np.column_stack(columns)
 fit=optimize.least_squares(objective,np.array([peak,0.]),jac=jacobian,bounds=([5,-24],[100,24]),x_scale=[40,10],max_nfev=35,ftol=1e-5,xtol=1e-5,gtol=1e-5)
 fit.x=np.array([fit.x[0],0.,fit.x[1]])
 print('fit',ident,fit.x,'train envelope',base_loss,'->',np.sqrt(np.mean(fit.fun**2)),flush=True)
 # Final comparison at native rate, full recording from zero state.
 outputs=[]
 for label,params in [('baseline',initial),('calibrated',fit.x)]:
  z,gr=render(x,fs,params,mode)
  zr=rms(z,fs);xr=rms(x,fs);yr_full=rms(y,fs)
  vm=mask_times(len(zr),50,validation)&(yr_full>10**(-55/20))&(xr>10**(-55/20))
  sample_mask=mask_times(len(x),fs,validation)
  # Frequency comparison concatenates complete validation windows, then Welch.
  yy=np.asarray(y[sample_mask],dtype=float);zz=z[sample_mask].astype(float)
  f,py=signal.welch(yy,fs,nperseg=8192);_,pz=signal.welch(zz,fs,nperseg=8192)
  spectral_mask=(f>=40)&(f<=16000)&(py>py.max()*1e-6)
  spectral_error=10*np.log10(np.maximum(pz,1e-20)/np.maximum(py,1e-20))
  level_offset=20*np.log10(np.linalg.norm(zz)/np.linalg.norm(yy))
  metrics={'envelope_rmse_db':float(np.sqrt(np.mean((db(zr)[vm]-db(yr_full)[vm])**2))),
   'envelope_mae_db':float(np.mean(abs(db(zr)[vm]-db(yr_full)[vm]))),
   'esr':float(np.sum((zz-yy)**2)/np.sum(yy**2)),
   'level_error_db':float(level_offset),
   'spectral_rmse_db':float(np.sqrt(np.mean(spectral_error[spectral_mask]**2))),
   'spectral_shape_rmse_db':float(np.sqrt(np.mean((spectral_error[spectral_mask]-level_offset)**2)))}
  outputs.append({'label':label,'params':params.tolist(),'metrics':metrics,'rms':zr,'gr':gr[::882], 'f':f,'spectrum_error':spectral_error})
  print(label,metrics,flush=True)
 # Empirical program-dependent curves: bins are NOT stationary transfer curves.
 bins=np.arange(-45,-3,2.);curve=[]
 for low in bins:
  bm=vm&(db(xr)>=low)&(db(xr)<low+2)
  if bm.sum()<15:continue
  curve.append([low+1,float(np.median(db(yr_full)[bm])),*[float(np.median(db(o['rms'])[bm])) for o in outputs],int(bm.sum())])
 np.savetxt(a.out/f'{ident}-curve.csv',np.array(curve),delimiter=',',header='input_rms_dbfs,target_output_dbfs,baseline_output_dbfs,calibrated_output_dbfs,frames',comments='')
 times=np.arange(len(xr))*.020
 env=np.column_stack([times,db(xr),db(yr_full),*[db(o['rms']) for o in outputs]])
 np.savetxt(a.out/f'{ident}-envelope.csv',env[vm],delimiter=',',header='time_s,input_dbfs,target_dbfs,baseline_dbfs,calibrated_dbfs',comments='')
 np.savetxt(a.out/f'{ident}-spectrum.csv',np.column_stack([outputs[0]['f'],outputs[0]['spectrum_error'],outputs[1]['spectrum_error']]),delimiter=',',header='frequency_hz,baseline_error_db,calibrated_error_db',comments='')
 fig,axes=plt.subplots(3,1,figsize=(11,12));c=np.array(curve)
 axes[0].plot(c[:,0],c[:,1],'k.-',label='Hardware');axes[0].plot(c[:,0],c[:,2],'.-',label='v0.4.0 core');axes[0].plot(c[:,0],c[:,3],'.-',label='Calibrated profile')
 axes[0].set(xlabel='Input RMS (dBFS)',ylabel='Output RMS (dBFS)',title='Program-dependent binned I/O curve (held-out windows)')
 segment=(times>=510)&(times<530)
 axes[1].plot(times[segment],db(yr_full)[segment],'k',label='Hardware',lw=1)
 for o in outputs:axes[1].plot(times[segment],db(o['rms'])[segment],label=o['label'],alpha=.8,lw=1)
 axes[1].set(xlabel='Time (s)',ylabel='20 ms RMS (dBFS)',title='Held-out envelope; continuous history retained')
 for o in outputs:axes[2].semilogx(o['f'][1:],o['spectrum_error'][1:],label=o['label'],lw=1)
 axes[2].axhline(0,color='k',lw=.7);axes[2].set(xlim=(40,16000),ylim=(-25,25),xlabel='Frequency (Hz)',ylabel='PSD difference (dB)',title='Welch spectrum error against hardware (no per-clip gain fitting)')
 for ax in axes:ax.grid(alpha=.25);ax.legend()
 fig.suptitle(f'SignalTrain subset {ident}: '+('Limit' if mode else 'Compress')+f', Peak Reduction {peak:g}')
 fig.tight_layout();fig.savefig(a.out/f'{ident}-report.png',dpi=160);plt.close(fig)
 results.append({'id':ident,'target':target.name,'mode':mode,'peak':peak,'alignment_lags':lags,'alignment_correlations':correlations,'fit_success':bool(fit.success),'fit_message':fit.message,'fit_nfev':fit.nfev,'train_envelope_before_db':base_loss,'train_envelope_after_db':float(np.sqrt(np.mean(fit.fun**2))),'initial':initial.tolist(),'profile':fit.x.tolist(),'validation':{o['label']:o['metrics'] for o in outputs}})
 (a.out/'results.json').write_text(json.dumps({'source':'https://zenodo.org/records/3824876','source_subset':'Official Test repurposed as temporally split development subset','fit_ranges_s':train,'validation_ranges_s':validation,'fit_rate':fit_fs,'evaluation_rate':fs,'results':results,'files':manifest},indent=2))
 print('Saved',ident,flush=True)
