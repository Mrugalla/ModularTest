#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>
#include "Vec2D.h"

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
	static constexpr float tau = 6.28318530718f;
	static float msInSamples(float ms, float Fs) noexcept { return ms * Fs * .001f; }
	static float hzInSamples(float hz, float Fs) noexcept { return Fs / hz; }
	static float hzInSlewRate(float hz, float Fs) noexcept { return 1.f / hzInSamples(hz, Fs); }
	static float dbInGain(float db) noexcept { return std::pow(10.f, db * .05f); }
	static float gainInDb(float gain) noexcept { return 20.f * std::log10(gain); }
	/* values and bias are normalized [0,1] lin curve at around bias = .6 for some reason */
	static float weight(float value, float bias) noexcept {
		if (value == 0.f) return 0.f;
		const float b0 = std::pow(value, bias);
		const float b1 = std::pow(value, 1 / bias);
		return b1 / b0;
	}
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
			const auto numChannels = block.numChannels();
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
		void set(const float value, const int ch, const int s) noexcept { block(ch, s) = value; }
		void limit(const int numSamples) noexcept { block.limit(numSamples); }
		// GET NORMAL
		float getSumValue(const int ch) const { return sumValue[ch]->load(); }
		float get(const int ch, const int s) const noexcept { return block(ch, s); }
		Vec2D<float>& data() noexcept { return block.data(); }
		const Vec2D<float>& data() const noexcept { return block.data(); }
		// GET CONVERTED
		const float denormalized(const int ch, const int s) const noexcept { return rap.convertFrom0to1(juce::jlimit(0.f ,1.f, block(ch, s))); }
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
		Destination(const juce::Identifier& dID, Vec2D<float>* destB, float defaultAtten = 1.f, bool defaultBidirectional = false) :
			Identifiable(dID),
			destBlock(destB),
			attenuvertor(defaultAtten),
			bidirectional(defaultBidirectional)
		{}
		Destination(Destination& d) :
			Identifiable(d.id),
			destBlock(d.destBlock),
			attenuvertor(d.getValue()),
			bidirectional(d.isBidirectional())
		{}
		void processBlock(const Block& block, const int numChannels, const int numSamples) noexcept {
			const auto g = getValue();
			if(isBidirectional())
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						destBlock->operator()(ch, s) += 2.f * (block(ch, s) - .5f) * g;
			else
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						destBlock->operator()(ch, s) += block(ch, s) * g;
		}
		void setValue(float value) noexcept { attenuvertor.set(value); }
		float getValue() const noexcept { return attenuvertor.get(); }
		void setBirectional(bool b) noexcept { bidirectional.set(b); }
		bool isBidirectional() const noexcept { return bidirectional.get(); }
	protected:
		Vec2D<float>* destBlock;
		juce::Atomic<float> attenuvertor;
		juce::Atomic<bool> bidirectional;
	};

	/*
	* a module that can modulate destinations
	*/
	struct Modulator :
		public Identifiable
	{
		Modulator(const juce::Identifier& mID) :
			Identifiable(mID),
			params(),
			destinations(),
			outValue(),
			Fs(1)
		{}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
			params(),
			destinations(),
			outValue(),
			Fs(1)
		{}
		// SET
		virtual void prepareToPlay(const int numChannels, const double sampleRate) {
			if (outValue.size() != numChannels) {
				outValue.clear();
				for (auto c = 0; c < numChannels; ++c)
					outValue.push_back(juce::Atomic<float>(0.f));
			}
			Fs = static_cast<float>(sampleRate);
		}
		void addDestination(const juce::Identifier& dID, Vec2D<float>& destB, float atten = 1.f, bool bidirec = false) {
			if (hasDestination(dID)) return;
			destinations.push_back({dID, &destB, atten, bidirec});
		}
		void removeDestination(const juce::Identifier& dID) {
			for (auto d = 0; d < destinations.size(); ++d) {
				auto& dest = destinations[d];
				if (dest == dID) {
					destinations.erase(destinations.begin() + d);
					return;
				}
			}
		}
		void removeDestinations(const Modulator* other) {
			const auto& otherParams = other->getParameters();
			for (const auto op : otherParams)
				if (hasDestination(op->id))
					removeDestination(op->id);
		}
		virtual void addStuff(const juce::String& /*sID*/, const VectorAnything& /*stuff*/) {}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			getDestination(pID)->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) = 0;
		void processDestinations(Block& block, const int numChannels, const int numSamples) noexcept {
			for (auto& destination : destinations)
				destination.processBlock(block, numChannels, numSamples);
		}
		void storeOutValue(Block& block, const int lastSample) {
			for(auto ch = 0; ch < outValue.size(); ++ch)
				outValue[ch].set(block(ch, lastSample));
		}
		// GET
		bool hasDestination(const juce::Identifier& pID) const noexcept {
			return getDestination(pID) != nullptr;
		}
		Destination* getDestination(const juce::Identifier& pID) noexcept {
			for (auto d = 0; d < destinations.size(); ++d)
				if (destinations[d] == pID)
					return &destinations[d];
			return nullptr;
		}
		const Destination* getDestination(const juce::Identifier& pID) const noexcept {
			for (auto d = 0; d < destinations.size(); ++d)
				if (destinations[d] == pID)
					return &destinations[d];
			return nullptr;
		}
		const float getAttenuvertor(const juce::Identifier& pID) const noexcept {
			return getDestination(pID)->getValue();
		}
		const std::vector<Destination>& getDestinations() const noexcept {
			return destinations;
		}
		float getOutValue(const int ch) const noexcept { return outValue[ch].get(); }
		const std::vector<const Parameter*>& getParameters() const noexcept { return params; }
		bool usesParameter(const juce::Identifier& pID) const noexcept {
			for (const auto p : params)
				if (p->id == pID)
					return true;
			return false;
		}
		bool modulates(Modulator& other) const noexcept {
			for (const auto p : params)
				if (other.hasDestination(p->id))
					return true;
			return false;
		}
	protected:
		std::vector<const Parameter*> params;
		std::vector<Destination> destinations;
		std::vector<juce::Atomic<float>> outValue;
		float Fs;
	};

	/*
	* a macro modulator
	*/
	struct MacroModulator :
		public Modulator
	{
		MacroModulator(const Parameter* makroParam) :
			Modulator(makroParam->id)
		{ params.push_back(makroParam); }
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block(ch, s) = params[0]->get(ch, s);
			const auto lastSample = audioBuffer.getNumSamples() - 1;
			storeOutValue(block, lastSample);
		}
	};

	/*
	* an envelope follower modulator
	*/
	class EnvelopeFollowerModulator :
		public Modulator
	{
		enum { Gain, Attack, Release, Bias, Width };
	public:
		EnvelopeFollowerModulator(const juce::String& mID, const Parameter* inputGain, const Parameter* atkParam,
			const Parameter* rlsParam, const Parameter* biasParam, const Parameter* widthParam) :
			Modulator(mID),
			env()
		{
			params.push_back(inputGain);
			params.push_back(atkParam);
			params.push_back(rlsParam);
			params.push_back(biasParam);
			params.push_back(widthParam);
		}
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
				const auto atkInMs = params[Attack]->denormalized(ch, 0);
				const auto rlsInMs = params[Release]->denormalized(ch, 0);
				const auto bias = 1.f - juce::jlimit(0.f ,1.f, params[Bias]->get(ch, 0));
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
					block(ch, s) = processBias(block(ch, s), bias);
				}
			}
			processWidth(block, numChannels, numSamples);
			storeOutValue(block, lastSample);
		}
	protected:
		std::vector<float> env;
	private:
		inline void getSamples(const juce::AudioBuffer<float>& audioBuffer, Block& block) {
			const auto samples = audioBuffer.getArrayOfReadPointers();
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block(ch, s) = std::abs(samples[ch][s]) * dbInGain(params[Gain]->denormalized(ch, s));
		}

		const inline float makeAutoGain(const float atkSpeed, const float rlsSpeed) const noexcept {
			return 1.f + std::sqrt(rlsSpeed / atkSpeed);
		}
		const inline float processBias(const float value, const float biasV) const noexcept { return std::pow(value, biasV); }

		inline void processWidth(Block& block, const int numChannels, const int numSamples) noexcept {
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = block(0, s) + params[Width]->get(ch, s) * (block(ch, s) - block(0, s));
		}
	};

	/*
	* an lfo modulator
	*/
	class LFOModulator :
		public Modulator
	{
		enum { Sync, Rate, Width, WaveTable };
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
		LFOModulator(const juce::String& mID, const Parameter* syncParam, const Parameter* rateParam,
			const Parameter* wdthParam, const Parameter* waveTableParam, const param::MultiRange& ranges) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			phase(),
			waveTables(),
			fsInv(0.f)
		{
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(wdthParam);
			params.push_back(waveTableParam);
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			phase.resize(numChannels);
			fsInv = 1.f / Fs;
		}
		void addStuff(const juce::String& sID, const VectorAnything& stuff) override {
			if (sID == "wavetables") {
				const auto tablesCount = static_cast<int>(stuff.size()) - 1;
				const auto tableSize = stuff.get<int>(0);
				waveTables.resize(tablesCount, *tableSize);
				for (auto i = 1; i < stuff.size(); ++i) {
					const auto wtFunc = stuff.get<std::function<float(float)>>(i);
					waveTables.addWaveTable(*wtFunc, i - 1);
				}	
			}
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const bool isFree = params[Sync]->get(0, 0) < .5f;

			if (isFree)
				for (auto ch = 0; ch < numChannels; ++ch) {
					const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(ch, lastSample));
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
					const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(ch, lastSample));
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
			storeOutValue(block, lastSample);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase;
		WaveTables waveTables;
		float fsInv;

		inline void processWidth(Block& block, const int lastSample, const int numChannels, const int numSamples) {
			const auto width = params[Width]->denormalized(0, lastSample) * .5f;
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
			for (auto ch = 0; ch < numChannels; ++ch) {
				const auto wtValue = static_cast<int>(params[WaveTable]->denormalized(ch, 0));
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = waveTables(block(ch, s), wtValue);
			}
		}
	};

	/*
	* a random modulator
	*/
	class RandomModulator :
		public Modulator
	{
		enum { Sync, Rate, Bias, Width, Smooth };
		struct LP1Pole {
			LP1Pole() :
				env(0.f),
				cutoff(.0001f)
			{}
			void processBlock(float* block, const int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s)
					block[s] = process(block[s]);
			}
			const float process(const float sample) noexcept {
				env += cutoff * (sample - env);
				return env;
			}
			float env, cutoff;
		};
		struct LP1PoleOrder {
			LP1PoleOrder(int order) :
				filters()
			{ filters.resize(order); }
			void setCutoff(float amount, float rateInSlew) {
				const auto cutoff = 1.f + amount * (rateInSlew - 1.f);
				for (auto& f : filters)
					f.cutoff = cutoff;
			}
			void processBlock(float* block, const int numSamples) noexcept {
				for (auto f = 0; f < filters.size(); ++f)
					filters[f].processBlock(block, numSamples);
			}
			const float process(float sample) noexcept {
				for (auto& f : filters)
					sample = f.process(sample);
				return sample;
			}
			std::vector<LP1Pole> filters;
		};
	public:
		RandomModulator(const juce::String& mID, const Parameter* syncParam, const Parameter* rateParam,
			const Parameter* biasParam, const Parameter* widthParam, const Parameter* smoothParam,
			const param::MultiRange& ranges) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			phase(), randValue(),
			smoothing(),
			rand(juce::Time::currentTimeMillis()),
			fsInv(1), rateInHz(1)
		{
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(biasParam);
			params.push_back(widthParam);
			params.push_back(smoothParam);
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			const auto filterOrder = 3;
			smoothing.resize(numChannels, filterOrder);
			phase.resize(numChannels, 0);
			randValue.resize(numChannels, 0);
			fsInv = 1.f / Fs;
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			synthesizeRandomSignal(block, playHead, numChannels, numSamples);
			processWidth(block, numChannels, numSamples);
			processSmoothing(block, numChannels, numSamples);
			storeOutValue(block, numSamples - 1);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase, randValue;
		std::vector<LP1PoleOrder> smoothing;
		juce::Random rand;
		float fsInv, rateInHz;
	private:
		void synthesizeRandomSignal(Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead, const int numChannels, const int numSamples) {
			const auto lastSample = numSamples - 1;
			const bool isFree = params[Sync]->get(0, 0) < .5f;

			if (isFree)
				for (auto ch = 0; ch < numChannels; ++ch) {
					const auto rateValue = juce::jlimit(0.f ,1.f , params[Rate]->get(ch, lastSample));
					rateInHz = multiRange(freeID).convertFrom0to1(rateValue);
					const auto inc = rateInHz * fsInv;
					synthesizeRandomSignal(block, phase[ch], inc, numSamples, ch);
				}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto ppq = playHead.ppqPosition * .25;
				for (auto ch = 0; ch < numChannels; ++ch) {
					const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(ch, lastSample));
					rateInHz = multiRange(syncID).convertFrom0to1(rateValue);
					const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rateInHz);
					const auto ppqCh = static_cast<float>(ppq) / rateInHz;
					auto newPhase = (ppqCh - std::floor(ppqCh));
					phase[ch] = newPhase;
					synthesizeRandomSignal(block, newPhase, inc, numSamples, ch);
				}
			}
		}
		void synthesizeRandomSignal(Block& block, const float curPhase, const float inc, const int numSamples, const int ch) {
			phase[ch] = curPhase;
			for (auto s = 0; s < numSamples; ++s) {
				phase[ch] += inc;
				if (phase[ch] >= 1.f) {
					--phase[ch];
					randValue[ch] = getBiasedValue(rand.nextFloat(), ch, s);
				}
				block(ch, s) = randValue[ch];
			}
		}
		void processWidth(Block& block, const int numChannels, const int numSamples) noexcept {
			for (auto ch = 1; ch < numChannels; ++ch) {
				const auto narrowness = (1.f - juce::jlimit(0.f, 1.f, params[Width]->get(ch, 0)));
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) += narrowness * (block(0, s) - block(ch, s));
			}
		}
		void processSmoothing(Block& block, const int numChannels, const int numSamples) {
			const auto rateInSlew = hzInSlewRate(rateInHz, Fs);
			for (auto ch = 0; ch < numChannels; ++ch) {
				const auto magicNumber = .9998f;
				const auto smoothValue = weight(params[Smooth]->get(ch, 0), magicNumber);
				smoothing[ch].setCutoff(smoothValue, rateInSlew);
				smoothing[ch].processBlock(&block(ch, 0), numSamples);
			}	
		}
		const float getBiasedValue(float value, const int ch, const int s) noexcept {
			const auto bias = params[Bias]->get(ch, s);
			if (bias < .5f) {
				const auto a = bias * 2.f;
				return std::atan(std::tan(value * pi - .5f * pi) * a) / pi + .5f;
			}
			const auto a = 1.f - (2.f * bias - 1.f);
			return std::atan(std::tan(value * pi - .5f * pi) / a) / pi + .5f;
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
			bidirec("bidirec"),
			param("PARAM")
		{}
		const juce::Identifier modSys;
		const juce::Identifier modulator;
		const juce::Identifier destination;
		const juce::Identifier id;
		const juce::Identifier atten;
		const juce::Identifier bidirec;
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
		Matrix(const Matrix& other) :
			parameters(other.parameters),
			modulators(other.modulators),
			curPosInfo(getDefaultPlayHead()),
			block(other.block),
			selectedModulator(other.selectedModulator)
		{}
		// SET
		void prepareToPlay(const int numChannels, const int blockSize, const double sampleRate) {
			block.prepareToPlay(numChannels, blockSize);
			for (auto& p : parameters)
				p->prepareToPlay(numChannels, blockSize, sampleRate);
			for (auto& m : modulators)
				m->prepareToPlay(numChannels, sampleRate);
		}
		void setSmoothingLengthInSamples(const juce::Identifier& pID, float length) noexcept {
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
					const auto bidirec = destChild.getProperty(type.bidirec).toString() == "0" ? false : true;
					addDestination(mID, dID, dValue, bidirec);
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

			for (const auto& mod : modulators) {
				juce::ValueTree modChild(type.modulator);
				modChild.setProperty(type.id, mod->id.toString(), nullptr);
				const auto& destVec = mod->getDestinations();
				for (const auto& d : destVec) {
					juce::ValueTree destChild(type.destination);
					destChild.setProperty(type.id, d.id.toString(), nullptr);
					destChild.setProperty(type.atten, d.getValue(), nullptr);
					destChild.setProperty(type.bidirec, d.isBidirectional() ? 1 : 0, nullptr);
					modChild.appendChild(destChild, nullptr);
				}
				modSysChild.appendChild(modChild, nullptr);
			}
		}
		// ADD MODULATORS
		std::shared_ptr<Modulator> addMacroModulator(const juce::Identifier& pID) {
			const auto p = getParameter(pID)->get();
			modulators.push_back(std::make_shared<MacroModulator>(p));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addEnvelopeFollowerModulator(const juce::Identifier& gainPID,
			const juce::Identifier& atkPID, const juce::Identifier& rlsPID,
			const juce::Identifier& biasPID, const juce::Identifier& wdthPID, int idx) {
			const auto gainP = getParameter(gainPID)->get();
			const auto atkP = getParameter(atkPID)->get();
			const auto rlsP = getParameter(rlsPID)->get();
			const auto biasP = getParameter(biasPID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const juce::String idString("EnvFol" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, gainP, atkP, rlsP, biasP, wdthP));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addLFOModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& wdthPID, const juce::Identifier& waveTablePID,
			const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID)->get();
			const auto rateP = getParameter(ratePID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const auto waveTableP = getParameter(waveTablePID)->get();
			const juce::String idString("LFO" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<LFOModulator>(idString, syncP, rateP, wdthP, waveTableP, ranges));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addRandomModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& biasPID, const juce::Identifier& widthPID, const juce::Identifier& smoothPID,
			const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID)->get();
			const auto rateP = getParameter(ratePID)->get();
			const auto biasP = getParameter(biasPID)->get();
			const auto widthP = getParameter(widthPID)->get();
			const auto smoothP = getParameter(smoothPID)->get();
			const juce::String idString("Rand" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<RandomModulator>(idString, syncP, rateP, biasP, widthP, smoothP, ranges));
			return modulators[modulators.size() - 1];
		}
		// MODIFY / REPLACE
		void selectModulator(const juce::Identifier& mID) { selectedModulator = getModulator(mID)->id; }
		void addDestination(const juce::Identifier& mID, const juce::Identifier& pID, const float atten = 1.f, const bool bidirec = false) {
			auto param = getParameter(pID)->get();
			auto thisMod = getModulator(mID);

			for (auto t = 0; t < modulators.size(); ++t) {
				auto maybeThisMod = modulators[t];
				if (maybeThisMod == thisMod)
					for (auto m = 0; m < modulators.size(); ++m) {
						auto otherMod = modulators[m];
						if (otherMod != thisMod)
							if (otherMod->usesParameter(pID)) {
								thisMod->addDestination(param->id, param->data(), atten, bidirec);
								if (t > m)
									std::swap(modulators[t], modulators[m]);
								if(otherMod->modulates(*thisMod))
									otherMod->removeDestinations(thisMod.get());
								return;
							}
					}
			} // fix thing, mod can modulate parameter of own thing
			thisMod->addDestination(param->id, param->data(), atten, bidirec);
		}
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, Vec2D<float>& destBlock, const float atten = 1.f) {
			getModulator(mID)->addDestination(dID, destBlock, atten);
		}
		void removeDestination(const juce::Identifier& mID, const juce::Identifier& dID) {
			getModulator(mID)->removeDestination(dID);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioPlayHead* playHead) {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			if (playHead) playHead->getCurrentPosition(curPosInfo);
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
		std::shared_ptr<Modulator> getSelectedModulator() noexcept { return getModulator(selectedModulator); }
		std::shared_ptr<Modulator> getModulator(const juce::Identifier& mID) noexcept {
			for (auto& mod : modulators) {
				auto m = mod.get();
				if (m->hasID(mID))
					return mod;
			}
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
	* destination: bias / weight
	* 
	* if parameter smoothing not needed: parameter buffersize = 1
	*	(a lot of the modulators don't need: less ram maybe?)
	* 
	* envelopeFollowerModulator
	*	rewrite db to gain calculation so that it happens before parameter smoothing instead
	*	if atk or rls cur smoothing: sample-based param-update
	* 
	* lfoModulator
	*	how to handle dynamic param ranges (wtf did i mean with this)
	*	add pump curve wavetable
	* 
	* all wavy mods (especially temposync ones)
	*	phase parameter
	* 
	* editor
	*	find more elegant way for vectorizing all modulatable parameters
	*	
	*/
}