Windows x64 VST3 and macOS Universal VST3/AU testing release.

- Photorealistic parchment / black-steel skin based on the user's original reference: stitched leather, brass, feather VU, textured knobs, live editable readouts, animated order controls, and glowing blade ratio buttons.
- SETUP opens presets, Stereo/Mid-Side, Link, Auto Gain, oversampling and sidechain HPF without changing the main reference layout.
- LA-2A now adapts ZeroComp's AGPL LDR heat architecture: realized envelope drives 1 s charge / 3 s cooling; fixed 10 ms attack, approximately half-release at 60 ms, and a history-dependent 1–15 s slow branch remain.
- Global BYPASS is saved/automatable and shares the host's latency-aligned smoothed bypass path. Stage bypasses remain independent.
- LA Gain faceplate reads 0–100 to match the concept; host parameter remains 0–40 dB, so existing automation retains its meaning (38 on the panel = 15.2 dB).
- Existing MIT JFET/magnetic stages, Windows scanner ordinal-345 regression fix, and native platform validation gates are retained.

The reference composition/materials are preserved using an edited backplate and reconstructed live widgets; this is not a pixel-identical static screenshot. This release changes optical recovery from 0.3.0; saved parameters are compatible but rendered audio can differ. Hardware sound equivalence is not established.

Windows: replace the entire plugin folder, then rescan. macOS: Intel + Apple Silicon, macOS 11+, ad-hoc signed, not Apple-notarized. See included INSTALL.md.

Source: https://github.com/yunm4833-design/LAssassin-76/tree/v0.4.0 . Project and adapted ZeroComp section AGPL-3.0-or-later; fetcomp sections MIT; fonts SIL OFL 1.1. Complete attribution and model decisions are included in the source.
