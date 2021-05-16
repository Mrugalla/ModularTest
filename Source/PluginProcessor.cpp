#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>
#define ResetAPVTS false

ModularTestAudioProcessor::ModularTestAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    lfoFreeSyncRanges(),
    apvts(*this, nullptr, "Params", param::createParameters(apvts, lfoFreeSyncRanges)),
    matrix2(std::make_shared<modSys2::Matrix>(apvts))
#endif
{
    auto m = matrix2.load();
    m->addMacroModulator(param::getID(param::ID::Macro0));
    m->addMacroModulator(param::getID(param::ID::Macro1));
    m->addMacroModulator(param::getID(param::ID::Macro2));
    m->addMacroModulator(param::getID(param::ID::Macro3));

    m->addEnvelopeFollowerModulator(
        param::getID(param::ID::EnvFolAtk),
        param::getID(param::ID::EnvFolRls),
        param::getID(param::ID::EnvFolWdth),
        0
    );
    auto lfoMod = m->addLFOModulator(
        param::getID(param::ID::LFOSync),
        param::getID(param::ID::LFORate),
        param::getID(param::ID::LFOWdth),
        param::getID(param::ID::LFOWaveTable),
        lfoFreeSyncRanges,
        0
    );

    std::vector<Anything> waveTableInfo;
    auto tableSizePointer = Anything::make<int>(512);
    const auto tableSaw = [](float x) { return x; };
    const auto tableSine = [t = modSys2::tau](float x) { return .5f * std::sin(x * t) + .5f; };
    const auto tableSquare = [](float x) { return x < .5f ? 0.f : 1.f; };
    auto sawPtr = Anything::make<std::function<float(float)>>(tableSaw);
    auto sinePtr = Anything::make<std::function<float(float)>>(tableSaw);
    auto sqrPtr = Anything::make<std::function<float(float)>>(tableSaw);
    waveTableInfo.push_back(tableSizePointer); // tableSize
    waveTableInfo.push_back(sawPtr);
    waveTableInfo.push_back(sinePtr);
    waveTableInfo.push_back(sqrPtr);
    lfoMod->addStuff("wavetables", waveTableInfo);
    tableSizePointer.reset<int>();
    sawPtr.reset<std::function<float(float)>>();
    sinePtr.reset<std::function<float(float)>>();
    sqrPtr.reset<std::function<float(float)>>();
}

ModularTestAudioProcessor::~ModularTestAudioProcessor()
{
}

//==============================================================================
const juce::String ModularTestAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ModularTestAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ModularTestAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ModularTestAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ModularTestAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ModularTestAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ModularTestAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ModularTestAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ModularTestAudioProcessor::getProgramName (int index)
{
    return {};
}

void ModularTestAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void ModularTestAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    auto m = matrix2.load();
    m->prepareToPlay(getChannelCountOfBus(false, 0), samplesPerBlock, sampleRate);

    auto sec = (float)sampleRate;
    m->setSmoothingLengthInSamples(param::getID(param::ID::LFOSync), 0); // example for snappy param
    m->setSmoothingLengthInSamples(param::getID(param::ID::LFOWaveTable), 0);
    m->setSmoothingLengthInSamples(param::getID(param::ID::ModulesMix), sec / 8); // example for quick param
    m->setSmoothingLengthInSamples(param::getID(param::ID::Depth), sec); // example for slow param
    m->setSmoothingLengthInSamples(param::getID(param::ID::EnvFolAtk), sec / 64);
    m->setSmoothingLengthInSamples(param::getID(param::ID::EnvFolRls), sec / 64);
    m->setSmoothingLengthInSamples(param::getID(param::ID::EnvFolWdth), sec / 64);
}

void ModularTestAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ModularTestAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ModularTestAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto matx = matrix2.load();
    matx->processBlock(buffer, getPlayHead());
}

bool ModularTestAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* ModularTestAudioProcessor::createEditor() { return new ModularTestAudioProcessorEditor (*this); }

void ModularTestAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    // VALUETREE TO BINARY
    auto m = matrix2.load();
    m->getState(apvts);

#if ResetAPVTS
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ModularTestAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

#if ResetAPVTS
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif

    // BINARY TO VALUETREE
    auto m = matrix2.copy();
    m->setState(apvts);
    matrix2.replaceWith(m);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ModularTestAudioProcessor();
}
