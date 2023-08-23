#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Cfg.h"

//==============================================================================
class OscillatorBox : public gin::ParamBox
{
public:
    OscillatorBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        auto& osc = proc.oscParams[idx];

        addEnable (osc.enable);

        addControl (new gin::Select (osc.wave), 0, 0);
        addControl (new gin::Knob (osc.tune, true), 1, 0);
        addControl (new gin::Select (osc.voices), 2, 0);
        addControl (detune = new gin::Knob (osc.detune), 3, 0);

        addControl (pw = new gin::Knob (osc.pulsewidth), 0, 1);
        addControl (new gin::Knob (osc.finetune, true), 1, 1);
        addControl (spread = new gin::Knob (osc.spread), 2, 1);
        addControl (trans = new gin::Knob (osc.voicesTrns, true), 3, 1);

        watchParam (osc.wave);
        watchParam (osc.voices);
    }

    void paramChanged() override
    {
        gin::ParamBox::paramChanged();

        auto& osc = proc.oscParams[idx];
        pw->setEnabled ((gin::Wave) int (osc.wave->getProcValue()) == gin::Wave::pulse);

        trans->setEnabled (osc.voices->getProcValue() > 1);
        detune->setEnabled (osc.voices->getProcValue() > 1);
        spread->setEnabled (osc.voices->getProcValue() > 1);
    }

    WavetableAudioProcessor& proc;
    int idx = 0;
    gin::ParamComponent::Ptr pw, trans, detune, spread;
};

//==============================================================================
class FilterBox : public gin::ParamBox
{
public:
    FilterBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        auto& flt = proc.filterParams[idx];

        addEnable (flt.enable);

        auto freq = new gin::Knob (flt.frequency);
        addControl (freq, 0, 0);
        addControl (new gin::Knob (flt.resonance), 1, 0);
        addControl (new gin::Knob (flt.amount, true), 2, 0);

        addControl (new gin::Knob (flt.keyTracking), 0, 1);
        addControl (new gin::Select (flt.type), 1, 1);
        addControl (v = new gin::Knob (flt.velocityTracking), 2, 1);

        freq->setLiveValuesCallback ([this] ()
                                     {
            if (proc.filterParams[idx].amount->getUserValue()      != 0.0f ||
                proc.filterParams[idx].keyTracking->getUserValue() != 0.0f ||
                proc.modMatrix.isModulated (gin::ModDstId (proc.filterParams[idx].frequency->getModIndex())))
                return proc.getLiveFilterCutoff (idx);
            return juce::Array<float>();
        });
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged ();

        auto& flt = proc.filterParams[idx];
        v->setEnabled (flt.amount->getUserValue() != 0.0f);
    }

    WavetableAudioProcessor& proc;
    int idx = 0;
    gin::ParamComponent::Ptr v;
};

//==============================================================================
class FilterADSRArea : public gin::ParamArea
{
public:
    FilterADSRArea (WavetableAudioProcessor& proc_, int idx_)
        : proc (proc_), idx (idx_)
    {
        auto& flt = proc.filterParams[idx];

        adsr = new gin::ADSRComponent ();
        adsr->setParams (flt.attack, flt.decay, flt.sustain, flt.release);
        addControl (adsr);

        addControl (a = new gin::Knob (flt.attack));
        addControl (d = new gin::Knob (flt.decay));
        addControl (s = new gin::Knob (flt.sustain));
        addControl (r = new gin::Knob (flt.release));

        watchParam (flt.amount);
    }

    void paramChanged() override
    {
        gin::ParamArea::paramChanged ();

        auto& flt = proc.filterParams[idx];
        a->setEnabled (flt.amount->getUserValue() != 0.0f);
        d->setEnabled (flt.amount->getUserValue() != 0.0f);
        s->setEnabled (flt.amount->getUserValue() != 0.0f);
        r->setEnabled (flt.amount->getUserValue() != 0.0f);
        adsr->setEnabled (flt.amount->getUserValue() != 0.0f);
    }

    WavetableAudioProcessor& proc;
    int idx;
    gin::ParamComponent::Ptr a, d, s, r;
    gin::ADSRComponent* adsr;
};

//==============================================================================
class MixBox : public gin::ParamBox
{
public:
    MixBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
    }

    void paramChanged () override
    {
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class LFOBox : public gin::ParamBox
{
public:
    LFOBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        auto& lfo = proc.lfoParams[idx];

        addEnable (lfo.enable);

        addControl (r = new gin::Knob (lfo.rate), 0, 0);
        addControl (b = new gin::Select (lfo.beat), 0, 0);
        addControl (new gin::Knob (lfo.depth, true), 1, 0);
        addControl (new gin::Knob (lfo.fade, true), 0, 1);
        addControl (new gin::Knob (lfo.delay), 1, 1);

        watchParam (lfo.sync);

        setSize (112, 163);
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged ();

        auto& lfo = proc.lfoParams[idx];
        r->setVisible (! lfo.sync->isOn());
        b->setVisible (lfo.sync->isOn());
    }

    WavetableAudioProcessor& proc;
    int idx;
    gin::ParamComponent::Ptr r, b;
};

//==============================================================================
class LFOArea : public gin::ParamArea
{
public:
    LFOArea (WavetableAudioProcessor& proc_, int idx_)
        : proc (proc_), idx (idx_)
    {
        auto& lfo = proc.lfoParams[idx];

        addControl (new gin::Select (lfo.wave));
        addControl (new gin::Switch (lfo.sync));
        addControl (new gin::Knob (lfo.phase, true));
        addControl (new gin::Knob (lfo.offset, true));

        auto l = new gin::LFOComponent();
        l->setParams (lfo.wave, lfo.sync, lfo.rate, lfo.beat, lfo.depth, lfo.offset, lfo.phase, lfo.enable);
        addControl (l);

        setSize (186, 163);
    }

    void paramChanged () override
    {
        gin::ParamArea::paramChanged ();
    }

    WavetableAudioProcessor& proc;
    int idx;
};

//==============================================================================
class GateBox : public gin::ParamBox
{
public:
    GateBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Gate"), proc (proc_)
    {
        addControl (new gin::Select (proc.gateParams.beat), 0, 0);
        addControl (new gin::Knob (proc.gateParams.length), 1, 0);
        addControl (new gin::Knob (proc.gateParams.attack), 0, 1);
        addControl (new gin::Knob (proc.gateParams.release), 1, 1);

        setSize (112, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class GateArea : public gin::ParamArea
{
public:
    GateArea (WavetableAudioProcessor& proc_)
        : gin::ParamArea ("Pattern"), proc (proc_)
    {
        g = new gin::GateEffectComponent();
        g->setParams (proc.gateParams.length, proc.gateParams.l, proc.gateParams.r, proc.gateParams.enable);
        addControl (g);

        setSize (272, 163);
    }

    void paramChanged () override
    {
        gin::ParamArea::paramChanged ();
    }

    void resized() override
    {
        g->setBounds (getLocalBounds().withSizeKeepingCentre (getWidth(), 128));
    }

    WavetableAudioProcessor& proc;
    gin::GateEffectComponent* g;
};

//==============================================================================
class ChorusBox : public gin::ParamBox
{
public:
    ChorusBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Chorus"), proc (proc_)
    {
        addControl (new gin::Knob (proc.chorusParams.delay), 0, 0);
        addControl (new gin::Knob (proc.chorusParams.rate), 1, 0);
        addControl (new gin::Knob (proc.chorusParams.mix), 2, 0);

        addControl (new gin::Knob (proc.chorusParams.depth), 0.5f, 1.0f);
        addControl (new gin::Knob (proc.chorusParams.width), 1.5f, 1.0f);

        setSize (168, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class DistortBox : public gin::ParamBox
{
public:
    DistortBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Distort"), proc (proc_)
    {
        addControl (new gin::Knob (proc.distortionParams.amount), 0, 0);
        addControl (new gin::Knob (proc.distortionParams.highpass), 1, 0);
        addControl (new gin::Knob (proc.distortionParams.output), 0, 1);
        addControl (new gin::Knob (proc.distortionParams.mix), 1, 1);

        setSize (112, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class EQBox : public gin::ParamBox
{
public:
    EQBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("EQ"), proc (proc_)
    {
        addControl (new gin::Knob (proc.eqParams.loFreq), 0, 0);
        addControl (new gin::Knob (proc.eqParams.loGain), 1, 0);
        addControl (new gin::Knob (proc.eqParams.loQ), 2, 0);
        addControl (new gin::Knob (proc.eqParams.mid1Freq), 3, 0);
        addControl (new gin::Knob (proc.eqParams.mid1Gain), 4, 0);
        addControl (new gin::Knob (proc.eqParams.mid1Q), 5, 0);
        addControl (new gin::Knob (proc.eqParams.mid2Freq), 0, 1);
        addControl (new gin::Knob (proc.eqParams.mid2Gain), 1, 1);
        addControl (new gin::Knob (proc.eqParams.mid2Q), 2, 1);
        addControl (new gin::Knob (proc.eqParams.hiFreq), 3, 1);
        addControl (new gin::Knob (proc.eqParams.hiGain), 4, 1);
        addControl (new gin::Knob (proc.eqParams.hiQ), 5, 1);

        setSize (308, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class CompressBox : public gin::ParamBox
{
public:
    CompressBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Compress"), proc (proc_)
    {
        addControl (new gin::Knob (proc.compressorParams.ratio), 0, 0);
        addControl (new gin::Knob (proc.compressorParams.threshold), 1, 0);
        addControl (new gin::Knob (proc.compressorParams.gain), 2, 0);

        addControl (new gin::Knob (proc.compressorParams.attack), 0.5f, 1.0f);
        addControl (new gin::Knob (proc.compressorParams.release), 1.5f, 1.0f);

        setSize (168, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class DelayBox : public gin::ParamBox
{
public:
    DelayBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Delay"), proc (proc_)
    {
        addControl (t = new gin::Knob (proc.delayParams.time), 0, 0);
        addControl (b = new gin::Select (proc.delayParams.beat), 0, 0);
        addControl (new gin::Knob (proc.delayParams.fb), 1, 0);
        addControl (new gin::Knob (proc.delayParams.cf), 2, 0);

        addControl (new gin::Switch (proc.delayParams.sync), 0, 1);
        addControl (new gin::Knob (proc.delayParams.mix), 1.5f, 1.0f);

        watchParam (proc.delayParams.sync);

        setSize (168, 163);
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged();

        t->setVisible (! proc.delayParams.sync->isOn());
        b->setVisible (proc.delayParams.sync->isOn());
    }

    WavetableAudioProcessor& proc;
    gin::ParamComponent::Ptr t, b;
};

//==============================================================================
class ReverbBox : public gin::ParamBox
{
public:
    ReverbBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Reverb"), proc (proc_)
    {
        addControl (new gin::Knob (proc.reverbParams.damping), 0, 0);
        addControl (new gin::Knob (proc.reverbParams.freezeMode), 1, 0);
        addControl (new gin::Knob (proc.reverbParams.roomSize), 2, 0);
        addControl (new gin::Knob (proc.reverbParams.width), 0.5f, 1.0f);
        addControl (new gin::Knob (proc.reverbParams.mix), 1.5f, 1.0f);

        setSize (168, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class LimitBox : public gin::ParamBox
{
public:
    LimitBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox (""), proc (proc_)
    {
        addControl (new gin::Knob (proc.limiterParams.attack), 0, 0);
        addControl (new gin::Knob (proc.limiterParams.release), 1, 0);
        addControl (new gin::Knob (proc.limiterParams.threshold), 0, 1);
        addControl (new gin::Knob (proc.limiterParams.gain), 1, 1);

        setSize (112, 163);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class ScopeArea : public gin::ParamArea
{
public:
    ScopeArea (WavetableAudioProcessor& proc_)
        : gin::ParamArea ("Scope"), proc (proc_)
    {
        scope = new gin::TriggeredScope (proc.fifo);
        scope->setNumChannels (2);
        scope->setTriggerMode (gin::TriggeredScope::TriggerMode::Up);
        scope->setColour (gin::TriggeredScope::lineColourId, juce::Colours::transparentBlack);
        addControl (scope);

        setSize (272, 163);
    }

    void resized() override
    {
        scope->setBounds (getLocalBounds().withSizeKeepingCentre (getWidth(), 140));
    }

    WavetableAudioProcessor& proc;
    gin::TriggeredScope* scope;
};
