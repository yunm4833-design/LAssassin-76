Windows x64 VST3 and macOS Universal VST3/AU testing release.

- Larger, heavier retro typography: embedded Cinzel Decorative Bold titles and Zilla Slab Bold controls/readouts.
- Native Windows load/processing/parameter test and ordinal-345 regression gate.
- macOS Universal architecture checks, ad-hoc signing, and AU validation.
- Same original optical/FET serial dynamics, parameter IDs, presets and UI controls.

Windows: replace the entire plugin folder and rescan. macOS builds support Intel and Apple Silicon on macOS 11+, are ad-hoc signed, and are not Apple-notarized.

Source and build instructions: https://github.com/yunm4833-design/LAssassin-76 . Project AGPL-3.0-or-later; fonts SIL OFL 1.1. See the included INSTALL.md and license notices.

Robustness audit: double-precision/expm1 envelopes, long-release sample-rate regression, nonfinite input/parameter protection, and malformed-channel safety. See Documentation/MODEL-STATUS.md for measured checks and hardware-calibration limits.

Visual status: larger embedded retro typography is included. Pixel-level matching of the original generated concept requires its original PNG asset; the current vector skin remains the development baseline.
