#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Colores y tipografia del diseno original (HTML)
namespace theme
{
    inline const juce::Colour background { 0xff0b0b0d };
    inline const juce::Colour text       { 0xfff2f2f2 };
    inline const juce::Colour accent     { 0xff8fd0ff };
    inline const juce::Colour track      { 0xff26262c };
    inline const juce::Colour orbit      { 0xff2e2e36 };
    inline const juce::Colour star       { 0xff4a4a54 };
    inline const juce::Colour meterBack  { 0xff1c1c21 };
    inline const juce::Colour buttonLine { 0xff3a3a42 };
    inline const juce::Colour ledOff     { 0xff2a2a30 };

    juce::Font mono (float px, float letterSpacingEm = 0.0f);
    juce::Font serifItalic (float px, float letterSpacingEm = 0.0f);
}

//==============================================================================
class OrbitalLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
class OrbitalKnob final : public juce::Component
{
public:
    OrbitalKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                 const juce::String& label, int diameter);

    int getDiameter() const { return diameter; }
    int getTotalHeight() const { return diameter + 30; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String label;
    int diameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitalKnob)
};

//==============================================================================
class BypassButton final : public juce::Button
{
public:
    BypassButton() : juce::Button ("Bypass") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
};

//==============================================================================
class OrbitalPanel final : public juce::Component
{
public:
    static constexpr int baseWidth  = 440;
    static constexpr int baseHeight = 700;

    explicit OrbitalPanel (OrbitalReverbProcessor&);
    ~OrbitalPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setLevel (float newLevel);

private:
    struct Group
    {
        juce::String title;
        int top = 0;
        std::vector<OrbitalKnob*> knobs;
    };

    void layoutGroup (Group&, int top);
    void paintFrame (juce::Graphics&, juce::Rectangle<float> box);
    void paintOrbits (juce::Graphics&, juce::Rectangle<float> box);

    OrbitalLookAndFeel lookAndFeel;
    std::vector<std::unique_ptr<OrbitalKnob>> knobs;
    std::vector<Group> groups;
    BypassButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<int> titleArea, meterArea;
    float level = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitalPanel)
};

//==============================================================================
class OrbitalReverbEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit OrbitalReverbEditor (OrbitalReverbProcessor&);
    ~OrbitalReverbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    OrbitalReverbProcessor& processor;
    OrbitalPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitalReverbEditor)
};
