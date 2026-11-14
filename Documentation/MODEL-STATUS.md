# Model status and calibration boundary

The implementation matches the requested control layout and serial gain structure. It is a behavioral model with attributed open-source sections, **not a verified perfect reproduction of a particular LA-2A or 1176 hardware revision**. A model name or an open-source implementation does not establish hardware accuracy.

## Audited and improved in 0.3.0

- T4 fast/slow/history states now use double precision and an `expm1` step. This avoids cancellation in `1-exp(-dt/tau)` and rounding of long release poles at oversampled rates.
- FET feedback reduction uses the same stable step computation; the local feedback slope still controls numerical stability at high ratios and 20 us timing.
- Input/output gain factors shared by the two FET channels are calculated once per frame.
- Parameter snapshots reject nonfinite values and clamp to cached native parameter bounds without string lookups, allocation or locks on the audio thread.
- Nonfinite upstream audio becomes zero; pathological inputs above +36 dBFS are bounded before stateful processing. Ordinary floating-point headroom is preserved.
- Unexpected channel layouts return safely instead of indexing outside a buffer.
- Both platform builds retain the native Windows scanner regression, including the Common Controls v6 dependency and delayed COMCTL32 imports.

- Host bypass now shares the same latency-aligned dry path and a 20 ms transition; the first bypassed block is immediately dry. The default JUCE bypass had no latency compensation.
- JFET root cancellation, passive branch behavior and magnetic insertion gain have dedicated regression checks.

## Objective checks

- T4 memory recovery after 0.1 s vs 8 s sustained compression.
- Long-release agreement at 44.1 / 96 / 192 / 768 kHz within 0.01 dB for the defined stimulus.
- Internal T4 10 ms attack reaches `1-exp(-1)` of a constant target within the specified tolerance.
- Input increases FET GR; FET Output does not return into its own feedback detector.
- LA makeup gain changes the following FET's GR.
- Linked channels share GR; All/fast timings remain finite across tested rates.
- Mix=0 latency alignment; state restore; automation; large-block splitting; ordinary C++ allocation monitoring.
- NaN, infinity, extreme samples and invalid raw parameters do not poison later processing.

These validate the implementation and its defined behavior, not hardware equivalence. Nominal attack/release constants are not interchangeable with an entire unit's measured 10–90% response.

## External models reviewed

- [betodealmeida/compressor1176](https://github.com/betodealmeida/compressor1176), GPLv3: reviewed as a comparison implementation. Its published code uses feed-forward detection, so it was not adopted for this project's required feedback topology. No code copied.
- [pwing93/WingComp](https://github.com/pwing93/WingComp): reviewed the LA-2A-based project description; no explicit repository license was present in the root listing, so no source was imported.

After the user supplied additional sources, the JFET divider and Jiles-Atherton sections from Paulllux/fetcomp-dsp were adapted under MIT. Full provenance and the exact revision are in ThirdParty/fetcomp-dsp. The outer detector/control, routing and parameter layer remain project implementations. In 0.4.0 the optical envelope also adapts ZeroComp’s LDR heat architecture; provenance is in ThirdParty/ZeroComp. No nested upstream oversampling or +5.8 dB calibration trim is imported.

## Hardware-calibrated reproduction still requires

A specified hardware revision and reference level; recorded input/output and GR data for stepped levels, 0.1–10 s bursts and recovery; ratio/knee curves; frequency response; H2/H3/IMD vs drive and GR; All-mode transient behavior; and level-matched listening against the reference unit.

Current transformer color includes a gentle blend of a voltage-driven Jiles-Atherton flux/history core adapted from the cited MIT source. It is not a model identified against a specific measured transformer. The FET cell uses a closed-form shunt-JFET divider with passive branch limits. Amplifier/tube color still uses normalized asymmetric saturation, not identified complete transistor/tube circuits. The T4 memory is phenomenological. Attack/release, gain-pot calibration and All-mode coefficients remain calibration constants. These limits are explicit rather than hidden behind a “perfect clone” claim.


## 0.4.0 optical and interface updates

`OpticalMemory.h` isolates fast/slow envelope and heat history. Heat follows the
actual fast GR envelope normalized by 18 dB (ZeroComp architecture), charging
with 1 s and cooling with 3 s. The two T4 branches are still averaged, not maxed,
and fixed 10 ms attack / fast 60 ms T90 / slow 1–15 s timing is retained.
Double precision and expm1 avoid long-pole rounding at high oversampled rates.

Regression checks cover approximately half recovery at 60 ms, gradual heating
and cooling, one-transient heat rejection, finite/reset behavior, duration-
dependent recovery and sample-rate invariance. The new UI/host global bypass is
checked for state persistence and impulse delay alignment. Editor tests cover
numeric Gain scale conversion, directional order buttons and the SETUP drawer.

The skin now uses the supplied reference backplate composition and generated
material sprites with live controls. Fixed artwork is raster; numerical values,
needles, lighting and switches are dynamic. Native-reference-size and default
window screenshots are generated by the actual editor test, not a mock-up.
