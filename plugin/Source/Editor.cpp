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
    addAndMakeVisible (fltADSR);

    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        addAndMakeVisible (&lfos[i]);
        addAndMakeVisible (&lfoGraphs[i]);
    }

    addAndMakeVisible (gate);
    addAndMakeVisible (pattern);
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
    // LFO mod items
    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        modItems[i].onClick = [this, i]
        {
            modItems[0].setSelected (i == 0);
            modItems[1].setSelected (i == 1);
            modItems[2].setSelected (i == 2);

            lfoBox.setPage (i);
        };
    }
    modItems[0].onClick();
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
