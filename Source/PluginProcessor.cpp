#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace assassin;
namespace
{
constexpr const char *ids[count] = {"in",     "out",     "mix",   "order",       "laBypass", "peak",
                                    "laGain", "limit",   "hf",    "fetBypass",   "fetIn",    "fetOut",
                                    "attack", "release", "ratio", "allInternal", "link",     "ms",
                                    "hpf",    "auto",    "os"};
const char *names[5] = {"Default Vocal", "Rap Forward", "Pop Level", "Podcast", "Parallel Edge"};
} // namespace
juce::AudioProcessorValueTreeState::ParameterLayout LAssassinProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto f = [&](const char *id, const char *name, float lo, float hi, float def)
    {
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                                                          juce::NormalisableRange<float>(lo, hi, .01f), def));
    };
    auto b = [&](const char *id, const char *name, bool def)
    { p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, def)); };
    auto c = [&](const char *id, const char *name, juce::StringArray choices, int def)
    { p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, choices, def)); };
    f("in", "In (dB)", -24, 24, 0);
    f("out", "Out (dB)", -24, 24, 0);
    f("mix", "Mix (%)", 0, 100, 100);
    c("order", "Order", {"1176 > LA-2A", "LA-2A > 1176"}, 0);
    b("laBypass", "LA-2A Bypass", false);
    f("peak", "Peak Reduction", 0, 100, 35);
    f("laGain", "Gain (dB)", 0, 40, 0);
    b("limit", "Limit", false);
    b("hf", "HF Emphasis", false);
    b("fetBypass", "1176 Bypass", false);
    f("fetIn", "Input (dB)", -24, 24, 0);
    f("fetOut", "Output (dB)", -24, 24, 0);
    f("attack", "Attack", 1, 7, 4);
    f("release", "Release", 1, 7, 4);
    c("ratio", "Ratio", {"4", "8", "12", "20", "All"}, 0);
    b("link", "Stereo Link", true);
    c("ms", "Channel Mode", {"Stereo", "Mid-Side"}, 0);
    f("hpf", "Sidechain HPF (Hz)", 0, 300, 80);
    b("auto", "Auto Gain", false);
    c("os", "Oversampling", {"2x", "4x"}, 0);
    return p;
}
LAssassinProcessor::LAssassinProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "LAssassinState", layout())
{
    for (size_t i = 0; i < count; ++i)
        if (i != allMode)
        {
            raw[i] = apvts.getRawParameterValue(ids[i]);
            const auto *parameter = apvts.getParameter(ids[i]);
            minimum[i] = parameter->getNormalisableRange().start;
            maximum[i] = parameter->getNormalisableRange().end;
            defaults[i] = parameter->convertFrom0to1(parameter->getDefaultValue());
        }
}
Values LAssassinProcessor::snapshot() const
{
    Values v{};
    for (size_t i = 0; i < count; ++i)
        if (raw[i])
        {
            const float value = raw[i]->load(std::memory_order_relaxed);
            v[i] = std::isfinite(value) ? std::clamp(value, minimum[i], maximum[i]) : defaults[i];
        }
    const int index = std::clamp(int(v[ratio]), 0, 4);
    constexpr float ratios[]{4, 8, 12, 20, 35};
    v[ratio] = ratios[index];
    v[allMode] = index == 4 ? 1.f : 0.f;
    v[mix] *= .01f;
    return v;
}
bool LAssassinProcessor::isBusesLayoutSupported(const BusesLayout &l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
           l.getMainInputChannelSet() == l.getMainOutputChannelSet();
}
void LAssassinProcessor::prepareToPlay(double sr, int block)
{
    fs = sr;
    maxBlock = std::max(1, block);
    work.setSize(2, maxBlock);
    const auto v = snapshot();
    const int a = x2.prepare(sr, maxBlock, v), b = x4.prepare(sr, maxBlock, v), latency = std::max(a, b);
    x2.setAlignment(latency - a);
    x4.setAlignment(latency - b);
    for (auto &d : dryDelay)
        d.prepare(latency);
    setLatencySamples(latency);
    const float initial[]{v[inGain], v[outGain], v[mix], v[autoGain], v[os]};
    for (size_t i = 0; i < global.size(); ++i)
    {
        global[i].reset(sr, i == 3 ? .2 : .020);
        global[i].setCurrentAndTargetValue(initial[i]);
    }
    dryPower = wetPower = autoDb = 0;
    powerPole = pole(.4, sr);
    autoPole = pole(1.5, sr);
}
void LAssassinProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    if (buffer.getNumChannels() != 2)
    {
        buffer.clear();
        return;
    }
    const auto v = snapshot();
    x2.targets(v);
    x4.targets(v);
    const float targets[]{v[inGain], v[outGain], v[mix], v[autoGain], v[os]};
    for (size_t i = 0; i < global.size(); ++i)
        global[i].setTargetValue(targets[i]);
    float im = 0, om = 0, la = 0, fet = 0, total = 0;
    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxBlock)
    {
        const int n = std::min(maxBlock, buffer.getNumSamples() - offset);
        // Store per-sample global values without allocation. Engine buffers remain internal.
        // Global input and domain conversion here; remaining controls tick in the output loop.
        for (int i = 0; i < n; ++i)
        {
            const float g = dbGain(global[0].getNextValue());
            auto finiteInput = [](float sample)
            {
                // Limit only pathological >+36 dBFS inputs; preserve ordinary float headroom.
                return std::isfinite(sample) ? std::clamp(sample, -64.f, 64.f) : 0.f;
            };
            const float l = finiteInput(buffer.getSample(0, offset + i));
            const float r = finiteInput(buffer.getSample(1, offset + i));
            buffer.setSample(0, offset + i, l);
            buffer.setSample(1, offset + i, r);
            im = std::max({im, std::abs(l), std::abs(r)});
            work.setSample(0, i, l * g);
            work.setSample(1, i, r * g);
        }
        // Domain rotation and inverse rotation are smoothed at the oversampled rate.
        x2.process(work.getReadPointer(0), work.getReadPointer(1), n);
        x4.process(work.getReadPointer(0), work.getReadPointer(1), n);
        const float osWeight = v[os];
        const float candidate = x2.meter[2] * (1 - osWeight) + x4.meter[2] * osWeight;
        if (candidate > total)
        {
            total = candidate;
            la = x2.meter[0] * (1 - osWeight) + x4.meter[0] * osWeight;
            fet = x2.meter[1] * (1 - osWeight) + x4.meter[1] * osWeight;
        }
        for (int i = 0; i < n; ++i)
        {
            const float outputGain = dbGain(global[1].getNextValue()), blend = global[2].getNextValue();
            const float ag = global[3].getNextValue(), osBlend = global[4].getNextValue();
            float wet[2], dry[2];
            for (int c = 0; c < 2; ++c)
            {
                dry[c] = dryDelay[size_t(c)].tick(buffer.getSample(c, offset + i));
                wet[c] = (x2.output(c)[i] * (1 - osBlend) + x4.output(c)[i] * osBlend) * outputGain;
            }
            const float dp = .5f * (dry[0] * dry[0] + dry[1] * dry[1]),
                        wp = .5f * (wet[0] * wet[0] + wet[1] * wet[1]);
            dryPower = dp + powerPole * (dryPower - dp);
            wetPower = wp + powerPole * (wetPower - wp);
            // Freeze during silence. Never undo instantaneous GR; match long-term energy only.
            const float target = (dryPower > 1.e-8f && wetPower > 1.e-10f)
                                     ? std::clamp(10.f * std::log10(dryPower / wetPower), -12.f, 12.f)
                                     : autoDb;
            autoDb = target + autoPole * (autoDb - target);
            const float compensation = dbGain(autoDb * ag);
            for (int c = 0; c < 2; ++c)
            {
                const float y = dry[c] + blend * (wet[c] * compensation - dry[c]);
                buffer.setSample(c, offset + i, y);
                om = std::max(om, std::abs(y));
            }
        }
    }
    laGR.store(la, std::memory_order_relaxed);
    fetGR.store(fet, std::memory_order_relaxed);
    totalGR.store(total, std::memory_order_relaxed);
    inputDb.store(gainDb(im), std::memory_order_relaxed);
    outputDb.store(gainDb(om), std::memory_order_relaxed);
}
const juce::String LAssassinProcessor::getProgramName(int i)
{
    return names[std::clamp(i, 0, 4)];
}
void LAssassinProcessor::setCurrentProgram(int index)
{
    currentProgram = std::clamp(index, 0, 4);
    // Message-thread/host program operation; never invoked by processBlock.
    struct Start
    {
        float peak, gain, in, out, atk, rel, ratio, mix, hp;
    };
    constexpr Start presets[] = {{35, 3, 0, 0, 4, 4, 0, 100, 80},
                                 {28, 2, 6, -3, 3, 6, 1, 100, 100},
                                 {45, 5, 2, -2, 5, 4, 0, 100, 90},
                                 {40, 4, -2, 1, 5, 3, 0, 100, 120},
                                 {25, 0, 8, -6, 2, 7, 4, 35, 100}};
    const auto &s = presets[currentProgram.load(std::memory_order_relaxed)];
    auto set = [&](const char *id, float value)
    {
        auto *p = apvts.getParameter(id);
        p->setValueNotifyingHost(p->convertTo0to1(value));
    };
    set("in", 0);
    set("out", 0);
    set("order", 0);
    set("laBypass", 0);
    set("fetBypass", 0);
    set("limit", 0);
    set("hf", 0);
    set("link", 1);
    set("ms", 0);
    set("auto", 0);
    set("os", 0);
    set("peak", s.peak);
    set("laGain", s.gain);
    set("fetIn", s.in);
    set("fetOut", s.out);
    set("attack", s.atk);
    set("release", s.rel);
    set("ratio", s.ratio);
    set("mix", s.mix);
    set("hpf", s.hp);
}
void LAssassinProcessor::getStateInformation(juce::MemoryBlock &dest)
{
    auto tree = apvts.copyState();
    tree.setProperty("program", currentProgram.load(std::memory_order_relaxed), nullptr);
    auto xml = tree.createXml();
    copyXmlToBinary(*xml, dest);
}
void LAssassinProcessor::setStateInformation(const void *data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentProgram = std::clamp(int(tree.getProperty("program", 0)), 0, 4);
            apvts.replaceState(tree);
        }
}
juce::AudioProcessorEditor *LAssassinProcessor::createEditor()
{
    return new LAssassinEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new LAssassinProcessor();
}
