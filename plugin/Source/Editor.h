#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Panels.h"

//==============================================================================
class Editor : public juce::Component,
               private gin::Parameter::ParameterListener,
               private juce::AsyncUpdater
{
public:
    Editor (WavetableAudioProcessor& proc_);
    ~Editor() override;

    void setupCallbacks();
    void resized() override;

private:
    void valueUpdated (gin::Parameter*) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;

    WavetableAudioProcessor& proc;

    OscillatorBox oscillators[Cfg::numOSCs] { { "oscillator 1", proc, 0 }, { "oscillator 2", proc, 1 } };
    NoiseBox noise                          { "noise", proc };
    SubBox sub                              { "sub", proc };

    ADSRBox adsr                            { "adsr", proc };
    FilterBox filter                        { "filter", proc };

    LFOBox lfos[Cfg::numLFOs]               { { "LFO 1", proc, 0 }, { "LFO 2", proc, 1 }, { "LFO 3", proc, 2 } };
    ENVBox envs[Cfg::numENVs]               { { "ENV 1", proc, 0 }, { "ENV 2", proc, 1 }, { "ENV 3", proc, 2 } };
    StepBox step                            { "step", proc };
    ModBox mod                              { "mod", proc };
    MatrixBox mtx                           { "mtx", proc };
    GlobalBox global                        { "global", proc };

    GateBox gate { proc };
    ChorusBox chorus { proc };
    DistortBox distort { proc };
    DelayBox delay { proc };
    ReverbBox reverb { proc };

    gin::ComponentGrid fx { "fx" };

    gin::Layout layout { *this };
};
