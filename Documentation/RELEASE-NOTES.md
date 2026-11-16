LAssassin-76 0.5.0 — SignalTrain optical mapping test release.

- 2A now includes the measured SignalTrain operating-point mapping and post-colour output reference trim at Compress/65, Limit/65 and Limit/80.
- New SETUP → 2A SIGNALTRAIN switch: on for new instances, off automatically for saved sessions predating this parameter. Turn off to compare the original mapping.
- Other knob positions use monotone piecewise interpolation with engineering endpoints; they are not individually hardware-calibrated. Attack, two-part release and heat-memory constants remain as before.
- Reference trim is part of the serial audio gain structure and correctly drives the following compressor; GR meters show attenuation only. Parameter changes are smoothed.
- Existing parameter IDs/units, Windows scanner fix, native x64 load validation, macOS Universal VST3/AU packaging and signing checks are retained.

The historical offline report and the integrated 2x/4x optical-stage validation have distinct protocols; see Documentation/Calibration. Hardware-equivalence percentages are not claimed. The 1176 DSP is unchanged.

Windows: replace the whole VST3 bundle, then rescan. macOS Universal supports Intel and Apple Silicon on macOS 11+; testing packages are ad-hoc signed, not Apple-notarized.

Source: https://github.com/yunm4833-design/LAssassin-76/tree/v0.5.0 . Project/ZeroComp section AGPL-3.0-or-later; fetcomp sections MIT; fonts SIL OFL 1.1. Included notices identify all source adaptations.
