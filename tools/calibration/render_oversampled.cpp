// Isolated 2A stage using the production JUCE FIR oversampler and actual core.
// Args: input float32 mono raw, output raw, rate, exponent(1=2x,2=4x), PR, Limit, Cal.
#include "DSP/Dynamics.h"
#include <fstream>
#include <iostream>
#include <juce_dsp/juce_dsp.h>
int main(int argc, char **argv)
{
    if (argc != 8)
        return 2;
    std::ifstream input(argv[1], std::ios::binary);
    std::ofstream output(argv[2], std::ios::binary);
    if (!input || !output)
        return 3;
    const double fs = std::stod(argv[3]);
    const int exponent = std::stoi(argv[4]);
    assassin::LASettings settings;
    settings.peak = std::stof(argv[5]);
    settings.limit = std::stof(argv[6]);
    settings.calibration = std::stof(argv[7]);
    settings.hpf = 0;
    settings.gain = 0;
    juce::dsp::Oversampling<float> oversampler(
        2, size_t(exponent), juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler.initProcessing(512);
    oversampler.reset();
    assassin::OpticalCompressor optical;
    optical.prepare(fs * double(oversampler.getOversamplingFactor()));
    juce::AudioBuffer<float> buffer(2, 512);
    std::array<float, 512> raw{};
    while (input.read(reinterpret_cast<char *>(raw.data()), sizeof(raw)) || input.gcount() > 0)
    {
        const int n = int(input.gcount() / std::streamsize(sizeof(float)));
        buffer.copyFrom(0, 0, raw.data(), n);
        buffer.copyFrom(1, 0, raw.data(), n);
        auto base = juce::dsp::AudioBlock<float>(buffer).getSubBlock(0, size_t(n));
        auto up = oversampler.processSamplesUp(base);
        auto *l = up.getChannelPointer(0);
        auto *r = up.getChannelPointer(1);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            optical.process(l[i], r[i], settings);
        oversampler.processSamplesDown(base);
        output.write(reinterpret_cast<const char *>(buffer.getReadPointer(0)), std::streamsize(n) * std::streamsize(sizeof(float)));
    }
    std::cout << "latency " << int(std::lround(oversampler.getLatencyInSamples())) << "\n";
    return output ? 0 : 4;
}
