"""Plot exported measurements, including the frozen fixed-gain control."""
import json,os,pathlib
os.environ.setdefault('MPLCONFIGDIR','/tmp/lassassin-mpl')
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
out=pathlib.Path(__file__).resolve().parents[2]/'Documentation/Calibration'
data=json.loads((out/'results.json').read_text())
with PdfPages(out/'measurement-plots.pdf') as pdf:
 for r in data['results']:
  id=r['id'];trim=r['validation']['gain_only_control']['training_only_trim_db']
  curve=np.loadtxt(out/f'{id}-curve.csv',delimiter=',',skiprows=1)
  env=np.loadtxt(out/f'{id}-envelope.csv',delimiter=',',skiprows=1)
  spec=np.loadtxt(out/f'{id}-spectrum.csv',delimiter=',',skiprows=1)
  fig,ax=plt.subplots(3,1,figsize=(11,12))
  ax[0].plot(curve[:,0],curve[:,1],'k.-',label='Hardware')
  for y,label,style in [(curve[:,2],'v0.4.0 core','.-'),(curve[:,2]+trim,'Fixed gain only','--'),(curve[:,3],'Calibrated profile','.-')]:ax[0].plot(curve[:,0],y,style,label=label)
  ax[0].set(xlabel='Input RMS (dBFS)',ylabel='Output RMS (dBFS)',title='Program-dependent binned I/O curve; not a stationary sweep')
  m=(env[:,0]>=510)&(env[:,0]<530)
  for col,shift,label,style in [(2,0,'Hardware','k-'),(3,0,'v0.4.0 core','-'),(3,trim,'Fixed gain only','--'),(4,0,'Calibrated profile','-')]:ax[1].plot(env[m,0],env[m,col]+shift,style,label=label,lw=.8,alpha=.85)
  ax[1].set(xlabel='Time (s)',ylabel='20 ms RMS (dBFS)',title='Held-out output envelope; full preceding state retained')
  for y,label,style in [(spec[:,1],'v0.4.0 core','-'),(spec[:,1]+trim,'Fixed gain only','--'),(spec[:,2],'Calibrated profile','-')]:ax[2].semilogx(spec[1:,0],y[1:],style,label=label,lw=1)
  ax[2].axhline(0,color='k',lw=.6);ax[2].set(xlim=(40,16000),xlabel='Frequency (Hz)',ylabel='Output PSD difference (dB)',title='Welch spectrum error; no validation-clip gain fitting')
  for axis in ax:axis.grid(alpha=.25);axis.legend(fontsize=9)
  fig.suptitle(f'SignalTrain {id}: '+('Limit' if r['mode'] else 'Compress')+f' / PR {r["peak"]:g}')
  fig.tight_layout();fig.savefig(out/f'{id}-report.png',dpi=150);pdf.savefig(fig);plt.close(fig)
