#pragma once
#include "DSP/Engine.h"
#include <juce_audio_processors/juce_audio_processors.h>
static_assert(std::atomic<float>::is_always_lock_free, "Audio parameter/meter atomics must be lock-free");
class LAssassinProcessor final : public juce::AudioProcessor
{
  public:
    LAssassinProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    using juce::AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using juce::AudioProcessor::processBlockBypassed;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LAssassin-76"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 5; }
    int getCurrentProgram() override { return currentProgram.load(std::memory_order_relaxed); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorValueTreeState apvts;
    // UI reads atomics only; meter ballistics belong to the editor.
    std::atomic<float> laGR{0}, fetGR{0}, totalGR{0}, inputDb{-180}, outputDb{-180};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();

  private:
    void processInternal(juce::AudioBuffer<float> &, juce::MidiBuffer &, bool hostBypassed);
    assassin::Values snapshot() const;
    std::array<std::atomic<float> *, assassin::count> raw{};
    assassin::Values minimum{}, maximum{}, defaults{};
    assassin::Engine x2{1}, x4{2};
    std::array<assassin::Delay, 2> dryDelay;
    juce::AudioBuffer<float> work;
    std::array<juce::SmoothedValue<float>, 5> global;
    juce::SmoothedValue<float> hostWet;
    bool firstBlock = true;
    double fs = 48000;
    int maxBlock = 512;
    std::atomic<int> currentProgram{0};
    float dryPower = 0, wetPower = 0, autoDb = 0, powerPole = 0, autoPole = 0;
};
