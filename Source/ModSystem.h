#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
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
	* makes sure things can be identified
	*/
	struct Identifiable {
		Identifiable(const juce::Identifier& tID) :
			id(tID)
		{}
		Identifiable(const juce::String& tID) :
			id(tID)
		{}
		bool hasID(const juce::Identifier& otherID) const { return id == otherID; }
		bool operator==(const Identifiable& other) const { return id == other.id; }
		juce::Identifier id;
	};

	/*
	* base class for everything that has a block
	*/
	struct Block {
		void setNumChannels(const int ch) { block.resize(ch); }
		void setBlockSize(const int b) {
			for(auto& ch: block)
				ch.resize(b, 0);
		}
		const float operator()(const int ch, const int s) const { return block[ch][s]; }
		float& operator()(const int ch, const int s) { return block[ch][s]; }
		void limit(const int numSamples) {
			for(auto& ch: block)
				for (auto s = 0; s < numSamples; ++s)
					if (ch[s] < 0.f) ch[s] = 0.f;
					else if (ch[s] > 1.f) ch[s] = 1.f;
		}
		float* data(const int ch) { return block[ch].data(); }
		const float* data(const int ch) const { return block[ch].data(); }
	protected:
		std::vector<std::vector<float>> block;
	};

	/*
	* smoothens a signal with a simple 1pole lp
	*/
	struct Lowpass {
		Lowpass() : e(0.f), cutoff(1.f) {}
		void setCutoff(float x) { cutoff = x; }
		void processBlock(Block& block, const float target, const int numSamples, const int ch) {
			for (auto s = 0; s < numSamples; ++s) {
				e += cutoff * (target - e);
				block(ch, s) = e;
			}
		}
	private:
		float e, cutoff;
	};

	/*
	* parameter smoothing by interpolating with a sine wave
	*/
	class Smoothing2 {
	public:
		Smoothing2() :
			env(0.f),
			startValue(0.f), endValue(0.f), rangeValue(0.f),
			idx(0.f), length(22050.f),
			isWorking(false)
		{}
		void setLength(const float samples) { length = samples; }
		void processBlock(Block& block, const float dest, const int numSamples, const int ch) {
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

		void setNewDestination(const float dest) {
			startValue = env;
			endValue = dest;
			rangeValue = endValue - startValue;
			idx = 0.f;
			isWorking = true;
		}
		void processWork(Block& block, const float dest, const int numSamples, const int ch) {
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
		void bypass(Block& block, const float dest, const int numSamples, const int ch) {
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
		void setNumChannels(const int ch) {
			block.setNumChannels(ch);
			if (sumValue.size() == ch) return;
			sumValue.clear();
			for(auto c = 0; c < ch; ++c)
				sumValue.push_back(std::make_shared<std::atomic<float>>(0.f));
		}
		void setSampleRate(double sampleRate) { Fs = static_cast<float>(sampleRate); }
		void setBlockSize(const int b) { block.setBlockSize(b); }
		void setSmoothingLengthInSamples(const float length) {
			smoothing.setLength(length);
		}
		// PROCESS
		void processBlock(const int numSamples) {
			const int numChannels = sumValue.size();
			const auto nextValue = rap.convertTo0to1(parameter->load());
			smoothing.processBlock(block, nextValue, numSamples, 0);
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block(ch, s) = block(0, s);
		}
		void storeSumValue(const int numChannels, const int lastSample) {
			for(auto ch = 0; ch < numChannels; ++ch)
				sumValue[ch]->store(block(ch, lastSample));
		}
		float& operator()(const int ch, const int s) { return block(ch, s); }
		void limit(const int numSamples) { block.limit(numSamples); }
		// GET NORMAL
		float getSumValue(const int ch) const { return sumValue[ch]->load(); }
		bool operator==(const Parameter& other) { return id == other.id; }
		const float& operator()(const int ch, const int s) const { return block(ch, s); }
		float* data(const int ch) { return block.data(ch); }
		const float* data(const int ch) const { return block.data(ch); }
		// GET CONVERTED
		float denormalized(const int ch, const int s) const { return rap.convertFrom0to1(block(ch, s)); }
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter& rap;
		std::vector<std::shared_ptr<std::atomic<float>>> sumValue;
		Block block;
		Smoothing2 smoothing;
		float Fs;
	};

	/*
	* a reference to a parameter and its gain
	*/
	struct Destination :
		public Identifiable
	{
		Destination(Parameter& p, float defaultAtten = 1.f) :
			Identifiable(p.id),
			parameter(p),
			attenuvertor(defaultAtten)
		{}
		void processBlock(const Block& block, const int numChannels, const int numSamples) {
			const auto g = attenuvertor.load();
			for(auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					parameter(ch, s) += block(ch, s) * g;
		}
		void setValue(float value) { attenuvertor.store(value); }
		float getValue() const { return attenuvertor.load(); }
	protected:
		Parameter& parameter;
		std::atomic<float> attenuvertor;
	};

	/*
	* a module that modulates based on input, then sends to parameter destinations
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
		virtual void setNumChannels(const int ch) {
			if (outValue.size() == ch) return;
			outValue.clear();
			for(auto c = 0; c < ch; ++c)
				outValue.push_back(std::make_shared<std::atomic<float>>(0.f));
		}
		virtual void setSampleRate(double sampleRate) { Fs = static_cast<float>(sampleRate); }
		void addDestination(Parameter& p, float atten = 1.f) {
			if (hasDestination(p.id)) return;
			destinations.push_back(std::make_shared<Destination>(
				p, atten
				));
		}
		void removeDestination(const Parameter& p) {
			for (auto d = 0; d < destinations.size(); ++d) {
				auto& destination = destinations[d];
				auto dest = destination.get();
				if (dest->id == p.id) {
					destinations.erase(destinations.begin() + d);
					return;
				}
			}
		}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			auto d = getDestination(pID);
			return d->get()->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) = 0;
		void processDestinations(Block& block, const int numChannels, const int numSamples) {
			for (auto& destination : destinations)
				destination.get()->processBlock(block, numChannels, numSamples);
		}
		void storeOutValue(const float lastSampleValue, const int ch) {
			outValue[ch]->store(lastSampleValue);
		}
		// GET
		bool hasDestination(const juce::Identifier& pID) const {
			const auto d = getDestination(pID);
			return d != nullptr;
		}
		const std::shared_ptr<Destination>* getDestination(const juce::Identifier& pID) const {
			for (auto& destination : destinations) {
				auto d = destination.get();
				if (d->id == pID)
					return &destination;
			}
			return nullptr;
		}
		float getAttenuvertor(const juce::Identifier& pID) const {
			auto d = getDestination(pID);
			return d->get()->getValue();
		}
		const std::vector<std::shared_ptr<Destination>>& getDestinations() const { return destinations; }
		float getOutValue(const int ch) const { return outValue[ch]->load(); }
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
			for(auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
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
		EnvelopeFollowerModulator(const juce::String& mID, const Parameter& atkParam, const Parameter& rlsParam, const Parameter& widthParam) :
			Modulator(mID),
			attackParameter(atkParam),
			releaseParameter(rlsParam),
			widthParameter(widthParam),
			env()
		{}
		// SET
		void setNumChannels(const int ch) {
			Modulator::setNumChannels(ch);
			env.resize(ch, 0.f);
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

		float makeAutoGain(const float atkSpeed, const float rlsSpeed) {
			//auto a = std::sqrt(atkSpeed);
			//auto r = std::sqrt(rlsSpeed);
			//return (a + r) / a;
			return 1.f + std::sqrt(rlsSpeed / atkSpeed);
		}

		inline void processWidth(Block& block, const int numChannels, const int numSamples, const int lastSample) {
			auto lastSampleValue = block(0, lastSample);
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
	struct LFOModulator :
		public Modulator
	{
		LFOModulator(const juce::String& mID, const Parameter& syncParam, const Parameter& rateParam, const Parameter& wdthParam, const param::MultiRange& ranges) :
			Modulator(mID),
			syncParameter(syncParam),
			rateParameter(rateParam),
			widthParameter(wdthParam),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			fsInv(0.f), phase(0.f)
		{}
		// SET
		void setNumChannels(const int ch) {
			Modulator::setNumChannels(ch);
			phase.resize(ch);
		}
		void setSampleRate(double sampleRate) override {
			if (Fs != sampleRate) {
				Modulator::setSampleRate(sampleRate);
				fsInv = 1.f / Fs;
			}
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			for (auto ch = 0; ch < numChannels; ++ch) {
				const auto rateValue = rateParameter(ch, lastSample);
				const bool isFree = syncParameter(ch, 0) < .5f;
				if (isFree) {
					const auto rate = multiRange(freeID).convertFrom0to1(rateValue);
					const auto inc = rate * fsInv;
					for (auto s = 0; s < numSamples; ++s) {
						phase[ch] += inc;
						if (phase[ch] >= 1.f)
							--phase[ch];
						block(ch, s) = phase[ch];
					}
				}
				else {
					const auto bpm = static_cast<float>(playHead.bpm);
					const auto bps = bpm / 60.f;
					const auto quarterNoteLengthInSamples = Fs / bps;
					const auto barLengthInSamples = quarterNoteLengthInSamples * 4.f;
					const auto rate = multiRange(syncID).convertFrom0to1(rateValue);
					const auto inc = 1.f / (barLengthInSamples * rate);
					const auto ppq = static_cast<float>(playHead.ppqPosition) * .25f / rate;
					const auto curPhase = (ppq - std::floor(ppq));
					phase[ch] = curPhase;
					for (auto s = 0; s < numSamples; ++s) {
						block(ch, s) = phase[ch];
						phase[ch] += inc;
						if (phase[ch] >= 1.f)
							--phase[ch];
					}
				}
			}

			processWidth(block, lastSample, numChannels, numSamples);
			for (auto ch = 0; ch < numChannels; ++ch)
				storeOutValue(block(ch, lastSample), ch);
		}
	protected:
		const Parameter& syncParameter;
		const Parameter& rateParameter;
		const Parameter& widthParameter;
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase;
		float fsInv;

		inline void processWidth(Block& block, const int lastSample, const int numChannels, const int numSamples) {
			const auto width = widthParameter.denormalized(0, lastSample) * .5f;
			if(width > 0.f)
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
	};

	/*
	* a random lfo modulator
	*/
	struct RandModulator :
		public Modulator
	{
		RandModulator(const juce::String& mID, const Parameter& syncParam, const Parameter& rateParam,
			const Parameter& depthParam, const Parameter& smoothParam, const Parameter& biasParam) :
			Modulator(mID),
			rateParameter(rateParam),
			depthParameter(depthParam),
			smoothParameter(smoothParam),
			biasParameter(biasParam),
			fsInv(0.f), randValue(0.f),
			rateIdx(0)
		{}
		void setSampleRate(double sampleRate) override {
			if (Fs != sampleRate) {
				Modulator::setSampleRate(sampleRate);
				fsInv = 1.f / Fs;
			}
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const auto rateInMs = rateParameter(0, lastSample);
			const auto rateLength = msInSamples(rateInMs, Fs);
			for (auto s = 0; s < numSamples; ++s) {
				++rateIdx;
				if (rateIdx >= rateLength) {
					rateIdx = 0;
					const auto sign = rand.nextFloat() < .5f ? -1.f : 1.f;
					randValue = sign * rand.nextFloat() * depthParameter(0, s);
				}
				block(0, s) = randValue;
			}
			storeOutValue(block(0, lastSample), 0);
		}
	protected:
		const Parameter& rateParameter;
		const Parameter& depthParameter;
		const Parameter& smoothParameter;
		const Parameter& biasParameter;
		juce::Random rand;
		float fsInv, randValue;
		int rateIdx;
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
		void setNumChannels(const int ch) {
			block.setNumChannels(ch);
			for (auto& p : parameters)
				p->setNumChannels(ch);
			for (auto& m : modulators)
				m->setNumChannels(ch);
		}
		void setSampleRate(double sampleRate) {
			for (auto& p : parameters) p.get()->setSampleRate(sampleRate);
			for (auto& m : modulators) m.get()->setSampleRate(sampleRate);
		}
		void setBlockSize(const int b) {
			block.setBlockSize(b);
			for (auto& p : parameters)
				p.get()->setBlockSize(b);
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
		void addMacroModulator(const juce::Identifier& pID) {
			const auto p = getParameter(pID)->get();
			modulators.push_back(std::make_shared<MacroModulator>(*p));
		}
		void addEnvelopeFollowerModulator(const juce::Identifier& atkPID, const juce::Identifier& rlsPID, const juce::Identifier& wdthPID, int idx) {
			const auto atkP = getParameter(atkPID)->get();
			const auto rlsP = getParameter(rlsPID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const juce::String idString("EnvFol" + idx);
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, *atkP, *rlsP, *wdthP));
		}
		void addLFOModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID, const juce::Identifier& wdthPID, const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID)->get();
			const auto rateP = getParameter(ratePID)->get();
			const auto wdthP = getParameter(wdthPID)->get();
			const juce::String idString("Phase" + idx);
			modulators.push_back(std::make_shared<LFOModulator>(idString, *syncP, *rateP, *wdthP, ranges));
		}
		// MODIFY / REPLACE
		void selectModulator(const juce::Identifier& mID) { selectedModulator = getModulator(mID)->get()->id; }
		void addDestination(const juce::Identifier& mID, const juce::Identifier& pID, const float atten = 1.f) {
			getModulator(mID)->get()->addDestination(*getParameter(pID)->get(), atten);
		}
		void removeDestination(const juce::Identifier& mID, const juce::Identifier& pID) {
			getModulator(mID)->get()->removeDestination(*getParameter(pID)->get());
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
		std::shared_ptr<Modulator>* getSelectedModulator() {
			return getModulator(selectedModulator);
		}
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

	/*
	* to do:
	* temposync
	*	bpm to beatlength etc
	* 
	* envelopeFollowerModulator
	*	if atk or rls cur smoothing: sample-based param-update	
	* 
	* lfoModulator
	*	free/sync >> sync playhead
	*	how to handle dynamic param ranges (wtf did i mean with this)
	* 
	* randModulator
	*	continue implementing what's there
	*	or add stuff to lfoMod
	*/
}