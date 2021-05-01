#pragma once
#include "ReleasePool.h"
#include <JuceHeader.h>

namespace param {
	enum class ID { Macro0, Macro1, Macro2, Macro3, Depth, ModulesMix };

	static juce::String getName(ID i) {
		switch (i) {
		case ID::Macro0: return "Macro 0";
		case ID::Macro1: return "Macro 1";
		case ID::Macro2: return "Macro 2";
		case ID::Macro3: return "Macro 3";
		case ID::Depth: return "Depth";
		case ID::ModulesMix: return "ModulesMix";
		default: return "";
		}
	}
	static juce::String getName(int i) { getName(static_cast<ID>(i)); }
	static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
	static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue) {
		auto normalStr = [](float value, int) { return static_cast<juce::String>(std::rint(value * 100.f)); };
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), juce::NormalisableRange<float>(0.f, 1.f), defaultValue,
			getName(i), juce::AudioProcessorParameter::Category::genericParameter, normalStr
		);
	}
	static std::unique_ptr<juce::AudioParameterBool> createPBool(ID i, bool defaultValue, std::function<juce::String(bool value, int maxLen)> func) {
		return std::make_unique<juce::AudioParameterBool>(
			getID(i), getName(i), defaultValue, getName(i), func
			);
	}
	static std::unique_ptr<juce::AudioParameterChoice> createPChoice(ID i, const juce::StringArray& choices, int defaultValue) {
		return std::make_unique<juce::AudioParameterChoice>(
			getID(i), getName(i), choices, defaultValue, getName(i)
			);
	}
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, const juce::NormalisableRange<float>& range, float defaultValue,
		std::function<juce::String(float value, int maxLen)> stringFromValue = nullptr) {
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), range, defaultValue, getName(i), juce::AudioProcessorParameter::Category::genericParameter,
			stringFromValue
			);
	}

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

		parameters.push_back(createParameter(ID::Macro0, 0.f));
		parameters.push_back(createParameter(ID::Macro1, 0.f));
		parameters.push_back(createParameter(ID::Macro2, 0.f));
		parameters.push_back(createParameter(ID::Macro3, 0.f));
		parameters.push_back(createParameter(ID::Depth, 1.f));
		parameters.push_back(createParameter(ID::ModulesMix, 0.5f));

		return { parameters.begin(), parameters.end() };
	}
};

#include "ModSystem.h"

struct ModularTestAudioProcessor :
	public juce::AudioProcessor
{
    ModularTestAudioProcessor();
    ~ModularTestAudioProcessor() override;
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
	const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
	int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
	void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
	ReleasePool releasePool;
	ThreadSafeObject<modSys2::Matrix> matrix2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModularTestAudioProcessor)
};
