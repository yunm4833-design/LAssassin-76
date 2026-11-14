# Supplied model sources and integration decisions

| Source | Verified implementation / license | Decision |
|---|---|---|
| [Paulllux/fetcomp-dsp](https://github.com/Paulllux/fetcomp-dsp) | MIT; circuit-derived shunt JFET, feedback network, Jiles-Atherton core; upstream notes reference-plugin calibration | Adapt the cell and core, retain attribution and source revision; use stable double solver and existing outer OS |
| [yonie/WetCompressor](https://github.com/yonie/WetCompressor) | MIT; standard-C++ FET cell, multistage color and two-node sidechain | Reviewed topology and detector stability; no source imported |
| [Faust compressors.lib](https://github.com/grame-cncm/faustlibraries/blob/master/compressors.lib) | Per-module licenses; 1176_R4 declares STK-4.3 and aliases compressor_stereo(4,-6,0.0008,0.5) | Useful generic limiter baseline, not the circuit implementation needed here |
| [aim-qmul/4a2a](https://github.com/aim-qmul/4a2a) | MPL-2.0; learned mapping from Peak Reduction 40–100 to feed-forward parameters, Newton fitting offline | Calibration reference; not substituted for the required two-part T4 memory or extended 0–100 range |
| [csteinmetz1/neural-2a](https://github.com/csteinmetz1/neural-2a) | README permits its author's code; JUCE/LibTorch have separate licenses; source build notes x86 and custom M1 builds | Do not add a large inference dependency without validating model weights and real-time behavior |
| [Jun-Murakami/ZeroComp](https://github.com/Jun-Murakami/ZeroComp) | AGPLv3; Opto mode explicitly describes its LDR heat as a rough approximation | Adapted realized-envelope-driven heat (1 s charge / 3 s cooling) in OpticalMemory.h; retain double precision, fixed T4 timing, and equal fast/slow weighting; source/license in ThirdParty/ZeroComp |
| [DiffAPF/LA-2A](https://github.com/DiffAPF/LA-2A) | MPL-2.0; differentiable **all-pole** filter/compressor training, SignalTrain and learned YAML parameters | Training/calibration reference, not a drop-in C++ circuit or runtime Newton solver |

These repositories are not all interchangeable LA-2A/1176 hardware models. No hardware-equivalence claim follows merely from their names. Current measurable behavior and remaining calibration limits are documented in MODEL-STATUS.md.
