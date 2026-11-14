# Model status and calibration boundary

The implementation matches the requested control layout and serial gain structure. It is an original behavior model, **not a verified perfect reproduction of a particular LA-2A or 1176 hardware revision**. A model name or an open-source implementation does not establish hardware accuracy.

## Audited and improved in 0.3.0

- T4 fast/slow/history states now use double precision and an `expm1` step. This avoids cancellation in `1-exp(-dt/tau)` and rounding of long release poles at oversampled rates.
- FET feedback reduction uses the same stable step computation; the local feedback slope still controls numerical stability at high ratios and 20 us timing.
- Input/output gain factors shared by the two FET channels are calculated once per frame.
- Parameter snapshots reject nonfinite values and clamp to cached native parameter bounds without string lookups, allocation or locks on the audio thread.
- Nonfinite upstream audio becomes zero; pathological inputs above +36 dBFS are bounded before stateful processing. Ordinary floating-point headroom is preserved.
- Unexpected channel layouts return safely instead of indexing outside a buffer.
- Both platform builds retain the native Windows scanner regression, including the Common Controls v6 dependency and delayed COMCTL32 imports.

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

No third-party DSP code was added in this audit. The project remains independently implemented under the selected AGPL license.

## Hardware-calibrated reproduction still requires

A specified hardware revision and reference level; recorded input/output and GR data for stepped levels, 0.1–10 s bursts and recovery; ratio/knee curves; frequency response; H2/H3/IMD vs drive and GR; All-mode transient behavior; and level-matched listening against the reference unit.

Current transformer color is a low-band saturation residual, not a hysteresis/circuit solver. The tube/FET curves are normalized asymmetric saturation, not an identified transistor/tube circuit. The T4 memory is phenomenological. Attack/release, gain-pot calibration and All-mode coefficients remain calibration constants. These limits are explicit rather than hidden behind a “perfect clone” claim.
