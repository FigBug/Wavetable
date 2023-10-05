#include "Editor.h"

Editor::Editor (WavetableAudioProcessor& proc_)
    : proc ( proc_ )
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
    addAndMakeVisible (gate);
    addAndMakeVisible (chorus);
    addAndMakeVisible (distort);
    addAndMakeVisible (delay);
    addAndMakeVisible (reverb);

    setupCallbacks();
}

void Editor::setupCallbacks()
{
}

void Editor::resized()
{
   #if JUCE_DEBUG
    auto f = juce::File (__FILE__).getChildFile ("../../Resources/layout.json");

    layout.setLayout ("layout.json", f);
   #else
    layout.setLayout ("layout.json");
   #endif
}
