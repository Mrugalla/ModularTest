#include "PluginProcessor.h"
#include "PluginEditor.h"

ModularTestAudioProcessorEditor::ModularTestAudioProcessorEditor(ModularTestAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),

    macrosLabel("Macros", "Macros"),

    macro0P(audioProcessor, param::getID(param::ID::Macro0), audioProcessor.getChannelCountOfBus(false, 0), param::getID(param::ID::Macro0)),
    macro1P(audioProcessor, param::getID(param::ID::Macro1), audioProcessor.getChannelCountOfBus(false, 0), param::getID(param::ID::Macro1)),
    macro2P(audioProcessor, param::getID(param::ID::Macro2), audioProcessor.getChannelCountOfBus(false, 0), param::getID(param::ID::Macro2)),
    macro3P(audioProcessor, param::getID(param::ID::Macro3), audioProcessor.getChannelCountOfBus(false, 0), param::getID(param::ID::Macro3)),

    globalsLabel("Globals", "Globals"),

    depthP(audioProcessor, param::getID(param::ID::Depth), audioProcessor.getChannelCountOfBus(false, 0)),
    modulesMixP(audioProcessor, param::getID(param::ID::ModulesMix), audioProcessor.getChannelCountOfBus(false, 0)),

    envFolAtkP(audioProcessor, param::getID(param::ID::EnvFolAtk), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolRlsP(audioProcessor, param::getID(param::ID::EnvFolRls), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolWdthP(audioProcessor, param::getID(param::ID::EnvFolWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolDisplay(0, audioProcessor.getChannelCountOfBus(false, 0)),

    lfoSyncP(audioProcessor, param::getID(param::ID::LFOSync), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoRateP(audioProcessor, param::getID(param::ID::LFORate), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoWdthP(audioProcessor, param::getID(param::ID::LFOWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoWaveTableP(audioProcessor, param::getID(param::ID::LFOWaveTable), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoDisplay(0, audioProcessor.getChannelCountOfBus(false, 0)),

    modulesLabel("Modules", "Modules"),

    macro0Dragger(audioProcessor, param::getID(param::ID::Macro0), { &depthP, &modulesMixP }),
    macro1Dragger(audioProcessor, param::getID(param::ID::Macro1), { &depthP, &modulesMixP }),
    macro2Dragger(audioProcessor, param::getID(param::ID::Macro2), { &depthP, &modulesMixP }),
    macro3Dragger(audioProcessor, param::getID(param::ID::Macro3), { &depthP, &modulesMixP }),
    envFolDragger(audioProcessor, juce::String("EnvFol" + 0), { &depthP, &modulesMixP }),
    lfoDragger(audioProcessor, juce::String("Phase" + 0), { &depthP, &modulesMixP })
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

    addAndMakeVisible(envFolAtkP); addAndMakeVisible(envFolRlsP);
    addAndMakeVisible(envFolWdthP); addAndMakeVisible(envFolDisplay);

    addAndMakeVisible(lfoSyncP); addAndMakeVisible(lfoRateP);
    addAndMakeVisible(lfoWdthP); addAndMakeVisible(lfoWaveTableP);
    addAndMakeVisible(lfoDisplay);

    addAndMakeVisible(macro0Dragger); addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger); addAndMakeVisible(macro3Dragger);
    addAndMakeVisible(envFolDragger); addAndMakeVisible(lfoDragger);

    setOpaque(true);
    setResizable(true, true);
    startTimerHz(25);
    setSize (850, 550);
}

void ModularTestAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::black); }

void ModularTestAudioProcessorEditor::resized() {
    auto x = 0.f;
    auto y = 0.f;
    auto height = (float)getHeight() / 5.f;
    const auto width = getWidth() / 5.f;
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

    x += width;
    y = 0.f;

    const auto modulesX = x;
    const auto moduleHeight = height / 2.f;
    auto moduleObjWidth = width / 2.f;
    envFolDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolDisplay.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolAtkP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolRlsP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolWdthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    
    x = modulesX;
    y += moduleHeight;

    lfoDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    lfoDisplay.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    lfoSyncP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    lfoRateP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    lfoWdthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    lfoWaveTableP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
}

void ModularTestAudioProcessorEditor::timerCallback() {
    const auto matrix = audioProcessor.matrix.loadCurrentPtr();
    
    macro0P.timerCallback(matrix); macro1P.timerCallback(matrix);
    macro2P.timerCallback(matrix); macro3P.timerCallback(matrix);
    depthP.timerCallback(matrix); modulesMixP.timerCallback(matrix);

    macro0Dragger.timerCallback(matrix); macro1Dragger.timerCallback(matrix);
    macro2Dragger.timerCallback(matrix); macro3Dragger.timerCallback(matrix);

    envFolAtkP.timerCallback(matrix); envFolRlsP.timerCallback(matrix);
    envFolWdthP.timerCallback(matrix); envFolDisplay.timerCallback(matrix);
    envFolDragger.timerCallback(matrix);

    lfoSyncP.timerCallback(matrix); lfoRateP.timerCallback(matrix);
    lfoWdthP.timerCallback(matrix); lfoWaveTableP.timerCallback(matrix);
    lfoDisplay.timerCallback(matrix); lfoDragger.timerCallback(matrix);
}