# ZeroComp attribution and adaptation

Source: https://github.com/Jun-Murakami/ZeroComp
Revision: `3ddae24179585486c8a9f06b39d246a7ae9e817f`
File: `plugin/src/dsp/Compressor.cpp`, Opto envelope / LDR heat section.
Copyright (C) 2026 Jun Murakami. SPDX: AGPL-3.0-or-later. License copied here.

`Source/DSP/OpticalMemory.h` adapts the realized fast-envelope-driven heat target
(`envelopeDb / 18`), separate heat/cool updates (1 s / 3 s), and slow envelope
whose recovery depends on accumulated heat. This gives the optical unit actual
open-source-derived envelope architecture, without importing the unrelated
compressor UI, user timing controls, or FET mode.

Changes in LAssassin-76: double precision and stable expm1 integration; fixed
10 ms attack; equal-weight fast and slow states so approximately half the gain
reduction releases in 60 ms; slow time constant interpolated in seconds from
1 to 15 s instead of interpolating decay coefficients. Upstream uses max of
fast/slow states, which would remove the requested fast half-release, so that
combiner is deliberately different. Finite-state guards and deterministic reset
are included. There is no new dependency or process-time allocation.

This remains a behavioral model. ZeroComp itself describes the heat mechanism
as an approximation, not a measured T4B component fit. No hardware-equivalence
claim is implied by this adaptation.
