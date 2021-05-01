#pragma once

static juce::Rectangle<float> maxQuadIn(const juce::Rectangle<float>& b) {
    const auto minDimen = std::min(b.getWidth(), b.getHeight());
    const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
    const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
    return { x, y, minDimen, minDimen };
}

#include "ModSystemEditor.h"
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct ModularTestAudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
    ModularTestAudioProcessorEditor (ModularTestAudioProcessor&);
protected:
    ModularTestAudioProcessor& audioProcessor;

    juce::Label macrosLabel;
    modSys2Editor::Parameter macro0P, macro1P, macro2P, macro3P;

    juce::Label globalsLabel;
    modSys2Editor::Parameter depthP, modulesMixP;

    juce::Label modulesLabel;

    modSys2Editor::ModulatorDragger macro0Dragger, macro1Dragger, macro2Dragger, macro3Dragger;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModularTestAudioProcessorEditor)
};