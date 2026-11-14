# Adapted physical sections

Source: https://github.com/Paulllux/fetcomp-dsp
Pinned revision: `de18f5ac793e36397c725abdca7fcb8c08760ce2`
License: MIT, Copyright (c) 2026 Paul Ulrix; full text in LICENSE.

Source/DSP/PhysicalStages.h adapts the closed-form JFET divider and the voltage-driven Jiles-Atherton core from FetLimiterDsp.h. It uses std::tanh rather than MathUtils.h, so the polynomial approximation implementation is not imported.

Changes: rationalized double-precision root; passive branch limits; double field/history states; bounded incremental permeability; removal of the upstream +5.8 dB calibration trim. Existing LAssassin feedback control and outer 2x/4x oversampling remain responsible for timing and anti-aliasing. No nested oversampling or hidden gain normalization is introduced.

Upstream states that its current calibration targets a reference plugin, not a directly measured hardware core. This adaptation retains that accuracy limitation.
