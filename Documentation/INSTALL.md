# Install LAssassin-76

## Windows 10/11 x64 — VST3

Exit the DAW. Replace the entire old `LAssassin-76.vst3` folder in
`C:\Program Files\Common Files\VST3\` with the folder from this archive.
Keep `Contents/x86_64-win/LAssassin-76.vst3` inside the outer bundle.
In Ableton Live, hold **left Alt** while clicking **Rescan** after replacing an older version.
Use a stereo audio track or bus. The ordinal-345 scanner fix is retained.

## macOS 11+ — Apple Silicon and Intel

Universal VST3: copy `LAssassin-76.vst3` into `~/Library/Audio/Plug-Ins/VST3/`.
Universal AU: copy `LAssassin-76.component` into `~/Library/Audio/Plug-Ins/Components/`.
Exit and restart the DAW, then rescan its plugin list. Logic uses the AU version.

These are **ad-hoc signed testing builds, not Apple Developer ID signed or notarized**.
macOS may require explicit approval via its security prompts. Do not disable Gatekeeper globally.
For broadly distributed production builds, Developer ID signing and notarization are still needed.

## Controls

Drag knobs vertically, double-click to reset, or type into a value box.
The embedded Cinzel Decorative Bold / Zilla Slab Bold fonts are shared across both platforms.
Stereo-only I/O; the DSP is an original behavior model awaiting hardware calibration.

## License and source

AGPL-3.0-or-later; fonts retain SIL OFL 1.1. Corresponding source is linked in SOURCE.txt.
