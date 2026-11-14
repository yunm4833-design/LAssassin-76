# LAssassin-76

C++17 / JUCE 8 双级压缩器行为模型，包含项目实现及 MIT 授权的 JFET／磁滞核心，VST3 / Standalone，以及 macOS 上的 AU。

这是可操作、可测量、可进一步校准的 DSP 原型。没有复制商业插件代码，也没有使用游戏素材。它不是基于某台实机逐元件测量的电路仿真；绝对旋钮位置、THD 和 All 模式都需要用参考硬件校准。v0.2.0 已将视觉概念实现为可交互 JUCE 界面：象牙白光电区、黑钢 FET 区、羽毛 VU 指针、黄铜滚花旋钮、刀锋比例按钮、红色 All 状态、中央 Order 切换和刀光动画、全局同步衰减条。背景纹理和装饰由原创矢量/程序化绘制生成，未使用官方素材。所有参数均有实际绑定；支持旋钮双击复位、数值编辑、悬停提示和等比例缩放。

## 工程结构

```
CMakeLists.txt
Source/
  PluginProcessor.h/.cpp   APVTS、状态保存、预设、全局增益、并联与电平表
  PluginEditor.h/.cpp      可操作编辑器、预设下拉框、GR 与输入输出显示
  DSP/
    Dynamics.h            OpticalCompressor、FETCompressor、检测与染色模块
    Engine.h              双 Order 路径、2x/4x 引擎、M/S 与延迟对齐
 tests/
  dsp_tests.cpp            不依赖 JUCE 的模型测试
  integration_tests.cpp    完整 AudioProcessor 集成测试
```

## 构建

安装 C++17 编译器、CMake >= 3.22、Git；首次配置会拉取固定 JUCE 8.0.6。也可传 `-DJUCE_PATH=/absolute/path/to/JUCE` 使用本地副本。商业分发前按实际用途确认 JUCE 许可证。

Linux 开发依赖可安装：

```sh
sudo apt-get install build-essential cmake ninja-build pkg-config \
  libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

macOS：使用 Xcode command-line tools，执行同样的 CMake 命令（可省略 `-G Ninja`）；自动加入 AU 格式。Windows：使用 VS 2022 的 CMake 工具或 `-G "Visual Studio 17 2022" -A x64`，构建时加 `--config Release`。AU 只能在 macOS 构建和验证。

产物位于 `build/LAssassin76_artefacts/Release/` 的 VST3、Standalone、AU 子目录。不会自动安装到系统插件目录。

独立模型测试可不安装 JUCE：

```sh
g++ -std=c++17 -O2 -I Source tests/dsp_tests.cpp -o dsp_tests
./dsp_tests
```

## 信号流与电平约定

```
                         ┌─1176 → LA-2A─┐
原始输入 → In → M/S编码 → │             │ → M/S解码 → Out → Auto Gain → wet
                         └─LA-2A → 1176─┘
原始输入 → 与wet相同的延迟 → dry
输出 = dry × (1-Mix) + wet × Mix
```

两条串联路径各自保存独立状态，持续运行，Order 用 20 ms 交叉淡化切换。两种 OS 引擎也持续运行且延迟一致，因此 OS 切换不改变宿主延迟。代价是较高 CPU 消耗：两种采样率、两种顺序都在计算，而非只计算当前选项。所有非线性均在过采样域内，In/Out/Mix 和长期 Auto Gain 在基础采样率处理。

干声取自全局 In 之前：Mix=0 是延迟补偿后的原始输入；In 和 Out 只作用于湿支路。Mix 使用线性混合，相关信号在中点不会额外提升约 3 dB。宿主获得的固定延迟是两个 FIR 过采样引擎延迟的最大值。较短湿路径再补齐整数延迟，干声补同样的延迟。FIR 的幅频响应仍可能使极高频干湿叠加与理想直通略有差异。

1.0 数字幅值对应 0 dBFS；无隐式模拟电压标定。1176 固定阈值为 -18 dBFS 的**反馈检测节点**电平。Input 直接驱动该单元，Output 不返回检测环路。LA-2A Gain 位于其衰减器之后，进入管级非线性，再进入下一级。没有级间归一化或隐藏的逐级自动补偿，所以前一级 makeup 会真实改变后一级压缩和饱和。

独立 Bypass 平滑交叉淡化到该单元的输入，仍更新被旁路单元的状态。旁路包含该单元的增益、压缩与染色，不包含全局 In/Out。由于状态继续运行，重新启用时不从零重新建立光电记忆。

Stereo Link 使用左右检测信号的最大值，完全链接时两声道共享 GR，但保留各自音频染色状态。关闭时两声道独立。M/S 模式使用单位正交矩阵：M=(L+R)/sqrt(2)，S=(-L+R)/sqrt(2)，处理后反变换。S 的极性约定在反变换中抵消。切换过程平滑插值矩阵旋转角。M/S + Link 开启时 M 和 S 共享 GR；关闭时分别压缩。

两个模块各有独立的侧链 HPF 状态，共享一个 0–300 Hz 控件；0 Hz 为关闭。HPF 只影响检测，音频低频不被高通。当前为一阶 6 dB/oct 模型。

## T4 光电模型

`OpticalCompressor` 的每个声道保存快光敏状态 F、慢光敏状态 S、历史状态 M。先把检测幅度转换成 dB，再经过 6 dB 宽软膝，计算目标衰减 G。

```
threshold = -6 - 0.54 × PeakReduction  [dBFS]
Compress ratio = 3:1
Limit ratio = 100:1
F ← G + aF × (F-G)
S ← G + aS × (S-G)
GR = 0.5 F + 0.5 S
```

F 和 S 上升时均用约 10 ms 指数时间常数。恢复时，F 的 T90 是 60 ms，即快支路 60 ms 恢复约 90%，对应总 GR 快速恢复约 45%，随后主要由 S 控制。这里区分 T90 和时间常数：快支路 tau=60/ln(10) ms，并不是 60 ms tau。

0.4.0 起，`OpticalMemory.h` 适配 ZeroComp（AGPL-3.0-or-later）的 LDR 热记忆架构：M 追踪实际快包络 clamp(F/18,0,1)，蓄热 tau=1 s，冷却 tau=3 s。短暂尖峰先经过快包络，不能立即把记忆状态加热。慢支路的恢复 tau=1+14M 秒。因此深压缩和长时间压缩会积累更大的 M，使后续尾巴更长；M 在恢复过程中逐步下降。Peak Reduction=0 禁止生成新的 GR，已有 T4 状态仍按释放恢复；需要立即平滑退出整个单元时使用 Bypass。

HF Emphasis 在检测支路约 2.2 kHz 以上增强高频，最高渐近增益约 9.5 dB，可让齿音更多触发压缩，主音频不直接加高频。

管级采用小信号斜率归一化的不对称饱和：

```
y = [tanh(d(x+b)) - tanh(db)] / [d(1-tanh(db)^2)]
```

b 控制偶次成分，d 控制驱动。后接 5 Hz DC blocker 去除动态直流，以及 18 kHz 一阶低通。0.3.0 起，变压器使用从 fetcomp-dsp（MIT）适配的电压驱动 Jiles–Atherton 磁滞核心，以轻微比例混入信号。保留场强／磁化历史和泄漏低通，未引入原上游的固定增益修整；没有针对某一实测铁芯识别全部物理参数。

## 1176 反馈 FET 模型

检测取前一个过采样采样点的衰减后信号，位置在 FET 失真与 Output 之前。因果延迟避免隐式代数环。**不能直接给反馈信号套前馈的斜率**：本实现用 (R-1) 的反馈控制斜率，使静态平衡近似满足：

```
g = (R-1) × (input_dB - g - threshold)
g = (1 - 1/R) × (input_dB - threshold)
```

离散控制更新还除以当前反馈环的局部斜率（1+控制曲线斜率），避免 20 us、高比例、一采样延迟组合导致显式递推的采样级振荡；这也让旋钮时间更接近闭环包络时间，而非未归一化开环时间。

2 dB 软硬过渡膝比光电单元硬。Input 控制输入电平，没有阈值旋钮；比例为 4、8、12、20，All 单独改变模式。控制量有 60 dB 数值上限。

Attack 与 Release 采用对数映射，数值越大越快：

```
attack = 800us × (20us/800us)^((knob-1)/6)
release = 1.1s × (0.050s/1.1s)^((knob-1)/6)
```

这些是内部指数状态的名义时间常数，不是实测总系统 10–90% 响应；反馈、膝和节目都会改变实测时间。深压缩最多把释放拉长约 65%。All 使用约 35:1、额外降低 4 dB 阈值、4 dB 膝宽、约 3.5 倍初始起音时间常数、0.65 倍释放系数和更多失真，让瞬态穿透后快速受控，形成攻击性和过冲感。它是经典行为的原创近似，尚非对某台硬件 All 模式的复刻。

FET 非线性也采用上述不对称函数，但驱动与偏置随 GR 增长：JFET 分流器由闭式二次方程求解；其后 Class A 级 d=0.14+0.4×depth+0.8×All，b=-0.18，depth=clamp(GR/24,0,1)。零偏置的 tanh 含奇次成分，非零偏置再加入偶次；深压缩会增加两者。输入与输出各有轻微变压器/宽带饱和与高频滚降。

## 自动增益与表头

Auto Gain 比较延迟对齐的原始干声和 Out 之后湿声的双声道平均功率。能量平滑 tau=400 ms，补偿增益再平滑 tau=1.5 s，范围 ±12 dB，低于检测门限时冻结。关闭 Auto Gain 时把应用量在 200 ms 内退到零；后台估计仍运行以减少打开时等待。它是长期 RMS 能量匹配，不是 LUFS 匹配，也不是按瞬时 GR 反向补偿，仍可能改变宏观动态。

GR 显示检测模型的衰减，不把饱和、In/Out 或 Auto Gain 当作 GR。每个块选择总 GR 最大的那个声道和采样时刻，显示此刻的两级 GR，因此 `Total=LA+FET`（dB）始终成立。非链接时不把不同声道的单级峰值相加。交叉淡化期间是两种路径的显示插值；淡化输出本身不严格等价于单一 dB 衰减器。

输入/输出是块内 sample-peak dBFS，不是 true peak。当前 UI 60 Hz 读取原子表值，使用独立的视觉起落包络和青蓝辉光；尚未增加音频线程峰值保持，可能漏读两个 UI 帧之间的短暂峰值。

## 参数与预设

所有用户参数在 APVTS 注册，支持宿主自动化和 XML/ValueTree 状态恢复。Ratio 是五选一参数，All 是其内部派生状态，不额外暴露一个可能冲突的 All 开关。

- 全局：In/Out ±24 dB，Mix 0–100%，Order，2x/4x，Link，Stereo/Mid-Side，HPF 0–300 Hz，Auto Gain。
- LA-2A：Bypass、Peak Reduction 0–100、Gain 0–40 dB、Compress/Limit、HF Emphasis。
- 1176：Bypass、Input/Output ±24 dB、Attack/Release 1–7、4/8/12/20/All。
- 预设：Default Vocal、Rap Forward、Pop Level、Podcast、Parallel Edge。均为起点，实际 GR 依赖输入电平；没有加载用户外部文件的预设浏览器，DAW 状态可完整保存。

LA-2A Gain 采用明确的 dB 显示，而非硬件 0–100 面板映射，便于检查级间增益；需要原始面板体验时，可以在 Editor 将同一增益参数映射到非线性的 0–100 显示，不改变参数 ID。

## 实时安全

- `prepareToPlay` 分配过采样器、工作缓冲与延迟环；`processBlock` 不 resize、不创建 vector、不读文件、不调用 APVTS copyState、不获取锁。
- 参数原子指针在构造时缓存，音频块只做原子读取。动态连续参数、旁路、顺序、比例和模式均做 20 ms 平滑；Auto Gain 开关 200 ms。未使用音频线程监听器回调或异步 UI 调用。
- 超出 prepare 时最大 block size 的输入分块处理；不会因此在回调中扩容。
- `ScopedNoDenormals` 防止长释放状态引起非正规数性能下降。
- UI 只读取原子表值。状态保存/恢复、预设批量应用和编辑器创建是非实时操作，调用方须在合适的宿主/消息线程执行；不宣称整个插件生命周期都无锁。
- 两种 OS 和两种 Order 始终计算，换取无冷启动状态的自动化。适合原型验证；上线前应测低缓冲 CPU，并缓存固定系数/减少逐采样 exp、log、pow 调用。
- 测试中的分配计数覆盖此线程的普通 C++ new/new[]，不覆盖 malloc、对齐 new 或系统调用；结合代码审阅检查无锁，不将其当作形式证明。

## 校准：先测量，再等响度听辨

1. **静态曲线**：关闭 Auto Gain，Mix=100%，旁路另一级，关闭 HF/HPF。从 -60 到 -3 dBFS 输入 1 kHz 正弦，每档至少保持 10–20 s；光电尾巴很长，必要时每档重置实例。输出电平与 GR 分别记录，先调整阈值映射、膝宽和比例，再调染色。FET 的输入变压器会在高电平偏离理想比例，应同时查看内部 GR 与实际输入/输出曲线。
2. **起音/释放**：使用 1 kHz 正弦幅度阶跃和带限噪声突发，采样记录包络。分别测 10–90%、63% 或 T90，明确指标，不能把旋钮名义 tau 当作系统实测时间。T4 用 0.1、1、5、10 秒突发比较 60 ms、1 s、5 s、15 s 后残余 GR，调整 .5/.5 权重、10 ms、快支路 T90、记忆充放电与 1–15 s 范围。
3. **FET 比例与过冲**：分别测四种比例在稳态的斜率，检查 20 us 起音在最低支持采样率/2x 时的数值稳定性。All 用鼓与爆破辅音检查瞬态峰值、恢复速度与尾巴毛刺；调整其阈值偏移、攻击倍数、释放系数、反馈检测位置与饱和驱动，避免仅凭音量判断“更有攻击性”。
4. **谐波与频响**：用 100 Hz、1 kHz 正弦从小信号扫到较高电平，FFT 检查 H2/H3/THD 与 GR 的关系。调管级偏置 .12、drive 1.05；FET 的 depth 系数；变压器残差 .08、140 Hz、drive 1.7；高频极点 18/21/22 kHz。`asymmetric` 的归一化保证小信号斜率约 1，但不保证等 RMS，必须做等响度比较。
5. **混叠**：在 44.1/48 kHz 下用高频正弦/双音、强驱动、All 和高 LA Gain 测 2x/4x，比较非谐波折返。4x 不能彻底消除极端饱和混叠；需要更高 OS 或带限非线性时应另行设计。
6. **串联**：固定第一级旋钮，只增加其 Output/Gain，确认第二级 GR 增长；交换 Order 后保留各旋钮，验证次序差异。对照实机时匹配进入每一级的参考电平，不能只匹配最终输出。
7. **并联与路由**：两级 Bypass、In/Out=0、Auto Gain 关，Mix 从 0 扫到 100%，检查延迟和频响；分别测 L-only、同相、反相输入及 M/S 编解码。检测器 HPF 与 HF Emphasis 应改变 GR 而不直接 EQ 干声。
8. **听辨**：按人声爆破音、持续元音、气声齿音、说唱短句、播客底噪依次听，等响度 A/B；听清字头、呼吸回升、尾字自然度和声像稳定性。先用 1176 抓约 2–5 dB 瞬态、LA 平约 1–3 dB 起步，再按素材调节。

## 当前验证边界

独立测试覆盖 T4 历史依赖、Input 驱动、链接与多个采样率下有限输出。集成测试检查固定延迟的干声脉冲、超大块分割、Order/OS/M-S/Link/Auto/Bypass 自动化、状态恢复与回调内 C++ 堆分配。它们不替代 DAW 宿主验证、插件验证器、真实硬件对照、别名频谱测试或盲听。具体本次构建结果见 `VALIDATION.md`。

## Windows v0.2.0 安装

Windows 10/11、64 位 VST3 宿主。将压缩包里的整个 `LAssassin-76.vst3` 文件夹复制到 `C:\Program Files\Common Files\VST3\`，保留 `Contents/x86_64-win/` 目录结构。重启宿主并重新扫描 VST3。不要只复制最内层的二进制文件。

界面旋钮上下拖动调整，双击复位，数值框支持键盘输入。Ratio 五个按钮互斥。Compress/Limit 关闭为 Compress，开启为 Limit。Order 中的文字表示顺序；独立 Bypass 包含该单元的增益和染色。

Windows 本机构建可安装 VS 2022 C++ 桌面开发与 CMake，然后执行 `scripts/build-windows.ps1`。交叉构建使用 `cmake/clang-cl-xwin.cmake`，需提供 Windows SDK/MSVC CRT 和已安装的宿主 JUCE 包；MinGW 不受 JUCE 8 支持。

## v0.2.1 扫描修复

修复 Windows 序数 345 加载失败：显式嵌入 DLL 清单以使用 Common Controls v6，并延迟加载 COMCTL32。`tests/check_windows_bundle.py` 防止此问题回归。压缩包附独立的 Windows VST3 加载诊断程序与脚本。

0.3.0 宿主 Bypass 与内部干声共享延迟补偿，并做 20 ms 淡化；第一次回调即旁路时直接输出延迟对齐的干声。新增开源模型来源审查见 Documentation/OPEN-SOURCE-MODELS.md。


## 0.4.0 写实交互皮肤

界面使用用户重新提供的 1738×905 概念图制作底板，再叠加真实可操作的材质旋钮、金色指针、羽毛 VU、拨杆和比例按钮。默认窗口为 1390×724，可等比例缩放；没有外部皮肤安装步骤。双击数值框输入，旋钮垂直拖动，双击旋钮恢复默认值。

LA Gain 面板按原图显示 0–100，和宿主保留的 0–40 dB 参数线性对应：面板 38 即 +15.2 dB。这个映射是本插件显示刻度，不是实测真机电位器曲线。Input/Output 与全局 In/Out 保持 dB。主面板顶部 BYPASS 与宿主旁路参数共用保存、自动化和延迟对齐；每级 ON/OFF 独立旁路包含本级增益、压缩和染色。

SETUP 展开预设、Link、Stereo/Mid-Side、Auto Gain、Sidechain HPF 和 2x/4x。它们仍然支持原参数自动化。输入输出电平在底部数值框下显示，LA GR 在 VU 内、FET GR 在 Attack/Release 中间，总 GR 在同步条右侧。

静态底板忠于参考构图与材质，动态控件是重建的，不能把运行界面宣称为与静态参考逐像素一致。光电记忆算法更新会改变声音，建议用副本工程对照 0.3.0；旧参数 ID 和单位不变。
