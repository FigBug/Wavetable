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

    FilterBox filter                        { "filter", proc };

    FilterADSRArea fltADSR                  { proc };

    LFOBox lfos[Cfg::numLFOs]               { { "LFO 1", proc, 0 }, { "LFO 2", proc, 1 }, { "LFO 3", proc, 2 } };
    LFOArea lfoGraphs[Cfg::numLFOs]         { { proc, 0 }, { proc, 1 }, { proc, 2 } };

    gin::HeaderItem modItems[8]             { { "LFO 1", proc.lfoParams[0].enable,  proc.modMatrix, proc.modSrcMonoLFO[0], proc.modSrcLFO[0] },
                                              { "LFO 2", proc.lfoParams[1].enable,  proc.modMatrix, proc.modSrcMonoLFO[1], proc.modSrcLFO[1] },
                                              { "LFO 3", proc.lfoParams[2].enable,  proc.modMatrix, proc.modSrcMonoLFO[2], proc.modSrcLFO[2] },
                                              { "ENV 1", proc.envParams[0].enable,  proc.modMatrix, {}, proc.modSrcEnv[0] },
                                              { "ENV 2", proc.envParams[1].enable,  proc.modMatrix, {}, proc.modSrcEnv[1] },
                                              { "ENV 3", proc.envParams[2].enable,  proc.modMatrix, {}, proc.modSrcEnv[2] },
                                              { "STEP",  proc.stepLfoParams.enable, proc.modMatrix, proc.modSrcMonoStep, proc.modSrcStep },
                                              { "MIDI",  nullptr } };
    gin::HeaderRow modHeader;

    gin::HeaderItem fxItems[5]              { { "GATE",     proc.gateParams.enable       },
                                              { "CHORUS",   proc.chorusParams.enable     },
                                              { "DISTORT",  proc.distortionParams.enable },
                                              { "DELAY",    proc.delayParams.enable      },
                                              { "REVERB",   proc.reverbParams.enable     } };

    gin::HeaderRow fxHeader;


    GateBox gate { proc };
    GateArea pattern { proc };
    ChorusBox chorus { proc };
    DistortBox distort { proc };
    DelayBox delay { proc };
    ReverbBox reverb { proc };
    ScopeArea scope { proc };

    gin::BoxArea lfoBox;
    gin::BoxArea effects;

    gin::Layout layout { *this };
};
