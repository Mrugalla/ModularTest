#pragma once
#include <JuceHeader.h>

namespace modSys2Editor {
	/*
	* A basic plugin parameter
	* can be assigned to a modulator to select it on mouseDown
	*/
	class Parameter :
		public juce::Component,
		public modSys2::Identifiable
	{
		struct SelectedModulatorGainDragger :
			public juce::Component
		{
			SelectedModulatorGainDragger(ModularTestAudioProcessor& p, Parameter& param) :
				processor(p),
				parameter(param),
				dragStartValue(0.f)
			{}
		protected:
			ModularTestAudioProcessor& processor;
			Parameter& parameter;
			float dragStartValue;

			void paint(juce::Graphics& g) override {
				g.setColour(juce::Colours::limegreen);
				auto matrix = processor.matrix.getUpdatedPtr();
				const auto dest = matrix->getSelectedModulator()->getDestination(parameter.id);
				if (dest == nullptr) return;
				const juce::String txt = dest->isBidirectional() ? "Mb" : "M";
				g.drawFittedText(txt, getLocalBounds(), juce::Justification::centred, 1);
				g.drawEllipse(getLocalBounds().toFloat(), 1);
			}
			void mouseDown(const juce::MouseEvent&) override {
				auto matrix = processor.matrix.getUpdatedPtr();
				const auto slcm = matrix->getSelectedModulator();
				auto atten = slcm->getAttenuvertor(parameter.id);
				dragStartValue = atten;
			}
			void mouseDrag(const juce::MouseEvent& evt) override {
				const auto matrix = processor.matrix.getUpdatedPtr();
				const auto distance = evt.getDistanceFromDragStartY();
				const auto v = -distance / static_cast<float>(getHeight());
				const auto speed = evt.mods.isShiftDown() ? .01f : .1f;
				const auto value = juce::jlimit(-1.f, 1.f, dragStartValue + v * speed);
				auto s = matrix->getSelectedModulator();
				s->setAttenuvertor(matrix->getParameter(parameter.id)->id, value);
			}
			void mouseUp(const juce::MouseEvent& evt) override {
				if (evt.mouseWasDraggedSinceMouseDown()) return;
				else if (evt.mods.isRightButtonDown()) {
					auto matrix = processor.matrix.getCopyOfUpdatedPtr();
					const auto m = matrix->getSelectedModulator();
					matrix->removeDestination(m->id, parameter.id);
					processor.matrix.replaceUpdatedPtrWith(matrix);
				}
				else {
					auto dest = processor.matrix->getSelectedModulator()->getDestination(parameter.id);
					dest->setBirectional(!dest->isBidirectional());
				}
			}

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectedModulatorGainDragger)
		};
	public:
		Parameter(ModularTestAudioProcessor& p, const juce::String& pID, const int numChannels, const juce::String& linkedModID = "") :
			processor(p),
			modSys2::Identifiable(pID),
			parameter(*p.apvts.getParameter(id)),
			attach(parameter, [this](float) { repaint(); }, nullptr),
			linkedModulatorID(),
			modGainDragger(p, *this),
			sumValue(0), curModSumValue(0)
		{
			if(linkedModID.isNotEmpty()) linkedModulatorID = linkedModID;
			addChildComponent(modGainDragger);
			attach.sendInitialUpdate();
		}
		void timerCallback(const std::shared_ptr<modSys2::Matrix>& matrix) {
			const auto value = parameter.getValue();
			const auto selectedMod = matrix.get()->getSelectedModulator();
			auto cmsv = value;
			if (selectedMod != nullptr) {
				auto sm = selectedMod;
				if (sm->hasDestination(id)) {
					modGainDragger.setVisible(true);
					const auto atten = sm->getAttenuvertor(id);
					cmsv += atten;
				}
				else modGainDragger.setVisible(false);
			}
			bool needRepaint = false;
			const auto sv = matrix->getParameter(id)->getSumValue();
			if (curModSumValue != cmsv || sumValue != sv) {
				curModSumValue = cmsv;
				sumValue = sv;
				needRepaint = true;
			}
			if(needRepaint) repaint();
		}
	protected:
		ModularTestAudioProcessor& processor;
		juce::RangedAudioParameter& parameter;
		juce::ParameterAttachment attach;
		juce::Identifier linkedModulatorID;
		SelectedModulatorGainDragger modGainDragger;
		float sumValue, curModSumValue;

		void mouseDown(const juce::MouseEvent&) override { selectLinkedModulator(); }

		void selectLinkedModulator() {
			if (linkedModulatorID.isValid()) {
				const auto matrix = processor.matrix.getUpdatedPtr();
				matrix.get()->selectModulator(linkedModulatorID);
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameter)
	};

	/*
	* a basic plugin parameter with some look and feel
	*/
	struct ParameterExample :
		public Parameter
	{
		ParameterExample(ModularTestAudioProcessor& p, const juce::String& pID, const int numChannels, const juce::String& linkedModID = "") :
			Parameter(p, pID, numChannels, linkedModID),
			dragStartValue(0)
		{ }
	protected:
		float dragStartValue;

		void paint(juce::Graphics& g) override {
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			g.setColour(juce::Colours::darkslateblue);
			g.drawRect(0.f, 0.f, width, height);

			const auto value = parameter.getValue();
			const auto x = value * width;
			juce::Rectangle<float> valueArea(0.f, 0.f, x, height);
			g.fillRect(valueArea);
			
			const auto sumX = static_cast<int>(curModSumValue * width);
			auto startX = x;
			auto rangeX = sumX - x;
			if (rangeX < 0) {
				startX += rangeX;
				rangeX *= -1.f;
			}
			g.setColour(juce::Colours::rebeccapurple);
			juce::Rectangle<float> rangeArea(startX, 0.f, rangeX, height);
			g.fillRect(rangeArea);
			
			g.setColour(juce::Colours::blue.brighter(.4f));
			g.drawVerticalLine(sumX, 0.f, height);
			g.drawVerticalLine(sumX - 1, 0.f, height);
			g.drawVerticalLine(sumX + 1, 0.f, height);
			const auto xInt = static_cast<int>(x);
			g.drawVerticalLine(xInt - 1, 0.f, height);
			g.drawVerticalLine(xInt, 0.f, height);
			g.drawVerticalLine(xInt + 1, 0.f, height);

			const auto sumValueX = static_cast<int>(sumValue * width);
			g.setColour(juce::Colours::purple);
			g.drawVerticalLine(sumValueX - 1, 0.f, height);
			g.drawVerticalLine(sumValueX, 0.f, height);
			g.drawVerticalLine(sumValueX + 1, 0.f, height);

			const auto actualValue = parameter.getCurrentValueAsText();
			const auto valueStr = parameter.getName(64) + ":\n" + actualValue + "\nsum: " + juce::String(sumValue).substring(0, 4);
			g.setColour(juce::Colours::white);
			g.drawFittedText(valueStr, getLocalBounds(), juce::Justification::centredTop, 2);
		}
		void resized() override { modGainDragger.setBounds(0, 0, getWidth() / 6, getHeight() / 6); }
		void mouseDown(const juce::MouseEvent& evt) override {
			Parameter::mouseDown(evt);
			dragStartValue = parameter.getValue();
			attach.beginGesture();
		}
		void mouseDrag(const juce::MouseEvent& evt) override {
			const auto distance = evt.getDistanceFromDragStartX();
			const auto v = distance / static_cast<float>(getWidth());
			const auto value = juce::jlimit(0.f, 1.f, dragStartValue + v);
			attach.setValueAsPartOfGesture(parameter.convertFrom0to1(value));
		}
		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) {}
			else {
				const auto value = evt.position.x / static_cast<float>(getWidth());
				attach.setValueAsPartOfGesture(parameter.convertFrom0to1(value));
			}
			attach.endGesture();
		}

		void selectLinkedModulator() {
			if (linkedModulatorID.isValid()) {
				const auto matrix = processor.matrix.getUpdatedPtr();
				matrix.get()->selectModulator(linkedModulatorID);
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterExample)
	};

	/*
	* component used to select a modulator and/or drag it to a destination
	*/
	struct ModulatorDragger :
		public juce::Component,
		public modSys2::Identifiable
	{
		ModulatorDragger(ModularTestAudioProcessor& p, const juce::String& mID, std::vector<Parameter*>& modulatables) :
			Identifiable(mID),
			processor(p),
			draggerfall(),
			bounds(),
			modulatableParameters(modulatables),
			hoveredParameter(nullptr),
			selected(false)
		{}
		void setQBounds(juce::Rectangle<int> b) {
			bounds = b;
			setBounds(bounds);
		}
		void timerCallback(const std::shared_ptr<modSys2::Matrix>& matrix) {
			auto s = matrix.get()->getModulator(id) == matrix.get()->getSelectedModulator();
			if (selected != s) {
				selected = s;
				repaint();
			}
		}
	protected:
		ModularTestAudioProcessor& processor;
		juce::ComponentDragger draggerfall;
		juce::Rectangle<int> bounds;
		std::vector<Parameter*>& modulatableParameters;
		Parameter* hoveredParameter;
		bool selected;

		void mouseDown(const juce::MouseEvent& evt) override {
			const auto matrix = processor.matrix.getUpdatedPtr();
			matrix.get()->selectModulator(id);
			selected = true;
			draggerfall.startDraggingComponent(this, evt);
		}
		void mouseDrag(const juce::MouseEvent& evt) override {
			draggerfall.dragComponent(this, evt, nullptr);
			hoveredParameter = getHoveredParameter();
		}
		void mouseUp(const juce::MouseEvent&) override {
			if (hoveredParameter != nullptr) {
				auto matrix = processor.matrix.getCopyOfUpdatedPtr();
				const auto m = matrix->getModulator(id);
				const auto p = matrix->getParameter(hoveredParameter->id);
				const auto pValue = processor.apvts.getRawParameterValue(p->id);
				const auto atten = 1.f - *pValue;
				matrix->addDestination(m->id, p->id, modSys2::ChannelSetup::Left, atten, false);
				processor.matrix.replaceUpdatedPtrWith(matrix);
				hoveredParameter = nullptr;
			}
			setBounds(bounds);
			repaint();
		}
		void paint(juce::Graphics& g) override {
			if (hoveredParameter == nullptr)
				if (selected)
					g.setColour(juce::Colours::rebeccapurple);
				else
					g.setColour(juce::Colours::darkslateblue);
			else
				g.setColour(juce::Colours::blue.brighter(.4f));
			
			const auto tBounds = getLocalBounds().toFloat();
			g.drawRoundedRectangle(tBounds, 2, 2);
			juce::Point<float> centre(tBounds.getX() + tBounds.getWidth() * .5f, tBounds.getY() + tBounds.getHeight() * .5f);
			const auto arrowHead = tBounds.getWidth() * .25f;
			g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getBottom() }), 2, arrowHead, arrowHead);
			g.drawArrow(juce::Line<float>(centre, { tBounds.getX(), centre.y }), 2, arrowHead, arrowHead);
			g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getY() }), 2, arrowHead, arrowHead);
			g.drawArrow(juce::Line<float>(centre, { tBounds.getRight(), centre.y }), 2, arrowHead, arrowHead);
		}

		Parameter* getHoveredParameter() const {
			for (const auto& p: modulatableParameters)
				if (!getBounds().getIntersection(p->getBounds()).isEmpty())
					return p;
			return nullptr;
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorDragger)
	};

	/*
	* displays envelope follower data
	*/
	struct EnvelopeFollowerDisplay :
		public juce::Component,
		public modSys2::Identifiable
	{
		EnvelopeFollowerDisplay(int envFolIdx, const int numChannels) :
			juce::Component(),
			modSys2::Identifiable(juce::String("EnvFol" + juce::String(envFolIdx))),
			curValue()
		{ curValue.resize(numChannels, 0.f); }
		void timerCallback(const std::shared_ptr<modSys2::Matrix>& matrix) {
			bool needRepaint = false;
			const auto mod = matrix->getModulator(id);
			for (auto ch = 0; ch < curValue.size(); ++ch) {
				const auto newValue = mod->getOutValue(ch);
				if (curValue[ch] != newValue) {
					curValue[ch] = newValue;
					needRepaint = true;
				}
			}
			if(needRepaint) repaint();
		}
	protected:
		std::vector<float> curValue;
		
		void paint(juce::Graphics& g) override {
			const auto height = static_cast<float>(getHeight());
			const auto width = static_cast<float>(getWidth());
			g.setColour(juce::Colours::rebeccapurple);
			g.drawRect(getLocalBounds());
			for (auto ch = 0; ch < curValue.size(); ++ch) {
				const auto y = static_cast<int>(height - curValue[ch] * height);
				g.drawHorizontalLine(y, 0.f, width);
				g.drawHorizontalLine(y - 1, 0.f, width);
				g.drawHorizontalLine(y + 1, 0.f, width);
			}
		}
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollowerDisplay)
	};

	/*
	* display for lfo & rand mod data
	*/
	struct LFODisplay :
		public juce::Component,
		public modSys2::Identifiable
	{
		LFODisplay(const juce::String& mID, const int numChannels) :
			juce::Component(),
			modSys2::Identifiable(mID),
			img(juce::Image::RGB, 1, 1, true),
			curValue()
		{ curValue.resize(numChannels, 0.f); }
		void timerCallback(const std::shared_ptr<modSys2::Matrix>& matrix) {
			bool needRepaint = false;
			for (auto ch = 0; ch < curValue.size(); ++ch) {
				const auto mod = matrix->getModulator(id);
				auto newValue = mod->getOutValue(ch);
				if (curValue[ch] != newValue) {
					curValue[ch] = newValue;
					needRepaint = true;
				}
			}
			if (needRepaint) repaint();
		}
	protected:
		juce::Image img;
		std::vector<float> curValue;
		void resized() override { img = img.rescaled(getWidth(), getHeight(), juce::Graphics::lowResamplingQuality).createCopy(); }

		void paint(juce::Graphics& g) override {
			juce::Graphics g0{ img };
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			g0.fillAll(juce::Colour(0x77000000));
			g0.setColour(juce::Colours::rebeccapurple);
			g0.drawRect(getLocalBounds());
			for (auto ch = 0; ch < curValue.size(); ++ch) {
				const auto x = curValue[ch] * width;
				const auto y = height - curValue[ch] * height;
				juce::Rectangle<float> valuePt(x - 1, y - 1, 3, 3);
				g0.fillEllipse(valuePt);
			}
			g.drawImageAt(img, 0, 0, false);
		}
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFODisplay)
	};
}