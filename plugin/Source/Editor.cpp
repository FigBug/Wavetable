#include "Editor.h"

Editor::Editor (WavetableAudioProcessor& proc_)
    : proc ( proc_ )
{
    for (auto& o : oscillators) addAndMakeVisible (o);
    for (auto& l : lfos)        addAndMakeVisible (l);
    for (auto& l : lfoGraphs)   addAndMakeVisible (l);

    addAndMakeVisible (sub);
    addAndMakeVisible (noise);
    addAndMakeVisible (filter);

    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        addAndMakeVisible (&lfos[i]);
        addAndMakeVisible (&lfoGraphs[i]);
    }

    addAndMakeVisible (gate);
    addAndMakeVisible (chorus);
    addAndMakeVisible (distort);
    addAndMakeVisible (delay);
    addAndMakeVisible (reverb);
    addAndMakeVisible (scope);

    addAndMakeVisible (effects);

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
