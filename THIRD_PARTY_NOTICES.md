# Third-party notices

LAssassin-76 project code is distributed under AGPL-3.0-or-later. See LICENSE.

- JUCE 8.0.6: https://github.com/juce-framework/JUCE/tree/8.0.6 — used under its AGPLv3 option. CMake fetches this exact tag. JUCE carries additional notices for its bundled dependencies; see the linked source tree and module headers.
- Steinberg VST3 SDK: bundled in JUCE, with its license at modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt (or the corresponding LICENSE file in that directory).
- Cinzel Decorative Bold and Zilla Slab Bold: SIL OFL 1.1; see Assets/Fonts.

Corresponding project source, build scripts and release tags: https://github.com/yunm4833-design/LAssassin-76 . Each binary release links to its matching source tag; the exact JUCE dependency remains available at the pinned source URL above. No Windows SDK, MSVC CRT development libraries, or Apple SDK files are redistributed in the source repository.

- Paulllux/fetcomp-dsp: MIT, Copyright (c) 2026 Paul Ulrix. Adapted JFET/Jiles-Atherton sections are identified in Source/DSP/PhysicalStages.h; see ThirdParty/fetcomp-dsp/LICENSE and README.md for the exact revision and changes.

- Jun-Murakami/ZeroComp: AGPL-3.0-or-later, Copyright (C) 2026 Jun Murakami. Adapted LDR heat architecture is identified in Source/DSP/OpticalMemory.h; see ThirdParty/ZeroComp/LICENSE and README.md for source revision and modifications.
- Photorealistic UI backplate and knob atlas: AI-assisted assets derived from the user-supplied reference. Included under the project license to the extent applicable; see Assets/Skin/README.md.
