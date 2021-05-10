#pragma once
#include "ReleasePool.h"
#include <JuceHeader.h>

namespace param {
	enum class ID { Macro0, Macro1, Macro2, Macro3, Depth, ModulesMix, EnvFolAtk, EnvFolRls, EnvFolWdth, LFOSync, LFORate, LFOWdth };

	static juce::String getName(ID i) {
		switch (i) {
		case ID::Macro0: return "Macro 0";
		case ID::Macro1: return "Macro 1";
		case ID::Macro2: return "Macro 2";
		case ID::Macro3: return "Macro 3";
		case ID::Depth: return "Depth";
		case ID::ModulesMix: return "ModulesMix";
		case ID::EnvFolAtk: return "EnvFolAtk";
		case ID::EnvFolRls: return "EnvFolRls";
		case ID::EnvFolWdth: return "EnvFolWdth";
		case ID::LFOSync: return "LFOSync";
		case ID::LFORate: return "LFORate";
		case ID::LFOWdth: return "LFOWdth";
		default: return "";
		}
	}
	static juce::String getName(int i) { getName(static_cast<ID>(i)); }
	static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
	static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

	struct MultiRange {
		struct Range
		{
			Range(const juce::String& rID, const juce::NormalisableRange<float>& r) :
				id(rID),
				range(r)
			{}
			bool operator==(const juce::Identifier& rID) const { return id == rID; }
			const juce::NormalisableRange<float>& operator()() const { return range; }
			const juce::Identifier& getID() const { return id; }
		protected:
			const juce::Identifier id;
			const juce::NormalisableRange<float> range;
		};
		MultiRange() :
			ranges()
		{}
		void add(const juce::String& rID, const juce::NormalisableRange<float>&& r) { ranges.push_back({ rID, r }); }
		const juce::NormalisableRange<float>& operator()(const juce::Identifier& rID) const {
			for (const auto& r : ranges)
				if (r == rID)
					return r();
			return ranges[0]();
		}
		const juce::Identifier& getID(const juce::String&& idStr) const {
			for (const auto& r : ranges)
				if (r.getID().toString() == idStr)
					return r.getID();
			return ranges[0].getID();
		}
	private:
		std::vector<Range> ranges;
	};

	static juce::NormalisableRange<float> quadBezierRange(float min, float max, float shape) {
		// 0 <= SHAPE < 1 && SHAPE && SHAPE != .5
		auto rangedShape = shape * (max - min) + min;
		return juce::NormalisableRange<float>(
			min, max,
			[rangedShape](float start, float end, float normalized) {
				auto lin0 = start + normalized * (rangedShape - start);
				auto lin1 = rangedShape + normalized * (end - rangedShape);
				auto lin2 = lin0 + normalized * (lin1 - lin0);
				return lin2;
			},
			[rangedShape](float start, float end, float value) {
				auto start2 = 2 * start;
				auto shape2 = 2 * rangedShape;
				auto t0 = start2 - shape2;
				auto t1 = std::sqrt(std::pow(shape2 - start2, 2.f) - 4.f * (start - value) * (start - shape2 + end));
				auto t2 = 2.f * (start - shape2 + end);
				auto y = (t0 + t1) / t2;
				return juce::jlimit(0.f, 1.f, y);
			}
			);
	}
	static std::vector<float> getTempoSyncValues(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<float> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			const auto beat = 1.f / static_cast<float>(denominator);
			const auto euclid = beat * 3.f / 4.f;
			const auto triplet = beat * 2.f / 3.f;
			rates.emplace_back(beat);
			rates.emplace_back(euclid);
			rates.emplace_back(triplet);
		}
		const auto denominator = 1 << range;
		const auto beat = 1.f / static_cast<float>(denominator);
		rates.emplace_back(beat);
		return rates;
	}
	static std::vector<juce::String> getTempoSyncStrings(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<juce::String> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			juce::String beat("1/" + juce::String(denominator));
			rates.emplace_back(beat);
			rates.emplace_back(beat + ".");
			rates.emplace_back(beat + "t");
		}
		const auto denominator = 1 << range;
		juce::String beat("1/" + juce::String(denominator));
		rates.emplace_back(beat);
		return rates;
	}
	static juce::NormalisableRange<float> getTempoSyncRange(const std::vector<float>& rates) {
		return juce::NormalisableRange<float>(
			0.f, static_cast<float>(rates.size()),
			[rates](float, float end, float normalized) {
				const auto idx = static_cast<int>(normalized * end);
				if (idx >= end) return rates[idx - 1];
				return rates[idx];
			},
			[rates](float, float end, float mapped) {
				for (auto r = 0; r < end; ++r)
					if (rates[r] == mapped)
						return static_cast<float>(r) / end;
				return 0.f;
			}
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
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue,
		const juce::NormalisableRange<float>& range = juce::NormalisableRange<float>({ 0.f,1.f },0.f),
		std::function<juce::String(float value, int maxLen)> stringFromValue = [](float value, int) { return static_cast<juce::String>(std::rint(value * 100.f)); }) {
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), range, defaultValue, getName(i),
			juce::AudioProcessorParameter::Category::genericParameter,
			stringFromValue
			);
	}

	static std::function<juce::String(float, int)> getMsStr() {
		return [](float value, int) {
			return static_cast<juce::String>(std::rint(value)).substring(0, 5) + " ms"; };
	}

	static std::function<juce::String(bool, int)> getSyncStr() {
		return [](bool value, int) {
			return value ? static_cast<juce::String>("Sync") : static_cast<juce::String>("Free");
		};
	}
	static std::function<juce::String(float, int)> getRateStr(
		const juce::AudioProcessorValueTreeState& apvts,
		const juce::NormalisableRange<float>& freeRange,
		const std::vector<juce::String>& syncStrings)
	{
		return [&tree = apvts, freeRange, syncStrings](float value, int) {
			const auto syncP = tree.getRawParameterValue(getID(ID::LFOSync));
			const auto sync = syncP->load();
			if (sync == 0.f) {
				value = freeRange.convertFrom0to1(value);
				return static_cast<juce::String>(std::rint(value)).substring(0, 4);
			}
			else {
				const auto idx = static_cast<int>(value * syncStrings.size());
				return value < 1.f ? syncStrings[idx] : syncStrings[idx - 1];
			}
		};
	}

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters(const juce::AudioProcessorValueTreeState& apvts, MultiRange& lfoFreeSyncRanges) {
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

		parameters.push_back(createParameter(ID::Macro0, 0.f));
		parameters.push_back(createParameter(ID::Macro1, 0.f));
		parameters.push_back(createParameter(ID::Macro2, 0.f));
		parameters.push_back(createParameter(ID::Macro3, 0.f));
		parameters.push_back(createParameter(ID::Depth, 1.f));
		parameters.push_back(createParameter(ID::ModulesMix, 0.5f));

		parameters.push_back(createParameter(ID::EnvFolAtk, 1.f, juce::NormalisableRange<float>(6.f, 1000.f), getMsStr()));
		parameters.push_back(createParameter(ID::EnvFolRls, 0.5f, juce::NormalisableRange<float>(6.f, 1000.f), getMsStr()));
		parameters.push_back(createParameter(ID::EnvFolWdth, 1.f));

		auto tsValues = getTempoSyncValues(6);
		auto tsStrings = getTempoSyncStrings(6);

		lfoFreeSyncRanges.add("free", { .1f, 20.f, 1.f });
		lfoFreeSyncRanges.add("sync", getTempoSyncRange(tsValues));
		auto rateStr = getRateStr(apvts, lfoFreeSyncRanges("free"), tsStrings);

		parameters.push_back(createPBool(ID::LFOSync, false, getSyncStr()));
		parameters.push_back(createParameter(ID::LFORate, .5f, juce::NormalisableRange<float>(0.f, 1.f, 0.f), rateStr));
		parameters.push_back(createParameter(ID::LFOWdth, 0.f, juce::NormalisableRange<float>(-1.f, 1.f, 0.f)));
		
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

	param::MultiRange lfoFreeSyncRanges;
	juce::AudioProcessorValueTreeState apvts;
	ThreadSafeObject<modSys2::Matrix> matrix2;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModularTestAudioProcessor)
};
