#pragma once
#include "PluginProcessor.h"

class AssassinLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    AssassinLookAndFeel();
    juce::Font getLabelFont(juce::Label &) override;
    juce::Font getComboBoxFont(juce::ComboBox &) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getSliderPopupFont(juce::Slider &) override;
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float, juce::Slider &) override;
    void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool, bool) override;
    void drawButtonText(juce::Graphics &, juce::TextButton &, bool, bool) override;
    void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
    void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int, juce::ComboBox &) override;
};

class LAssassinEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit LAssassinEditor(LAssassinProcessor &);
    ~LAssassinEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    struct Dial
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void timerCallback() override;
    void createBackground();
    void drawVu(juce::Graphics &);
    void drawMeters(juce::Graphics &);
    void drawBypass(juce::Graphics &, juce::Rectangle<float>);
    void put(juce::Component &, int, int, int, int);
    LAssassinProcessor &owner;
    AssassinLookAndFeel theme;
    juce::Component surface;
    juce::TooltipWindow tooltips{this, 600};
    std::array<Dial, 10> dials;
    juce::TextButton laBypass{"BYPASS"}, fetBypass{"BYPASS"}, order{"1176\nLA-2A"};
    juce::ToggleButton limit{"COMPRESS / LIMIT"}, hf{"HF EMPHASIS"}, link{"STEREO LINK"},
        automatic{"AUTO GAIN"};
    std::array<juce::TextButton, 5> ratios;
    juce::ComboBox presets, mode, oversampling;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::unique_ptr<juce::ParameterAttachment> ratioAttachment;
    juce::Image background;
    float optical = 0, fet = 0, total = 0, input = -90, output = -90, sweep = 0, pulse = 0;
    bool previousOrder = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LAssassinEditor)
};
