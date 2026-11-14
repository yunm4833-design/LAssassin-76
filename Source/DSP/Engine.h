#pragma once
#include "Dynamics.h"
#include <juce_dsp/juce_dsp.h>
namespace assassin
{
enum Param
{
    inGain,
    outGain,
    mix,
    order,
    laBypass,
    peak,
    laGain,
    limit,
    hf,
    fetBypass,
    fetIn,
    fetOut,
    attack,
    release,
    ratio,
    allMode,
    link,
    ms,
    hpf,
    autoGain,
    os,
    count
};
using Values = std::array<float, count>;
class Delay
{
    std::vector<float> data;
    size_t pos = 0;
    int delay = 0;

  public:
    void prepare(int samples)
    {
        delay = samples;
        data.assign(size_t(std::max(1, samples)), 0.f);
        pos = 0;
    }
    float tick(float x)
    {
        if (delay == 0)
            return x;
        float y = data[pos];
        data[pos] = x;
        if (++pos == data.size())
            pos = 0;
        return y;
    }
};
struct Chain
{
    OpticalCompressor la;
    FETCompressor fet;
    void prepare(double fs)
    {
        la.prepare(fs);
        fet.prepare(fs);
    }
    void process(float &l, float &r, const Values &v, bool reverse)
    {
        LASettings lp{v[peak], v[laGain], v[limit], v[hf], v[link], v[hpf]};
        FETSettings fp{v[fetIn], v[fetOut], v[attack], v[release], v[ratio], v[allMode], v[link], v[hpf]};
        auto optical = [&]
        {
            const float dl = l, dr = r;
            la.process(l, r, lp);
            l += (dl - l) * v[laBypass];
            r += (dr - r) * v[laBypass];
        };
        auto field = [&]
        {
            const float dl = l, dr = r;
            fet.process(l, r, fp);
            l += (dl - l) * v[fetBypass];
            r += (dr - r) * v[fetBypass];
        };
        if (reverse)
        {
            optical();
            field();
        }
        else
        {
            field();
            optical();
        }
    }
};
// Both orders run continuously. This trades CPU for state-continuous switching.
class Engine
{
    juce::dsp::Oversampling<float> oversampler;
    std::array<Chain, 2> chains;
    std::array<juce::SmoothedValue<float>, count> smooth;
    Values v{};
    juce::AudioBuffer<float> buffer;
    std::array<Delay, 2> align;

  public:
    std::array<float, 3> meter{}; // optical/FET at the channel and instant of peak total GR
    Engine(int exponent)
        : oversampler(2, size_t(exponent), juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true,
                      true)
    {
    }
    int prepare(double sr, int block, const Values &initial)
    {
        oversampler.initProcessing(size_t(block));
        oversampler.reset();
        buffer.setSize(2, block);
        const double fs = sr * double(oversampler.getOversamplingFactor());
        for (auto &c : chains)
            c.prepare(fs);
        for (size_t i = 0; i < count; ++i)
        {
            smooth[i].reset(fs, .020);
            smooth[i].setCurrentAndTargetValue(initial[i]);
        }
        return int(std::lround(oversampler.getLatencyInSamples()));
    }
    void setAlignment(int samples)
    {
        for (auto &d : align)
            d.prepare(samples);
    }
    void targets(const Values &target)
    {
        for (size_t i = 0; i < count; ++i)
            smooth[i].setTargetValue(target[i]);
    }
    void process(const float *l, const float *r, int n)
    {
        buffer.copyFrom(0, 0, l, n);
        buffer.copyFrom(1, 0, r, n);
        auto base = juce::dsp::AudioBlock<float>(buffer).getSubBlock(0, size_t(n));
        auto up = oversampler.processSamplesUp(base);
        auto *left = up.getChannelPointer(0);
        auto *right = up.getChannelPointer(1);
        meter = {};
        for (size_t i = 0; i < up.getNumSamples(); ++i)
        {
            for (size_t k = 0; k < count; ++k)
                v[k] = smooth[k].getNextValue();
            const float angle = v[ms] * float(pi * .25);
            const float cs = std::cos(angle), sn = std::sin(angle);
            float a = cs * left[i] + sn * right[i], b = -sn * left[i] + cs * right[i], c = a, d = b;
            chains[0].process(a, b, v, false);
            chains[1].process(c, d, v, true);
            const float domainL = a + v[order] * (c - a), domainR = b + v[order] * (d - b);
            left[i] = cs * domainL - sn * domainR;
            right[i] = sn * domainL + cs * domainR;
            for (size_t channel = 0; channel < 2; ++channel)
            {
                float optical =
                    (chains[0].la.gr[channel] * (1 - v[order]) + chains[1].la.gr[channel] * v[order]) *
                    (1 - v[laBypass]);
                float fet =
                    (chains[0].fet.gr[channel] * (1 - v[order]) + chains[1].fet.gr[channel] * v[order]) *
                    (1 - v[fetBypass]);
                if (optical + fet > meter[2])
                    meter = {optical, fet, optical + fet};
            }
        }
        oversampler.processSamplesDown(base);
        for (int c = 0; c < 2; ++c)
        {
            auto *dst = buffer.getWritePointer(c);
            for (int i = 0; i < n; ++i)
                dst[i] = align[size_t(c)].tick(dst[i]);
        }
    }
    const float *output(int c) const { return buffer.getReadPointer(c); }
};
} // namespace assassin
