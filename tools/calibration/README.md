# SignalTrain optical-core calibration

This is an offline, bounded operating-point calibration of the actual production
`OpticalCompressor` in v0.4.0, not a replacement neural model. No test WAVs are
committed. SignalTrain 1.1, Benjamin Colburn and Scott Hawley, is available at
https://zenodo.org/records/3824876 (record license CC BY 4.0; individual source
material provenance is described by the record).

## Reproduce

Python: numpy, scipy, matplotlib. Standard C++17 compiler. From repository root:

```sh
python3 tools/calibration/download_signaltrain.py /workspace/signaltrain-data
g++ -O3 -std=c++17 -shared -fPIC -ISource tools/calibration/render_optical.cpp -o /tmp/lassassin-optical.so
XDG_CACHE_HOME=/tmp/lassassin-fontcache MPLCONFIGDIR=/tmp/lassassin-mpl OPENBLAS_NUM_THREADS=1 python3 tools/calibration/calibrate.py /workspace/signaltrain-data
python3 tools/calibration/gain_control.py /workspace/signaltrain-data
python3 tools/calibration/make_report.py
python3 tools/calibration/plot_report.py
```

The archive is 20.7 GB but the downloader streams and keeps only six selected
WAVs (~953 MB). The original archive MD5 is published by Zenodo; because this
process intentionally stops after selected pairs, it does **not** verify that
whole-archive digest. Extracted files have SHA-256 and expected length checks.
Keep partial downloads separate; never treat a truncated WAV as valid.

## Protocol and limits

- Files 235 (Compress/65), 256 (Limit/65), 259 (Limit/80), mono 44.1 kHz,
  900 seconds each. Other hardware controls are held fixed in this dataset.
- These files are in the archive's **Test** folder. We deliberately repurpose
  them as a development subset with separated time splits. Results are **not**
  an untouched official benchmark score and do not establish generalization
  across independent hardware, recording sessions, or all knob settings.
- Fit ranges: [30,60), [120,150), [210,240) seconds, 90 s per recording.
  Held-out ranges: [330,390), [510,570), [690,750), [810,870), 240 s each.
  No fitting of gains, timing or profile parameters on held-out windows.
- Fitting renders the complete preceding 240 s at 11.025 kHz; final validation
  renders the full 900 s from zero state at native 44.1 kHz. This preserves
  preceding optical history rather than resetting at each evaluated excerpt.
  The hardware state before t=0 is unknown; early startup is excluded.
- Independent profile per measured operating point: effective Peak Reduction and linear output reference trim. Input drive is
  fixed at zero dB to prevent the optimizer from replacing optical dynamics
  with tube overdrive. These are effective
  calibration coordinates, not identified physical knob settings. Their values
  can be correlated; do not interpret them as independently recovered parts.
  Production Gain is zero; post-trim is applied after colour, not as its Gain pot.
- Fixed detector: HPF off, HF Emphasis off, linked duplicated mono. This isolates
  2A; FET/global processing and plugin routing are excluded. The model uses the
  production core at the specified rate with **no outer plugin oversampling**.
  These are core results, not end-to-end VST3/2x/4x host measurements.
- Output reference trim is fitted once on calibration windows, frozen for
  validation. No per-excerpt output normalization. Unknown hardware makeup and
  converter calibration prevent interpreting output/input RMS as absolute GR.
- Objective: 20 ms block-RMS envelope error in dB, target floor -55 dBFS.
  Float32 model requires explicit 0.1-unit finite differences; tiny zero-relative
  numerical steps silently lose gain sensitivity.
- Evaluate envelope RMSE/MAE, raw waveform ESR, output level error, Welch PSD
  difference and level-removed spectral shape difference. A lower envelope
  error does not guarantee lower waveform or spectral error.
- Empirical I/O curve: median output per 2 dB input-RMS bin, minimum 15 frames,
  from held-out program material. It includes attack/release history and is
  **not** a stationary hardware transfer sweep. Spectrum error is output PSD
  difference, not a measured LTI frequency response or isolated THD.
- Welch: 8192 samples, default Hann/50% overlap, concatenated validation windows;
  RMS metrics use nonoverlapping 20 ms blocks. Spectral scalar scores cover
  40 Hz–16 kHz where target PSD is within 60 dB of its maximum. Plot includes
  all frequency bins so noise-floor differences remain visible.
- No changes to the production DSP defaults or released binaries are made by
  these scripts. The fitted profiles are dataset-specific candidates awaiting
  broader operating-point coverage and plugin-level validation.

The fixed-gain-only control also derives its trim solely from calibration windows,
then freezes it on validation windows. It is included to avoid mistaking level
alignment for improvement in dynamic behavior.


## 0.5.0 integration check

The earlier fitting scripts keep their historical protocol. The new sparse
mapping is implemented in `Source/DSP/OpticalCalibration.h`; `LASettings` still
defaults to calibration off for direct-core callers, while new plugin instances
explicitly enable the APVTS parameter. Older saved plugin states migrate to off.

Build the optional renderer with `-DLASSASSIN_CALIBRATION_TOOLS=ON`, then run:

```sh
python3 tools/calibration/validate_oversampled.py /workspace/signaltrain-data
```

This uses the actual JUCE FIR oversampler at 2x/4x, 90 seconds of warm-up, then
60 seconds of held-out audio per measured operating point. It isolates the optical
stage; full processor routing/state/automation are checked by CTest separately.
Do not directly compare its shorter-protocol scores with the historical report.
