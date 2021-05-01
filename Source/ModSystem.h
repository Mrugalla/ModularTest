#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>

namespace modSys2 {
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
	* a parameter, its block and a lowpass filter
	*/
	struct Parameter :
		public Identifiable
	{
		Parameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID) :
			Identifiable(pID),
			parameter(apvts.getRawParameterValue(pID)),
			sumValue(0),
			block(),
			smoothing()
		{}
		// SET
		void setBlockSize(const int b) { block.setBlockSize(b); }
		// PROCESS
		void processBlock(const int numSamples) {
			const auto value = parameter->load();
			smoothing.processBlock(block, value, numSamples);
		}
		void storeSumValue(const int numSamples) { sumValue.store(block[numSamples - 1]); }
		float& operator[](const float i) { return block[i]; }
		void limit(const int numSamples) { block.limit(numSamples); }
		// GET
		float getSumValue() const { return sumValue.load(); }
		bool operator==(const Parameter& other) { return id == other.id; }
		const float& operator[](const float i) const { return block[i]; }
		float* data() { return block.data(); }
		const float* data() const { return block.data(); }
	protected:
		std::atomic<float>* parameter;
		std::atomic<float> sumValue;
		Block block;
		Lowpass smoothing;
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
			destinations()
		{}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
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
	protected:
		std::vector<std::shared_ptr<Destination>> destinations;
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
	* a phase modulator
	*/
	struct PhaseModulator :
		public Modulator
	{
		PhaseModulator(const juce::String& mID, const Parameter& rateParam) :
			Modulator(mID),
			rateParameter(rateParam),
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
			for (auto s = 0; s < numSamples; ++s) {
				const auto inc = rateParameter[s] * fsInv;
				phase += inc;
				if (phase >= 1.f) --phase;
				block[s] = phase;
			}
		}
	protected:
		const Parameter& rateParameter;
		float fsInv, phase;
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
		void setSampleRate(double sampleRate) { for (auto& m : modulators) m.get()->setSampleRate(sampleRate); }
		void setBlockSize(const int b) {
			block.setBlockSize(b);
			for (auto& p : parameters)
				p.get()->setBlockSize(b);
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
	* make repository of this thing
	* implement modular value strings for parameters
	*/
}