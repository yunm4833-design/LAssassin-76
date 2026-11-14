# Photorealistic skin assets

`Backplate.png` (1738 × 905) is an AI-assisted clean backplate derived from the
reference concept image supplied by the user on 2026-11-14. Moving controls,
numeric values and illumination were removed for live JUCE rendering.
`Knobs.png` is a transparent four-quadrant material atlas generated to match the
same concept. The original generated files are retained outside this source tree.

The plugin embeds both images; no image files need to be installed separately.
Code in PluginEditor.cpp defines the reference coordinates, knob mapping,
feather pointer, physical switches, blade buttons, readouts, metering and glow.
The skin preserves the reference composition/materials, but moving widgets are
reconstructed and additional functional controls are included; it is not a
pixel-identical copy of the static screenshot in every parameter state.

Assets are provided with the project under AGPL-3.0-or-later to the extent
copyright applies. No official game logos, Ubisoft marks or game textures are
included. The title and fixed artwork are raster assets; live text uses the
bundled OFL fonts. For maximum detail render at native 1738 × 905 or below.
