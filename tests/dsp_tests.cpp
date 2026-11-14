#include "DSP/Dynamics.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace assassin;
static void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
static float opticalTail(double hold, double fs = 48000)
{
    OpticalCompressor la;
    la.prepare(fs);
    LASettings p;
    p.peak = 70;
    p.hpf = 0;
    for (int i = 0; i < int(hold * fs); ++i)
    {
        float l = .5f, r = .5f;
        la.process(l, r, p);
    }
    for (int i = 0; i < int(fs); ++i)
    {
        float l = 0, r = 0;
        la.process(l, r, p);
    }
    return la.gr[0];
}
int main()
{
    try
    {
        const float quiet = JfetCell::process(1.e-12f, 6.f, 0.f);
        check(std::abs(quiet / 1.e-12f - dbGain(-6.f)) < 1.e-5f, "JFET root lost quiet signal precision");
        for (float reduction : {0.f, 6.f, 20.f, 60.f})
            for (float value : {-.8f, -.01f, .01f, .8f})
            {
                const float y = JfetCell::process(value, reduction, 1.f);
                check(std::isfinite(y) && std::abs(y) <= std::abs(value) + 1.e-6f,
                      "JFET divider became active or unstable");
            }
        for (double sr : {44100., 192000., 768000.})
        {
            MagneticTransformer transformer;
            transformer.prepare(sr);
            double incoming = 0, outgoing = 0;
            for (int i = 0; i < int(sr * .2); ++i)
            {
                const float x = float(.2 * std::sin(2 * pi * 50 * i / sr));
                const float y = transformer.process(x);
                check(std::isfinite(y), "magnetic state diverged");
                incoming += x * x;
                outgoing += y * y;
            }
            check(outgoing <= incoming * 1.02, "magnetic stage introduced hidden level gain");
        }
        {
            OpticalMemory envelope;
            envelope.prepare(48000);
            for (int i = 0; i < 14400; ++i)
                envelope.process(18);
            const auto held = envelope.state();
            check(held.heat > .1 && held.heat < .4, "LDR charge must accumulate over time");
            const float start = envelope.process(18);
            float at60ms = 0;
            for (int i = 0; i < 2880; ++i)
                at60ms = envelope.process(0);
            check(at60ms / start > .50f && at60ms / start < .58f,
                  "T4 must release approximately half the GR in 60 ms");
            const double warm = envelope.state().heat;
            for (int i = 0; i < 48000; ++i)
                envelope.process(0);
            check(envelope.state().heat < warm && envelope.state().heat > .01,
                  "LDR must cool gradually after the fast envelope releases");
            check(envelope.state().slowSeconds >= 1 && envelope.state().slowSeconds <= 15,
                  "T4 slow time constant escaped bounds");
            envelope.reset();
            envelope.process(50);
            check(envelope.state().heat < .00001, "One transient must not instantly heat the LDR");
            check(std::isfinite(envelope.process(std::numeric_limits<float>::quiet_NaN())),
                  "Optical memory must tolerate invalid control input");
            envelope.reset();
            check(envelope.process(0) == 0, "Optical memory reset retained state");
        }
        check(opticalTail(8) > opticalTail(.1) * 1.15f, "T4 memory must lengthen recovery");
        const float tailReference = opticalTail(2, 44100);
        for (double sr : {96000., 192000., 768000.})
            check(std::abs(opticalTail(2, sr) - tailReference) < .01f,
                  "T4 long release changed with oversampled rate");
        {
            OpticalCompressor optical;
            optical.prepare(48000);
            LASettings settings;
            settings.peak = 70;
            settings.hpf = 0;
            const float target = knee(gainDb(.5f) - (-6.f - .54f * settings.peak), 3, 6);
            for (int i = 0; i < 480; ++i)
            {
                float l = .5f, r = l;
                optical.process(l, r, settings);
            }
            check(std::abs(optical.gr[0] / target - float(1 - std::exp(-1.))) < .002f,
                  "T4 nominal 10ms attack mismatch");
        }
        {
            FETCompressor a, b;
            a.prepare(96000);
            b.prepare(96000);
            FETSettings pa, pb;
            pa.hpf = pb.hpf = 0;
            pb.output = 12;
            for (int i = 0; i < 48000; ++i)
            {
                float l = float(.3 * std::sin(2 * pi * 997 * i / 96000.)), r = l, x = l, y = r;
                a.process(l, r, pa);
                b.process(x, y, pb);
            }
            check(std::abs(a.gr[0] - b.gr[0]) < 1.e-5f, "FET Output incorrectly entered feedback detector");
        }
        for (double fs : {44100., 48000., 96000., 192000.})
        {
            OpticalCompressor la;
            FETCompressor fet;
            la.prepare(fs);
            fet.prepare(fs);
            LASettings lp;
            FETSettings fp;
            lp.peak = 100;
            fp.input = 18;
            fp.all = 1;
            fp.ratio = 35;
            fp.attack = 7;
            fp.release = 7;
            float largest = 0;
            for (int i = 0; i < int(fs * .3); ++i)
            {
                float l = float(.8 * std::sin(2 * pi * 997 * i / fs)), r = l * .3f;
                fet.process(l, r, fp);
                la.process(l, r, lp);
                check(std::isfinite(l) && std::isfinite(r), "non-finite output");
                largest = std::max(largest, std::abs(l));
            }
            check(largest < 20, "unstable output");
            check(la.gr[0] >= 0 && fet.gr[0] > 0, "GR must be nonnegative");
            check(std::abs(fet.gr[0] - fet.gr[1]) < .001f, "linked channels must share GR");
        }
        // Hardware-style control behavior, measured at the model output rather than UI state.
        float ratioGR[4]{};
        constexpr float ratios[]{4, 8, 12, 20};
        for (size_t k = 0; k < 4; ++k)
        {
            FETCompressor unit;
            unit.prepare(96000);
            FETSettings settings;
            settings.ratio = ratios[k];
            settings.hpf = 0;
            for (int i = 0; i < 24000; ++i)
            {
                float l = float(.3 * std::sin(2 * pi * 997 * i / 96000.)), r = l;
                unit.process(l, r, settings);
            }
            ratioGR[k] = unit.gr[0];
        }
        for (size_t k = 1; k < 4; ++k)
            check(ratioGR[k] > ratioGR[k - 1], "FET ratio buttons must increase steady GR");
        float transientGR[2]{};
        for (int fast = 0; fast < 2; ++fast)
        {
            FETCompressor unit;
            unit.prepare(192000);
            FETSettings settings;
            settings.hpf = 0;
            settings.attack = fast ? 7.f : 1.f;
            for (int i = 0; i < 40; ++i)
            {
                float l = .4f, r = l;
                unit.process(l, r, settings);
            }
            transientGR[fast] = unit.gr[0];
        }
        check(transientGR[1] > transientGR[0] + 1, "FET Attack 7 must catch transients faster than 1");
        float recovery[2]{};
        for (int fast = 0; fast < 2; ++fast)
        {
            FETCompressor unit;
            unit.prepare(96000);
            FETSettings settings;
            settings.hpf = 0;
            settings.release = fast ? 7.f : 1.f;
            for (int i = 0; i < 24000; ++i)
            {
                float l = float(.3 * std::sin(2 * pi * 997 * i / 96000.)), r = l;
                unit.process(l, r, settings);
            }
            const float initial = unit.gr[0];
            for (int i = 0; i < 9600; ++i)
            {
                float l = 0, r = 0;
                unit.process(l, r, settings);
            }
            recovery[fast] = unit.gr[0] / initial;
        }
        check(recovery[1] < recovery[0] * .5f, "FET Release 7 must recover faster than 1");
        auto opticalAmount = [](float limit, float hpf, float hf, double hz)
        {
            OpticalCompressor unit;
            unit.prepare(96000);
            LASettings settings;
            settings.peak = 50;
            settings.limit = limit;
            settings.hpf = hpf;
            settings.hf = hf;
            for (int i = 0; i < 48000; ++i)
            {
                float l = float(.3 * std::sin(2 * pi * hz * i / 96000.)), r = l;
                unit.process(l, r, settings);
            }
            return unit.gr[0];
        };
        check(opticalAmount(1, 0, 0, 997) > opticalAmount(0, 0, 0, 997) + 2,
              "LA Limit must exceed Compress GR");
        check(opticalAmount(0, 0, 1, 8000) > opticalAmount(0, 0, 0, 8000) + 2,
              "HF Emphasis must increase high-frequency triggering");
        check(opticalAmount(0, 150, 0, 30) < opticalAmount(0, 0, 0, 30) - 2,
              "Sidechain HPF must reduce bass triggering");
        float result[2]{};
        for (int drive = 0; drive < 2; ++drive)
        {
            FETCompressor f;
            f.prepare(96000);
            FETSettings p;
            p.hpf = 0;
            p.input = float(drive * 12);
            p.attack = 4;
            for (int i = 0; i < 96000; ++i)
            {
                float l = float(.1 * std::sin(2 * pi * 997 * i / 96000.)), r = l;
                f.process(l, r, p);
            }
            result[drive] = f.gr[0];
        }
        check(result[1] > result[0] + 3, "FET input must drive fixed threshold");
        // Makeup gain of the first unit must genuinely drive the second unit.
        float secondGR[2]{};
        for (int boost = 0; boost < 2; ++boost)
        {
            OpticalCompressor first;
            FETCompressor second;
            first.prepare(96000);
            second.prepare(96000);
            LASettings lp;
            lp.peak = 0;
            lp.gain = float(boost * 12);
            FETSettings fp;
            fp.hpf = 0;
            for (int i = 0; i < 96000; ++i)
            {
                float l = float(.05 * std::sin(2 * pi * 997 * i / 96000.)), r = l;
                first.process(l, r, lp);
                second.process(l, r, fp);
            }
            secondGR[boost] = second.gr[0];
        }
        check(secondGR[1] > secondGR[0] + 2, "First-stage makeup must drive second-stage detector");
        std::cout << "DSP tests passed; T4 1s tail: short=" << opticalTail(.1) << " long=" << opticalTail(8)
                  << " dB\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
