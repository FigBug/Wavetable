#include "Editor.h"

Editor::Editor (WavetableAudioProcessor& proc_)
    : proc (proc_)
{
    for (auto& o : oscillators) addAndMakeVisible (o);
    for (auto& l : lfos)        addAndMakeVisible (l);
    for (auto& e : envs)        addAndMakeVisible (e);

    addAndMakeVisible (sub);
    addAndMakeVisible (noise);
    addAndMakeVisible (adsr);
    addAndMakeVisible (filter);
    addAndMakeVisible (step);
    addAndMakeVisible (mod);
    addAndMakeVisible (mtx);
    addAndMakeVisible (global);

    fx.addAndMakeVisible (gate);
    fx.addAndMakeVisible (chorus);
    fx.addAndMakeVisible (distort);
    fx.addAndMakeVisible (delay);
    fx.addAndMakeVisible (reverb);

    fx.onDragStart = [] (const juce::MouseEvent& e)
    {
        if (dynamic_cast<gin::ParamHeader*> (e.originalComponent))
            return true;
        return false;
    };
    fx.onOrderChanged = [this] (int, int)
    {
        proc.fxParams.fx1->setUserValue (fx.getChildComponent(0)->getProperties()["fxId"]);
        proc.fxParams.fx2->setUserValue (fx.getChildComponent(1)->getProperties()["fxId"]);
        proc.fxParams.fx3->setUserValue (fx.getChildComponent(2)->getProperties()["fxId"]);
        proc.fxParams.fx4->setUserValue (fx.getChildComponent(3)->getProperties()["fxId"]);
        proc.fxParams.fx5->setUserValue (fx.getChildComponent(4)->getProperties()["fxId"]);

        handleAsyncUpdate();
    };

    proc.fxParams.fx1->addListener (this);
    proc.fxParams.fx2->addListener (this);
    proc.fxParams.fx3->addListener (this);
    proc.fxParams.fx4->addListener (this);
    proc.fxParams.fx5->addListener (this);

    addAndMakeVisible (fx);

    setupCallbacks();
    handleAsyncUpdate();
}

Editor::~Editor()
{
    proc.fxParams.fx1->removeListener (this);
    proc.fxParams.fx2->removeListener (this);
    proc.fxParams.fx3->removeListener (this);
    proc.fxParams.fx4->removeListener (this);
    proc.fxParams.fx5->removeListener (this);
}

void Editor::handleAsyncUpdate()
{
    if (fx.isDragInProgress())
        return;
    
    fx.removeAllChildren();

    auto getComp = [this] (int fxId) -> juce::Component*
    {
        if (fxId == fxGate)     return &gate;
        if (fxId == fxChorus)   return &chorus;
        if (fxId == fxDistort)  return &distort;
        if (fxId == fxDelay)    return &delay;
        if (fxId == fxReverb)   return &reverb;

        jassertfalse;
        return nullptr;
    };

    fx.addAndMakeVisible (getComp (proc.fxParams.fx1->getUserValueInt()));
    fx.addAndMakeVisible (getComp (proc.fxParams.fx2->getUserValueInt()));
    fx.addAndMakeVisible (getComp (proc.fxParams.fx3->getUserValueInt()));
    fx.addAndMakeVisible (getComp (proc.fxParams.fx4->getUserValueInt()));
    fx.addAndMakeVisible (getComp (proc.fxParams.fx5->getUserValueInt()));

    if (fx.getWidth() > 0)
        fx.resized();
}

void Editor::setupCallbacks()
{
}

void Editor::resized()
{
   #if JUCE_DEBUG
    auto f = juce::File (__FILE__).getChildFile ("../../Resources/layout.json");

    layout.setLayout ({f});
   #else
    layout.setLayout ({"layout.json"});
   #endif
    
    handleAsyncUpdate();
}
