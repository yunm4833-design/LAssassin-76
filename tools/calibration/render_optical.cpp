// Offline measurement adapter for the production optical core. No copied model.
#include "DSP/Dynamics.h"
extern "C" void render_optical(const float *input, float *output, float *gr, int n,
                               double fs, float peak, float limit, float inputDb, float outputDb)
{
    assassin::OpticalCompressor unit;
    unit.prepare(fs);
    assassin::LASettings settings;
    settings.peak = peak;
    settings.limit = limit;
    settings.gain = 0;
    settings.hpf = 0;
    settings.hf = 0;
    const float in = assassin::dbGain(inputDb), out = assassin::dbGain(outputDb);
    for (int i = 0; i < n; ++i)
    {
        float l = input[i] * in, r = l;
        unit.process(l, r, settings);
        output[i] = l * out;
        gr[i] = unit.gr[0];
    }
}
