#include "DSP/Dynamics.h"
#include <iostream>
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
