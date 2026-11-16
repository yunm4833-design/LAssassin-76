#include "PluginEditor.h"
#include <iostream>
#include <stdexcept>
static void check(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
static juce::Component *find(juce::Component &parent, const juce::String &id)
{
    if (parent.getComponentID() == id)
        return &parent;
    for (auto *child : parent.getChildren())
        if (auto *result = find(*child, id))
            return result;
    return nullptr;
}
static void pump()
{
    for (int i = 0; i < 9; ++i)
    {
        juce::Thread::sleep(20);
        juce::Timer::callPendingTimersSynchronously();
    }
}
int main(int argc, char **argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        LAssassinProcessor processor;
        processor.prepareToPlay(48000, 128);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        check(editor->getWidth() == 1390 && editor->getHeight() == 724, "wrong editor bounds");
        const char *sliders[]{"peak",    "laGain", "fetIn", "fetOut", "attack",
                              "release", "in",     "out",   "mix",    "hpf"};
        for (auto *id : sliders)
        {
            auto *slider = dynamic_cast<juce::Slider *>(find(*editor, id));
            check(slider != nullptr, "missing slider");
            slider->setValue(slider->getMaximum(), juce::sendNotificationSync);
            check(std::abs(processor.apvts.getRawParameterValue(id)->load() - float(slider->getMaximum())) <
                      .02f,
                  "slider not bound to parameter");
            check(slider->getBounds().getWidth() > 0, "slider has no bounds");
        }
        for (auto *id :
             {"laBypass", "fetBypass", "order", "limit", "hf", "link", "auto", "bypass", "laCalibration"})
        {
            auto *button = dynamic_cast<juce::Button *>(find(*editor, id));
            check(button != nullptr, "missing button");
            const bool value = !button->getToggleState();
            button->setToggleState(value, juce::sendNotificationSync);
            check(std::abs(processor.apvts.getRawParameterValue(id)->load() - (value ? 1.f : 0.f)) < .001f,
                  "button not bound to parameter");
        }
        for (int i = 0; i < 5; ++i)
        {
            auto *button = dynamic_cast<juce::Button *>(find(*editor, "ratio" + juce::String(i)));
            check(button != nullptr, "missing ratio button");
            button->onClick();
            pump();
            check(int(processor.apvts.getRawParameterValue("ratio")->load()) == i, "ratio button not bound");
        }
        auto *ratio = processor.apvts.getParameter("ratio");
        ratio->setValueNotifyingHost(ratio->convertTo0to1(1));
        pump();
        check(dynamic_cast<juce::Button *>(find(*editor, "ratio1"))->getToggleState(),
              "host automation did not update ratio buttons");
        for (auto *id : {"ms", "os"})
        {
            auto *combo = dynamic_cast<juce::ComboBox *>(find(*editor, id));
            check(combo != nullptr, "missing combo");
            combo->setSelectedId(2, juce::sendNotificationSync);
            check(processor.apvts.getRawParameterValue(id)->load() > .9f, "combo not bound");
        }
        auto *presets = dynamic_cast<juce::ComboBox *>(find(*editor, "presets"));
        presets->setSelectedId(2, juce::sendNotificationSync);
        check(processor.getCurrentProgram() == 1, "preset not applied");
        auto *setup = dynamic_cast<juce::Button *>(find(*editor, "setup"));
        auto *options = find(*editor, "options");
        check(setup && options && !options->isVisible(), "setup should start collapsed");
        setup->setToggleState(true, juce::dontSendNotification);
        setup->onClick();
        check(options->isVisible(), "setup panel did not open");
        setup->setToggleState(false, juce::dontSendNotification);
        setup->onClick();
        auto *reverse = dynamic_cast<juce::Button *>(find(*editor, "orderReverse"));
        auto *forward = dynamic_cast<juce::Button *>(find(*editor, "orderForward"));
        reverse->onClick();
        check(processor.apvts.getRawParameterValue("order")->load() > .5f, "reverse order button failed");
        forward->onClick();
        check(processor.apvts.getRawParameterValue("order")->load() < .5f, "forward order button failed");
        auto *gainValue = dynamic_cast<juce::Label *>(find(*editor, "laGainValue"));
        gainValue->setText("38", juce::sendNotificationSync);
        check(std::abs(processor.apvts.getRawParameterValue("laGain")->load() - 15.2f) < .02f,
              "LA faceplate scale conversion failed");
        processor.setCurrentProgram(0);
        auto set = [&](const char *id, float value)
        {
            auto *p = processor.apvts.getParameter(id);
            p->setValueNotifyingHost(p->convertTo0to1(value));
        };
        set("bypass", 0);
        set("peak", 42);
        set("laGain", 15.2f);
        set("fetIn", -18);
        set("attack", 3);
        set("release", 5);
        set("fetOut", -6);
        set("ratio", 4);
        set("mix", 75);
        set("hf", 1);
        juce::AudioBuffer<float> buffer(2, 128);
        juce::MidiBuffer midi;
        for (int b = 0; b < 100; ++b)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 128; ++i)
                    buffer.setSample(c, i,
                                     float(.8 * std::sin(2 * assassin::pi * 997 * (b * 128 + i) / 48000.)));
            processor.processBlock(buffer, midi);
        }
        pump();
        if (argc > 1)
        {
            auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            juce::File file(argv[1]);
            file.deleteFile();
            juce::FileOutputStream stream(file);
            check(stream.openedOk(), "cannot write snapshot");
            juce::PNGImageFormat format;
            check(format.writeImageToStream(snapshot, stream), "PNG encode failed");
            stream.flush();
            setup->setToggleState(true, juce::dontSendNotification);
            setup->onClick();
            auto panelSnapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            juce::File panelFile = file.getSiblingFile("UI-setup-preview.png");
            panelFile.deleteFile();
            juce::FileOutputStream panelStream(panelFile);
            check(format.writeImageToStream(panelSnapshot, panelStream), "setup PNG encode failed");
            setup->setToggleState(false, juce::dontSendNotification);
            setup->onClick();
            editor->setSize(1738, 905);
            auto nativeSnapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            juce::File nativeFile = file.getSiblingFile("UI-native-preview.png");
            nativeFile.deleteFile();
            juce::FileOutputStream nativeStream(nativeFile);
            check(format.writeImageToStream(nativeSnapshot, nativeStream), "native PNG encode failed");
        }
        editor->setSize(1043, 543);
        check(editor->getWidth() == 1043, "resize failed");
        std::cout << "Editor tests passed: all controls, host automation, preset, resize and render\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
