
// PLUGIN EDITOR.CPP
/*

#include "PluginProcessor.h"
#include "PluginEditor.h"

ModularTestAudioProcessorEditor::ModularTestAudioProcessorEditor (ModularTestAudioProcessor& p)
    : AudioProcessorEditor (&p),
    audioProcessor (p),
    modulatorsGroup(), modulatableGroup(),

    macrosLabel("Macro", "Macro"),
    macro0(p, param::ID::Macro0, &modulatorsGroup),
    macro1(p, param::ID::Macro1, &modulatorsGroup),
    macro2(p, param::ID::Macro2, &modulatorsGroup),
    macro3(p, param::ID::Macro3, &modulatorsGroup),
    macro0Dragger(macro0, modulatorsGroup, modulatableGroup),
    macro1Dragger(macro1, modulatorsGroup, modulatableGroup),
    macro2Dragger(macro2, modulatorsGroup, modulatableGroup),
    macro3Dragger(macro3, modulatorsGroup, modulatableGroup),

    globalsLabel("Depth &\nModulesMix", "Depth &\nModulesMix"),
    depth(p, param::ID::Depth, audioProcessor.depth, nullptr),
    modulesMix(p, param::ID::ModulesMix, audioProcessor.modulesMix, nullptr)
{
    addAndMakeVisible(macrosLabel);
    macrosLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(macro0);
    addAndMakeVisible(macro1);
    addAndMakeVisible(macro2);
    addAndMakeVisible(macro3);
    modulatorsGroup.add(&macro0); modulatorsGroup.add(&macro1);
    modulatorsGroup.add(&macro2); modulatorsGroup.add(&macro3);
    modulatorsGroup.select(&macro1);

    addAndMakeVisible(globalsLabel);
    globalsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(depth);
    addAndMakeVisible(modulesMix);
    modulatableGroup.add(&depth); modulatableGroup.add(&modulesMix);

    addAndMakeVisible(macro0Dragger);
    addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger);
    addAndMakeVisible(macro3Dragger);

    setResizable(true, true);
    setSize (700, 300);
}

//==============================================================================
void ModularTestAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
}

void ModularTestAudioProcessorEditor::resized() {
    auto x = 0.f;
    auto y = 0.f;
    auto height = (float)getHeight() / 5.f;
    const auto width = getWidth() / 4.f;
    const auto knobWidth = width / 2.f;
    macrosLabel.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, width, height)).toNearestInt());
    y += height;
    macro0.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).toNearestInt());
    y += height;
    macro1.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).toNearestInt());
    y += height;
    macro2.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).toNearestInt());
    y += height;
    macro3.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).toNearestInt());

    x += knobWidth;
    y = height;
    macro0Dragger.setQBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro1Dragger.setQBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro2Dragger.setQBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());
    y += height;
    macro3Dragger.setQBounds(makeMinQuad(juce::Rectangle<float>(x, y, knobWidth, height)).reduced(10).toNearestInt());

    x += knobWidth;
    y = 0.f;

    height = (float)getHeight() / 3.f;
    globalsLabel.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, width, height)).toNearestInt());
    y += height;
    depth.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, width, height)).toNearestInt());
    y += height;
    modulesMix.setBounds(makeMinQuad(juce::Rectangle<float>(x, y, width, height)).toNearestInt());
}


*/

// PLUGIN EDITOR.H
/*
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>

static juce::Rectangle<float> makeMinQuad(const juce::Rectangle<float>& b) {
    const auto minDimen = std::min(b.getWidth(), b.getHeight());
    const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
    const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
    return { x, y, minDimen, minDimen };
}

struct Parameter :
    public juce::Component
{
    Parameter(ModularTestAudioProcessor& p, param::ID id) :
        selected(true),
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float) { repaint(); }, nullptr)
    {}
    bool selected;
protected:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
};

struct ParameterGroup {
    const size_t size() const { return parameters.size(); }
    void add(Parameter* p) { parameters.push_back(p); }
    void select(Parameter* p) {
        selected = p;
        for (auto& par : parameters)
            par->selected = par == selected;
        selected->getParentComponent()->repaint();
    }
    bool isSelected(Parameter* p) const { return p != nullptr && p == selected; }
    bool overlapsWith(const juce::Component* c) const {
        for (const auto& p : parameters)
            if (!c->getBounds().getIntersection(p->getBounds()).isEmpty()) return true;
        return false;
    }
private:
    std::vector<Parameter*> parameters;
    Parameter* selected;
};

struct Dial :
    public juce::Component
{
    Dial(std::function<float()> getValueF, std::function<bool()> isSelectedF) :
        centre(0, 0),
        lastDistance(0), radius(0),
        getValueFunc(getValueF),
        isSelectedFunc(isSelectedF),
        mouseDownFunc(), mouseUpFunc(), mouseDragFunc(), mouseWheelFunc()
    {}
protected:
    juce::Point<float> centre;
    float lastDistance, radius;
    std::function<float()> getValueFunc;
    std::function<bool()> isSelectedFunc;
    std::function<void(const juce::MouseEvent&)> mouseDownFunc, mouseUpFunc, mouseDragFunc, mouseWheelFunc;

    void resized() override {
        const auto width = (float)getWidth();
        radius = width * .5f;
        centre.setXY(radius, radius);
    }
    void paint(juce::Graphics& g) override {
        auto thicc = 2.f;
        const auto bounds = getLocalBounds().toFloat().reduced(thicc);
        const auto tickLength = radius - thicc * 2;
        juce::Line<float> tick(0, 0, 0, tickLength);
        const auto normalValue = getValueFunc();
        const auto translation = juce::AffineTransform::translation(centre);
        const auto startAngle = 3.14 * .25f;
        const auto angleLength = 3.14 * 2 - startAngle;
        const auto angleRange = angleLength - startAngle;
        if (isSelectedFunc()) g.setColour(juce::Colour(0xff9944ff));
        else g.setColour(juce::Colours::darkgrey);
        g.drawEllipse(bounds, thicc);
        thicc = normalValue + thicc;
        const auto tickCountInv = 1.f / 16.f;
        for (auto x = 0.f; x <= 1.f; x += tickCountInv) {
            auto tickT = tick;
            const auto angle = startAngle + (x * normalValue) * angleRange;
            const auto rotation = juce::AffineTransform::rotation(angle);
            tickT.applyTransform(rotation.followedBy(translation));
            g.drawLine(tickT, thicc);
        }
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        lastDistance = 0;
        mouseDownFunc(evt);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        mouseUpFunc(evt);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto height = static_cast<float>(getHeight());
        const auto dist = static_cast<float>(-evt.getDistanceFromDragStartY()) / height;
        const auto dRate = dist - lastDistance;
        mouseDragFunc(evt);
        lastDistance = dist;
        repaint();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (evt.mods.isAnyMouseButtonDown()) return;
        const auto speed = evt.mods.isShiftDown() ? .005f : .05f;
        const auto gestr = wheel.deltaY > 0 ? speed : -speed;
        mouseWheelFunc(evt);
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dial)
};

struct DialParameter :
    public Parameter
{
    DialParameter(ModularTestAudioProcessor& p, param::ID id, ParameterGroup* parameterGroup = nullptr) :
        Parameter(p, id),
        group(parameterGroup),
        centre(0,0),
        lastDistance(0), radius(0)
    {}
protected:
    ParameterGroup* group;
    juce::Point<float> centre;
    float lastDistance, radius;

    void resized() override {
        const auto width = (float)getWidth();
        radius = width * .5f;
        centre.setXY(radius, radius);
    }
    void paint(juce::Graphics& g) override {
        auto thicc = 2.f;
        const auto bounds = getLocalBounds().toFloat().reduced(thicc);
        const auto tickLength = radius - thicc * 2;
        juce::Line<float> tick(0, 0, 0, tickLength);
        const auto normalValue = param->getValue();
        const auto translation = juce::AffineTransform::translation(centre);
        const auto startAngle = 3.14 * .25f;
        const auto angleLength = 3.14 * 2 - startAngle;
        const auto angleRange = angleLength - startAngle;
        if(selected) g.setColour(juce::Colour(0xff9944ff));
        else g.setColour(juce::Colours::darkgrey);
        g.drawEllipse(bounds, thicc);
        thicc = normalValue + thicc;
        const auto tickCountInv = 1.f / 16.f;
        for (auto x = 0.f; x <= 1.f; x += tickCountInv) {
            auto tickT = tick;
            const auto angle = startAngle + (x * normalValue) * angleRange;
            const auto rotation = juce::AffineTransform::rotation(angle);
            tickT.applyTransform(rotation.followedBy(translation));
            g.drawLine(tickT, thicc);
        }
    }
    void mouseDown(const juce::MouseEvent&) override {
        attach.beginGesture();
        lastDistance = 0;
        if (group != nullptr)
            group->select(this);
    }
    void mouseUp(const juce::MouseEvent&) override {
        attach.endGesture();
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto height = static_cast<float>(getHeight());
        const auto dist = static_cast<float>(-evt.getDistanceFromDragStartY()) / height;
        const auto dRate = dist - lastDistance;
        const auto speed = evt.mods.isShiftDown() ? .05f : 1.f;
        const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dRate * speed);
        attach.setValueAsPartOfGesture(param->convertFrom0to1(newValue));
        lastDistance = dist;
        repaint();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (evt.mods.isAnyMouseButtonDown()) return;
        const auto speed = evt.mods.isShiftDown() ? .005f : .05f;
        const auto gestr = wheel.deltaY > 0 ? speed : -speed;
        attach.setValueAsCompleteGesture(
            param->convertFrom0to1(param->getValue() + gestr)
        );
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DialParameter)
};

struct DialHittestRadial :
    public DialParameter
{
    DialHittestRadial(ModularTestAudioProcessor& p, param::ID id, ParameterGroup* parameterGroup = nullptr) :
        DialParameter(p, id, parameterGroup)
    {}
private:
    bool hitTest(int x, int y) override {
        juce::Line<float> distFromCentre(centre, juce::Point<float>(x, y));
        return distFromCentre.getLength() < radius;
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DialHittestRadial)
};

struct DialModulatable :
    public Parameter
{
    DialModulatable(ModularTestAudioProcessor& p, param::ID id, mod::ModulatableParameter& mp, ParameterGroup* parameterGroup = nullptr) :
        Parameter(p, id),
        modulatable(mp),
        modulatorDial(
            [&]() { return modulatable.getValue(); },
            [&]() { return false; }
        ),
        dial(p, id, parameterGroup)
    {
        addAndMakeVisible(modulatorDial);
        addAndMakeVisible(dial);
    }
protected:
    mod::ModulatableParameter& modulatable;
    Dial modulatorDial;
    DialHittestRadial dial;

    void resized() override {
        modulatorDial.setBounds(getLocalBounds());
        dial.setBounds(getLocalBounds().toFloat().reduced(20).toNearestInt());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DialModulatable)
};

struct MacroDragger :
    public juce::Component
{
    MacroDragger(DialParameter& mak, ParameterGroup& modulators, ParameterGroup& modulatable) :
        macro(mak),
        modulatorsGroup(modulators),
        modulatableGroup(modulatable),
        dragger(),
        bounds(),
        overlappingWithModulatable(false)
    {}
    void setQBounds(juce::Rectangle<int> b) {
        bounds = b;
        setBounds(bounds);
    }
protected:
    DialParameter& macro;
    ParameterGroup& modulatorsGroup;
    ParameterGroup& modulatableGroup;
    juce::ComponentDragger dragger;
    juce::Rectangle<int> bounds;
    bool overlappingWithModulatable;

    void mouseDown(const juce::MouseEvent& evt) override {
        modulatorsGroup.select(&macro);
        dragger.startDraggingComponent(this, evt);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        overlappingWithModulatable = modulatableGroup.overlapsWith(this);
        dragger.dragComponent(this, evt, nullptr);
    }
    void mouseUp(const juce::MouseEvent&) override {
        setBounds(bounds);
        overlappingWithModulatable = false;
    }

    void paint(juce::Graphics& g) override {
        if (macro.selected)
            if(overlappingWithModulatable) g.setColour(juce::Colour(0xff44ff99));
            else g.setColour(juce::Colour(0xff9944ff));
        else g.setColour(juce::Colours::darkgrey);

        const auto bounds = getLocalBounds().toFloat().reduced(2);
        g.drawRoundedRectangle(bounds, 2, 2);
        juce::Point<float> centre(bounds.getX() + bounds.getWidth() * .5f, bounds.getY() + bounds.getHeight() * .5f);
        const auto arrowHead = bounds.getWidth() * .25f;
        g.drawArrow(juce::Line<float>(centre, { centre.x, bounds.getBottom() }), 2, arrowHead, arrowHead);
        g.drawArrow(juce::Line<float>(centre, { bounds.getX(), centre.y }), 2, arrowHead, arrowHead);
        g.drawArrow(juce::Line<float>(centre, { centre.x, bounds.getY() }), 2, arrowHead, arrowHead);
        g.drawArrow(juce::Line<float>(centre, { bounds.getRight(), centre.y }), 2, arrowHead, arrowHead);
    }
};

struct ButtonChoiceNonParam :
    public juce::Component
{
    ButtonChoiceNonParam(ModularTestAudioProcessor& p, param::ID id) :
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float) { repaint(); })
    {}
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    void paint(juce::Graphics& g) override {
        const auto thicc = 2.f;
        const auto bounds = getLocalBounds().toFloat().reduced(thicc);
        g.setColour(juce::Colour(0xff9944ff));
        g.drawRoundedRectangle(bounds, thicc, thicc);
        g.drawFittedText(param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return;
        attach.setValueAsCompleteGesture(1.f - param->getValue());
        repaint();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceNonParam)
};

struct ModularTestAudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    ModularTestAudioProcessorEditor (ModularTestAudioProcessor&);
protected:
    ModularTestAudioProcessor& audioProcessor;
    ParameterGroup modulatorsGroup, modulatableGroup;

    juce::Label macrosLabel;
    DialParameter macro0, macro1, macro2, macro3;
    MacroDragger macro0Dragger, macro1Dragger, macro2Dragger, macro3Dragger;

    juce::Label globalsLabel;
    DialModulatable depth, modulesMix;

    juce::Label modulesLabel;
    // 2 buttons for module choice
    // module 0 component
    // module 1 component

    void paint(juce::Graphics&) override;
    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModularTestAudioProcessorEditor)
};
*/