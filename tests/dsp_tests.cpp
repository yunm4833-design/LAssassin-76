#include "DSP/Dynamics.h"
#include <iostream>
#include <stdexcept>
using namespace assassin;
static void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
static float opticalTail(double hold)
{
    OpticalCompressor la;
    la.prepare(48000);
    LASettings p;
    p.peak = 70;
    p.hpf = 0;
    for (int i = 0; i < int(hold * 48000); ++i)
    {
        float l = .5f, r = .5f;
        la.process(l, r, p);
    }
    for (int i = 0; i < 48000; ++i)
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
