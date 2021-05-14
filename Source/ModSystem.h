#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>
#include "Vec2D.h"

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
	static constexpr float tau = 6.28318530718f;
	static float msInSamples(float ms, float Fs) { return ms * Fs * .001f; }
	static juce::AudioPlayHead::CurrentPositionInfo getDefaultPlayHead() {
		juce::AudioPlayHead::CurrentPositionInfo cpi;
		cpi.bpm = 120;
		cpi.editOriginTime = 0;
		cpi.frameRate = juce::AudioPlayHead::FrameRateType::fps25;
		cpi.isLooping = false;
		cpi.isPlaying = true;
		cpi.isRecording = false;
		cpi.ppqLoopEnd = 1;
		cpi.ppqLoopStart = 0;
		cpi.ppqPosition = 420;
		cpi.ppqPositionOfLastBarStart = 69;
		cpi.timeInSamples = 0;
		cpi.timeInSeconds = 0;
		cpi.timeSigDenominator = 1;
		cpi.timeSigNumerator = 1;
		return cpi;
	}

	/*
	* spline interpolation that expects indexes that never go out of bounds
	* to reduce need for if-statements
	*/
	namespace spline {
		static constexpr int Size = 4;
		// hermit cubic spline
		// hornersheme
		// thx peter
		static float process(const float* data, const float x) noexcept {
			const auto iFloor = std::floor(x);
			auto i0 = static_cast<int>(iFloor);
			auto i1 = i0 + 1;
			auto i2 = i0 + 2;
			auto i3 = i0 + 3;

			const auto frac = x - iFloor;
			const auto v0 = data[i0];
			const auto v1 = data[i1];
			const auto v2 = data[i2];
			const auto v3 = data[i3];

			const auto c0 = v1;
			const auto c1 = .5f * (v2 - v0);
			const auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
			const auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

			return ((c3 * frac + c2) * frac + c1) * frac + c0;
		}
	};

	/*
	* makes sure things can be identified
	*/
	struct Identifiable {
		Identifiable(const juce::Identifier& tID) :
			id(tID)
		{}
		Identifiable(const juce::String& tID) :
			id(tID)
		{}
		bool hasID(const juce::Identifier& otherID) const noexcept { return id == otherID; }
		bool operator==(const Identifiable& other) const noexcept { return id == other.id; }
		juce::Identifier id;
	};

	/*
	* an audio block
	*/
	struct Block {
		// SET
		void prepareToPlay(const int numChannels, const int blockSize) {
			block.resize(numChannels, blockSize);
		}
		// PROCESS
		float& operator()(const int ch, const int s) noexcept { return block(ch, s); }
		void limit(const int numSamples) noexcept {
			for (auto ch = 0; ch < block.data.size(); ++ch)
				for (auto s = 0; s < numSamples; ++s)
					if (block(ch, s) < 0.f) block(ch, s) = 0.f;
					else if (block(ch, s) > 1.f) block(ch, s) = 1.f;
		}
		// GET
		const float operator()(const int ch, const int s) const noexcept { return block(ch, s); }
		Vec2D<float>& data() noexcept { return block; }
		const Vec2D<float>& data() const noexcept { return block; }
		const size_t numChannels() const noexcept { return block.data.size(); }
		const size_t numSamples() const noexcept { return block.data[0].size(); }
	protected:
		Vec2D<float> block;
	};

	/*
	* parameter smoothing by interpolating with a sine wave
	*/
	struct Smoothing2 {
		Smoothing2() :
			env(0.f),
			startValue(0.f), endValue(0.f), rangeValue(0.f),
			idx(0.f), length(22050.f),
			isWorking(false)
		{}
		void setLength(const float samples) noexcept { length = samples; }
		void processBlock(Block& block, const float dest, const int numSamples, const int ch) noexcept {
			if (!isWorking) {
				if (env == dest)
					return bypass(block, dest, numSamples, ch);
				setNewDestination(dest);
			}
			else if (length == 0)
				return bypass(block, dest, numSamples, ch);
			processWork(block, dest, numSamples, ch);
		}
	protected:
		float env, startValue, endValue, rangeValue, idx, length;
		bool isWorking;

		void setNewDestination(const float dest) noexcept {
			startValue = env;
			endValue = dest;
			rangeValue = endValue - startValue;
			idx = 0.f;
			isWorking = true;
		}
		void processWork(Block& block, const float dest, const int numSamples, const int ch) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				if (idx >= length) {
					if (dest != endValue)
						setNewDestination(dest);
					else {
						for (auto s1 = s; s1 < numSamples; ++s1)
							block(ch, s1) = endValue;
						isWorking = false;
						return;
					}
				}
				const auto curve = .5f - std::cos(idx * pi / length) * .5f;
				block(ch, s) = env = startValue + curve * rangeValue;
				++idx;
			}
		}
		void bypass(Block& block, const float dest, const int numSamples, const int ch) noexcept {
			for (auto s = 0; s < numSamples; ++s)
				block(ch, s) = dest;
		}
	};

	/*
	* a parameter, its block and a lowpass filter
	*/
	struct Parameter :
		public Identifiable
	{
		Parameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID) :
			Identifiable(pID),
			parameter(apvts.getRawParameterValue(pID)),
			rap(*apvts.getParameter(pID)),
			sumValue(),
			block(),
			smoothing(),
			Fs(1.f)
		{}
		// SET
		void prepareToPlay(const int numChannels, const int blockSize, double sampleRate) {
			Fs = static_cast<float>(sampleRate);
			block.prepareToPlay(numChannels, blockSize);
			if (sumValue.size() == numChannels) return;
			sumValue.clear();
			for (auto c = 0; c < numChannels; ++c)
				sumValue.push_back(std::make_shared<std::atomic<float>>(0.f));
		}
		void setSmoothingLengthInSamples(const float length) noexcept { smoothing.setLength(length); }
		// PROCESS
		void processBlock(const int numSamples) noexcept {
			const int numChannels = block.numChannels();
			const auto nextValue = rap.convertTo0to1(parameter->load());
			smoothing.processBlock(block, nextValue, numSamples, 0);
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = block(0, s);
		}
		void storeSumValue(const int numChannels, const int lastSample) {
			for (auto ch = 0; ch < numChannels; ++ch)
				sumValue[ch]->store(block(ch, lastSample));
		}
		float& operator()(const int ch, const int s) { return block(ch, s); }
		void limit(const int numSamples) noexcept { block.limit(numSamples); }
		// GET NORMAL
		float getSumValue(const int ch) const { return sumValue[ch]->load(); }
		const float& operator()(const int ch, const int s) const noexcept { return block(ch, s); }
		Vec2D<float>& data() noexcept { return block.data(); }
		const Vec2D<float>& data() const noexcept { return block.data(); }
		// GET CONVERTED
		const float denormalized(const int ch, const int s) const noexcept { return rap.convertFrom0to1(block(ch, s)); }
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter& rap;
		std::vector<std::shared_ptr<std::atomic<float>>> sumValue;
		Block block;
		Smoothing2 smoothing;
		float Fs;
	};

	/*
	* a modulator's destination pointer
	*/
	struct Destination :
		public Identifiable
	{
		Destination(const juce::Identifier& dID, Vec2D<float>& destB, float defaultAtten = 1.f) :
			Identifiable(dID),
			destBlock(destB),
			attenuvertor(defaultAtten)
		{}
		void processBlock(const Block& block, const int numChannels, const int numSamples) noexcept {
			const auto g = attenuvertor.load();
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					destBlock(ch, s) += block(ch, s) * g;
		}
		void setValue(float value) noexcept { attenuvertor.store(value); }
		const float getValue() const noexcept { return attenuvertor.load(); }
	protected:
		Vec2D<float>& destBlock;
		std::atomic<float> attenuvertor;
	};

	/*
	* a module that can modulate destinations
	*/
	struct Modulator :
		public Identifiable
	{
		Modulator(const juce::Identifier& mID) :
			Identifiable(mID),
			outValue(),
			destinations()
		{}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
			outValue(),
			destinations()
		{}
		// SET
		virtual void prepareToPlay(const int numChannels, const double sampleRate) {
			if (outValue.size() != numChannels) {
				outValue.clear();
				for (auto c = 0; c < numChannels; ++c)
					outValue.push_back(std::make_shared<std::atomic<float>>(0.f));
			}
			Fs = static_cast<float>(sampleRate);
		}
		void addDestination(const juce::Identifier& dID, Vec2D<float>& destB, float atten = 1.f) {
			if (hasDestination(dID)) return;
			destinations.push_back(std::make_shared<Destination>(
				dID, destB, atten
				));
		}
		void removeDestination(const juce::Identifier& dID) {
			for (auto d = 0; d < destinations.size(); ++d) {
				auto& destination = destinations[d];
				auto dest = destination.get();
				if (dest->id == dID) {
					destinations.erase(destinations.begin() + d);
					return;
				}
			}
		}
		virtual void addStuff(const juce::Identifier& sID, const std::vector<Anything>& stuff) {}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			auto d = getDestination(pID);
			return d->get()->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) = 0;
		void processDestinations(Block& block, const int numChannels, const int numSamples) noexcept {
			for (auto& destination : destinations)
				destination.get()->processBlock(block, numChannels, numSamples);
		}
		void storeOutValue(const float lastSampleValue, const int ch) {
			outValue[ch]->store(lastSampleValue);
		}
		// GET
		bool hasDestination(const juce::Identifier& pID) const noexcept {
			const auto d = getDestination(pID);
			return d != nullptr;
		}
		const std::shared_ptr<Destination>* getDestination(const juce::Identifier& pID) const noexcept {
			for (auto& destination : destinations) {
				auto d = destination.get();
				if (d->id == pID)
					return &destination;
			}
			return nullptr;
		}
		const float getAttenuvertor(const juce::Identifier& pID) const noexcept {
			auto d = getDestination(pID);
			return d->get()->getValue();
		}
		const std::vector<std::shared_ptr<Destination>>& getDestinations() const noexcept {
			return destinations;
		}
		const float getOutValue(const int ch) const noexcept { return outValue[ch]->load(); }
	protected:
		std::vector<std::shared_ptr<Destination>> destinations;
		std::vector<std::shared_ptr<std::atomic<float>>> outValue;
		float Fs;
	};

	/*
	* a macro modulator
	*/
	struct MacroModulator :
		public Modulator
	{
		MacroModulator(const Parameter& param) :
			Modulator(param.id),
			parameter(param)
		{}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block(ch, s) = parameter(ch, s);
		}
	protected:
		const Parameter& parameter;
	};

	/*
	* an envelope follower modulator
	*/
	struct EnvelopeFollowerModulator :
		public Modulator
	{
		EnvelopeFollowerModulator(const juce::String& mID, const Parameter& atkParam,
			const Parameter& rlsParam, const Parameter& widthParam) :
			Modulator(mID),
			attackParameter(atkParam),
			releaseParameter(rlsParam),
			widthParameter(widthParam),
			env()
		{}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			env.resize(numChannels, 0.f);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			const int numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			getSamples(audioBuffer, block);
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch) {
				const auto atkInMs = attackParameter.denormalized(ch, lastSample);
				const auto rlsInMs = releaseParameter.denormalized(ch, lastSample);
				const auto atkInSamples = msInSamples(atkInMs, Fs);
				const auto rlsInSamples = msInSamples(rlsInMs, Fs);
				const auto atkSpeed = 1.f / atkInSamples;
				const auto rlsSpeed = 1.f / rlsInSamples;
				const auto gain = makeAutoGain(atkSpeed, rlsSpeed);
				for (auto s = 0; s < numSamples; ++s) {
					if (env[ch] < block(ch, s))
						env[ch] += atkSpeed * (block(ch, s) - env[ch]);
					else if (env[ch] > block(ch, s))
						env[ch] += rlsSpeed * (block(ch, s) - env[ch]);
					block(ch, s) = env[ch] * gain;
				}
			}
			processWidth(block, numChannels, numSamples, lastSample);
			for (auto ch = 1; ch < numChannels; ++ch)
				storeOutValue(block(ch, lastSample), ch);
		}
	protected:
		const Parameter& attackParameter;
		const Parameter& releaseParameter;
		const Parameter& widthParameter;
		std::vector<float> env;
	private:
		inline void getSamples(const juce::AudioBuffer<float>& audioBuffer, Block& block) {
			const auto samples = audioBuffer.getArrayOfReadPointers();
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block(ch, s) = std::abs(samples[ch][s]);
		}

		const float makeAutoGain(const float atkSpeed, const float rlsSpeed) noexcept {
			return 1.f + std::sqrt(rlsSpeed / atkSpeed);
		}

		inline void processWidth(Block& block, const int numChannels, const int numSamples, const int lastSample) {
			const auto lastSampleValue = block(0, lastSample);
			storeOutValue(lastSampleValue, 0);
			const auto width = widthParameter(0, lastSample);
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = block(0, s) + width * (block(ch, s) - block(0, s));
		}
	};

	/*
	* an lfo modulator
	*/
	class LFOModulator :
		public Modulator
	{
		class WaveTables {
			enum { TableIdx, SampleIdx };
		public:
			WaveTables():
				tables(),
				tableSize()
			{}
			void resize(const int tablesCount, const int _tableSize) {
				tableSize = _tableSize;
				tables.resize(tablesCount, tableSize + spline::Size);
			}
			void addWaveTable(const std::function<float(float)>& func, const int tableIdx) {
				auto x = 0.f;
				const auto inc = 1.f / static_cast<float>(tableSize);
				for (auto i = 0; i < tableSize; ++i, x += inc)
					tables(tableIdx, i) = func(x);
				for (auto i = 0; i < spline::Size; ++i)
					tables(tableIdx, i + tableSize) = tables(tableIdx, i);
			}
			const float operator()(const float phase, const int tableIdx) const noexcept {
				const auto x = phase * tableSize;
				return spline::process(tables.data[tableIdx].data(), x);
			}
		protected:
			Vec2D<float> tables;
			int tableSize;
		};
	public:
		LFOModulator(const juce::String& mID, const Parameter& syncParam, const Parameter& rateParam,
			const Parameter& wdthParam, const Parameter& waveTableParam, const param::MultiRange& ranges) :
			Modulator(mID),
			syncParameter(syncParam),
			rateParameter(rateParam),
			widthParameter(wdthParam),
			waveTableParameter(waveTableParam),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			phase(),
			waveTables(),
			fsInv(0.f)
		{}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			phase.resize(numChannels);
			fsInv = 1.f / Fs;
		}
		void addStuff(const juce::Identifier& sID, const std::vector<Anything>& stuff) override {
			if (sID.toString() == "wavetables") {
				const auto tablesCount = stuff.size() - 1;
				const auto tableSize = stuff[0].get<int>();
				waveTables.resize(tablesCount, *tableSize);
				for (auto i = 1; i < stuff.size(); ++i) {
					const auto wtFunc = stuff[i].get<std::function<float(float)>>();
					waveTables.addWaveTable(*wtFunc, i - 1);
				}	
			}
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const bool isFree = syncParameter(0, 0) < .5f;

			if (isFree)
				for (auto ch = 0; ch < numChannels; ++ch) {
					const auto rateValue = rateParameter(ch, lastSample);
					const auto rate = multiRange(freeID).convertFrom0to1(rateValue);
					const auto inc = rate * fsInv;
					processPhase(block, inc, ch, numSamples);
				}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto ppq = playHead.ppqPosition * .25;
				for (auto ch = 0; ch < numChannels; ++ch) {
					const auto rateValue = rateParameter(ch, lastSample);
					const auto rate = multiRange(syncID).convertFrom0to1(rateValue);
					const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rate);
					const auto ppqCh = static_cast<float>(ppq) / rate;
					auto newPhase = (ppqCh - std::floor(ppqCh));
					phase[ch] = newPhase;
					processPhase(block, inc, ch, numSamples);
				}
			}
			processWidth(block, lastSample, numChannels, numSamples);
			processWaveTable(block, numChannels, numSamples);
			for (auto ch = 0; ch < numChannels; ++ch)
				storeOutValue(block(ch, lastSample), ch);
		}
	protected:
		const Parameter& syncParameter;
		const Parameter& rateParameter;
		const Parameter& widthParameter;
		const Parameter& waveTableParameter;
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase;
		WaveTables waveTables;
		float fsInv;

		inline void processWidth(Block& block, const int lastSample, const int numChannels, const int numSamples) {
			const auto width = widthParameter.denormalized(0, lastSample) * .5f;
			if (width > 0.f)
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s) {
						auto offsetSample = block(ch, s) + width;
						if (offsetSample >= 1.f)
							--offsetSample;
						block(ch, s) = offsetSample;
					}
			else
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s) {
						auto offsetSample = block(ch, s) + width;
						if (offsetSample < 0.f)
							++offsetSample;
						block(ch, s) = offsetSample;
					}
		}

		void processPhase(Block& block, const float inc, const int ch, const int numSamples) {
			for (auto s = 0; s < numSamples; ++s) {
				phase[ch] += inc;
				if (phase[ch] >= 1.f)
					--phase[ch];
				block(ch, s) = phase[ch];
			}
		}

		void processWaveTable(Block& block, const int numChannels, const int numSamples) {
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = waveTables(block(ch, s), static_cast<int>(waveTableParameter.denormalized(ch, s)));
		}
	};

	/*
	* some identifiers used for serialization
	*/
	struct Type {
		Type() :
			modSys("MODSYS"),
			modulator("MOD"),
			destination("DEST"),
			id("id"),
			atten("atten"),
			param("PARAM")
		{}
		const juce::Identifier modSys;
		const juce::Identifier modulator;
		const juce::Identifier destination;
		const juce::Identifier id;
		const juce::Identifier atten;
		const juce::Identifier param;
	};
	/*
	* the thing that handles everything in the end
	*/
	struct Matrix {
		Matrix(juce::AudioProcessorValueTreeState& apvts) :
			parameters(),
			modulators(),
			curPosInfo(getDefaultPlayHead()),
			block(),
			selectedModulator()
		{
			const Type type;
			auto& state = apvts.state;
			const auto numChildren = state.getNumChildren();
			for (auto c = 0; c < numChildren; ++c) {
				const auto& pChild = apvts.state.getChild(c);
				if (pChild.hasType(type.param)) {
					const auto pID = pChild.getProperty(type.id).toString();
					parameters.push_back(std::make_shared<Parameter>(apvts, pID));
				}
			}
		}
		Matrix(const Matrix* other) :
			parameters(other->parameters),
			modulators(other->modulators),
			curPosInfo(getDefaultPlayHead()),
			block(other->block),
			selectedModulator(other->selectedModulator)
		{}
		// SET
		void prepareToPlay(const int numChannels, const int blockSize, const double sampleRate) {
			block.prepareToPlay(numChannels, blockSize);
			for (auto& p : parameters)
				p->prepareToPlay(numChannels, blockSize, sampleRate);
			for (auto& m : modulators)
				m->prepareToPlay(numChannels, sampleRate);
		}
		void setSmoothingLengthInSamples(const juce::Identifier& pID, float length) {
			auto p = getParameter(pID);
			p->get()->setSmoothingLengthInSamples(length);
		}
		// SERIALIZE
		void setState(juce::AudioProcessorValueTreeState& apvts) {
			// BINARY TO VALUETREE
			auto& state = apvts.state;
			const Type type;
			auto modSysChild = state.getChildWithName(type.modSys);
			if (!modSysChild.isValid()) return;
			auto numModulators = modSysChild.getNumChildren();
			for (auto m = 0; m < numModulators; ++m) {
				const auto modChild = modSysChild.getChild(m);
				const auto mID = modChild.getProperty(type.id).toString();
				const auto numDestinations = modChild.getNumChildren();
				for (auto d = 0; d < numDestinations; ++d) {
					const auto destChild = modChild.getChild(d);
					const auto dID = destChild.getProperty(type.id).toString();
					const auto dValue = static_cast<float>(destChild.getProperty(type.atten));
					addDestination(mID, dID, dValue);
				}
			}
		}
		void getState(juce::AudioProcessorValueTreeState& apvts) {
			// VALUETREE TO BINARY
			auto& state = apvts.state;
			const Type type;
			auto modSysChild = state.getChildWithName(type.modSys);
			if (!modSysChild.isValid()) {
				modSysChild = juce::ValueTree(type.modSys);
				state.appendChild(modSysChild, nullptr);
			}
			modSysChild.removeAllChildren(nullptr);

			for (const auto& modulator : modulators) {
				const auto mod = modulator.get();
				juce::ValueTree modChild(type.modulator);
				modChild.setProperty(type.id, mod->id.toString(), nullptr);
				const auto& destVec = mod->getDestinations();
				for (const auto& dest : destVec) {
					const auto d = dest.get();
					juce::ValueTree destChild(type.destination);
					destChild.setProperty(type.id, d->id.toString(), nullptr);
					destChild.setProperty(type.atten, d->getValue(), nullptr);
					modChild.appendChild(destChild, nullptr);
				}
				modSysChild.appendChild(modChild, nullptr);
			}
		}
		// ADD MODULATORS
		std::shared_ptr<Modulator>& addMacroModulator(const juce::Identifier& pID) {
			const auto p = getParameter(pID)->get();
			modulators.push_back(std::make_shared<MacroModulator>(*p));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator>& addEnvelopeFollowerModulator(const juce::Identifier& atkPID, const juce::Identifier& rlsPID, const juce::Identifier& wdthPID, int idx) {
			const auto atkP = getParameter(atkPID)->get();
			const auto rlsP = getParameter(rlsPID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const juce::String idString("EnvFol" + idx);
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, *atkP, *rlsP, *wdthP));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator>& addLFOModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& wdthPID, const juce::Identifier& waveTablePID,
			const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID)->get();
			const auto rateP = getParameter(ratePID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const auto waveTableP = getParameter(waveTablePID)->get();
			const juce::String idString("Phase" + idx);
			modulators.push_back(std::make_shared<LFOModulator>(idString, *syncP, *rateP, *wdthP, *waveTableP, ranges));
			return modulators[modulators.size() - 1];
		}
		// MODIFY / REPLACE
		void selectModulator(const juce::Identifier& mID) { selectedModulator = getModulator(mID)->get()->id; }
		void addDestination(const juce::Identifier& mID, const juce::Identifier& pID, const float atten = 1.f) {
			auto param = getParameter(pID)->get();
			getModulator(mID)->get()->addDestination(param->id, param->data(), atten);
		}
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, Vec2D<float>& destBlock, const float atten = 1.f) {
			getModulator(mID)->get()->addDestination(dID, destBlock, atten);
		}
		void removeDestination(const juce::Identifier& mID, const juce::Identifier& dID) {
			getModulator(mID)->get()->removeDestination(dID);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioPlayHead* playHead) {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			if (playHead != nullptr) playHead->getCurrentPosition(curPosInfo);
			for (auto& p : parameters) p.get()->processBlock(numSamples);
			for (auto& m : modulators) {
				m->processBlock(audioBuffer, block, curPosInfo);
				m->processDestinations(block, numChannels, numSamples);
			}
			const auto lastSample = numSamples - 1;
			for (auto& parameter : parameters) {
				auto p = parameter.get();
				p->limit(numSamples);
				p->storeSumValue(numChannels, lastSample);
			}
		}
		// GET
		std::shared_ptr<Modulator>* getSelectedModulator() { return getModulator(selectedModulator); }
		std::shared_ptr<Modulator>* getModulator(const juce::Identifier& mID) {
			for (auto& modulator : modulators)
				if (modulator.get()->hasID(mID))
					return &modulator;
			return nullptr;
		}
		std::shared_ptr<Parameter>* getParameter(const juce::Identifier& pID) {
			for (auto& parameter : parameters)
				if (parameter.get()->hasID(pID))
					return &parameter;
			return nullptr;
		}
	protected:
		std::vector<std::shared_ptr<Parameter>> parameters;
		std::vector<std::shared_ptr<Modulator>> modulators;
		juce::AudioPlayHead::CurrentPositionInfo curPosInfo;
		Block block;
		juce::Identifier selectedModulator;
	};

	/* to do:
	* 
	* modulators
	*	can be bidirectional
	* 
	* envelopeFollowerModulator
	*	if atk or rls cur smoothing: sample-based param-update	
	* 
	* lfoModulator
	*	free/sync >> sync playhead
	*	how to handle dynamic param ranges (wtf did i mean with this)
	* 
	* add random lfo
	*/
}