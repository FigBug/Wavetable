#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Panels.h"

//==============================================================================
class Editor : public juce::Component
{
public:
    Editor (WavetableAudioProcessor& proc_);

    void setupCallbacks();
    void resized() override;

private:

    WavetableAudioProcessor& proc;

    OscillatorBox oscillators[Cfg::numOSCs] { { "oscillator 1", proc, 0 }, { "oscillator 2", proc, 1 } };
    NoiseBox noise                          { "noise", proc };
    SubBox sub                              { "sub", proc };

    FilterBox filter                        { "filter", proc };

    LFOBox lfos[Cfg::numLFOs]               { { "LFO 1", proc, 0 }, { "LFO 2", proc, 1 }, { "LFO 3", proc, 2 } };
    LFOArea lfoGraphs[Cfg::numLFOs]         { { proc, 0 }, { proc, 1 }, { proc, 2 } };

    GateBox gate { proc };
    ChorusBox chorus { proc };
    DistortBox distort { proc };
    DelayBox delay { proc };
    ReverbBox reverb { proc };
    ScopeArea scope { proc };

    gin::BoxArea lfoBox;
    gin::BoxArea effects;

    gin::Layout layout { *this };
};
