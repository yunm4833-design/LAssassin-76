#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace assassin
{
constexpr double pi = 3.14159265358979323846;
inline float dbGain(float d)
{
    return std::pow(10.0f, d * .05f);
}
inline float gainDb(float g)
{
    return 20.0f * std::log10(std::max(g, 1.e-9f));
}
inline float pole(double seconds, double fs)
{
    return float(std::exp(-1.0 / (seconds * fs)));
}
// A direct small step avoids cancellation in 1 - exp(-1/(tau*fs)).
inline double envelopeStep(double seconds, double fs)
{
    return -std::expm1(-1.0 / (seconds * fs));
}
inline float knee(float above, float ratio, float width)
{
    const float slope = 1.f - 1.f / ratio;
    if (above <= -width * .5f)
        return 0.f;
    if (above >= width * .5f)
        return slope * above;
    const float t = above + width * .5f;
    return slope * t * t / (2.f * width);
}
inline float asymmetric(float x, float drive, float bias)
{
    const float t = std::tanh(drive * bias);
    return (std::tanh(drive * (x + bias)) - t) / (drive * (1.f - t * t));
}
struct LowPass
{
    float z = 0, a = 0;
    void prepare(double fs, double hz)
    {
        a = float(std::exp(-2.0 * pi * std::min(hz, fs * .45) / fs));
        z = 0;
    }
    float tick(float x)
    {
        z = x + a * (z - x);
        return z;
    }
};
struct DCBlock
{
    float x1 = 0, y1 = 0, a = 0;
    void prepare(double fs)
    {
        a = float(std::exp(-2 * pi * 5 / fs));
        x1 = y1 = 0;
    }
    float tick(float x)
    {
        float y = x - x1 + a * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};
struct Colour
{
    LowPass bass, treble;
    DCBlock dc;
    void prepare(double fs, double cutoff)
    {
        bass.prepare(fs, 140);
        treble.prepare(fs, cutoff);
        dc.prepare(fs);
    }
    float tick(float x, float drive, float bias)
    {
        const float low = bass.tick(x);
        // Low-band transformer residual; unity small-signal transfer.
        x += .08f * (std::tanh(1.7f * low) / 1.7f - low);
        return treble.tick(dc.tick(asymmetric(x, drive, bias)));
    }
};
struct Detector
{
    LowPass emphasis;
    float lp = 0;
    double fs = 48000;
    void prepare(double sr)
    {
        fs = sr;
        lp = 0;
        emphasis.prepare(sr, 2200);
    }
    float tick(float x, float hpHz, float hf)
    {
        const float a = float(std::exp(-2 * pi * hpHz / fs));
        lp = x + a * (lp - x);
        const float highpassed = hpHz > .5f ? x - lp : x;
        const float low = emphasis.tick(highpassed);
        return std::abs(highpassed + 2.f * hf * (highpassed - low));
    }
};
struct LASettings
{
    float peak = 35, gain = 0, limit = 0, hf = 0, link = 1, hpf = 80;
};
class OpticalCompressor
{
    struct Channel
    {
        Detector detector;
        Colour colour;
        double fast = 0, slow = 0, memory = 0;
    };
    std::array<Channel, 2> ch;
    double fs = 48000;
    double attack = 0, fastRelease = 0, memoryUp = 0, memoryDown = 0;

  public:
    std::array<float, 2> gr{};
    void prepare(double sr)
    {
        fs = sr;
        attack = envelopeStep(.010, fs);
        fastRelease = envelopeStep(.060 / std::log(10.0), fs); // fast branch T90 = 60 ms
        memoryUp = envelopeStep(.5, fs);
        memoryDown = envelopeStep(5, fs);
        for (auto &c : ch)
        {
            c = {};
            c.detector.prepare(fs);
            c.colour.prepare(fs, 18000);
        }
        gr = {};
    }
    void process(float &l, float &r, const LASettings &p)
    {
        float x[2]{l, r};
        float levels[2];
        for (size_t c = 0; c < 2; ++c)
            levels[c] = ch[c].detector.tick(x[c], p.hpf, p.hf);
        const float linked = std::max(levels[0], levels[1]);
        for (size_t c = 0; c < 2; ++c)
        {
            auto &s = ch[c];
            const float det = levels[c] + p.link * (linked - levels[c]);
            const float target =
                p.peak < .001f
                    ? 0.f
                    : std::clamp(knee(gainDb(det) - (-6.f - .54f * p.peak), 3.f + 97.f * p.limit, 6), 0.f,
                                 50.f);
            const float memoryTarget = std::clamp(target / 24.f, 0.f, 1.f);
            const double ma = memoryTarget > s.memory ? memoryUp : memoryDown;
            s.memory += ma * (memoryTarget - s.memory);
            const double slowRelease = envelopeStep(1.0 + 14.0 * s.memory, fs);
            const double fa = target > s.fast ? attack : fastRelease;
            const double sa = target > s.slow ? attack : slowRelease;
            s.fast += fa * (target - s.fast);
            s.slow += sa * (target - s.slow);
            gr[c] = float(.5 * (s.fast + s.slow));
            x[c] = s.colour.tick(x[c] * dbGain(p.gain - gr[c]), 1.05f, .12f);
        }
        l = x[0];
        r = x[1];
    }
};
struct FETSettings
{
    float input = 0, output = 0, attack = 4, release = 4, ratio = 4, all = 0, link = 1, hpf = 80;
};
class FETCompressor
{
    struct Channel
    {
        Detector detector;
        Colour inputColour, outputColour;
        float feedback = 0;
        double reduction = 0;
        DCBlock fetDC;
    };
    std::array<Channel, 2> ch;
    double fs = 48000;

  public:
    std::array<float, 2> gr{};
    void prepare(double sr)
    {
        fs = sr;
        gr = {};
        for (auto &c : ch)
        {
            c = {};
            c.detector.prepare(fs);
            c.inputColour.prepare(fs, 22000);
            c.outputColour.prepare(fs, 21000);
            c.fetDC.prepare(fs);
        }
    }
    void process(float &l, float &r, const FETSettings &p)
    {
        float x[2]{l, r}, det[2];
        const float inputGain = dbGain(p.input), outputGain = dbGain(p.output);
        for (size_t c = 0; c < 2; ++c)
        {
            x[c] = ch[c].inputColour.tick(x[c] * inputGain, .75f, .025f);
            det[c] = ch[c].detector.tick(ch[c].feedback, p.hpf, 0);
        }
        const float linked = std::max(det[0], det[1]);
        const double attack = .0008 * std::pow(.000020 / .0008, (p.attack - 1.f) / 6.f);
        const double release = 1.1 * std::pow(.050 / 1.1, (p.release - 1.f) / 6.f);
        for (size_t c = 0; c < 2; ++c)
        {
            auto &s = ch[c];
            const float level = det[c] + p.link * (linked - det[c]);
            // Feedback equation at equilibrium: GR=(R-1)*(input-GR-threshold).
            // ALL uses a lower threshold, a soft control ceiling and slower initial attack.
            const float ratio = p.ratio + (35.f - p.ratio) * p.all;
            const float over = gainDb(level) - (-18.f - 4.f * p.all);
            const float control = knee(over, ratio, 2.f + 2.f * p.all) * ratio;
            const float target = std::clamp(control, 0.f, 60.f);
            const double atk = attack * (1. + 2.5 * p.all);
            const double rel =
                release * (1. + .65 * std::clamp(float(s.reduction) / 24.f, 0.f, 1.f)) * (1. - .35 * p.all);
            const double delta = envelopeStep(target > s.reduction ? atk : rel, fs);
            // Normalize by the local closed-loop slope. A naive explicit update
            // becomes unstable at 20us with high ratios and a one-sample feedback tap.
            const float width = 2.f + 2.f * p.all;
            const float localSlope = (ratio - 1.f) * std::clamp((over + width * .5f) / width, 0.f, 1.f);
            const double step = delta / (1.0 + localSlope);
            s.reduction += step * (target - s.reduction);
            gr[c] = float(s.reduction);
            float y = x[c] * dbGain(-float(s.reduction));
            s.feedback = y; // tap before makeup/output transformer; one OS sample causal delay
            const float depth = std::clamp(float(s.reduction) / 24.f, 0.f, 1.f);
            const float drive = .65f + 1.6f * depth + 2.0f * p.all;
            const float bias = .015f + .065f * depth + .08f * p.all;
            y = s.fetDC.tick(asymmetric(y, drive, bias));
            x[c] = s.outputColour.tick(y * outputGain, .75f, .025f);
        }
        l = x[0];
        r = x[1];
    }
};
} // namespace assassin
