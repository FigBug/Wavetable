#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Panels.h"

//==============================================================================
class Editor : public juce::Component
{
public:
    Editor (WavetableAudioProcessor& proc_)
        : proc ( proc_ )
    {
        for (auto& o : oscillators) addAndMakeVisible (o);
        for (auto& l : lfos)        addAndMakeVisible (l);
        for (auto& l : lfoGraphs)   addAndMakeVisible (l);

        addAndMakeVisible (filter);
        addAndMakeVisible (fltADSR);
        addAndMakeVisible (mix);

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

    void setupCallbacks()
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

    void resized() override
    {
        auto rc = getLocalBounds();

        auto rcOsc = rc.removeFromTop (163);
        for (auto& o : oscillators) { o.setBounds (rcOsc.removeFromLeft (224)); rcOsc.removeFromLeft (1); };

        rc.removeFromTop (1);

        auto rcFlt = rc.removeFromTop (163);
        filter.setBounds (rcFlt.removeFromLeft (168));  rcFlt.removeFromLeft (1);
        fltADSR.setBounds (rcFlt.removeFromLeft (186));  rcFlt.removeFromLeft (1);
        mix.setBounds (rcFlt.removeFromLeft (187));         rcFlt.removeFromLeft (1);

        auto rcB1 = rc.removeFromTop (26);
        modHeader.setBounds (rcB1);

        auto rcMod = rc.removeFromTop (163);
        lfoBox.setBounds (rcMod);

        auto rcB2 = rc.removeFromTop (26);
        fxHeader.setBounds (rcB2);

        auto rcFX = rc.removeFromTop (163);
        effects.setBounds (rcFX);
    }

    WavetableAudioProcessor& proc;

    OscillatorBox oscillators[Cfg::numOSCs] { { "oscillator 1", proc, 0 }, { "oscillator 2", proc, 1 } };

    FilterBox filter                        { "filter 1", proc };

    FilterADSRArea fltADSR                  { proc };

    MixBox mix                              { "osc mix", proc };

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
};

//==============================================================================
class WavetableAudioProcessorEditor : public gin::ProcessorEditor
{
public:
    WavetableAudioProcessorEditor (WavetableAudioProcessor&);
    ~WavetableAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WavetableAudioProcessor& vaProc;

    Editor editor { vaProc };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
