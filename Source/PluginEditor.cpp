#include "PluginEditor.h"
#include "LAssassinFonts.h"
#include "LAssassinSkin.h"
namespace
{
const juce::Colour ivory{0xffeee4cb}, ink{0xff171819}, red{0xffa3141a}, cyan{0xff6fe7ff}, brass{0xffb79a62};
constexpr int width = 1738, height = 905;
juce::Font retroFont(float size, bool display = false)
{
    static auto displayFace = juce::Typeface::createSystemTypefaceFor(
        LAssassinFonts::CinzelDecorativeBold_ttf, LAssassinFonts::CinzelDecorativeBold_ttfSize);
    static auto controlFace = juce::Typeface::createSystemTypefaceFor(LAssassinFonts::ZillaSlabBold_ttf,
                                                                      LAssassinFonts::ZillaSlabBold_ttfSize);
    return juce::Font(juce::FontOptions(display ? displayFace : controlFace).withHeight(size));
}
void text(juce::Graphics &g, const juce::String &s, juce::Rectangle<float> r, float size, juce::Colour colour)
{
    g.setColour(colour);
    g.setFont(retroFont(size));
    g.drawText(s, r, juce::Justification::centred);
}
juce::Path blade(juce::Rectangle<float> b)
{
    juce::Path p;
    p.startNewSubPath(b.getX() + 15, b.getY());
    p.lineTo(b.getRight(), b.getY());
    p.lineTo(b.getRight() - 15, b.getBottom());
    p.lineTo(b.getX(), b.getBottom());
    p.closeSubPath();
    return p;
}
void lamp(juce::Graphics &g, float x, float y, float radius, bool on)
{
    if (on)
        for (int i = 5; i > 0; --i)
        {
            g.setColour(cyan.withAlpha(.025f * float(6 - i)));
            g.fillEllipse(x - radius - float(i) * 2, y - radius - float(i) * 2, radius * 2 + float(i) * 4,
                          radius * 2 + float(i) * 4);
        }
    g.setColour(brass.darker(.5f));
    g.fillEllipse(x - radius - 2, y - radius - 2, radius * 2 + 4, radius * 2 + 4);
    g.setGradientFill(juce::ColourGradient(on ? juce::Colours::white : juce::Colour(0xff435050),
                                           x - radius * .3f, y - radius * .4f, on ? cyan.darker(.25f) : ink,
                                           x + radius, y + radius, true));
    g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
}
void lever(juce::Graphics &g, float x, float y, float radius, bool up)
{
    g.setColour(juce::Colours::black.withAlpha(.6f));
    g.fillEllipse(x - radius, y - radius + 5, radius * 2, radius * 2);
    g.setGradientFill(juce::ColourGradient(ivory, x - radius, y - radius, brass.darker(.8f), x + radius,
                                           y + radius, false));
    g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
    g.setColour(ink);
    g.fillEllipse(x - radius * .76f, y - radius * .76f, radius * 1.52f, radius * 1.52f);
    g.setColour(brass);
    g.drawEllipse(x - radius * .61f, y - radius * .61f, radius * 1.22f, radius * 1.22f, 2.f);
    const float dx = up ? 12.f : -10.f, dy = up ? -34.f : -20.f;
    g.setColour(juce::Colours::black.withAlpha(.6f));
    g.drawLine(x + 3, y + 3, x + dx + 4, y + dy + 5, 16);
    g.setGradientFill(juce::ColourGradient(ivory, x - 6, y, juce::Colour(0xff3f403b), x + 7, y, false));
    g.drawLine(x, y, x + dx, y + dy, 14);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white, x + dx - 3, y + dy - 5,
                                           juce::Colour(0xff686b64), x + dx + 5, y + dy + 6, false));
    g.fillEllipse(x + dx - 7, y + dy - 7, 14, 14);
}
} // namespace

AssassinLookAndFeel::AssassinLookAndFeel()
{
    knobs = juce::ImageCache::getFromMemory(LAssassinSkin::Knobs_png, LAssassinSkin::Knobs_pngSize);
    setColour(juce::Slider::textBoxTextColourId, ivory);
    setColour(juce::Slider::textBoxBackgroundColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, brass);
    setColour(juce::Slider::trackColourId, cyan);
    setColour(juce::Slider::thumbColourId, brass);
    setColour(juce::ComboBox::textColourId, ivory);
    setColour(juce::PopupMenu::backgroundColourId, ink);
    setColour(juce::PopupMenu::textColourId, ivory);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, red);
    setColour(juce::Label::textColourId, ivory);
    setColour(juce::TooltipWindow::backgroundColourId, ivory);
    setColour(juce::TooltipWindow::textColourId, ink);
    setColour(juce::TextEditor::backgroundColourId, ink);
    setColour(juce::TextEditor::textColourId, ivory);
    setColour(juce::TextEditor::highlightColourId, red);
}
juce::Font AssassinLookAndFeel::getLabelFont(juce::Label &label)
{
    return retroFont(label.getProperties().contains("readout") ? 32.f : 23.f);
}
juce::Font AssassinLookAndFeel::getComboBoxFont(juce::ComboBox &)
{
    return retroFont(23);
}
juce::Font AssassinLookAndFeel::getPopupMenuFont()
{
    return retroFont(21);
}
juce::Font AssassinLookAndFeel::getSliderPopupFont(juce::Slider &)
{
    return retroFont(24);
}
void AssassinLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float pos,
                                           float start, float end, juce::Slider &slider)
{
    const int sprite = int(slider.getProperties()["sprite"]);
    const float diameter = float(std::min(w, h)), radius = diameter * .5f;
    const float cx = float(x) + float(w) * .5f, cy = float(y) + float(h) * .5f;
    const float angle = start + pos * (end - start);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    // Each atlas quadrant includes transparent padding. Expand it so the physical
    // grip (84% of the tile) occupies the actual calibrated knob diameter.
    const float box = diameter / .84f;
    const int tile = knobs.getWidth() / 2;
    g.drawImage(knobs, int(cx - box * .5f), int(cy - box * .5f), int(box), int(box), (sprite % 2) * tile,
                (sprite / 2) * tile, tile, tile);
    const auto normal = juce::Point<float>(std::cos(angle), std::sin(angle));
    const auto direction = juce::Point<float>(std::sin(angle), -std::cos(angle));
    const auto centre = juce::Point<float>(cx, cy);
    const auto tip = centre + direction * (radius * .99f);
    const auto tail = centre + direction * (radius * .25f);
    const auto left = centre + direction * (radius * .55f) - normal * (radius * .065f);
    const auto right = centre + direction * (radius * .55f) + normal * (radius * .065f);
    juce::Path needle;
    needle.startNewSubPath(tip);
    needle.lineTo(right);
    needle.lineTo(tail);
    needle.lineTo(left);
    needle.closeSubPath();
    juce::DropShadow(juce::Colours::black.withAlpha(.75f), 3, {2, 3}).drawForPath(g, needle);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffedb7), left.x, left.y, brass.darker(.8f),
                                           right.x, right.y, false));
    g.fillPath(needle);
    g.setColour(juce::Colour(0xffd8bd77));
    g.strokePath(needle, juce::PathStrokeType(.7f));
    g.setColour(ivory);
    g.drawLine({tail, tip}, .8f);
    if (slider.isMouseOverOrDragging())
    {
        juce::Path arc;
        arc.addCentredArc(cx, cy, radius + 4, radius + 4, 0, start, angle, true);
        for (int i = 3; i > 0; --i)
        {
            g.setColour(cyan.withAlpha(.16f));
            g.strokePath(arc, juce::PathStrokeType(float(i) * 2.f));
        }
    }
}
void AssassinLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &,
                                               bool over, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced(4, bool(b.getProperties()["blade"]) ? 14.f : 4.f);
    const bool active = b.getToggleState(), all = bool(b.getProperties()["all"]);
    const auto accent = all || bool(b.getProperties()["bypass"]) ? juce::Colour(0xffff454b) : cyan;
    juce::Path path;
    if (bool(b.getProperties()["blade"]))
        path = blade(bounds);
    else
        path.addRoundedRectangle(bounds, 4);
    juce::DropShadow(juce::Colours::black.withAlpha(.8f), 6, {0, 4}).drawForPath(g, path);
    if (active)
        for (int i = 4; i > 0; --i)
        {
            g.setColour(accent.withAlpha(all ? .20f : .14f));
            g.strokePath(path, juce::PathStrokeType(float(i) * 3.f));
        }
    const auto fill = active && all ? juce::Colour(0xff8f0d16) : juce::Colour(0xff292a27);
    g.setGradientFill(juce::ColourGradient(fill.brighter(over || down ? .4f : .2f), 0, bounds.getY(),
                                           fill.darker(.7f), 0, bounds.getBottom(), false));
    g.fillPath(path);
    g.setColour(active ? accent : brass);
    g.strokePath(path, juce::PathStrokeType(active ? 2.2f : 1.4f));
    g.setColour(ivory.withAlpha(.4f));
    g.drawLine(bounds.getX() + 17, bounds.getY() + 3, bounds.getRight() - 5, bounds.getY() + 3, .8f);
}
void AssassinLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool, bool)
{
    const float size = float(b.getProperties().getWithDefault("fontSize", 27.f));
    text(g, b.getButtonText(), b.getLocalBounds().toFloat().reduced(6, 2), size, ivory);
}
void AssassinLookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool over, bool)
{
    if (bool(b.getProperties()["lever"]))
    {
        lever(g, float(b.getWidth()) * .5f, float(b.getHeight()) * .7f, 25.f, !b.getToggleState());
        return;
    }
    const bool hf = bool(b.getProperties()["hf"]);
    const auto track = juce::Rectangle<float>(3, float(b.getHeight()) * .5f - 12, 51, 24);
    g.setColour(ink);
    g.fillRoundedRectangle(track.expanded(3), 3);
    g.setColour(brass);
    g.drawRoundedRectangle(track.expanded(3), 3, 1.5f);
    g.setGradientFill(
        juce::ColourGradient(ivory, 5, track.getY(), brass.darker(.25f), 20, track.getBottom(), false));
    g.fillRoundedRectangle(track.getX() + (b.getToggleState() ? 26.f : 2.f), track.getY() + 2, 23, 20, 2);
    if (hf)
        lamp(g, 85, float(b.getHeight()) * .5f, 10, b.getToggleState());
    else
        text(g, b.getButtonText(), {62, 0, float(b.getWidth() - 62), float(b.getHeight())}, 22,
             over ? juce::Colours::white : ivory);
}
void AssassinLookAndFeel::drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int,
                                       juce::ComboBox &)
{
    g.setColour(ink);
    g.fillRoundedRectangle(0, 0, float(w), float(h), 3);
    g.setColour(brass);
    g.drawRoundedRectangle(.5f, .5f, float(w) - 1, float(h) - 1, 3, 1);
    juce::Path p;
    p.addTriangle(float(w) - 22, float(h) * .4f, float(w) - 10, float(h) * .4f, float(w) - 16,
                  float(h) * .65f);
    g.fillPath(p);
}
void LAssassinEditor::OptionsPanel::paint(juce::Graphics &g)
{
    g.setGradientFill(
        juce::ColourGradient(juce::Colour(0xff34332d), 0, 0, ink, 0, float(getHeight()), false));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 9);
    g.setColour(brass);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 8, 2);
    text(g, "PRESET", {20, 12, 270, 27}, 22, brass);
    text(g, "CHANNEL MODE", {310, 12, 205, 27}, 22, brass);
    text(g, "OVERSAMPLING", {540, 12, 210, 27}, 22, brass);
    text(g, "SIDECHAIN HPF", {20, 106, 230, 30}, 22, brass);
    text(g, "LAssassin-76  |  0.4.0", {680, 150, 270, 28}, 20, ivory);
}
LAssassinEditor::LAssassinEditor(LAssassinProcessor &p) : AudioProcessorEditor(p), owner(p)
{
    setLookAndFeel(&theme);
    addAndMakeVisible(surface);
    surface.setSize(width, height);
    surface.addChildComponent(options);
    options.setComponentID("options");
    const char *ids[]{"peak", "laGain", "fetIn", "fetOut", "attack", "release", "in", "out", "mix", "hpf"};
    const char *titles[]{"Peak Reduction", "Gain", "Input", "Output", "Attack",
                         "Release",        "In",   "Out",   "Mix",    "Sidechain HPF"};
    const char *tips[]{"Optical compression drive. Drag vertically; double-click resets.",
                       "LA-2A makeup gain. Faceplate 0-100 = 0-40 dB; host automation remains in dB.",
                       "Drive the fixed-threshold FET compressor.",
                       "1176 makeup gain; drives the next stage.",
                       "1 = 800 us, 7 = 20 us. Higher is faster.",
                       "1 = 1.1 s, 7 = 50 ms. Higher is faster.",
                       "Wet-chain input gain. Dry is taken before In.",
                       "Wet-chain output gain.",
                       "Latency-aligned parallel mix.",
                       "Detector high-pass: 0 Hz = off."};
    for (size_t i = 0; i < dials.size(); ++i)
    {
        auto &d = dials[i];
        auto &s = d.slider;
        (i == 9 ? static_cast<juce::Component &>(options) : surface).addAndMakeVisible(s);
        s.setComponentID(ids[i]);
        s.setName(titles[i]);
        s.setTooltip(tips[i]);
        s.setSliderStyle(i == 9 ? juce::Slider::LinearHorizontal : juce::Slider::RotaryVerticalDrag);
        const float range = i < 2 || i >= 6 ? .75f : 2.f / 3.f;
        s.setRotaryParameters(juce::MathConstants<float>::pi * (2.f - range),
                              juce::MathConstants<float>::pi * (2.f + range), true);
        s.setTextBoxStyle(i == 9 ? juce::Slider::TextBoxRight : juce::Slider::NoTextBox, false, 100, 30);
        s.setScrollWheelEnabled(false);
        s.setMouseDragSensitivity(220);
        s.getProperties().set("sprite", i < 2 ? 0 : i < 4 ? 1 : i < 6 ? 2 : 3);
        s.setNumDecimalPlacesToDisplay(i == 0 || i == 8 || i == 9 ? 0 : 1);
        if (i == 9)
            s.setTextValueSuffix(" Hz");
        d.attachment =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, ids[i], s);
        auto *parameter = p.apvts.getParameter(ids[i]);
        s.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
        if (i < 9)
        {
            surface.addAndMakeVisible(d.value);
            d.value.setComponentID(juce::String(ids[i]) + "Value");
            d.value.setName(juce::String(titles[i]) + " value");
            d.value.setTooltip(tips[i]);
            d.value.setJustificationType(juce::Justification::centred);
            d.value.setBorderSize(juce::BorderSize<int>(0));
            d.value.getProperties().set("readout", true);
            d.value.setColour(juce::Label::textColourId, i < 2 ? ink : ivory);
            d.value.setEditable(false, true);
            d.value.onTextChange = [this, i]
            {
                auto &dial = dials[i];
                const auto valueText = dial.value.getText().trim();
                if (!valueText.containsAnyOf("0123456789"))
                {
                    updateReadout(i);
                    return;
                }
                const double value = valueText.getDoubleValue() / (i == 1 ? 2.5 : 1.0);
                auto *param = owner.apvts.getParameter(dial.slider.getComponentID());
                param->beginChangeGesture();
                dial.slider.setValue(value, juce::sendNotificationSync);
                param->endChangeGesture();
                updateReadout(i);
            };
            s.onValueChange = [this, i] { updateReadout(i); };
            updateReadout(i);
        }
    }
    auto bindButton = [&](juce::Button &button, const char *id, bool advanced = false)
    {
        (advanced ? static_cast<juce::Component &>(options) : surface).addAndMakeVisible(button);
        button.setComponentID(id);
        button.setClickingTogglesState(true);
        buttons.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, button));
    };
    bindButton(laBypass, "laBypass");
    bindButton(fetBypass, "fetBypass");
    bindButton(globalBypass, "bypass");
    bindButton(order, "order");
    bindButton(limit, "limit");
    bindButton(hf, "hf");
    bindButton(link, "link", true);
    bindButton(automatic, "auto", true);
    for (auto *b : {&laBypass, &fetBypass, &globalBypass})
        b->getProperties().set("bypass", true);
    laBypass.getProperties().set("fontSize", 18.f);
    fetBypass.getProperties().set("fontSize", 18.f);
    limit.getProperties().set("lever", true);
    order.getProperties().set("lever", true);
    hf.getProperties().set("hf", true);
    limit.setName("Compress / Limit");
    limit.setTooltip("Lever right/up: Compress (3:1). Left/down: Limit (100:1).");
    hf.setName("HF Emphasis");
    hf.setTooltip("High-frequency emphasis in the optical detector only.");
    order.setName("Order");
    order.setTooltip("Switch serial order with a short crossfade.");
    laBypass.setTooltip("Bypass optical compression, makeup and colour.");
    fetBypass.setTooltip("Bypass FET compression, makeup and colour.");
    globalBypass.setTooltip("Bypass the whole plugin with a latency-aligned, smoothed dry path.");
    automatic.setTooltip("Slow RMS compensation, limited to +/-12 dB.");
    link.setTooltip("Share the larger detector level across channels.");
    surface.addAndMakeVisible(orderForward);
    surface.addAndMakeVisible(orderReverse);
    orderForward.setComponentID("orderForward");
    orderReverse.setComponentID("orderReverse");
    orderForward.getProperties().set("fontSize", 22.f);
    orderReverse.getProperties().set("fontSize", 22.f);
    orderForward.onClick = [this] { order.setToggleState(false, juce::sendNotificationSync); };
    orderReverse.onClick = [this] { order.setToggleState(true, juce::sendNotificationSync); };
    const char *ratiosText[]{"4", "8", "12", "20", "ALL"};
    for (size_t i = 0; i < ratios.size(); ++i)
    {
        auto &b = ratios[i];
        surface.addAndMakeVisible(b);
        b.setButtonText(ratiosText[i]);
        b.setName(juce::String("Ratio ") + ratiosText[i]);
        b.setComponentID("ratio" + juce::String(int(i)));
        b.getProperties().set("blade", true);
        b.getProperties().set("all", i == 4);
        b.setTooltip(i == 4 ? "All-buttons: extreme ratio, timing and distortion."
                            : juce::String(ratiosText[i]) + ":1 FET ratio");
        b.onClick = [this, i] { ratioAttachment->setValueAsCompleteGesture(float(i)); };
    }
    ratioAttachment = std::make_unique<juce::ParameterAttachment>(
        *p.apvts.getParameter("ratio"),
        [this](float v)
        {
            const int selected = juce::jlimit(0, 4, int(std::lround(v)));
            for (size_t i = 0; i < ratios.size(); ++i)
                ratios[i].setToggleState(int(i) == selected, juce::dontSendNotification);
        },
        nullptr);
    ratioAttachment->sendInitialUpdate();
    auto bindCombo = [&](juce::ComboBox &combo, const char *id, juce::StringArray items)
    {
        options.addAndMakeVisible(combo);
        combo.setComponentID(id);
        combo.addItemList(items, 1);
        combos.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, id, combo));
    };
    bindCombo(mode, "ms", {"STEREO", "MID-SIDE"});
    bindCombo(oversampling, "os", {"2x OS", "4x OS"});
    options.addAndMakeVisible(presets);
    presets.setComponentID("presets");
    for (int i = 0; i < p.getNumPrograms(); ++i)
        presets.addItem(p.getProgramName(i), i + 1);
    presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { owner.setCurrentProgram(presets.getSelectedId() - 1); };
    surface.addAndMakeVisible(setup);
    setup.setComponentID("setup");
    setup.getProperties().set("fontSize", 21.f);
    setup.setTooltip("Presets, Stereo/Mid-Side, oversampling, sidechain HPF, Link and Auto Gain.");
    setup.setClickingTogglesState(true);
    setup.onClick = [this]
    {
        options.setVisible(setup.getToggleState());
        if (options.isVisible())
            options.toFront(false);
    };
    background =
        juce::ImageCache::getFromMemory(LAssassinSkin::Backplate_png, LAssassinSkin::Backplate_pngSize);
    previousOrder = order.getToggleState();
    setResizable(true, false);
    getConstrainer()->setFixedAspectRatio(double(width) / height);
    setResizeLimits(1043, 543, 2086, 1086);
    setSize(1390, 724);
    timerCallback();
    startTimerHz(60);
}
LAssassinEditor::~LAssassinEditor()
{
    stopTimer();
    ratioAttachment.reset();
    buttons.clear();
    combos.clear();
    for (auto &d : dials)
        d.attachment.reset();
    setLookAndFeel(nullptr);
}
void LAssassinEditor::updateReadout(size_t i)
{
    auto &d = dials[i];
    if (d.value.isBeingEdited())
        return;
    const double value = d.slider.getValue() * (i == 1 ? 2.5 : 1.0);
    const bool db = i == 2 || i == 3 || i == 6 || i == 7;
    d.value.setText(juce::String(value, i < 2 || i == 8 ? 0 : 1) + (db       ? " dB"
                                                                    : i == 8 ? "%"
                                                                             : ""),
                    juce::dontSendNotification);
}
void LAssassinEditor::resized()
{
    surface.setTransform(
        juce::AffineTransform::scale(float(getWidth()) / width, float(getHeight()) / height));
    const juce::Rectangle<int> knobBounds[]{
        {183, 429, 146, 146},  {517, 429, 146, 146},  {1085, 193, 150, 150},
        {1413, 193, 150, 150}, {1103, 435, 114, 114}, {1431, 435, 114, 114},
        {77, 763, 90, 90},     {306, 763, 90, 90},    {535, 763, 90, 90}};
    const juce::Rectangle<int> values[]{{211, 622, 98, 30},   {542, 622, 98, 30},  {1103, 369, 121, 29},
                                        {1428, 369, 124, 29}, {1117, 578, 92, 27}, {1443, 578, 93, 27},
                                        {181, 801, 80, 27},   {410, 801, 79, 27},  {639, 801, 71, 27}};
    for (size_t i = 0; i < 9; ++i)
    {
        dials[i].slider.setBounds(knobBounds[i]);
        dials[i].value.setBounds(values[i]);
    }
    laBypass.setBounds(687, 137, 72, 30);
    fetBypass.setBounds(1596, 144, 72, 30);
    globalBypass.setBounds(1530, 24, 122, 46);
    setup.setBounds(1405, 29, 104, 37);
    limit.setBounds(109, 641, 72, 75);
    hf.setBounds(632, 670, 102, 40);
    orderForward.setBounds(804, 329, 128, 48);
    orderReverse.setBounds(804, 382, 128, 48);
    order.setBounds(833, 436, 72, 75);
    for (size_t i = 0; i < ratios.size(); ++i)
        ratios[i].setBounds(1131 + int(i) * 102, 635, 107, 65);
    options.setBounds(448, 92, 988, 194);
    presets.setBounds(20, 45, 270, 38);
    mode.setBounds(310, 45, 205, 38);
    oversampling.setBounds(540, 45, 210, 38);
    link.setBounds(760, 45, 218, 38);
    automatic.setBounds(760, 100, 218, 38);
    dials[9].slider.setBounds(252, 107, 474, 33);
}
void LAssassinEditor::drawVu(juce::Graphics &g)
{
    const juce::Rectangle<int> clip{146, 201, 555, 159};
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(clip);
    const float glow = std::min(optical / 12.f, 1.f) * (1.f + .12f * std::sin(pulse));
    // Interpolate the actual engraved scale positions; 0 dB must rest at the
    // printed zero, not at the red +3 end of the original concept art.
    constexpr float db[]{0, 2, 3, 5, 7, 10, 20};
    constexpr juce::Point<float> points[]{{563, 257}, {507, 245}, {455, 242}, {390, 243},
                                          {335, 251}, {279, 267}, {222, 293}};
    const float value = juce::jlimit(0.f, 20.f, optical);
    size_t idx = 0;
    while (idx < 5 && value > db[idx + 1])
        ++idx;
    const float fraction = (value - db[idx]) / (db[idx + 1] - db[idx]);
    const auto scalePoint = points[idx] + (points[idx + 1] - points[idx]) * fraction;
    const juce::Point<float> foot{426, 363};
    const auto vector = scalePoint - foot;
    const float length = vector.getDistanceFromOrigin();
    const auto along = vector / length;
    const auto normal = juce::Point<float>(-along.y, along.x);
    const auto tip = scalePoint + along * 20.f;
    if (glow > .01f)
        for (int i = 6; i > 0; --i)
        {
            g.setColour(cyan.withAlpha(glow * .045f));
            g.drawLine({foot, tip}, float(i) * 5.f);
        }
    const auto root = foot + along * 30.f;
    juce::Path feather;
    feather.startNewSubPath(root);
    // Slightly serrated silhouette and individual dark barbs keep this visibly
    // a feather at normal plugin size, rather than a flat triangular pointer.
    for (int side : {-1, 1})
        for (int j = 0; j <= 32; ++j)
        {
            const int i = side == -1 ? j : 32 - j;
            const float t = float(i) / 32.f;
            const float spread = std::sin(t * juce::MathConstants<float>::pi) * (side == -1 ? 12.f : 9.f) *
                                 (i % 2 == 0 ? 1.f : .82f);
            feather.lineTo(root + (tip - root) * t + normal * (float(side) * spread));
        }
    feather.closeSubPath();
    const auto mid = (root + tip) * .5f;
    const auto shadeLeft = mid - normal * 12.f, shadeRight = mid + normal * 12.f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2a20), shadeLeft.x, shadeLeft.y,
                                           juce::Colour(0xffb7975a), shadeRight.x, shadeRight.y, false));
    g.fillPath(feather);
    g.setColour(brass);
    g.strokePath(feather, juce::PathStrokeType(.7f));
    for (int i = 1; i < 22; ++i)
    {
        const float t = float(i) / 23.f;
        const auto point = root + (tip - root) * t;
        const float spread = std::sin(t * juce::MathConstants<float>::pi) * 10.f;
        g.setColour(ivory.withAlpha(.7f));
        g.drawLine({point, point - along * 8.f - normal * spread}, .8f);
        g.setColour(ink.withAlpha(.85f));
        g.drawLine({point, point - along * 8.f + normal * spread * .75f}, 1.f);
    }
    g.setColour(brass);
    g.drawLine({foot, tip}, 2.4f);
    g.setColour(ivory);
    g.drawLine({foot, tip}, .7f);
    g.setColour(ink);
    g.fillEllipse(412, 350, 28, 28);
    g.setColour(brass);
    g.drawEllipse(413, 351, 26, 26, 3);
    text(g, "-" + juce::String(optical, 1) + " dB", {547, 332, 133, 22}, 22, ink);
}
void LAssassinEditor::drawMeters(juce::Graphics &g)
{
    // The concept's printed scale is intentionally nonuniform. Match those
    // physical tick positions so the illumination agrees with the dB readout.
    constexpr float marks[]{0, 3, 6, 12, 18, 24};
    constexpr float positions[]{800, 928, 1056, 1190, 1315, 1450};
    const float reduction = juce::jlimit(0.f, 24.f, total);
    size_t index = 0;
    while (index < 4 && reduction > marks[index + 1])
        ++index;
    const float fraction = (reduction - marks[index]) / (marks[index + 1] - marks[index]);
    const float end = positions[index] + fraction * (positions[index + 1] - positions[index]);
    const int lit = total > 24 ? 34 : juce::jlimit(0, 33, int(std::lround((end - 800.f) / 19.8f)));
    for (int i = 0; i < lit; ++i)
    {
        const auto r = juce::Rectangle<float>(798.f + float(i) * 19.8f, 797.f, 16.7f, 19.f);
        g.setColour(cyan.withAlpha(.2f));
        g.fillRect(r.expanded(2));
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.interpolatedWith(cyan, .35f),
                                               r.getCentreX(), r.getCentreY(), cyan.darker(.5f), r.getX(),
                                               r.getY(), true));
        g.fillRect(r);
    }
    text(g, "-" + juce::String(total, 1) + " dB", {1515, 782, 126, 45}, 34, cyan);
    text(g, "GR  -" + juce::String(fet, 1) + " dB", {1237, 576, 153, 28}, 23, cyan);
    for (int i = 0; i < 2; ++i)
    {
        const float x = 181.f + float(i) * 229.f, level = i == 0 ? input : output;
        g.setColour(ink);
        g.fillRect(x, 838.f, 80.f, 4.f);
        g.setColour(level >= 0 ? red : cyan);
        g.fillRect(x, 838.f, juce::jlimit(0.f, 1.f, (level + 60) / 60) * 80.f, 4.f);
        text(g, level < -89 ? "-inf dBFS" : juce::String(level, 1) + " dBFS", {x - 10, 846, 100, 17}, 17,
             ivory);
    }
    // The small global dials have no printed scale on the extracted backplate.
    for (int k = 0; k < 3; ++k)
        for (int i = 0; i <= 12; ++i)
        {
            const float a = juce::MathConstants<float>::pi * (1.25f + 1.5f * float(i) / 12.f);
            const float cx = 122.f + float(k) * 229.f;
            g.setColour(brass);
            g.drawLine(cx + std::sin(a) * 48.f, 808 - std::cos(a) * 48.f, cx + std::sin(a) * 54.f,
                       808 - std::cos(a) * 54.f, 1.f);
        }
    lamp(g, 1674, 48, 11, !globalBypass.getToggleState());
}
void LAssassinEditor::drawBypass(juce::Graphics &g, juce::Rectangle<float> b)
{
    g.setColour(ink.withAlpha(.49f));
    g.fillRoundedRectangle(b, 8);
    g.setColour(juce::Colours::white.withAlpha(.035f));
    for (float y = b.getY(); y < b.getBottom(); y += 4)
        g.drawHorizontalLine(int(y), b.getX(), b.getRight());
}
void LAssassinEditor::paint(juce::Graphics &g)
{
    g.addTransform(juce::AffineTransform::scale(float(getWidth()) / width, float(getHeight()) / height));
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImageAt(background, 0, 0);
    drawVu(g);
    drawMeters(g);
    const bool all = globalBypass.getToggleState();
    if (all || laBypass.getToggleState())
        drawBypass(g, {20, 96, 775, 622});
    if (all || fetBypass.getToggleState())
        drawBypass(g, {944, 96, 773, 622});
    if (sweep > 0)
    {
        const float y = 330 + (1 - sweep) * 190;
        for (int i = 5; i > 0; --i)
        {
            g.setColour(cyan.withAlpha(.075f * float(6 - i) * sweep));
            g.drawLine(799, y + 27, 941, y - 27, float(i));
        }
    }
}
void LAssassinEditor::timerCallback()
{
    auto follow = [](float old, float target, float fall)
    { return old + (target - old) * (target > old ? .48f : fall); };
    optical = follow(optical, owner.laGR.load(std::memory_order_relaxed), .075f);
    fet = follow(fet, owner.fetGR.load(std::memory_order_relaxed), .12f);
    total = optical + fet;
    input = follow(input, owner.inputDb.load(std::memory_order_relaxed), .06f);
    output = follow(output, owner.outputDb.load(std::memory_order_relaxed), .06f);
    pulse = std::fmod(pulse + .075f, juce::MathConstants<float>::twoPi);
    if (previousOrder != order.getToggleState())
    {
        previousOrder = order.getToggleState();
        sweep = 1;
    }
    sweep = std::max(0.f, sweep - 1.f / 12.f);
    orderForward.setToggleState(!previousOrder, juce::dontSendNotification);
    orderReverse.setToggleState(previousOrder, juce::dontSendNotification);
    laBypass.setButtonText(laBypass.getToggleState() ? "OFF" : "ON");
    fetBypass.setButtonText(fetBypass.getToggleState() ? "OFF" : "ON");
    const float laAlpha = laBypass.getToggleState() || globalBypass.getToggleState() ? .4f : 1.f;
    const float fetAlpha = fetBypass.getToggleState() || globalBypass.getToggleState() ? .4f : 1.f;
    for (size_t i = 0; i < 6; ++i)
    {
        dials[i].slider.setAlpha(i < 2 ? laAlpha : fetAlpha);
        dials[i].value.setAlpha(i < 2 ? laAlpha : fetAlpha);
    }
    limit.setAlpha(laAlpha);
    hf.setAlpha(laAlpha);
    for (auto &b : ratios)
        b.setAlpha(fetAlpha);
    const int selected = owner.getCurrentProgram() + 1;
    if (presets.getSelectedId() != selected)
        presets.setSelectedId(selected, juce::dontSendNotification);
    repaint();
}
