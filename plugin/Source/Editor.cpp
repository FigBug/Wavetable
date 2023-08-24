#include "Editor.h"

Editor::Editor (WavetableAudioProcessor& proc_)
    : proc ( proc_ )
{
    for (auto& o : oscillators) addAndMakeVisible (o);
    for (auto& l : lfos)        addAndMakeVisible (l);
    for (auto& l : lfoGraphs)   addAndMakeVisible (l);

    addAndMakeVisible (filter);
    addAndMakeVisible (fltADSR);

    for (auto& i : modItems)
        modHeader.addItem (i);

    addAndMakeVisible (modHeader);

    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        lfoBox.addBox (i, &lfos[i]);
        lfoBox.addBox (i, &lfoGraphs[i]);
    }
    addAndMakeVisible (lfoBox);
    lfoBox.setPage (0);

    for (auto& i : fxItems)
        fxHeader.addItem (i);

    addAndMakeVisible (fxHeader);

    effects.addBox (&gate);
    effects.addBox (&pattern);
    effects.addBox (&chorus);
    effects.addBox (&distort);
    effects.addBox (&delay);
    effects.addBox (&reverb);
    effects.addBox (&scope);

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
