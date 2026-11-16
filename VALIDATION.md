# v0.2.1 Windows 扫描修复（2026-11-14）

用户提供 UAPluginScanner.exe 的原生 Windows 错误：“无法定位序数 345”。
定位：COMCTL32.dll 的 ordinal 345（TaskDialogIndirect）为静态依赖，旧 DLL 缺少 Common Controls v6 激活清单；旧交叉工具链关闭自动 manifest 导致清单丢失。
修复：显式 RC 嵌入 RT_MANIFEST / ID 2，声明 Common Controls v6；COMCTL32 改为延迟加载，避免扫描器启动时解析 ordinal 345。
同时使用 Windows VST3 Manifest Helper 在 Wine 下读取实际 DLL 信息，生成并规范化 moduleinfo.json。

回归测试 tests/check_windows_bundle.py：旧包失败（Missing resource ID 24）；新包通过（Manifest ID 2、v6 依赖、无强制 COMCTL32 导入）。
WindowsProbe.exe 在 Wine 10 下实测：LoadLibrary、InitDll、工厂 3 个类、组件初始化、100 块音频、连接后的 22 个参数枚举通过。48 kHz 时延迟 61 samples。
仅验证了 Wine 和静态 PE 回归；真实 Windows / Live 修复效果待用户确认。源码 DSP 未改。

新二进制 SHA-256：`6a9dfd989b051388a33c98aeaf8dd6aaba8071fde81c6f065e9ede4680b4a7ff`

---

# v0.2.0 Windows UI 构建验证（2026-11-14）

- 完成自定义 LookAndFeel 和交互编辑器，所有参数使用 APVTS/ParameterAttachment 实际绑定。
- 原创程序化材质/矢量装饰、羽毛 VU、Ratio/All、Order 动画、Bypass 暗化、GR/电平、预设与缩放。
- Windows x64 交叉构建与链接成功：Clang 23 MSVC ABI + Microsoft Windows SDK / MSVC CRT，Release，静态 C++ 运行库。
- PE 检查：COFF-x86-64 / IMAGE_FILE_MACHINE_AMD64，导出 GetPluginFactory、InitDll、ExitDll。
- 导入检查仅见 Windows 系统 DLL，没有 MinGW 或 VC++ 运行库 DLL 依赖。
- Windows 交叉构建关闭自动 manifest 生成；VST3 bundle 内的二进制供宿主直接扫描。
- Windows SDK 在 Linux 上需要大小写别名；本次修正了 Dbghelp.h 和四个 .lib 名称的别名，未修改 DSP。
- 同一源码 Linux 自动测试 3/3 通过：dsp_models、processor_integration、editor_bindings，总计 3.72 s。
- UI 测试覆盖 10 个连续参数、开关、5 个 Ratio 回调、宿主参数回写、模式选择、预设、缩放与实际图像渲染。
- 未在 Windows DAW 中执行实际加载、音频设备播放或 Steinberg 全量 validator；用户宿主试音尚待进行。

二进制 SHA-256：`0c06c62c504d8d00cad3ac9c50e2a4edd50deb3047be9ef7b820b494cb279b4e`

---

以下保留初版构建验证记录：

# 本次验证记录

日期：2026-11-14。环境：Linux x86_64 / Debian 13，GCC 14.2，JUCE 8.0.6，Release，C++17。

## 构建

- CMake 配置成功。
- VST3 构建、链接成功；JUCE 的 VST3 helper 成功生成模块元数据。
- Standalone 构建、链接成功。
- 最终修改的增量构建无编译警告。
- VST3 的 ldd 依赖检查没有 not found，运行时依赖解析到系统库。
- 本机缺少开发依赖，验证时仅在 /tmp 中下载/解包开发工具和头文件，不改变工程的标准构建方式，不修改系统安装。

## 测试

```
Start 1: dsp_models
1/2 Test #1: dsp_models ....................... Passed  0.63 sec
Start 2: processor_integration
2/2 Test #2: processor_integration ............ Passed  1.04 sec
100% tests passed, 0 tests failed out of 2
```

模型测试覆盖：

- T4 历史依赖：相同测试电平，压缩 0.1 s 和 8 s 后停止信号；停止 1 s 时残余 GR 分别约 9.30 / 11.70 dB。
- 44.1 / 48 / 96 / 192 kHz 模型运行时的有限数值输出。
- 强驱动、All、最快起音/释放的有限输出检查。
- 完全链接时两声道共享 GR。
- 1176 Input 增加时固定阈值模型的 GR 增加。
- 前级 LA Gain 增加确实增加后级 FET 的 GR。

处理器测试覆盖：

- 44.1 / 48 / 96 kHz，prepare 最大块 64，而实际输入块 257 的分块处理。
- Mix=0 的双声道脉冲精确出现在已报告延迟处。
- Order / OS / M-S / Link / Auto Gain / 两级 Bypass 变化过程输出有限。
- OS 切换前后报告的延迟恒定。
- 总 GR 与两级显示的 GR 之和一致。
- APVTS 状态序列化/恢复。
- 被监测的 processBlock 调用内普通 C++ new/new[] 计数为零。

## 尚未验证

- macOS AU、Windows VST3 的本机编译与宿主运行。
- macOS auval / Steinberg 全量 validator / DAW 扫描加载与自动化。
- 独立程序的实际音频设备与定制 UI 交互。
- 真实硬件对照、THD/IMD/混叠测量、系统化过冲与时间常数曲线。
- 低缓冲实时 CPU 预算、长时间全参数随机自动化。
- malloc/对齐分配/系统调用级别的实时安全监测。

本结果证明代码可以在上述环境构建，且所列自动化检查通过，不代表完成商业插件的跨平台认证或模拟硬件校准。

## 0.3.0 local audit checks

Linux model, processor integration and UI binding tests: 3/3 pass (4.81 s). Added high-rate long-release agreement, 10 ms attack, Output-isolation from FET feedback, and invalid-audio/parameter recovery checks. CI native builds are the release gate. See Documentation/MODEL-STATUS.md.

## 0.3.0 physical-stage / bypass extension

Adapted MIT JFET/Jiles-Atherton stages. Quiet-signal root precision, passive cell branch and magnetic insertion-energy checks added. Host bypass now preserves 61-sample OS latency via the shared dry path, with a startup-bypass impulse regression. All local model/integration/editor gates passed after the adaptation. Source revision and license notices are bundled.

## 0.4.0 local release candidate (2026-11-14)

- Linux Release compiled without project compiler warnings; all three CTest
  suites pass: DSP models, processor integration, editor bindings.
- New OpticalMemory checks: about half the GR released after 60 ms; gradual LDR
  charge/cool; bounded 1–15 s slow branch; short-transient heat rejection;
  reset/nonfinite safety. Existing high-rate and history checks continue to pass.
- For the defined 0.5 DC / PR70 test, GR remaining after 1 s silence was 8.30243 dB
  following a 0.1 s hold and 11.6563 dB following an 8 s hold. These are algorithm
  regression values, not measured LA-2A hardware data.
- UI global bypass produces the same reported delay-aligned dry impulse as host
  bypass, survives save/load, and defaults off when restoring pre-0.4 sessions.
- Numeric Gain entry maps the reference faceplate's 38 to the existing +15.2 dB
  parameter. Directional order controls, parameter automation, setup visibility,
  presets and proportional resize are checked.
- UI-preview.png, UI-native-preview.png and UI-setup-preview.png are rendered
  from the actual JUCE editor with a 997 Hz test stimulus, not a design mockup.
  Visual inspection caught and fixed UTF-8 minus rendering and the nonuniform
  synchronization scale mapping. No physical mouse/DAW session is simulated by
  the binding test.
- Windows/macOS packages are gated by native CI build, CTest, Windows DLL probe
  and ordinal-345 regression, Universal architecture/signature checks and auval.
  See the v0.4.0 tag run for platform-specific outcomes.

## 0.5.0 SignalTrain mapping integration (2026-11-16)

- Three CTest suites pass: models, processor integration, editor bindings.
- Sparse mapping is monotone/finite over the full panel range; PR=0 has no
  reference trim. At all three measured anchors, integrated output matches the
  offline effective-PR + post-colour-trim placement and GR excludes reference trim.
- New mode is smoothed in the oversampled domain and tested during automation;
  new-state roundtrip preserves it, and old states migrate to the original mode.
- Actual editor snapshots show the SETUP toggle without overlapping other controls.
- Native 44.1 kHz data is rendered through the production optical core and JUCE
  FIR at 2x/4x, with 90 s preconditioning and a separate 60 s evaluation window.
  All three measured operating points improve envelope error. Full measured
  values and protocol limits: Documentation/Calibration/INTEGRATION-0.5.0.md.
- No constants in the 1176 DSP are changed; no external plugin comparison is
  used by this release. No process-time allocations or locks were introduced.
- Windows load/processing/manifest regression and macOS Universal/AU validation
  are required by the v0.5.0 tag release workflow.
