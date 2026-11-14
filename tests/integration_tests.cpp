#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
// Catch C++ heap allocations on this thread during processBlock (not OS malloc/locks).
static thread_local bool guard = false;
static std::atomic<int> allocations{0};
void *operator new(std::size_t n)
{
    if (guard)
        ++allocations;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n)
{
    return ::operator new(n);
}
void operator delete(void *p) noexcept
{
    std::free(p);
}
void operator delete[](void *p) noexcept
{
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept
{
    std::free(p);
}
void operator delete[](void *p, std::size_t) noexcept
{
    std::free(p);
}
static void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
static void set(LAssassinProcessor &p, const char *id, float value)
{
    auto *par = p.apvts.getParameter(id);
    par->setValueNotifyingHost(par->convertTo0to1(value));
}
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    try
    {
        for (double sr : {44100., 48000., 96000.})
        {
            LAssassinProcessor p;
            set(p, "mix", 0);
            p.prepareToPlay(sr, 64);
            const int latency = p.getLatencySamples();
            check(latency > 0, "OS latency not reported");
            juce::AudioBuffer<float> b(2, 257);
            juce::MidiBuffer midi;
            b.clear();
            b.setSample(0, 0, 1);
            b.setSample(1, 0, 1);
            guard = true;
            p.processBlock(b, midi);
            guard = false;
            for (int i = 0; i < b.getNumSamples(); ++i)
                check(std::abs(b.getSample(0, i) - (i == latency ? 1.f : 0.f)) < 1.e-5f,
                      "dry not latency aligned");
            set(p, "mix", 100);
            set(p, "peak", 80);
            set(p, "fetIn", 12);
            set(p, "ratio", 4);
            for (int block = 0; block < 120; ++block)
            {
                if (block == 20)
                    set(p, "order", 1);
                if (block == 40)
                    set(p, "os", 1);
                if (block == 60)
                    set(p, "ms", 1);
                if (block == 80)
                    set(p, "link", 0);
                if (block == 90)
                    set(p, "auto", 1);
                if (block == 100)
                {
                    set(p, "laBypass", 1);
                    set(p, "fetBypass", 1);
                }
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < b.getNumSamples(); ++i)
                        b.setSample(c, i,
                                    float(.3 * std::sin(2 * assassin::pi * 997 * (block * 257 + i) / sr)) *
                                        (c ? .5f : 1.f));
                guard = true;
                p.processBlock(b, midi);
                guard = false;
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < b.getNumSamples(); ++i)
                        check(std::isfinite(b.getSample(c, i)) && std::abs(b.getSample(c, i)) < 100,
                              "invalid output during switch");
                check(p.getLatencySamples() == latency, "latency changed on switch");
                check(std::abs(p.totalGR.load() - p.laGR.load() - p.fetGR.load()) < 1.e-4f,
                      "GR sum mismatch");
            }
            // Malformed upstream samples and hostile parameter values must not poison IIR history.
            b.clear();
            b.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
            b.setSample(1, 1, std::numeric_limits<float>::infinity());
            b.setSample(0, 2, std::numeric_limits<float>::max());
            p.apvts.getRawParameterValue("ratio")->store(std::numeric_limits<float>::quiet_NaN());
            p.apvts.getRawParameterValue("attack")->store(1.e20f);
            guard = true;
            p.processBlock(b, midi);
            guard = false;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < b.getNumSamples(); ++i)
                    check(std::isfinite(b.getSample(c, i)), "invalid input poisoned processor state");
            for (int block = 0; block < 8; ++block)
            {
                b.clear();
                guard = true;
                p.processBlock(b, midi);
                guard = false;
            }
            check(std::isfinite(p.outputDb.load()), "invalid input poisoned later meter values");
            set(p, "ratio", 0);
            set(p, "attack", 4);
            juce::AudioBuffer<float> invalidLayout(1, 16);
            invalidLayout.clear();
            guard = true;
            p.processBlock(invalidLayout, midi);
            guard = false;
            juce::MemoryBlock state;
            p.getStateInformation(state);
            LAssassinProcessor restored;
            restored.setStateInformation(state.getData(), int(state.getSize()));
            check(std::abs(restored.apvts.getRawParameterValue("peak")->load() - 80.f) < 1.e-5f,
                  "state restore failed");
        }
        {
            LAssassinProcessor bypassed;
            bypassed.prepareToPlay(48000, 64);
            juce::AudioBuffer<float> impulse(2, 257);
            impulse.clear();
            impulse.setSample(0, 0, 1);
            impulse.setSample(1, 0, 1);
            juce::MidiBuffer midi;
            guard = true;
            bypassed.processBlockBypassed(impulse, midi);
            guard = false;
            for (int i = 0; i < impulse.getNumSamples(); ++i)
                check(std::abs(impulse.getSample(0, i) - (i == bypassed.getLatencySamples() ? 1.f : 0.f)) <
                          1.e-5f,
                      "host bypass must retain reported latency");
        }
        check(allocations.load() == 0, "C++ heap allocation during processBlock");
        std::cout << "Integration tests passed: latency, block splitting, automation, state, no C++ heap "
                     "allocations\n";
    }
    catch (const std::exception &e)
    {
        guard = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
