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

    envFolGainP(audioProcessor, param::getID(param::ID::EnvFolGain), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolAtkP(audioProcessor, param::getID(param::ID::EnvFolAtk), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolRlsP(audioProcessor, param::getID(param::ID::EnvFolRls), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolBiasP(audioProcessor, param::getID(param::ID::EnvFolBias), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolWdthP(audioProcessor, param::getID(param::ID::EnvFolWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    envFolDisplay(0, audioProcessor.getChannelCountOfBus(false, 0)),

    lfoSyncP(audioProcessor, param::getID(param::ID::LFOSync), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoRateP(audioProcessor, param::getID(param::ID::LFORate), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoWdthP(audioProcessor, param::getID(param::ID::LFOWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoWaveTableP(audioProcessor, param::getID(param::ID::LFOWaveTable), audioProcessor.getChannelCountOfBus(false, 0)),
    lfoDisplay(juce::String("LFO0"), audioProcessor.getChannelCountOfBus(false, 0)),

    randSyncP(audioProcessor, param::getID(param::ID::RandSync), audioProcessor.getChannelCountOfBus(false, 0)),
    randRateP(audioProcessor, param::getID(param::ID::RandRate), audioProcessor.getChannelCountOfBus(false, 0)),
    randBiasP(audioProcessor, param::getID(param::ID::RandBias), audioProcessor.getChannelCountOfBus(false, 0)),
    randWidthP(audioProcessor, param::getID(param::ID::RandWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    randSmoothP(audioProcessor, param::getID(param::ID::RandSmooth), audioProcessor.getChannelCountOfBus(false, 0)),
    randDisplay(juce::String("Rand0"), audioProcessor.getChannelCountOfBus(false, 0)),

    perlinSyncP(audioProcessor, param::getID(param::ID::PerlinSync), audioProcessor.getChannelCountOfBus(false, 0)),
    perlinRateP(audioProcessor, param::getID(param::ID::PerlinRate), audioProcessor.getChannelCountOfBus(false, 0)),
    perlinOctavesP(audioProcessor, param::getID(param::ID::PerlinOctaves), audioProcessor.getChannelCountOfBus(false, 0)),
    perlinWidthP(audioProcessor, param::getID(param::ID::PerlinWdth), audioProcessor.getChannelCountOfBus(false, 0)),
    perlinDisplay(juce::String("Perlin0"), audioProcessor.getChannelCountOfBus(false, 0)),

    modulesLabel("Modules", "Modules"),

    modulatableParameters({ &depthP, &modulesMixP, &envFolGainP, &envFolAtkP, &envFolRlsP, &envFolBiasP, &envFolWdthP, &lfoSyncP, &lfoRateP, &lfoWdthP, &lfoWaveTableP, &randSyncP, &randRateP, &randBiasP, &randSmoothP, &randWidthP, &perlinSyncP, &perlinRateP, &perlinOctavesP, &perlinWidthP }),

    macro0Dragger(audioProcessor, param::getID(param::ID::Macro0), modulatableParameters),
    macro1Dragger(audioProcessor, param::getID(param::ID::Macro1), modulatableParameters),
    macro2Dragger(audioProcessor, param::getID(param::ID::Macro2), modulatableParameters),
    macro3Dragger(audioProcessor, param::getID(param::ID::Macro3), modulatableParameters),
    envFolDragger(audioProcessor, juce::String("EnvFol0"), modulatableParameters),
    lfoDragger(audioProcessor, juce::String("LFO0"), modulatableParameters),
    randDragger(audioProcessor, juce::String("Rand0"), modulatableParameters),
    perlinDragger(audioProcessor, juce::String("Perlin0"), modulatableParameters)
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

    addAndMakeVisible(envFolGainP);
    addAndMakeVisible(envFolAtkP); addAndMakeVisible(envFolRlsP);
    addAndMakeVisible(envFolBiasP);
    addAndMakeVisible(envFolWdthP); addAndMakeVisible(envFolDisplay);

    addAndMakeVisible(lfoSyncP); addAndMakeVisible(lfoRateP);
    addAndMakeVisible(lfoWdthP); addAndMakeVisible(lfoWaveTableP);
    addAndMakeVisible(lfoDisplay);

    addAndMakeVisible(randSyncP); addAndMakeVisible(randRateP);
    addAndMakeVisible(randBiasP); addAndMakeVisible(randWidthP);
    addAndMakeVisible(randSmoothP);
    addAndMakeVisible(randDisplay);

    addAndMakeVisible(perlinSyncP); addAndMakeVisible(perlinRateP);
    addAndMakeVisible(perlinOctavesP); addAndMakeVisible(perlinWidthP);
    addAndMakeVisible(perlinDisplay);

    addAndMakeVisible(macro0Dragger); addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger); addAndMakeVisible(macro3Dragger);
    addAndMakeVisible(envFolDragger); addAndMakeVisible(lfoDragger);
    addAndMakeVisible(randDragger); addAndMakeVisible(perlinDragger);

    setOpaque(true);
    setResizable(true, true);
    startTimerHz(25);
    setSize (950, 500);
}

void ModularTestAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::black); }

void ModularTestAudioProcessorEditor::resized() {
    auto x = 0.f;
    auto y = 0.f;
    auto height = (float)getHeight() / 5.f;
    const auto width = getWidth() / 6.f;
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
    auto moduleObjWidth = width * .5f;
    const auto draggerWidth = moduleObjWidth * .5f;
    envFolDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, draggerWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolDisplay.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolGainP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolAtkP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolRlsP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolBiasP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    envFolWdthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    
    x = modulesX;
    y += moduleHeight;

    lfoDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, draggerWidth, moduleHeight)).toNearestInt());
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

    x = modulesX;
    y += moduleHeight;

    randDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, draggerWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randDisplay.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randSyncP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randRateP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randBiasP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randWidthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    randSmoothP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());

    x = modulesX;
    y += moduleHeight;

    perlinDragger.setQBounds(maxQuadIn(juce::Rectangle<float>(x, y, draggerWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    perlinDisplay.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    perlinSyncP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    perlinRateP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    perlinOctavesP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
    x += moduleObjWidth;
    perlinWidthP.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, moduleObjWidth, moduleHeight)).toNearestInt());
}

void ModularTestAudioProcessorEditor::timerCallback() {
    ///*
    const auto matrix = audioProcessor.matrix.getUpdatedPtr();
    
    macro0P.timerCallback(matrix); macro1P.timerCallback(matrix);
    macro2P.timerCallback(matrix); macro3P.timerCallback(matrix);
    depthP.timerCallback(matrix); modulesMixP.timerCallback(matrix);

    macro0Dragger.timerCallback(matrix); macro1Dragger.timerCallback(matrix);
    macro2Dragger.timerCallback(matrix); macro3Dragger.timerCallback(matrix);

    envFolGainP.timerCallback(matrix);
    envFolAtkP.timerCallback(matrix); envFolRlsP.timerCallback(matrix);
    envFolBiasP.timerCallback(matrix);
    envFolWdthP.timerCallback(matrix); envFolDisplay.timerCallback(matrix);
    envFolDragger.timerCallback(matrix);

    lfoSyncP.timerCallback(matrix); lfoRateP.timerCallback(matrix);
    lfoWdthP.timerCallback(matrix); lfoWaveTableP.timerCallback(matrix);
    lfoDisplay.timerCallback(matrix); lfoDragger.timerCallback(matrix);

    randSyncP.timerCallback(matrix); randRateP.timerCallback(matrix);
    randBiasP.timerCallback(matrix); randWidthP.timerCallback(matrix);
    randSmoothP.timerCallback(matrix);
    randDisplay.timerCallback(matrix); randDragger.timerCallback(matrix);
    
    perlinSyncP.timerCallback(matrix); perlinRateP.timerCallback(matrix);
    perlinOctavesP.timerCallback(matrix); perlinWidthP.timerCallback(matrix);
    perlinDisplay.timerCallback(matrix); perlinDragger.timerCallback(matrix);
    //*/
}