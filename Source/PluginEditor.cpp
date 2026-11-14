#include "PluginEditor.h"
#include "LAssassinFonts.h"
namespace
{
const juce::Colour ivory{0xffe8e1cf}, ink{0xff191b1d}, red{0xffa3141a}, cyan{0xff6fe7ff}, brass{0xff8c754c};
constexpr int width = 1200, height = 680;
juce::Font retroFont(float size, bool display = false)
{
    static auto displayFace = juce::Typeface::createSystemTypefaceFor(
        LAssassinFonts::CinzelDecorativeBold_ttf, LAssassinFonts::CinzelDecorativeBold_ttfSize);
    static auto controlFace = juce::Typeface::createSystemTypefaceFor(LAssassinFonts::ZillaSlabBold_ttf,
                                                                      LAssassinFonts::ZillaSlabBold_ttfSize);
    return juce::Font(juce::FontOptions(display ? displayFace : controlFace).withHeight(size));
}
void text(juce::Graphics &g, const juce::String &s, juce::Rectangle<float> r, float size, juce::Colour c,
          juce::Justification align = juce::Justification::centred)
{
    g.setColour(c);
    g.setFont(retroFont(size * 1.15f));
    g.drawText(s, r, align);
}
void serif(juce::Graphics &g, const juce::String &s, juce::Rectangle<float> r, float size, juce::Colour c)
{
    g.setColour(c);
    g.setFont(retroFont(size < 25.f ? size * 1.2f : size * 1.05f, true));
    g.drawText(s, r, juce::Justification::centred);
}
juce::Path blade(juce::Rectangle<float> b)
{
    juce::Path p;
    p.startNewSubPath(b.getX() + 9, b.getY());
    p.lineTo(b.getRight(), b.getY());
    p.lineTo(b.getRight() - 9, b.getBottom());
    p.lineTo(b.getX(), b.getBottom());
    p.closeSubPath();
    return p;
}
void feather(juce::Graphics &g, juce::Point<float> start, juce::Point<float> end, juce::Colour colour,
             float spread)
{
    g.setColour(colour);
    g.drawLine({start, end}, 1.2f);
    const auto vector = end - start;
    const juce::Point<float> normal{-vector.y, vector.x};
    for (int i = 1; i < 12; ++i)
    {
        const float t = float(i) / 13.f;
        auto p = start + vector * t;
        const float shape = std::sin(t * juce::MathConstants<float>::pi) * spread;
        g.drawLine({p, p + vector * .12f + normal * shape}, .7f);
        g.drawLine({p, p + vector * .09f - normal * shape * .65f}, .7f);
    }
}
} // namespace
AssassinLookAndFeel::AssassinLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, ivory);
    setColour(juce::Slider::textBoxBackgroundColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, brass.withAlpha(.6f));
    setColour(juce::Slider::trackColourId, cyan);
    setColour(juce::Slider::thumbColourId, brass);
    setColour(juce::ComboBox::textColourId, ivory);
    setColour(juce::PopupMenu::backgroundColourId, ink);
    setColour(juce::PopupMenu::textColourId, ivory);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, red);
    setColour(juce::Label::textColourId, ivory);
    setColour(juce::TooltipWindow::backgroundColourId, ivory);
    setColour(juce::TooltipWindow::textColourId, ink);
}
juce::Font AssassinLookAndFeel::getLabelFont(juce::Label &)
{
    return retroFont(16.f);
}
juce::Font AssassinLookAndFeel::getComboBoxFont(juce::ComboBox &)
{
    return retroFont(16.f);
}
juce::Font AssassinLookAndFeel::getPopupMenuFont()
{
    return retroFont(17.f);
}
juce::Font AssassinLookAndFeel::getSliderPopupFont(juce::Slider &)
{
    return retroFont(16.f);
}
void AssassinLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float pos,
                                           float start, float end, juce::Slider &slider)
{
    const bool light = bool(slider.getProperties()["ivory"]), small = bool(slider.getProperties()["small"]);
    const auto label = light ? ink : ivory;
    const float radius = std::min(float(w) * .33f, float(h) * .34f), cx = float(x) + float(w) * .5f,
                cy = float(y) + float(h) * .55f;
    const float angle = start + pos * (end - start);
    const bool timing = bool(slider.getProperties()["timing"]);
    const int steps = timing ? 6 : 10;
    for (int i = 0; i <= steps; ++i)
    {
        const float a = start + float(i) / float(steps) * (end - start);
        const juce::Point<float> p{cx + std::sin(a) * (radius + 6), cy - std::cos(a) * (radius + 6)};
        const juce::Point<float> q{cx + std::sin(a) * (radius + 11), cy - std::cos(a) * (radius + 11)};
        g.setColour(label.withAlpha(.8f));
        g.drawLine({p, q}, 1.1f);
        if (!small)
        {
            const float v = timing ? float(i + 1)
                                   : float(slider.getMinimum() +
                                           (slider.getMaximum() - slider.getMinimum()) * double(i) / steps);
            const float rr = radius + 22;
            const auto s = juce::String(v, 0);
            text(g, s, {cx + std::sin(a) * rr - 16, cy - std::cos(a) * rr - 7, 32, 17}, 11, label);
        }
    }
    if (slider.isMouseOverOrDragging())
    {
        for (int i = 3; i >= 1; --i)
        {
            juce::Path arc;
            arc.addCentredArc(cx, cy, radius + 3.f + float(i), radius + 3.f + float(i), 0, start, angle,
                              true);
            g.setColour(cyan.withAlpha(.09f * float(4 - i)));
            g.strokePath(arc, juce::PathStrokeType(float(i + 1)));
        }
    }
    g.setColour(juce::Colours::black.withAlpha(.45f));
    g.fillEllipse(cx - radius - 2, cy - radius + 3, radius * 2 + 4, radius * 2 + 4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55585a), cx - radius, cy - radius, ink,
                                           cx + radius, cy + radius, false));
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
    for (int i = 0; i < 48; ++i)
    {
        const float a = float(i) * juce::MathConstants<float>::twoPi / 48.f;
        g.setColour(juce::Colours::black.withAlpha(.6f));
        g.drawLine(cx + std::sin(a) * (radius - 4), cy - std::cos(a) * (radius - 4),
                   cx + std::sin(a) * radius, cy - std::cos(a) * radius, 1.f);
    }
    const float inner = radius * .76f;
    g.setGradientFill(juce::ColourGradient(light ? ivory : juce::Colour(0xffb4a07c), cx - inner, cy - inner,
                                           light ? juce::Colour(0xffb6a57d) : juce::Colour(0xff514735),
                                           cx + inner, cy + inner, false));
    g.fillEllipse(cx - inner, cy - inner, inner * 2, inner * 2);
    g.setColour(brass);
    g.drawEllipse(cx - inner, cy - inner, inner * 2, inner * 2, 1.f);
    for (int i = 0; i < 9; ++i)
    {
        g.setColour(juce::Colours::white.withAlpha(.02f));
        g.drawEllipse(cx - inner + float(i) * 2, cy - inner + float(i) * 2, inner * 2 - float(i) * 4,
                      inner * 2 - float(i) * 4, .5f);
    }
    const juce::Point<float> tip{cx + std::sin(angle) * (radius - 3), cy - std::cos(angle) * (radius - 3)};
    const juce::Point<float> tail{cx + std::sin(angle) * radius * .28f, cy - std::cos(angle) * radius * .28f};
    g.setColour(ink);
    g.drawLine({tail, tip}, 5.f);
    g.setColour(slider.isMouseOverOrDragging() ? cyan : ivory);
    g.drawLine({tail, tip}, 2.3f);
}
void AssassinLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &,
                                               bool over, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced(1);
    const bool active = b.getToggleState(), all = bool(b.getProperties()["all"]),
               isBlade = bool(b.getProperties()["blade"]);
    const auto accent = all ? red : cyan;
    if (active)
    {
        g.setColour(accent.withAlpha(.16f));
        g.fillRoundedRectangle(bounds.expanded(1), 4);
    }
    auto fill =
        active ? (all ? juce::Colour(0xff670b10) : juce::Colour(0xff17353b)) : juce::Colour(0xff25282a);
    if (over || down)
        fill = fill.brighter(.15f);
    g.setGradientFill(
        juce::ColourGradient(fill.brighter(.12f), 0, 0, fill.darker(.3f), 0, bounds.getBottom(), false));
    if (isBlade)
    {
        auto p = blade(bounds);
        g.fillPath(p);
        g.setColour(active ? accent : brass);
        g.strokePath(p, juce::PathStrokeType(1.f));
    }
    else
    {
        g.fillRoundedRectangle(bounds, 3);
        g.setColour(active ? accent : brass.withAlpha(.7f));
        g.drawRoundedRectangle(bounds, 3, 1);
    }
}
void AssassinLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool, bool)
{
    g.setColour(ivory);
    g.setFont(retroFont(b.getHeight() > 40 ? 14.f : 13.f, true));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(3), juce::Justification::centred, 2);
}
void AssassinLookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool over, bool)
{
    const bool light = bool(b.getProperties()["ivory"]);
    const auto c = light ? ink : ivory;
    auto track = juce::Rectangle<float>(2, float(b.getHeight()) * .5f - 7, 30, 14);
    g.setColour(ink);
    g.fillRoundedRectangle(track, 3);
    g.setColour(brass);
    g.drawRoundedRectangle(track, 3, 1);
    g.setColour(b.getToggleState() ? cyan : juce::Colour(0xffa59b83));
    g.fillRoundedRectangle(track.getX() + (b.getToggleState() ? 16.f : 2.f), track.getY() + 2, 12, 10, 2);
    text(g, b.getButtonText(), {39, 0, float(b.getWidth() - 39), float(b.getHeight())}, 10.5f,
         over ? c.brighter(.2f) : c, juce::Justification::centredLeft);
}
void AssassinLookAndFeel::drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int,
                                       juce::ComboBox &)
{
    g.setColour(ink);
    g.fillRoundedRectangle(0, 0, float(w), float(h), 3);
    g.setColour(brass.withAlpha(.75f));
    g.drawRoundedRectangle(.5f, .5f, float(w) - 1, float(h) - 1, 3, 1);
    juce::Path p;
    p.startNewSubPath(float(w) - 17, float(h) * .43f);
    p.lineTo(float(w) - 9, float(h) * .43f);
    p.lineTo(float(w) - 13, float(h) * .64f);
    p.closeSubPath();
    g.setColour(brass);
    g.fillPath(p);
}
LAssassinEditor::LAssassinEditor(LAssassinProcessor &p) : AudioProcessorEditor(p), owner(p)
{
    setLookAndFeel(&theme);
    addAndMakeVisible(surface);
    surface.setSize(width, height);
    const char *ids[]{"peak", "laGain", "fetIn", "fetOut", "attack", "release", "in", "out", "mix", "hpf"};
    const char *titles[]{"Peak Reduction", "Gain", "Input", "Output", "Attack",
                         "Release",        "In",   "Out",   "Mix",    "Sidechain HPF"};
    const char *tips[]{"Optical compression drive. Double-click to reset.",
                       "LA-2A makeup gain; drives the following stage.",
                       "Drive the fixed-threshold FET compressor.",
                       "1176 makeup gain; drives the following stage.",
                       "1 = 800 us, 7 = 20 us. Higher is faster.",
                       "1 = 1.1 s, 7 = 50 ms. Higher is faster.",
                       "Wet-chain input gain. Dry signal is taken before In.",
                       "Wet-chain output gain.",
                       "Latency-aligned linear parallel mix.",
                       "Detector-only high-pass, 0 Hz = off."};
    for (size_t i = 0; i < dials.size(); ++i)
    {
        auto &d = dials[i];
        auto &s = d.slider;
        surface.addAndMakeVisible(s);
        s.setComponentID(ids[i]);
        s.setName(titles[i]);
        s.setTooltip(tips[i]);
        s.setSliderStyle(i == 9 ? juce::Slider::LinearHorizontal : juce::Slider::RotaryVerticalDrag);
        s.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f,
                              true);
        s.setTextBoxStyle(i == 9 ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow, false,
                          i >= 6 ? 78 : 104, 23);
        s.setPopupDisplayEnabled(false, false, this);
        s.setScrollWheelEnabled(false);
        s.getProperties().set("ivory", i < 2 || i == 6 || i == 7 || i == 8);
        s.getProperties().set("small", i >= 6);
        s.getProperties().set("timing", i == 4 || i == 5);
        if (i == 1 || i == 2 || i == 3 || i == 6 || i == 7)
            s.setTextValueSuffix(" dB");
        if (i == 8)
            s.setTextValueSuffix(" %");
        if (i == 9)
            s.setTextValueSuffix(" Hz");
        s.setNumDecimalPlacesToDisplay(i == 0 || i == 8 || i == 9 ? 0 : 1);
        d.attachment =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, ids[i], s);
        auto *parameter = p.apvts.getParameter(ids[i]);
        s.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
    }
    auto bindButton = [&](juce::Button &b, const char *id)
    {
        surface.addAndMakeVisible(b);
        b.setComponentID(id);
        b.setClickingTogglesState(true);
        buttons.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, b));
    };
    bindButton(laBypass, "laBypass");
    bindButton(fetBypass, "fetBypass");
    bindButton(order, "order");
    bindButton(limit, "limit");
    bindButton(hf, "hf");
    bindButton(link, "link");
    bindButton(automatic, "auto");
    limit.setTooltip("Off = Compress (3:1); on = Limit (100:1). T4 envelope remains active.");
    hf.setTooltip("High-frequency emphasis in the optical detector only.");
    limit.getProperties().set("ivory", true);
    hf.getProperties().set("ivory", true);
    order.setTooltip("Switch serial order. Both stages keep their own controls.");
    laBypass.setTooltip("Bypass optical compression, makeup gain and colour.");
    fetBypass.setTooltip("Bypass FET compression, makeup gain and colour.");
    automatic.setTooltip("Slow RMS compensation, limited to +/-12 dB.");
    link.setTooltip("Share the larger detector level across the two channels.");
    const char *values[]{"4", "8", "12", "20", "ALL"};
    for (size_t i = 0; i < ratios.size(); ++i)
    {
        auto &b = ratios[i];
        surface.addAndMakeVisible(b);
        b.setButtonText(values[i]);
        b.setName(juce::String("Ratio ") + values[i]);
        b.setComponentID("ratio" + juce::String(int(i)));
        b.getProperties().set("blade", true);
        b.getProperties().set("all", i == 4);
        b.setTooltip(i == 4 ? "All-buttons character: aggressive ratio, timing and distortion."
                            : juce::String(values[i]) + ":1 FET ratio");
        b.onClick = [this, i]
        {
            ratioAttachment->beginGesture();
            ratioAttachment->setValueAsPartOfGesture(float(i));
            ratioAttachment->endGesture();
        };
    }
    ratioAttachment = std::make_unique<juce::ParameterAttachment>(
        *p.apvts.getParameter("ratio"),
        [this](float value)
        {
            const int selected = juce::jlimit(0, 4, int(std::lround(value)));
            for (size_t i = 0; i < ratios.size(); ++i)
                ratios[i].setToggleState(int(i) == selected, juce::dontSendNotification);
        },
        nullptr);
    ratioAttachment->sendInitialUpdate();
    auto bindCombo = [&](juce::ComboBox &c, const char *id, juce::StringArray items)
    {
        surface.addAndMakeVisible(c);
        c.setComponentID(id);
        c.addItemList(items, 1);
        combos.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, id, c));
    };
    bindCombo(mode, "ms", {"STEREO", "MID-SIDE"});
    bindCombo(oversampling, "os", {"2x OS", "4x OS"});
    surface.addAndMakeVisible(presets);
    presets.setComponentID("presets");
    for (int i = 0; i < p.getNumPrograms(); ++i)
        presets.addItem(p.getProgramName(i), i + 1);
    presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { owner.setCurrentProgram(presets.getSelectedId() - 1); };
    previousOrder = order.getToggleState();
    createBackground();
    setResizable(true, false);
    getConstrainer()->setFixedAspectRatio(double(width) / height);
    setResizeLimits(960, 544, 1800, 1020);
    setSize(width, height);
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
void LAssassinEditor::put(juce::Component &c, int x, int y, int w, int h)
{
    c.setBounds(x, y, w, h);
}
void LAssassinEditor::resized()
{
    surface.setTransform(
        juce::AffineTransform::scale(float(getWidth()) / width, float(getHeight()) / height));
    put(dials[0].slider, 61, 301, 192, 158);
    put(dials[1].slider, 294, 301, 192, 158);
    put(dials[2].slider, 670, 142, 196, 155);
    put(dials[3].slider, 933, 142, 196, 155);
    put(dials[4].slider, 692, 316, 154, 124);
    put(dials[5].slider, 954, 316, 154, 124);
    put(dials[6].slider, 32, 538, 100, 83);
    put(dials[7].slider, 151, 538, 100, 83);
    put(dials[8].slider, 270, 538, 100, 83);
    put(dials[9].slider, 375, 637, 170, 24);
    put(laBypass, 440, 106, 73, 25);
    put(fetBypass, 1088, 106, 73, 25);
    put(limit, 65, 479, 181, 24);
    put(hf, 292, 479, 175, 24);
    put(order, 545, 267, 76, 60);
    for (size_t i = 0; i < ratios.size(); ++i)
        put(ratios[i], 715 + int(i) * 87, 465, 78, 29);
    put(presets, 917, 30, 245, 30);
    put(link, 31, 637, 137, 26);
    put(mode, 183, 637, 114, 26);
    put(automatic, 573, 637, 131, 26);
    put(oversampling, 738, 637, 94, 26);
}
void LAssassinEditor::createBackground()
{
    background = juce::Image(juce::Image::RGB, width, height, true);
    juce::Graphics g(background);
    g.fillAll(juce::Colour(0xff201513));
    g.setColour(brass);
    g.drawRoundedRectangle(7, 7, 1186, 666, 8, 1.5f);
    // Original stitched leather frame; no external textures or franchise assets.
    g.setColour(juce::Colour(0xff7e5340));
    for (int x = 20; x < 1180; x += 13)
    {
        g.drawLine(float(x), 4, float(x + 7), 4, 1);
        g.drawLine(float(x), 676, float(x + 7), 676, 1);
    }
    for (int y = 15; y < 670; y += 13)
    {
        g.drawLine(4, float(y), 4, float(y + 7), 1);
        g.drawLine(1196, float(y), 1196, float(y + 7), 1);
    }
    auto panel = [&](juce::Rectangle<float> b, bool light)
    {
        g.setGradientFill(juce::ColourGradient(light ? ivory : juce::Colour(0xff2e3030), b.getX(), b.getY(),
                                               light ? juce::Colour(0xffcdbf9f) : juce::Colour(0xff141719),
                                               b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle(b, 5);
        g.setColour(brass);
        g.drawRoundedRectangle(b.reduced(1), 5, 1);
        g.setColour((light ? ink : ivory).withAlpha(.16f));
        g.drawRoundedRectangle(b.reduced(6), 3, .8f);
    };
    panel({18, 16, 1164, 65}, true);
    panel({20, 94, 510, 420}, true);
    panel({636, 94, 544, 420}, false);
    panel({20, 525, 1160, 101}, false);
    panel({20, 632, 1160, 36}, false);
    juce::Random random(760276);
    for (int i = 0; i < 19000; ++i)
    {
        const float x = float(random.nextInt(width)), y = float(random.nextInt(height));
        g.setColour(juce::Colours::black.withAlpha(random.nextFloat() * .04f));
        g.fillRect(x, y, random.nextFloat() * 2 + .3f, .6f);
    }
    for (int i = 0; i < 310; ++i)
    {
        const float y = 98 + random.nextFloat() * 410;
        g.setColour(juce::Colours::white.withAlpha(.018f));
        g.drawLine(643, y, 1172, y, .4f);
    }
    auto rivet = [&](float x, float y)
    {
        g.setColour(ink);
        g.fillEllipse(x - 5, y - 4, 11, 11);
        g.setGradientFill(juce::ColourGradient(ivory, x - 4, y - 4, brass.darker(), x + 4, y + 4, false));
        g.fillEllipse(x - 4, y - 4, 8, 8);
        g.setColour(ink);
        g.drawLine(x - 2, y + 2, x + 2, y - 2, 1);
    };
    for (auto point : std::array<juce::Point<float>, 12>{{{31, 29},
                                                          {1168, 29},
                                                          {32, 107},
                                                          {518, 107},
                                                          {32, 501},
                                                          {518, 501},
                                                          {648, 107},
                                                          {1168, 107},
                                                          {648, 501},
                                                          {1168, 501},
                                                          {32, 536},
                                                          {1168, 536}}})
        rivet(point.x, point.y);
    serif(g, "LAssassin", {43, 20, 251, 52}, 38, ink);
    serif(g, "-76", {292, 20, 88, 52}, 40, red);
    text(g, "D U A L   S T A G E   D Y N A M I C S", {391, 32, 384, 22}, 12, ink);
    feather(g, {784, 64}, {881, 29}, brass.withAlpha(.65f), .08f);
    serif(g, "LA-2A", {166, 101, 213, 35}, 32, red);
    text(g, "O P T I C A L", {181, 131, 181, 17}, 10, ink);
    serif(g, "1176", {822, 100, 164, 36}, 32, ivory);
    text(g, "F E T", {867, 131, 77, 17}, 10, brass.brighter());
    feather(g, {42, 159}, {119, 119}, brass.withAlpha(.4f), .08f);
    feather(g, {1041, 143}, {1149, 145}, brass.withAlpha(.3f), .08f);
    for (int y = 117; y < 494; y += 19)
    {
        const float wave = std::sin(float(y) * .08f) * 8;
        g.setColour(brass.withAlpha(.7f));
        g.drawEllipse(578 + wave * .4f, float(y), 10, 24, 1);
        g.drawEllipse(578 - wave * .4f, float(y + 7), 10, 24, 1);
    }
    g.setColour(ink);
    g.fillRoundedRectangle(540, 226, 86, 149, 5);
    g.setColour(brass);
    g.drawRoundedRectangle(540, 226, 86, 149, 5, 1);
    serif(g, "ORDER", {544, 234, 78, 24}, 16, ivory);
    text(g, "SIGNAL FLOW", {541, 350, 84, 16}, 8, brass);
    serif(g, "PEAK REDUCTION", {49, 459, 217, 18}, 15, ink);
    serif(g, "GAIN", {307, 459, 167, 18}, 15, ink);
    serif(g, "INPUT", {697, 298, 141, 19}, 16, ivory);
    serif(g, "OUTPUT", {958, 298, 141, 19}, 16, ivory);
    serif(g, "ATTACK", {696, 440, 144, 19}, 15, ivory);
    serif(g, "RELEASE", {958, 440, 144, 19}, 15, ivory);
    text(g, "RATIO", {655, 468, 53, 23}, 11, brass);
    text(g, "IN", {46, 528, 72, 16}, 11, ivory);
    text(g, "OUT", {165, 528, 72, 16}, 11, ivory);
    text(g, "MIX", {284, 528, 72, 16}, 11, ivory);
    g.setColour(brass.withAlpha(.4f));
    g.drawVerticalLine(141, 541, 610);
    g.drawVerticalLine(260, 541, 610);
    g.drawVerticalLine(388, 541, 610);
    serif(g, "S Y N C H R O N I Z A T I O N", {408, 532, 503, 24}, 16, ivory);
    text(g, "SC HPF", {310, 639, 64, 21}, 10, ivory);
    text(g, "LAssassin-76  /  v0.3.0", {918, 640, 242, 20}, 10, brass);
}
void LAssassinEditor::drawVu(juce::Graphics &g)
{
    const juce::Rectangle<float> b{44, 159, 461, 131};
    if (optical > .1f)
        for (int i = 5; i >= 1; --i)
        {
            g.setColour(cyan.withAlpha(.012f * std::min(optical, 12.f) * (1.f + .15f * std::sin(pulse))));
            g.drawRoundedRectangle(b.expanded(float(i)), 5, float(i));
        }
    g.setColour(ink);
    g.fillRoundedRectangle(b.expanded(4), 6);
    g.setColour(brass);
    g.drawRoundedRectangle(b.expanded(4), 6, 2);
    g.setGradientFill(
        juce::ColourGradient(juce::Colour(0xffeee3be), 274, 178, juce::Colour(0xffbdaa7e), 274, 293, false));
    g.fillRoundedRectangle(b, 3);
    const juce::Point<float> centre{274, 329};
    const float radius = 131;
    constexpr float tickValues[]{20, 15, 10, 7, 5, 3, 1, 0};
    for (int i = 0; i < 8; ++i)
    {
        const float a = -1.07f + (1.f - std::sqrt(tickValues[i] / 20.f)) * 2.14f;
        const auto p = centre + juce::Point<float>(std::sin(a) * radius, -std::cos(a) * radius);
        const auto q = centre + juce::Point<float>(std::sin(a) * (radius - 9), -std::cos(a) * (radius - 9));
        g.setColour(i == 7 ? red : ink);
        g.drawLine({p, q}, 1.5f);
        const auto t = centre + juce::Point<float>(std::sin(a) * (radius + 14), -std::cos(a) * (radius + 14));
        text(g, juce::String(tickValues[i], 0), {t.x - 14, t.y - 9, 28, 18}, 12, ink);
    }
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 9, radius - 9, 0, -1.07f, 1.07f, true);
    g.setColour(ink);
    g.strokePath(arc, juce::PathStrokeType(.8f));
    const float fraction = 1.f - std::sqrt(juce::jlimit(0.f, 20.f, optical) / 20.f);
    const float a = -1.07f + fraction * 2.14f;
    const juce::Point<float> foot{274, 282};
    const auto tip = centre + juce::Point<float>(std::sin(a) * (radius - 12), -std::cos(a) * (radius - 12));
    feather(g, foot, tip, optical > .2f ? juce::Colour(0xff21515a) : ink, .032f);
    text(g, "VU", {60, 247, 52, 23}, 20, ink, juce::Justification::left);
    text(g, "GAIN REDUCTION", {315, 251, 170, 18}, 11, ink);
    text(g, juce::String(optical, 1) + " dB", {193, 266, 150, 18}, 11, ink);
}
void LAssassinEditor::drawMeters(juce::Graphics &g)
{
    const auto bar = juce::Rectangle<float>(410, 565, 491, 15);
    g.setColour(ink);
    g.fillRect(bar.expanded(3));
    g.setColour(brass);
    g.drawRect(bar.expanded(3), .7f);
    const float amount = juce::jlimit(0.f, 1.f, total / 24.f);
    const int lit = int(std::lround(amount * 40));
    for (int i = 0; i < 40; ++i)
    {
        auto r = juce::Rectangle<float>(bar.getX() + float(i) * 12.3f, bar.getY(), 10.3f, bar.getHeight());
        g.setColour(i < lit ? cyan.withMultipliedBrightness(.75f + amount * .25f) : juce::Colour(0xff283638));
        g.fillRect(r);
    }
    for (int i = 0; i < 5; ++i)
        text(g, juce::String(-i * 6), {398 + float(i) * 122.7f, 586, 30, 14}, 10, ivory);
    text(g, "TOTAL  -" + juce::String(total, 1) + " dB", {682, 602, 217, 16}, 11, cyan,
         juce::Justification::right);
    text(g, "LA  -" + juce::String(optical, 1) + " dB     FET  -" + juce::String(fet, 1) + " dB",
         {410, 602, 270, 16}, 11, ivory, juce::Justification::left);
    for (int m = 0; m < 2; ++m)
    {
        const float level = m == 0 ? input : output;
        const float y = 552 + float(m) * 32;
        const float n = juce::jlimit(0.f, 1.f, (level + 60.f) / 60.f);
        text(g, m == 0 ? "IN" : "OUT", {931, y - 4, 34, 16}, 10, ivory);
        g.setColour(juce::Colour(0xff293134));
        g.fillRect(972.f, y, 116.f, 7.f);
        g.setColour(level > -.1f ? red : cyan);
        g.fillRect(972.f, y, 116.f * n, 7.f);
        text(g, juce::String(level, 1), {1091, y - 5, 61, 17}, 10, level > -.1f ? red : ivory);
    }
    text(g, "SAMPLE PEAK / dBFS", {941, 607, 214, 12}, 8, brass);
    text(g, "GR  -" + juce::String(fet, 1) + " dB", {949, 111, 120, 18}, 11, cyan);
}
void LAssassinEditor::drawBypass(juce::Graphics &g, juce::Rectangle<float> b)
{
    g.setColour(ink.withAlpha(.49f));
    g.fillRoundedRectangle(b, 5);
    g.setColour(juce::Colours::white.withAlpha(.035f));
    for (float y = b.getY(); y < b.getBottom(); y += 4)
        g.drawHorizontalLine(int(y), b.getX(), b.getRight());
}
void LAssassinEditor::paint(juce::Graphics &g)
{
    g.addTransform(juce::AffineTransform::scale(float(getWidth()) / width, float(getHeight()) / height));
    g.drawImageAt(background, 0, 0);
    drawVu(g);
    drawMeters(g);
    if (laBypass.getToggleState())
        drawBypass(g, {20, 94, 510, 420});
    if (fetBypass.getToggleState())
        drawBypass(g, {636, 94, 544, 420});
    const bool reverse = order.getToggleState();
    text(g, reverse ? "LA > FET" : "FET > LA", {542, 331, 82, 16}, 10, cyan);
    // Brief original blade-light sweep through the central connector.
    if (sweep > 0)
    {
        const float y = 234 + (1 - sweep) * 121;
        for (int i = 5; i >= 1; --i)
        {
            g.setColour(cyan.withAlpha(.035f * float(6 - i) * sweep));
            g.drawLine(537, y + 19, 629, y - 19, float(i));
        }
    }
}
void LAssassinEditor::timerCallback()
{
    auto follow = [](float old, float target, float fall)
    { return target > old ? old + (target - old) * .48f : old + (target - old) * fall; };
    optical = follow(optical, owner.laGR.load(std::memory_order_relaxed), .075f);
    fet = follow(fet, owner.fetGR.load(std::memory_order_relaxed), .12f);
    total = optical + fet;
    input = follow(input, owner.inputDb.load(std::memory_order_relaxed), .06f);
    output = follow(output, owner.outputDb.load(std::memory_order_relaxed), .06f);
    pulse += .075f;
    if (pulse > juce::MathConstants<float>::twoPi)
        pulse -= juce::MathConstants<float>::twoPi;
    if (previousOrder != order.getToggleState())
    {
        previousOrder = order.getToggleState();
        sweep = 1;
    }
    sweep = std::max(0.f, sweep - 1.f / 12.f);
    order.setButtonText(previousOrder ? "LA-2A\n1176" : "1176\nLA-2A");
    const float laAlpha = laBypass.getToggleState() ? .4f : 1.f,
                fetAlpha = fetBypass.getToggleState() ? .4f : 1.f;
    for (size_t i = 0; i < 6; ++i)
        dials[i].slider.setAlpha(i < 2 ? laAlpha : fetAlpha);
    limit.setAlpha(laAlpha);
    hf.setAlpha(laAlpha);
    for (auto &b : ratios)
        b.setAlpha(fetAlpha);
    const int selected = owner.getCurrentProgram() + 1;
    if (presets.getSelectedId() != selected)
        presets.setSelectedId(selected, juce::dontSendNotification);
    repaint();
}
