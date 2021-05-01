#include "PluginProcessor.h"
#include "PluginEditor.h"

ModularTestAudioProcessorEditor::ModularTestAudioProcessorEditor(ModularTestAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),

    macrosLabel("Macros", "Macros"),

    macro0P(audioProcessor, param::getID(param::ID::Macro0), param::getID(param::ID::Macro0)),
    macro1P(audioProcessor, param::getID(param::ID::Macro1), param::getID(param::ID::Macro1)),
    macro2P(audioProcessor, param::getID(param::ID::Macro2), param::getID(param::ID::Macro2)),
    macro3P(audioProcessor, param::getID(param::ID::Macro3), param::getID(param::ID::Macro3)),

    globalsLabel("Globals", "Globals"),

    depthP(audioProcessor, param::getID(param::ID::Depth)),
    modulesMixP(audioProcessor, param::getID(param::ID::ModulesMix)),

    modulesLabel("Modules", "Modules"),

    macro0Dragger(audioProcessor, param::getID(param::ID::Macro0), { &depthP, &modulesMixP }),
    macro1Dragger(audioProcessor, param::getID(param::ID::Macro1), { &depthP, &modulesMixP }),
    macro2Dragger(audioProcessor, param::getID(param::ID::Macro2), { &depthP, &modulesMixP }),
    macro3Dragger(audioProcessor, param::getID(param::ID::Macro3), { &depthP, &modulesMixP })
{
    addAndMakeVisible(macrosLabel);
    macrosLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modulesLabel);
    modulesLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(globalsLabel);
    globalsLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(macro0P); addAndMakeVisible(macro1P);
    addAndMakeVisible(macro2P); addAndMakeVisible(macro3P);

    addAndMakeVisible(depthP); addAndMakeVisible(modulesMixP);

    addAndMakeVisible(macro0Dragger); addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger); addAndMakeVisible(macro3Dragger);

    setOpaque(true);
    setResizable(true, true);
    startTimerHz(25);
    setSize (700, 500);
}

void ModularTestAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::black); }

void ModularTestAudioProcessorEditor::resized() {
    auto x = 0.f;
    auto y = 0.f;
    auto height = (float)getHeight() / 5.f;
    const auto width = getWidth() / 4.f;
    const auto knobWidth = width / 2.f;

    macrosLabel.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, width, height)).reduced(10).toNearestInt());
    y += height;
    macro0P.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro1P.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro2P.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro3P.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());

    x += knobWidth;
    y = height;
    macro0Dragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(15).toNearestInt());
    y += height;
    macro1Dragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(15).toNearestInt());
    y += height;
    macro2Dragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(15).toNearestInt());
    y += height;
    macro3Dragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(15).toNearestInt());

    x += knobWidth;
    y = 0.f;

    height = (float)getHeight() / 3.f;
    globalsLabel.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, width, height)).toNearestInt());
    y += height;
    depthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, width, height)).reduced(10).toNearestInt());
    y += height;
    modulesMixP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, width, height)).reduced(10).toNearestInt());
}

void ModularTestAudioProcessorEditor::timerCallback() {
    const auto matrix = audioProcessor.matrix2.load();
    
    macro0P.timerCallback(matrix); macro1P.timerCallback(matrix);
    macro2P.timerCallback(matrix); macro3P.timerCallback(matrix);
    depthP.timerCallback(matrix); modulesMixP.timerCallback(matrix);

    macro0Dragger.timerCallback(matrix); macro1Dragger.timerCallback(matrix);
    macro2Dragger.timerCallback(matrix); macro3Dragger.timerCallback(matrix);
}