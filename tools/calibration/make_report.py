"""Build the human-readable report strictly from measured results.json."""
import json,pathlib,html
root=pathlib.Path(__file__).resolve().parents[2]
out=root/'Documentation/Calibration';data=json.loads((out/'results.json').read_text())
lines=['# SignalTrain 2A 校准与误差报告','',
'日期：2026-11-16。基线：LAssassin-76 v0.4.0 的实际光电 DSP 核心。',
'', '**本报告是三个工作点的离线校准，未改动已发布插件默认声音。不能将结果换算成硬件还原度百分比。**','',
'来源：[SignalTrain LA2A Dataset 1.1](https://zenodo.org/records/3824876)，Benjamin Colburn / Scott Hawley；记录许可 CC BY 4.0。',
'选用官方 Test 目录的 235、256、259，重新按时间划分为开发子集；不称为官方独立测试集成绩。','',
'## 验证结果','',
'下表均来自未参与拟合的时间段；格式为 **原模型 → 校准候选**。包络误差单位 dB，越低越好。ESR 是误差能量/参考能量，并非相似度百分比。','',
'| 工作点 | 包络 RMSE | 包络 MAE | 波形 ESR | 频谱 RMSE | 去电平频谱形状 RMSE |','|---|---:|---:|---:|---:|---:|']
for r in data['results']:
 b=r['validation']['baseline'];c=r['validation']['calibrated'];label=('Limit' if r['mode'] else 'Compress')+f" / PR {r['peak']:g}"
 fmt=lambda k:f"{b[k]:.3f} → {c[k]:.3f}"
 lines.append('| '+label+' | '+' | '.join(fmt(k) for k in ['envelope_rmse_db','envelope_mae_db','esr','spectral_rmse_db','spectral_shape_rmse_db'])+' |')
lines+=['','## 固定增益对照','',
'只用训练片段求一个固定输出补偿，验证时冻结。这样可区分单纯电平匹配和改变压缩控制的效果。', '',
'| 工作点 | 仅固定增益包络 RMSE | 校准候选包络 RMSE | 仅固定增益 ESR | 校准候选 ESR |',
'|---|---:|---:|---:|---:|']
for r in data['results']:
 g=r['validation']['gain_only_control'];c=r['validation']['calibrated']
 lines.append(f"| {'Limit' if r['mode'] else 'Compress'} / {r['peak']:g} | {g['envelope_rmse_db']:.3f} | {c['envelope_rmse_db']:.3f} | {g['esr']:.3f} | {c['esr']:.3f} |")
lines+=['','**解读：** Compress/65 和 Limit/80 的改善主要来自参考电平匹配，改变压缩映射后的额外收益较小。Limit/65 相对固定增益对照有明显改善。三个候选仍有数 dB 包络误差，不能称为完成真机复刻；仅覆盖这三个工作点，未回写发布版。', '','[三页测量图 PDF](measurement-plots.pdf) · [离线候选参数](candidate-profiles.json)']
lines+=['','## 拟合参数与收敛' ,'',
'拟合有效 PR 和输出 trim，输入 trim 固定为 0 dB，防止用管级过载替代光电压缩。它们是匹配该数据集的有效坐标，并非真机旋钮值。输出 trim 位于染色之后；本插件 Gain 旋钮位于染色之前，两者不可互换。', '',
'| 工作点 | 有效 PR | 输入 trim dB | 输出 trim dB | 训练包络 RMSE 前→后 | 优化器成功 |', '|---|---:|---:|---:|---:|---|']
for r in data['results']:
 q=r['profile'];lines.append(f"| {'Limit' if r['mode'] else 'Compress'} / {r['peak']:g} | {q[0]:.4f} | {q[1]:.4f} | {q[2]:.4f} | {r['train_envelope_before_db']:.3f} → {r['train_envelope_after_db']:.3f} | {r['fit_success']} |")
lines+=['','## 方法与解读','',
'- 每组录音长 900 秒、44.1 kHz 单声道。校准使用 30–60、120–150、210–240 秒；验证使用 330–390、510–570、690–750、810–870 秒。',
'- 拟合时以 11.025 kHz 连续运行至 240 秒；最终误差以原生 44.1 kHz 从头连续运行完整 900 秒计算。评价窗口前的光电记忆保留。',
'- 检测高通和 HF Emphasis 关闭，Gain=0，单声道复制成立体声链接。没有 1176、Mix、Auto Gain 或外层插件过采样；因此不是 VST3 完整链路测试。',
'- 四处延迟检查（1、100、350、700 秒）均要求相关峰为零整数采样偏移；未进行验证集增益匹配或时间重对齐。未估计亚采样相位偏移。',
'- 包络为不重叠 20 ms RMS，绝对门限 -55 dBFS。频谱为 Welch PSD，8192 点 Hann 窗；标量统计只计 40 Hz–16 kHz、参考谱峰下 60 dB 以内的频点。',
'- 曲线是验证节目按输入 RMS 分箱后的输出中位数，包含历史影响，不能据此声称测出了真机的稳态 ratio/knee。',
'- 图中的包络比较不是独立的起音/释放阶跃实验；没有足够证据给出真机 Attack、Release 或 15 秒恢复尾巴的拟合精度。',
'- 频谱差包含增益、非线性、噪声、相位与动态造成的影响，不等同于频响或单独的谐波失真。',
'- 校准只优化包络，频谱形状和波形未作为优化目标；它们可能改善较少，甚至变差。输出 trim 和有效 PR 存在相关性，不应解释成物理参数识别。',
'- 三个工作点不足以生成可靠的全旋钮范围映射，尤其没有 Compress/80、其它 PR 和 HF 开关数据。暂不据此改写发布版声音。','',
'## 图表与原始结果','']
for r in data['results']:
 ident=r['id'];lines += [f"### {ident}：{'Limit' if r['mode'] else 'Compress'} / PR {r['peak']:g}",'',f'![测量图]({ident}-report.png)','',f'[曲线 CSV]({ident}-curve.csv) · [包络 CSV]({ident}-envelope.csv) · [频谱 CSV]({ident}-spectrum.csv)','']
lines+=['[完整数值、哈希和划分记录](results.json) · [可复现方法与限制](../../tools/calibration/README.md)','',
'### 后续校准需要','',
'增加 Compress/Limit 各模式多个 PR 的独立录音；另留完全不参与拟合的录音做最终验收。然后在固定转换器参考电平下测量阶跃、长短突发、正弦扫幅和不同压缩量的谐波；最后验证插件 2x/4x 过采样、Gain 实际位置与串联模式。']
(out/'REPORT.zh-CN.md').write_text('\n'.join(lines)+'\n')
# Lightweight standalone HTML artifact, including figures as relative files.
body='<html lang="zh-CN"><meta charset="utf-8"><title>SignalTrain 2A 校准报告</title><style>body{max-width:1100px;margin:40px auto;font:17px/1.65 system-ui;padding:0 20px}img{width:100%}pre{white-space:pre-wrap}a{color:#176d91}</style><h1>SignalTrain 2A 校准报告</h1><p>完整说明和 CSV 在同目录 REPORT.zh-CN.md。</p><pre>'+html.escape('\n'.join(lines[:lines.index('## 图表与原始结果')]))+'</pre>'
for r in data['results']:body+=f'<h2>{r["target"]}</h2><img src="{r["id"]}-report.png">'
body+='</html>';(out/'report.html').write_text(body)
print(out/'REPORT.zh-CN.md')
