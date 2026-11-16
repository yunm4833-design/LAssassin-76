"""Validate the integrated 2A mapping through JUCE's actual 2x/4x FIR stages."""
import pathlib,json,subprocess,re,sys,os
import numpy as np
from scipy.io import wavfile
from scipy import signal
root=pathlib.Path(__file__).resolve().parents[2];data=pathlib.Path(sys.argv[1]);renderer=root/'build/render_oversampled';out=root/'Documentation/Calibration';rows=[]
# A distinct shorter protocol: 90 s warm-up (420–510), evaluate 510–570 s.
# No parameters are fitted or selected using this validation.
for id,peak,mode in [('235',65,0),('256',65,1),('259',80,1)]:
 fs,x=wavfile.read(data/f'input_{id}_.wav',mmap=True)
 target=next(data.glob(f'target_{id}_*.wav'));_,y=wavfile.read(target,mmap=True)
 work=pathlib.Path('/tmp/lassassin-oversampled');work.mkdir(exist_ok=True)
 xf=np.asarray(x[420*fs:570*fs]);yf=np.asarray(y[510*fs:570*fs],dtype=float)
 np.concatenate([xf,np.zeros(128,dtype=np.float32)]).astype(np.float32).tofile(work/'input.raw')
 def env(v):return 20*np.log10(np.maximum(np.sqrt(np.mean(v[:len(v)//882*882].astype(float).reshape(-1,882)**2,axis=1)),1e-9))
 ye=env(yf);xe=env(x[510*fs:570*fs]);mask=(ye>-55)&(xe>-55)
 f,py=signal.welch(yf,fs,nperseg=8192);fm=(f>=40)&(f<=16000)&(py>py.max()*1e-6)
 for exponent in [1,2]:
  for enabled in [0,1]:
   stdout=subprocess.check_output([str(renderer),str(work/'input.raw'),str(work/'output.raw'),str(fs),str(exponent),str(peak),str(mode),str(enabled)],text=True)
   latency=int(re.search(r'latency (\d+)',stdout)[1])
   z=np.fromfile(work/'output.raw',dtype=np.float32)[latency:latency+len(xf)]
   assert len(z)==len(xf) and np.isfinite(z).all()
   z=z[90*fs:].astype(float);ze=env(z)
   _,pz=signal.welch(z,fs,nperseg=8192);spectral=10*np.log10(np.maximum(pz,1e-20)/np.maximum(py,1e-20))
   row=dict(id=id,mode=mode,peak=peak,oversampling=2**exponent,calibration=enabled,latency_samples=latency,envelope_rmse_db=float(np.sqrt(np.mean((ze[mask]-ye[mask])**2))),esr=float(np.sum((z-yf)**2)/np.sum(yf**2)),spectral_rmse_db=float(np.sqrt(np.mean(spectral[fm]**2))))
   rows.append(row);print(row,flush=True)
   (out/'integrated-oversampling-results.json').write_text(json.dumps({'protocol':'90s warm-up at 420-510s; evaluate 510-570s; no refit; isolated production optical stage and JUCE FIR; HPF/HF/Gain=0; integer FIR delay removed','core_tag':'v0.5.0','rows':rows},indent=2))
assert len(rows)==12
lines=['# 0.5.0：2A 校准映射集成验证','',
'使用生产光电核心和相同配置的 JUCE FIR 过采样器，检查本次映射在 2x/4x 下的效果。没有重新拟合参数。','',
'每组输入取原录音 420–570 秒，前 90 秒预热，仅评价 510–570 秒。HPF/HF/Gain=0；移除过采样器报告的整数采样延迟。初始历史与原离线报告不同，不能直接合并两套数值。', '',
'| 工作点 | 过采样 | 包络 RMSE 原映射→新映射 dB | 频谱 RMSE 原→新 dB | 波形 ESR 原→新 |','|---|---:|---:|---:|---:|']
for id in ['235','256','259']:
 for osfactor in [2,4]:
  old=next(r for r in rows if r['id']==id and r['oversampling']==osfactor and r['calibration']==0)
  new=next(r for r in rows if r['id']==id and r['oversampling']==osfactor and r['calibration']==1)
  assert new['envelope_rmse_db']<old['envelope_rmse_db'],'Integrated mapping regressed measured operating point'
  label=('Limit' if old['mode'] else 'Compress')+f"/{old['peak']}"
  lines.append(f"| {label} | {osfactor}x | {old['envelope_rmse_db']:.3f} → {new['envelope_rmse_db']:.3f} | {old['spectral_rmse_db']:.3f} → {new['spectral_rmse_db']:.3f} | {old['esr']:.3f} → {new['esr']:.3f} |")
lines+=['','新实例默认开启 SETUP → 2A SIGNALTRAIN；可关闭对照原映射。旧工程自动关闭该模式。', '',
'仅三个点来自测量：Compress/65、Limit/65、Limit/80。PR=0 的映射/参考补偿为 0；PR=100 的有效 PR=100，参考补偿保持该模式最高测量点值；这些边界为工程选择。中间位置采用单调分段线性插值，不能称为全范围硬件校准。', '',
'本次调整有效 PR 和染色级之后的参考电平补偿，未重新拟合起音、释放或热记忆常数。LA Gain 位置与单位不变。参考补偿推动串联下一级，不计入 GR。', '',
'验证覆盖隔离的 2A+过采样模块；全插件另有状态保存、参数平滑、Order/Mix/旁路/大块处理与无 C++ 堆分配回归测试。此数据测试不包含 FET 声音校准，也不表示全插件与真机等效。', '',
'[原始测量 JSON](integrated-oversampling-results.json) · [历史离线报告](REPORT.zh-CN.md)']
(out/'INTEGRATION-0.5.0.md').write_text('\n'.join(lines)+'\n')
