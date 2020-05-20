#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Cfg.h"

//==============================================================================
class CommonBox : public gin::PagedControlBox
{
public:
    CommonBox (gin::ProcessorEditor& e, WavetableAudioProcessor& proc_)
        : gin::PagedControlBox (e), proc (proc_)
    {
        auto& g = proc.globalParams;

        addPage ("Main", 3, 2);

        addControl (0, new gin::Knob (g.level), 0, 0);
        addControl (0, new gin::Switch (g.mono), 1, 0);
        addControl (0, new gin::Select (g.glideMode), 2, 0);

        addControl (0, v = new gin::Knob (g.voices), 0, 1);
        addControl (0, l = new gin::Switch (g.legato), 1, 1);
        addControl (0, s = new gin::Knob (g.glideRate), 2, 1);

        watchParam (g.mono);
        watchParam (g.glideMode);

        addPage ("Control", 1, 2);

        addControl (1, new gin::Switch (g.mpe), 0, 0);
    }

    void paramChanged() override
    {
        gin::PagedControlBox::paramChanged();

        auto& g = proc.globalParams;
        v->setEnabled (! g.mono->isOn());
        l->setEnabled (g.glideMode->getProcValue() != 0.0f);
        s->setEnabled (g.glideMode->getProcValue() != 0.0f);
    }

    WavetableAudioProcessor& proc;
    ParamComponentPtr v, l, s;
};

//==============================================================================
class OscillatorBox : public gin::PagedControlBox
{
public:
    OscillatorBox (gin::ProcessorEditor& e, WavetableAudioProcessor& proc_)
        : gin::PagedControlBox (e), proc (proc_)
    {
        for ( int i = 0; i < numElementsInArray (proc.wtParams); i++)
        {
            auto& wt = proc.wtParams[i];

            addPage ("WT " + String (i + 1), 3, 4);
            addPageEnable (i * 2, wt.enable);

            addControl (i * 2, new gin::Knob (wt.tune, true), 1, 0);
            addControl (i * 2, new gin::Knob (wt.finetune, true), 2, 0);
            addControl (i * 2, new gin::Knob (wt.pan, true), 0, 1);
            addControl (i * 2, new gin::Knob (wt.level), 1, 1);

            addPage ("Unison " + String (i + 1), 2, 4);
            addControl (i * 2 + 1, new gin::Knob (wt.voices), 0, 0);
            addControl (i * 2 + 1, trans[i] = new gin::Knob (wt.voicesTrns, true), 1, 0);
            addControl (i * 2 + 1, detune[i] = new gin::Knob (wt.detune), 0, 1);
            addControl (i * 2 + 1, spread[i] = new gin::Knob (wt.spread), 1, 1);

            watchParam (wt.voices);
        }

        for ( int i = 0; i < numElementsInArray (proc.oscParams); i++)
        {
            auto& osc = proc.oscParams[i];

            addPage ("OSC " + String (i + 1), 3, 4);
            addPageEnable (i * 2, osc.enable);

            addControl (Cfg::numWTs * 2 + i * 2, new gin::Select (osc.wave), 0, 0);
            addControl (Cfg::numWTs * 2 + i * 2, new gin::Knob (osc.tune, true), 1, 0);
            addControl (Cfg::numWTs * 2 + i * 2, new gin::Knob (osc.finetune, true), 2, 0);
            addControl (Cfg::numWTs * 2 + i * 2, new gin::Knob (osc.pan, true), 0, 1);
            addControl (Cfg::numWTs * 2 + i * 2, new gin::Knob (osc.level), 1, 1);
            addControl (Cfg::numWTs * 2 + i * 2, pw[Cfg::numWTs + i] = new gin::Knob (osc.pulsewidth), 2, 1);

            watchParam (osc.wave);

            addPage ("Unison " + String (i + 1), 2, 4);
            addControl (Cfg::numWTs * 2 + i * 2 + 1, new gin::Knob (osc.voices), 0, 0);
            addControl (Cfg::numWTs * 2 + i * 2 + 1, trans[Cfg::numWTs + i] = new gin::Knob (osc.voicesTrns, true), 1, 0);
            addControl (Cfg::numWTs * 2 + i * 2 + 1, detune[Cfg::numWTs + i] = new gin::Knob (osc.detune), 0, 1);
            addControl (Cfg::numWTs * 2 + i * 2 + 1, spread[Cfg::numWTs + i] = new gin::Knob (osc.spread), 1, 1);

            watchParam (osc.voices);
        }

        setPageOpen (0, true);
        setPageOpen (2, true);
        setPageOpen (4, true);
    }

    void paramChanged() override
    {
        gin::PagedControlBox::paramChanged();

        for ( int i = 0; i < numElementsInArray (proc.wtParams); i++)
        {
            auto& wt = proc.oscParams[i];

            trans[i]->setEnabled (wt.voices->getProcValue() > 1);
            detune[i]->setEnabled (wt.voices->getProcValue() > 1);
            spread[i]->setEnabled (wt.voices->getProcValue() > 1);
        }

        for ( int i = 0; i < numElementsInArray (proc.oscParams); i++)
        {
            auto& osc = proc.oscParams[i];
            pw[Cfg::numWTs + i]->setEnabled ((gin::Wave) int (osc.wave->getProcValue()) == gin::Wave::pulse);

            trans[Cfg::numWTs + i]->setEnabled (osc.voices->getProcValue() > 1);
            detune[Cfg::numWTs + i]->setEnabled (osc.voices->getProcValue() > 1);
            spread[Cfg::numWTs + i]->setEnabled (osc.voices->getProcValue() > 1);
        }
    }

    WavetableAudioProcessor& proc;
    ParamComponentPtr pw[Cfg::numOSCs + Cfg::numWTs], trans[Cfg::numOSCs + Cfg::numWTs],
					  detune[Cfg::numOSCs + Cfg::numWTs], spread[Cfg::numOSCs + Cfg::numWTs];
};

//==============================================================================
class FilterAmpBox : public gin::PagedControlBox
{
public:
    FilterAmpBox (gin::ProcessorEditor& e, WavetableAudioProcessor& proc_)
        : gin::PagedControlBox (e), proc (proc_)
    {
        for (int i = 0; i < numElementsInArray (proc.filterParams); i++)
        {
            auto& flt = proc.filterParams[i];

            addPage ("Filter " + String (i + 1), 8, 2);
            addBottomButton (i, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcFilter[i], true));
            addPageEnable (i, flt.enable);

            addControl (i, new gin::Select (flt.type), 0, 0);
            auto freq = new gin::Knob (flt.frequency);
            addControl (i, freq, 1, 0);
            addControl (i, new gin::Knob (flt.resonance), 2, 0);

            addControl (i, new gin::Knob (flt.keyTracking), 0, 1);
            addControl (i, new gin::Knob (flt.amount, true), 1, 1);
            addControl (i, v[i] = new gin::Knob (flt.velocityTracking), 2, 1);

            adsr[i] = new gin::ADSRComponent ();
            adsr[i]->setParams (flt.attack, flt.decay, flt.sustain, flt.release);
            addControl (i, adsr[i], 3, 0, 3, 2);

            addControl (i, a[i] = new gin::Knob (flt.attack), 6, 0);
            addControl (i, d[i] = new gin::Knob (flt.decay), 7, 0);
            addControl (i, s[i] = new gin::Knob (flt.sustain), 6, 1);
            addControl (i, r[i] = new gin::Knob (flt.release), 7, 1);

            watchParam (flt.amount);

            freq->setLiveValuesCallback ([this, i] ()
            {
                if (proc.filterParams[i].amount->getUserValue()      != 0.0f ||
                    proc.filterParams[i].keyTracking->getUserValue() != 0.0f ||
                    proc.modMatrix.isModulated (gin::ModDstId (proc.filterParams[i].frequency->getModIndex())))
                    return proc.getLiveFilterCutoff (i);
                return Array<float>();
            });

			int n = numElementsInArray (proc.filterParams);
			auto& env = proc.adsrParams;

			addPage ("ADSR", 8, 2);
			addControl (n, new gin::Knob (env.attack), 0, 0);
			addControl (n, new gin::Knob (env.decay), 1, 0);
			addControl (n, new gin::Knob (env.sustain), 0, 1);
			addControl (n, new gin::Knob (env.release), 1, 1);
			addControl (n, new gin::Knob (env.velocityTracking), 2, 0);

			auto g = new gin::ADSRComponent ();
			g->setParams (env.attack, env.decay, env.sustain, env.release);
			addControl (n, g, 3, 0, 3, 2);
        }
    }

    void paramChanged () override
    {
        gin::PagedControlBox::paramChanged ();

        for ( int i = 0; i < numElementsInArray (proc.filterParams); i++)
        {
            auto& flt = proc.filterParams[i];
            v[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
            a[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
            d[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
            s[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
            r[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
            adsr[i]->setEnabled (flt.amount->getUserValue() != 0.0f);
        }
    }

    WavetableAudioProcessor& proc;
    ParamComponentPtr v[Cfg::numFilters], a[Cfg::numFilters], d[Cfg::numFilters], s[Cfg::numFilters], r[Cfg::numFilters];
    gin::ADSRComponent* adsr[Cfg::numFilters];
};

//==============================================================================
class ModulationBox : public gin::PagedControlBox
{
public:
    ModulationBox (gin::ProcessorEditor& e, WavetableAudioProcessor& proc_)
        : gin::PagedControlBox (e), proc (proc_)
    {
        int cnt = 0;
        for (int i = 0; i < numElementsInArray (proc.lfoParams); i++, cnt++)
        {
            auto& lfo = proc.lfoParams[i];

            addPage ("LFO" + String (i + 1), 5, 2);
            addPageEnable (cnt, lfo.enable);
            addBottomButton (cnt, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcLFO[i], true));
            addBottomButton (cnt, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcMonoLFO[i], false));

            addControl (cnt, new gin::Select (lfo.wave), 0, 0);
            addControl (cnt, new gin::Switch (lfo.sync), 1, 0);
            addControl (cnt, r[i] = new gin::Knob (lfo.rate), 2, 0);
            addControl (cnt, b[i] = new gin::Select (lfo.beat), 2, 0);

            addControl (cnt, new gin::Knob (lfo.depth, true), 0, 1);
            addControl (cnt, new gin::Knob (lfo.phase, true), 1, 1);
            addControl (cnt, new gin::Knob (lfo.offset, true), 2, 1);
            addControl (cnt, new gin::Knob (lfo.fade, true), 3, 1);
            addControl (cnt, new gin::Knob (lfo.delay), 4, 1);

            auto l = new gin::LFOComponent();
            l->setParams (lfo.wave, lfo.sync, lfo.rate, lfo.beat, lfo.depth, lfo.offset, lfo.phase, lfo.enable);
            addControl (cnt, l, 3, 0, 2, 1);

            watchParam (lfo.sync);
        }

        for (int i = 0; i < numElementsInArray (proc.envParams); i++, cnt++)
        {
            auto& env = proc.envParams[i];

            addPage ("ENV" + String (i + 1), 5, 2);
            addPageEnable (cnt, env.enable);
            addBottomButton (cnt, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcEnv[i], true));

            addControl (cnt, new gin::Knob (env.attack), 0, 0);
            addControl (cnt, new gin::Knob (env.decay), 1, 0);
            addControl (cnt, new gin::Knob (env.sustain), 0, 1);
            addControl (cnt, new gin::Knob (env.release), 1, 1);

            auto adsr = new gin::ADSRComponent ();
            adsr->setParams (env.attack, env.decay, env.sustain, env.release);
            addControl (cnt, adsr, 2, 0, 3, 2);
        }

        auto& stp = proc.stepLfoParams;
        addPage ("Step", 6, 2);
        addPageEnable (cnt, stp.enable);
        addBottomButton (cnt, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcStep, true));
        addBottomButton (cnt, new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcMonoStep, false));

        addControl (cnt, new gin::Select (stp.beat), 0, 0);
        addControl (cnt, new gin::Knob (stp.length), 0, 1);

        auto s = new gin::StepLFOComponent();
        s->setParams (stp.beat, stp.length, stp.level, stp.enable);
        addControl (cnt, s, 1, 0, 5, 2);
        cnt++;

        addPage ("All", 5, 2);
        addControl (cnt, new gin::ModSrcListBox (proc.modMatrix), 0, 0, 5, 2);
        cnt++;

        addPage ("Mod Matrix", 5, 2);
        addControl (cnt, new gin::ModMatrixBox (proc, proc.modMatrix), 0, 0, 5, 2);
        cnt++;
    }

    void paramChanged () override
    {
        gin::PagedControlBox::paramChanged ();

        for (int i = 0; i < numElementsInArray (proc.lfoParams); i++)
        {
            auto& lfo = proc.lfoParams[i];
            r[i]->setVisible (! lfo.sync->isOn());
            b[i]->setVisible (lfo.sync->isOn());
        }
    }

    WavetableAudioProcessor& proc;
    ParamComponentPtr r[Cfg::numLFOs], b[Cfg::numLFOs];
};

//==============================================================================
class EffectsBox : public gin::PagedControlBox
{
public:
    EffectsBox (gin::ProcessorEditor& e, WavetableAudioProcessor& proc_)
        : gin::PagedControlBox (e), proc (proc_)
    {
        int idx = 0;

        addPage ("Gate", 8, 2);
        addPageEnable (idx, proc.gateParams.enable);
        addControl (idx, new gin::Select (proc.gateParams.beat), 0, 0);
        addControl (idx, new gin::Knob (proc.gateParams.length), 1, 0);
        addControl (idx, new gin::Knob (proc.gateParams.attack), 0, 1);
        addControl (idx, new gin::Knob (proc.gateParams.release), 1, 1);
        auto g = new gin::GateEffectComponent ();
        g->setParams (proc.gateParams.length, proc.gateParams.l, proc.gateParams.r, proc.gateParams.enable);
        addControl (idx, g, 2, 0, 6, 2);
        idx++;

        addPage ("Chorus", 3, 2);
        addPageEnable (idx, proc.chorusParams.enable);
        addControl (idx, new gin::Knob (proc.chorusParams.delay), 0, 0);
        addControl (idx, new gin::Knob (proc.chorusParams.rate), 1, 0);
        addControl (idx, new gin::Knob (proc.chorusParams.depth), 2, 0);
        addControl (idx, new gin::Knob (proc.chorusParams.width), 0, 1);
        addControl (idx, new gin::Knob (proc.chorusParams.mix), 1, 1);
        idx++;

        addPage ("Distortion", 2, 2);
        addPageEnable (idx, proc.distortionParams.enable);
        addControl (idx, new gin::Knob (proc.distortionParams.amount), 0, 0);
        addControl (idx, new gin::Knob (proc.distortionParams.highpass), 1, 0);
        addControl (idx, new gin::Knob (proc.distortionParams.output), 0, 1);
        addControl (idx, new gin::Knob (proc.distortionParams.mix), 1, 1);
        idx++;

        addPage ("EQ", 6, 2);
        addPageEnable (idx, proc.eqParams.enable);
        addControl (idx, new gin::Knob (proc.eqParams.loFreq), 0, 0);
        addControl (idx, new gin::Knob (proc.eqParams.loGain), 1, 0);
        addControl (idx, new gin::Knob (proc.eqParams.loQ), 2, 0);
        addControl (idx, new gin::Knob (proc.eqParams.mid1Freq), 3, 0);
        addControl (idx, new gin::Knob (proc.eqParams.mid1Gain), 4, 0);
        addControl (idx, new gin::Knob (proc.eqParams.mid1Q), 5, 0);
        addControl (idx, new gin::Knob (proc.eqParams.mid2Freq), 0, 1);
        addControl (idx, new gin::Knob (proc.eqParams.mid2Gain), 1, 1);
        addControl (idx, new gin::Knob (proc.eqParams.mid2Q), 2, 1);
        addControl (idx, new gin::Knob (proc.eqParams.hiFreq), 3, 1);
        addControl (idx, new gin::Knob (proc.eqParams.hiGain), 4, 1);
        addControl (idx, new gin::Knob (proc.eqParams.hiQ), 5, 1);
        idx++;

        addPage ("Comp", 3, 2);
        addPageEnable (idx, proc.compressorParams.enable);
        addControl (idx, new gin::Knob (proc.compressorParams.attack), 0, 0);
        addControl (idx, new gin::Knob (proc.compressorParams.release), 1, 0);
        addControl (idx, new gin::Knob (proc.compressorParams.ratio), 2, 0);
        addControl (idx, new gin::Knob (proc.compressorParams.threshold), 0, 1);
        addControl (idx, new gin::Knob (proc.compressorParams.gain), 1, 1);
        idx++;

        addPage ("Delay", 3, 2);
        addPageEnable (idx, proc.delayParams.enable);
        addControl (idx, new gin::Switch (proc.delayParams.sync), 0, 0);
        addControl (idx, t = new gin::Knob (proc.delayParams.time), 1, 0);
        addControl (idx, b = new gin::Select (proc.delayParams.beat), 1, 0);
        addControl (idx, new gin::Knob (proc.delayParams.fb), 2, 0);
        addControl (idx, new gin::Knob (proc.delayParams.cf), 1, 1);
        addControl (idx, new gin::Knob (proc.delayParams.mix), 2, 1);
        idx++;

        watchParam (proc.delayParams.sync);

        addPage ("Reverb", 3, 2);
        addPageEnable (idx, proc.reverbParams.enable);
        addControl (idx, new gin::Knob (proc.reverbParams.damping), 0, 0);
        addControl (idx, new gin::Knob (proc.reverbParams.freezeMode), 1, 0);
        addControl (idx, new gin::Knob (proc.reverbParams.roomSize), 2, 0);
        addControl (idx, new gin::Knob (proc.reverbParams.width), 0, 1);
        addControl (idx, new gin::Knob (proc.reverbParams.mix), 1, 1);
        idx++;

        addPage ("Limiter", 2, 2);
        addPageEnable (idx, proc.limiterParams.enable);
        addControl (idx, new gin::Knob (proc.limiterParams.attack), 0, 0);
        addControl (idx, new gin::Knob (proc.limiterParams.release), 1, 0);
        addControl (idx, new gin::Knob (proc.limiterParams.threshold), 0, 1);
        addControl (idx, new gin::Knob (proc.limiterParams.gain), 1, 1);
        idx++;

        addPage ("Scope", 8, 2);
        auto scope = new gin::TriggeredScope (proc.fifo);
        scope->setNumChannels (2);
        scope->setTriggerMode (gin::TriggeredScope::TriggerMode::Up);
        scope->setColour (gin::TriggeredScope::lineColourId, Colours::transparentBlack);
        addControl (idx, scope, 0, 0, 8, 2);
        idx++;

        setPageOpen (0, false);
    }

    void paramChanged () override
    {
        gin::PagedControlBox::paramChanged ();

        t->setVisible (! proc.delayParams.sync->isOn());
        b->setVisible (proc.delayParams.sync->isOn());
    }

    WavetableAudioProcessor& proc;
    ParamComponentPtr t, b;
};
