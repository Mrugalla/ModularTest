#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
	static float msInSamples(float ms, float Fs) { return ms * Fs * .001f; }

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
		void setBlockSize(const int b) { block.resize(b, 0); }
		const float operator[](int i) const { return block[i]; }
		float& operator[](int i) { return block[i]; }
		void limit(const int numSamples) {
			for (auto s = 0; s < numSamples; ++s)
				if (block[s] < 0.f) block[s] = 0.f;
				else if (block[s] > 1.f) block[s] = 1.f;
		}
		float* data() { return block.data(); }
		const float* data() const { return block.data(); }
	protected:
		std::vector<float> block;
	};

	/*
	* smoothens a signal with a simple 1pole lp
	*/
	struct Lowpass {
		Lowpass() : e(0.f), cutoff(1.f) {}
		void setCutoff(float x) { cutoff = x; }
		void processBlock(Block& block, const float target, const int numSamples) {
			for (auto s = 0; s < numSamples; ++s) {
				e += cutoff * (target - e);
				block[s] = e;
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
		void processBlock(Block& block, const float dest, const int numSamples) {
			if (!isWorking) {
				if (env == dest)
					return bypass(block, dest, numSamples);
				setNewDestination(dest);
			}
			else if (length == 0)
				return bypass(block, dest, numSamples);
			processWork(block, dest, numSamples);
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
		void processWork(Block& block, const float dest, const int numSamples) {
			for (auto s = 0; s < numSamples; ++s) {
				if (idx >= length) {
					if (dest != endValue)
						setNewDestination(dest);
					else {
						for (auto s1 = s; s1 < numSamples; ++s1)
							block[s1] = endValue;
						isWorking = false;
						return;
					}
				}
				const auto curve = .5f - std::cos(idx * pi / length) * .5f;
				block[s] = env = startValue + curve * rangeValue;
				++idx;
			}
		}
		void bypass(Block& block, const float dest, const int numSamples) {
			for (auto s = 0; s < numSamples; ++s)
				block[s] = dest;
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
			sumValue(0),
			block(),
			smoothing(),
			Fs(1.f)
		{}
		// SET
		void setSampleRate(double sampleRate) { Fs = static_cast<float>(sampleRate); }
		void setBlockSize(const int b) { block.setBlockSize(b); }
		void setSmoothingLengthInSamples(const float length) { smoothing.setLength(length); }
		// PROCESS
		void processBlock(const int numSamples) {
			const auto nextValue = rap.convertTo0to1(parameter->load());
			smoothing.processBlock(block, nextValue, numSamples);
		}
		void storeSumValue(const int numSamples) { sumValue.store(block[numSamples - 1]); }
		float& operator[](const float i) { return block[i]; }
		void limit(const int numSamples) { block.limit(numSamples); }
		// GET NORMAL
		float getSumValue() const { return sumValue.load(); }
		bool operator==(const Parameter& other) { return id == other.id; }
		const float& operator[](const float i) const { return block[i]; }
		float* data() { return block.data(); }
		const float* data() const { return block.data(); }
		// GET CONVERTED
		float denormalized(const int s) const { return rap.convertFrom0to1(block[s]); }
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter& rap;
		std::atomic<float> sumValue;
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
		void processBlock(const Block& block, const int numSamples) {
			const auto g = attenuvertor.load();
			for (auto s = 0; s < numSamples; ++s)
				parameter[s] += block[s] * g;
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
			outValue(0.f),
			destinations()
		{}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
			outValue(0.f),
			destinations()
		{}
		// SET
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
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block) = 0;
		void processDestinations(Block& block, const int numSamples) {
			for (auto& destination : destinations)
				destination.get()->processBlock(block, numSamples);
		}
		void storeOutValue(const Block& block, const int numSamples){ outValue.store(block[numSamples - 1]); }
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
		float getOutValue() const { return outValue.load(); }
	protected:
		std::vector<std::shared_ptr<Destination>> destinations;
		std::atomic<float> outValue;
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
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block) override {
			const auto numSamples = audioBuffer.getNumSamples();
			for (auto s = 0; s < numSamples; ++s)
				block[s] = parameter[s];
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
		EnvelopeFollowerModulator(const juce::String& mID, const Parameter& atkParam, const Parameter& rlsParam) :
			Modulator(mID),
			attackParameter(atkParam),
			releaseParameter(rlsParam),
			env(0.f)
		{
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block) override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			getSamples(audioBuffer, block);
			const auto atkInMs = attackParameter.denormalized(lastSample);
			const auto rlsInMs = releaseParameter.denormalized(lastSample);
			const auto atkInSamples = msInSamples(atkInMs, Fs);
			const auto rlsInSamples = msInSamples(rlsInMs, Fs);
			const auto atkSpeed = 1.f / atkInSamples;
			const auto rlsSpeed = 1.f / rlsInSamples;
			const auto gain = makeAutoGain(atkSpeed, rlsSpeed);
			for (auto s = 0; s < numSamples; ++s) {
				if (env < block[s])
					env += atkSpeed * (block[s] - env);
				else if (env > block[s])
					env += rlsSpeed * (block[s] - env);
				block[s] = env * gain;
			}
			storeOutValue(block, numSamples);
		}
	protected:
		const Parameter& attackParameter;
		const Parameter& releaseParameter;
		float env;
	private:
		inline void getSamples(const juce::AudioBuffer<float>& audioBuffer, Block& block) {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto numChannels = audioBuffer.getNumChannels();
			const auto samples = audioBuffer.getArrayOfReadPointers();
			const auto chInv = 1.f / audioBuffer.getNumChannels();
			for (auto s = 0; s < numSamples; ++s)
				block[s] = samples[0][s];
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block[s] += samples[ch][s];
			for (auto s = 0; s < numSamples; ++s)
				block[s] = std::abs(block[s] * chInv);
		}

		const float makeAutoGain(const float atkSpeed, const float rlsSpeed) {
			auto a = std::sqrt(atkSpeed);
			auto r = std::sqrt(rlsSpeed);
			return (a + r) / a;
		}
	};

	/*
	* a phase modulator, not tested yet
	*/
	struct PhaseModulator :
		public Modulator
	{
		PhaseModulator(const juce::String& mID, const Parameter& syncParam, const Parameter& rateParam, const juce::NormalisableRange<float>& free) :
			Modulator(mID),
			syncParameter(syncParam),
			rateParameter(rateParam),
			freeRange(free),
			fsInv(0.f), phase(0.f)
		{}
		// SET
		void setSampleRate(double sampleRate) override {
			if (Fs != sampleRate) {
				Modulator::setSampleRate(sampleRate);
				fsInv = 1.f / Fs;
			}
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block) override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const auto rate = syncParameter[0] < .5f ?
				freeRange.convertFrom0to1(rateParameter[lastSample]) :
				rateParameter[lastSample]; // put beat sync here
			const auto inc = rate * fsInv;
			for (auto s = 0; s < numSamples; ++s) {
				phase += inc;
				if (phase >= 1.f)
					--phase;
				block[s] = phase;
			}
			storeOutValue(block, numSamples);
		}
	protected:
		const Parameter& syncParameter;
		const Parameter& rateParameter;
		const juce::NormalisableRange<float> freeRange;
		float fsInv, phase;
	};

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
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, Block& block) override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const auto rateInMs = rateParameter[lastSample];
			const auto rateLength = msInSamples(rateInMs, Fs);
			for (auto s = 0; s < numSamples; ++s) {
				++rateIdx;
				if (rateIdx >= rateLength) {
					rateIdx = 0;
					const auto sign = rand.nextFloat() < .5f ? -1.f : 1.f;
					randValue = sign * rand.nextFloat() * depthParameter[s];
				}
				block[s] = randValue;
			}

			storeOutValue(block, numSamples);
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
			block(other->block),
			selectedModulator(other->selectedModulator)
		{}
		// SET
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
		void addEnvelopeFollowerModulator(const juce::Identifier& atkPID, const juce::Identifier& rlsPID, int envFolIdx) {
			const auto atkP = getParameter(atkPID)->get();
			const auto rlsP = getParameter(rlsPID)->get();
			const juce::String idString("EnvFol" + envFolIdx);
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, *atkP, *rlsP));
		}
		void addPhaseModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID, const juce::NormalisableRange<float> freeRange, int phaseIdx) {
			const auto syncP = getParameter(syncPID)->get();
			const auto rateP = getParameter(ratePID)->get();
			const juce::String idString("Phase" + phaseIdx);
			modulators.push_back(std::make_shared<PhaseModulator>(idString, *syncP, *rateP, freeRange));
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
		void processBlock(const juce::AudioBuffer<float>& audioBuffer) {
			const auto numSamples = audioBuffer.getNumSamples();
			for (auto& p : parameters) p.get()->processBlock(numSamples);
			for (auto& m : modulators) {
				m->processBlock(audioBuffer, block);
				m->processDestinations(block, numSamples);
			}
			for (auto& parameter : parameters) {
				auto p = parameter.get();
				p->limit(numSamples);
				p->storeSumValue(numSamples);
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
		Block block;
		juce::Identifier selectedModulator;
	};

	/*
	* to do:
	* envelopeFollowerModulator
	*	if atk or rls cur smoothing: sample-based param-update	
	* 
	*	implement phase modulator
	*		how to handle dynamic param ranges
	* 
	* RandModulator
	*	continue implementing what's there
	*	solution to missing width param
	*/
}