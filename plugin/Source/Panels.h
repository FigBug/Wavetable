#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Cfg.h"

//==============================================================================
class OscillatorBox : public gin::ParamBox,
                      public Value::Listener
{
public:
    OscillatorBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        setName ( "osc" + juce::String ( idx + 1 ) );

        auto& osc = proc.oscParams[idx];

        setTitle (idx == 0 ? proc.osc1Table.toString() : proc.osc2Table.toString());
        if (idx == 0)
            proc.osc1Table.addListener (this);
        else
            proc.osc2Table.addListener (this);

        addEnable (osc.enable);

        addControl (new gin::SVGPluginButton (osc.retrig, gin::Assets::retrigger));
        addControl (new gin::Knob (osc.pos));
        addControl (new gin::Knob (osc.tune, true));
        addControl (new gin::Knob (osc.finetune, true));
        addControl (new gin::Knob (osc.level));
        addControl (new gin::Knob (osc.pan, true));

        addControl (new gin::Select (osc.voices));
        addControl (detune = new gin::Knob (osc.detune));
        addControl (spread = new gin::Knob (osc.spread));
        addControl (new gin::Knob (osc.formant, true));
        addControl (new gin::Knob (osc.bend, true));

        watchParam (osc.voices);

        wt = new gin::WavetableComponent();
        wt->setName ("wt");
        wt->setWavetables (idx == 0 ? &proc.osc1Tables : &proc.osc2Tables);
        wt->onFileDrop = [this] (const juce::File& f) { loadUserWavetable (f); };
        wt->addMouseListener (this, false);
        addControl (wt);

        auto addButton = new gin::SVGButton ("add", gin::Assets::add);
        addControl (addButton);
        addButton->onClick = [this]
        {
            auto chooser = std::make_shared<juce::FileChooser> (juce::String ("Load Wavetable"), juce::File(), juce::String ("*.wav"));

            auto chooserFlags = juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode;
            chooser->launchAsync (chooserFlags, [this, chooser] (const juce::FileChooser&)
            {
                auto f = chooser->getResult();
                if (! f.existsAsFile())
                    return;

                loadUserWavetable (f);
            } );
        };

        timer.startTimerHz (60);
        timer.onTimer = [this]
        {
            wt->setParams (proc.getLiveWTParams (idx));
        };

        auto& h = getHeader();
        h.addAndMakeVisible (nextButton);
        h.addAndMakeVisible (prevButton);
        h.addMouseListener (this, false);
        nextButton.onClick = [this] { proc.incWavetable (idx, +1); };
        prevButton.onClick = [this] { proc.incWavetable (idx, -1); };
    }

    ~OscillatorBox() override
    {
        if (idx == 0)
            proc.osc1Table.removeListener (this);
        else
            proc.osc2Table.removeListener (this);
    }
    
    void loadUserWavetable (const juce::File& f)
    {
        auto sz = gin::getWavetableSize (f);
        if (sz <= 0)
        {
            auto w = std::make_shared<gin::PluginAlertWindow> ("Import Wavetable", "Wav file does not contain Wavetable metadata. What is the wavetable size?", juce::AlertWindow::NoIcon, getParentComponent());
            w->setLookAndFeel (proc.processorOptions.lookAndFeel.get());
            w->addComboBox ("size", { "256", "512", "1024", "2048" }, "Samples per table:");
            w->getComboBoxComponent ("size")->setSelectedItemIndex (3);

            w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

            w->runAsync (*getParentComponent(), [this, w, f] (int ret)
            {
                auto userSize = w->getComboBoxComponent ("size")->getText().getIntValue();

                w->setVisible (false);
                if (ret == 1)
                    proc.loadUserWavetable (idx, f, userSize);
            });
        }
        else
        {
            proc.loadUserWavetable (idx, f, -1);
        }
    }
    
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.originalComponent != wt) return;
        
        auto& pos = *proc.oscParams[idx].pos;
        pos.beginUserAction();
        
        mouseDownValue = pos.getUserValue();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.originalComponent != wt) return;
        
        auto& pos = *proc.oscParams[idx].pos;
     
        pos.setUserValue (mouseDownValue - e.getDistanceFromDragStartY());
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        auto& h = getHeader();
        
        if (e.originalComponent == wt)
        {
            auto& pos = *proc.oscParams[idx].pos;
            pos.endUserAction();
        }
        else if (e.originalComponent == &h && e.mouseWasClicked() && e.x >= prevButton.getRight() && e.x <= nextButton.getX())
        {
            juce::StringArray tables;
            for (auto i = 0; i < BinaryData::namedResourceListSize; i++)
                if (juce::String (BinaryData::originalFilenames[i]).endsWith (".wt2048"))
                    tables.add (juce::String (BinaryData::originalFilenames[i]).upToLastOccurrenceOf (".wt2048", false, false));

            tables.sortNatural();

            std::map<juce::String, juce::PopupMenu> menus;

            for (auto t : tables)
            {
                auto prefix = t.upToFirstOccurrenceOf (" ", false, false);
                auto suffix = t.fromFirstOccurrenceOf (" ", false, false);

                menus[prefix].addItem (t, [this, t]
                {
                    if (idx == 0)
                    {
                        proc.userTable1.reset();
                        proc.osc1Table = t;
                    }
                    else
                    {
                        proc.userTable2.reset();
                        proc.osc2Table = t;
                    }

                    proc.reloadWavetables();
                });
            }

            juce::PopupMenu m;
            m.setLookAndFeel (&getLookAndFeel());
            for (auto itr : menus)
                m.addSubMenu (itr.first, itr.second);

            m.showMenuAsync ({});
        }
    }

    void resized() override
    {
        gin::ParamBox::resized();

        auto& h = getHeader();
        nextButton.setBounds (getWidth() / 2 + 100, h.getHeight() / 2 - 4, 8, 8);
        prevButton.setBounds (getWidth() / 2 - 108, h.getHeight() / 2 - 4, 8, 8);
    }

    void valueChanged (juce::Value&) override
    {
        setTitle (idx == 0 ? proc.osc1Table.toString() : proc.osc2Table.toString());
        wt->setWavetables (idx == 0 ? &proc.osc1Tables : &proc.osc2Tables);
    }

    void paramChanged() override
    {
        gin::ParamBox::paramChanged();

        auto& osc = proc.oscParams[idx];

        detune->setEnabled (osc.voices->getProcValue() > 1);
        spread->setEnabled (osc.voices->getProcValue() > 1);
    }

    WavetableAudioProcessor& proc;
    int idx = 0;
    gin::ParamComponent::Ptr detune, spread;
    gin::WavetableComponent* wt;
    float mouseDownValue;

    gin::CoalescedTimer timer;

    gin::SVGButton nextButton { "next", gin::Assets::next };
    gin::SVGButton prevButton { "prev", gin::Assets::prev };
};

//==============================================================================
class SubBox : public gin::ParamBox
{
public:
    SubBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("sub");

        auto& sub = proc.subParams;

        addEnable (sub.enable);

        addControl (new gin::SVGPluginButton (sub.retrig, gin::Assets::retrigger));
        addControl (new gin::Knob (sub.tune, true), 0, 0);
        addControl (new gin::Knob (sub.level), 1, 0);
        addControl (new gin::Knob (sub.pan, true), 0, 1);
        addControl (new gin::Select (sub.wave), 1, 1);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class NoiseBox : public gin::ParamBox
{
public:
    NoiseBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("noise");

        auto& noise = proc.noiseParams;

        addEnable (noise.enable);

        addControl (new gin::Select (noise.type), 0, 0);
        addControl (new gin::Knob (noise.level), 0, 0);
        addControl (new gin::Knob (noise.pan, true), 0, 1);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class ADSRBox : public gin::ParamBox
{
public:
    ADSRBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("adsr");

        auto& preset = proc.adsrParams;

        adsr = new gin::ADSRComponent ();
        adsr->setParams (preset.attack, preset.decay, preset.sustain, preset.release);
        adsr->phaseCallback = [this]
        {
            std::vector<std::pair<int, float>> res;

            for (auto v : proc.getActiveVoices())
                if (auto wtv = dynamic_cast<WavetableVoice*> (v))
                    res.push_back (wtv->adsr.getCurrentPhase());

            return res;
        };
        addControl (adsr, 0, 0, 4, 1);

        addControl (new gin::Knob (preset.attack), 0, 1);
        addControl (new gin::Knob (preset.decay), 1, 1);
        addControl (new gin::Knob (preset.sustain), 2, 1);
        addControl (new gin::Knob (preset.release), 3, 1);
        addControl (new gin::Knob (preset.velocityTracking), 4, 1);
        addControl (retrig = new gin::SVGPluginButton (preset.retrig, gin::Assets::retrigger));

        watchParam (proc.globalParams.mono);
        watchParam (proc.globalParams.glideMode);
    }

    void paramChanged() override
    {
        gin::ParamBox::paramChanged ();

        if (retrig != nullptr)
            retrig->setVisible (proc.globalParams.mono->isOn() && proc.globalParams.glideMode->getUserValue() > 0);
    }

    WavetableAudioProcessor& proc;
    gin::ParamComponent::Ptr a = nullptr, d = nullptr, s = nullptr, r = nullptr;
    gin::ADSRComponent* adsr = nullptr;
    gin::SVGPluginButton* retrig;
};

//==============================================================================
class FilterBox : public gin::ParamBox
{
public:
    FilterBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ( "flt" );

        auto& flt = proc.filterParams;

        addEnable (flt.enable);

        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcFilter, true));

        auto freq = new gin::Knob (flt.frequency);
        addControl (freq);
        addControl (new gin::Knob (flt.resonance));
        addControl (new gin::Knob (flt.amount, true));

        addControl (new gin::Knob (flt.keyTracking));
        addControl (new gin::Select (flt.type));
        addControl (v = new gin::Knob (flt.velocityTracking));

        adsr = new gin::ADSRComponent ();
        adsr->setParams (flt.attack, flt.decay, flt.sustain, flt.release);
        adsr->phaseCallback = [this, &flt]
        {
            std::vector<std::pair<int, float>> res;

            if (flt.amount->getUserValue() != 0.0f)
            {
                for (auto voice : proc.getActiveVoices())
                    if (auto wtv = dynamic_cast<WavetableVoice*> (voice))
                        res.push_back (wtv->filterADSR.getCurrentPhase());
            }

            return res;
        };
        addControl (adsr, 3, 0, 4, 1);

        addControl (a = new gin::Knob (flt.attack), 3, 1);
        addControl (d = new gin::Knob (flt.decay), 4, 1);
        addControl (s = new gin::Knob (flt.sustain), 5, 1);
        addControl (r = new gin::Knob (flt.release), 6, 1);

        freq->setLiveValuesCallback ([this] ()
        {
            if (proc.filterParams.amount->getUserValue()      != 0.0f ||
                proc.filterParams.keyTracking->getUserValue() != 0.0f ||
                proc.modMatrix.isModulated (gin::ModDstId (proc.filterParams.frequency->getModIndex())))
                return proc.getLiveFilterCutoff();
            return juce::Array<float>();
        });

        addControl (new gin::SVGPluginButton (flt.wt1, asset1));
        addControl (new gin::SVGPluginButton (flt.wt2, asset2));
        addControl (new gin::SVGPluginButton (flt.sub, assetS));
        addControl (new gin::SVGPluginButton (flt.noise, assetN));
        addControl (retrig = new gin::SVGPluginButton (flt.retrig, gin::Assets::retrigger));

        watchParam (flt.amount);
        watchParam (proc.globalParams.mono);
        watchParam (proc.globalParams.glideMode);
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged ();

        auto& flt = proc.filterParams;

        v->setEnabled (flt.amount->getUserValue() != 0.0f);
        a->setEnabled (flt.amount->getUserValue() != 0.0f);
        d->setEnabled (flt.amount->getUserValue() != 0.0f);
        s->setEnabled (flt.amount->getUserValue() != 0.0f);
        r->setEnabled (flt.amount->getUserValue() != 0.0f);
        adsr->setEnabled (flt.amount->getUserValue() != 0.0f);

        if (retrig != nullptr)
            retrig->setVisible (proc.globalParams.mono->isOn() && proc.globalParams.glideMode->getUserValue() > 0);
    }


    WavetableAudioProcessor& proc;
    gin::ParamComponent::Ptr v  = nullptr, a = nullptr, d = nullptr, s = nullptr, r = nullptr;
    gin::ADSRComponent* adsr = nullptr;
    gin::SVGPluginButton* retrig = nullptr;

    juce::String asset1 = "M0 96C0 60.7 28.7 32 64 32H384c35.3 0 64 28.7 64 64V416c0 35.3-28.7 64-64 64H64c-35.3 0-64-28.7-64-64V96zm236 35.2c-7.4-4.3-16.5-4.3-24-.1l-56 32c-11.5 6.6-15.5 21.2-8.9 32.7s21.2 15.5 32.7 8.9L200 193.4V336H160c-13.3 0-24 10.7-24 24s10.7 24 24 24h64 64c13.3 0 24-10.7 24-24s-10.7-24-24-24H248V152c0-8.6-4.6-16.5-12-20.8z";
    juce::String asset2 = "M64 32C28.7 32 0 60.7 0 96V416c0 35.3 28.7 64 64 64H384c35.3 0 64-28.7 64-64V96c0-35.3-28.7-64-64-64H64zM190.7 184.7l-24.2 18.4c-10.5 8-25.6 6-33.6-4.5s-6-25.6 4.5-33.6l24.2-18.4c15.8-12 35.2-18.4 55.1-18.1l3.4 .1c46.5 .7 83.8 38.6 83.8 85.1c0 23.5-9.7 46-26.9 62.1L212.7 336H296c13.3 0 24 10.7 24 24s-10.7 24-24 24H152c-9.8 0-18.7-6-22.3-15.2s-1.3-19.6 5.9-26.3L244.3 240.6c7.5-7 11.7-16.8 11.7-27.1c0-20.3-16.3-36.8-36.6-37.1l-3.4-.1c-9.1-.1-18 2.8-25.3 8.3z";
    juce::String assetS = "M64 32C28.7 32 0 60.7 0 96V416c0 35.3 28.7 64 64 64H384c35.3 0 64-28.7 64-64V96c0-35.3-28.7-64-64-64H64zM175.6 196.2c-1.1 6.4-.2 9.7 .6 11.6c1 2 2.9 4.6 7.4 7.7c10.1 6.8 25.7 11.5 46.8 17.4l2 .6 0 0c18.4 5.2 41.4 11.7 58.6 23.2c9.5 6.4 18.5 15.1 24.1 27.2c5.7 12.3 7 25.9 4.4 40.3c-4.7 26.3-23.1 43.3-45 52c-21.3 8.4-47 9.6-72.6 5.7l-.1 0 0 0c-16.3-2.6-43.7-10.7-57.3-15.1c-12.6-4-19.6-17.6-15.5-30.2s17.6-19.6 30.2-15.5c13.9 4.5 37.8 11.4 50 13.4c20.2 3 36.8 1.4 47.7-2.9c10.3-4.1 14.2-9.6 15.4-15.8c1.1-6.4 .2-9.7-.6-11.6c-1-2-2.9-4.6-7.4-7.7c-10.1-6.8-25.7-11.5-46.8-17.4l-2-.6c-18.4-5.2-41.4-11.7-58.6-23.2c-9.5-6.4-18.5-15.1-24.1-27.2c-5.7-12.3-7-25.9-4.4-40.3c4.7-26.3 23.1-43.3 45-52c21.3-8.4 47-9.6 72.6-5.7c8.1 1.2 24.4 4.8 32 6.7c12.8 3.3 20.6 16.4 17.3 29.2s-16.4 20.6-29.2 17.3c-6.7-1.7-21.3-4.9-27.3-5.7c-20.3-3.1-36.8-1.4-47.8 2.9c-10.3 4.1-14.2 9.6-15.4 15.8z";
    juce::String assetN = "M64 32C28.7 32 0 60.7 0 96V416c0 35.3 28.7 64 64 64H384c35.3 0 64-28.7 64-64V96c0-35.3-28.7-64-64-64H64zm90.3 104.5L288 294.5V152c0-13.3 10.7-24 24-24s24 10.7 24 24V360c0 10.1-6.3 19.1-15.7 22.5s-20.1 .7-26.6-7L160 217.5V360c0 13.3-10.7 24-24 24s-24-10.7-24-24V152c0-10.1 6.3-19.1 15.7-22.5s20.1-.7 26.6 7z";
};

//==============================================================================
class LFOBox : public gin::ParamBox
{
public:
    LFOBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        setName ("lfo" + juce::String (idx + 1));

        auto& lfo = proc.lfoParams[idx];

        addEnable (lfo.enable);

        addHeader ({"LFO 1", "LFO 2", "LFO 3"}, idx, proc.uiParams.activeLFO);

        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcLFO[idx], true));
        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcMonoLFO[idx], false));

        addControl (r = new gin::Knob (lfo.rate));
        addControl (b = new gin::Select (lfo.beat));
        addControl (new gin::Knob (lfo.depth, true));
        addControl (new gin::Knob (lfo.fade, true));
        addControl (new gin::Knob (lfo.delay));

        addControl (new gin::Select (lfo.wave));
        addControl (new gin::Switch (lfo.sync));
        addControl (new gin::Knob (lfo.phase, true));
        addControl (new gin::Knob (lfo.offset, true));
        
        auto l = new gin::LFOComponent();
        l->phaseCallback = [this, &lfo] 
        {
            std::vector<float> res;

            if (lfo.enable->isOn())
            {
                res.push_back (proc.modLFOs[idx].getCurrentPhase());

                for (auto v : proc.getActiveVoices())
                    if (auto wtv = dynamic_cast<WavetableVoice*> (v))
                        res.push_back (wtv->modLFOs[idx].getCurrentPhase());
            }

            return res;
        };
        l->setParams (lfo.wave, lfo.sync, lfo.rate, lfo.beat, lfo.depth, lfo.offset, lfo.phase, lfo.enable);
        addControl (l);
        
        addControl (new gin::SVGPluginButton (lfo.retrig, gin::Assets::retrigger));

        watchParam (lfo.sync);
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged ();

        if (r && b)
        {
            auto& lfo = proc.lfoParams[idx];
            r->setVisible (! lfo.sync->isOn());
            b->setVisible (lfo.sync->isOn());
        }
    }

    WavetableAudioProcessor& proc;
    int idx;
    gin::ParamComponent::Ptr r = nullptr;
    gin::ParamComponent::Ptr b = nullptr;
};

//==============================================================================
class ENVBox : public gin::ParamBox
{
public:
    ENVBox (const juce::String& name, WavetableAudioProcessor& proc_, int idx_)
        : gin::ParamBox (name), proc (proc_), idx (idx_)
    {
        setName ("env" + juce::String (idx + 1));

        auto& env = proc.envParams[idx];

        addEnable (env.enable);

        addHeader ({"ENV 1", "ENV 2", "ENV 3"}, idx, proc.uiParams.activeENV);

        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcEnv[idx], true));

        addControl (new gin::Knob (env.attack), 0, 1);
        addControl (new gin::Knob (env.decay), 1, 1);
        addControl (new gin::Knob (env.sustain), 2, 1);
        addControl (new gin::Knob (env.release), 3, 1);

        auto g = new gin::ADSRComponent();
        g->setParams (env.attack, env.decay, env.sustain, env.release);
        g->phaseCallback = [this, &env]
        {
            std::vector<std::pair<int, float>> res;

            if (env.enable->isOn())
            {
                for (auto v : proc.getActiveVoices())
                    if (auto wtv = dynamic_cast<WavetableVoice*> (v))
                        res.push_back (wtv->modADSRs[idx].getCurrentPhase());
            }

            return res;
        };
        addControl (g, 0, 0, 4, 1);
        addControl (retrig = new gin::SVGPluginButton (env.retrig, gin::Assets::retrigger));

        watchParam (proc.globalParams.mono);
        watchParam (proc.globalParams.glideMode);
    }

    void paramChanged () override
    {
        gin::ParamBox::paramChanged ();

        if (retrig != nullptr)
            retrig->setVisible (proc.globalParams.mono->isOn() && proc.globalParams.glideMode->getUserValue() > 0);
    }

    WavetableAudioProcessor& proc;
    gin::SVGPluginButton* retrig = nullptr;
    int idx;
};

//==============================================================================
class StepBox : public gin::ParamBox
{
public:
    StepBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("step");

        auto& prs = proc.stepLfoParams;

        addEnable (prs.enable);

        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcStep, true));
        addModSource (new gin::ModulationSourceButton (proc.modMatrix, proc.modSrcMonoStep, false));

        addControl (new gin::Select (prs.beat), 0, 1);
        addControl (new gin::Knob (prs.length), 1, 1);

        auto g = new gin::StepLFOComponent (Cfg::numStepLFOSteps);
        g->phaseCallback = [this, &prs]
        {
            std::vector<float> res;

            if (prs.enable->isOn())
            {
                res.push_back (proc.modStepLFO.getCurrentPhase());
                
                for (auto v : proc.getActiveVoices())
                    if (auto wtv = dynamic_cast<WavetableVoice*> (v))
                        res.push_back (wtv->modStepLFO.getCurrentPhase());
            }

            return res;
        };
        g->setParams (prs.beat, prs.length, prs.level, prs.enable);
        addControl (g, 0, 0, 4, 1);

        addControl (new gin::SVGPluginButton (prs.retrig, gin::Assets::retrigger));
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class ModBox : public gin::ParamBox
{
public:
    ModBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("mod");
        
        addHeader ({"SRC", "MTX"}, 0, proc.uiParams.activeMOD);

        addControl (new gin::ModSrcListBox (proc.modMatrix), 0, 0, 3, 2);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class MatrixBox : public gin::ParamBox
{
public:
    MatrixBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("mtx");

        addHeader ({"SRC", "MTX"}, 1, proc.uiParams.activeMOD);

        addControl (new gin::ModMatrixBox (proc, proc.modMatrix), 0, 0, 3, 2);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class GlobalBox : public gin::ParamBox
{
public:
    GlobalBox (const juce::String& name, WavetableAudioProcessor& proc_)
        : gin::ParamBox (name), proc (proc_)
    {
        setName ("global");

        addControl (new gin::Knob (proc.globalParams.level));
        addControl (new gin::Select (proc.globalParams.glideMode));
        addControl (rate = new gin::Knob (proc.globalParams.glideRate));
        addControl (new gin::Knob (proc.globalParams.voices));
        addControl (legato = new gin::Switch (proc.globalParams.legato));
        addControl (new gin::Switch (proc.globalParams.mono));
        addControl (pitchbend = new gin::Knob (proc.globalParams.pitchBend));

        watchParam (proc.globalParams.glideMode);
        watchParam (proc.globalParams.mpe);
        paramChanged();
    }
    
    void paramChanged() override
    {
        gin::ParamBox::paramChanged();

        rate->setEnabled (proc.globalParams.glideMode->getUserValueInt() > 0);
        legato->setEnabled (proc.globalParams.glideMode->getUserValueInt() > 0);

        pitchbend->setEnabled (! proc.globalParams.mpe->getUserValueBool());
    }

    WavetableAudioProcessor& proc;
    
    gin::Knob* rate;
    gin::Switch* legato;
    gin::Knob* pitchbend;
};

//==============================================================================
class GateBox : public gin::ParamBox
{
public:
    GateBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Gate"), proc (proc_)
    {
        setName ("gate");
        getProperties().set ("fxId", fxGate);

        addEnable (proc.gateParams.enable);
        getHeader().setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

        addControl (new gin::Select (proc.gateParams.beat));
        addControl (new gin::Knob (proc.gateParams.length));
        addControl (new gin::Knob (proc.gateParams.attack));
        addControl (new gin::Knob (proc.gateParams.release));

        auto g = new gin::GateEffectComponent (Cfg::numGateSteps);
        g->setParams (proc.gateParams.length, proc.gateParams.l, proc.gateParams.r, proc.gateParams.enable);
        addControl (g);
    }

    WavetableAudioProcessor& proc;
};

//==============================================================================
class ChorusBox : public gin::ParamBox
{
public:
    ChorusBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Chorus"), proc (proc_)
    {
        setName ("chorus");
        getProperties().set ("fxId", fxChorus);

        addEnable (proc.chorusParams.enable);
        getHeader().setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

        addControl (new gin::Knob (proc.chorusParams.delay), 0, 0);
        addControl (new gin::Knob (proc.chorusParams.rate), 1, 0);
        addControl (new gin::Knob (proc.chorusParams.mix), 2, 0);

        addControl (new gin::Knob (proc.chorusParams.depth), 0.5f, 1.0f);
        addControl (new gin::Knob (proc.chorusParams.width), 1.5f, 1.0f);
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
        setName ("distort");
        getProperties().set ("fxId", fxDistort);

        addEnable (proc.distortionParams.enable);
        getHeader().setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

        {
            page1 = new juce::Component ("page1");
            page1->setInterceptsMouseClicks (false, true);

            addControl (page1);

            addControl (page1, new gin::Knob (proc.distortionParams.amount));
        }

        {
            page2 = new juce::Component ("page2");
            page2->setInterceptsMouseClicks (false, true);

            addControl (page2);

            addControl (page2, new gin::Knob (proc.bitcrusherParams.rate));
            addControl (page2, new gin::Knob (proc.bitcrusherParams.rez));
            addControl (page2, new gin::Knob (proc.bitcrusherParams.hard));
            addControl (page2, new gin::Knob (proc.bitcrusherParams.mix));
        }

        {
            page3 = new juce::Component ("page3");
            page3->setInterceptsMouseClicks (false, true);

            addControl (page3);

            addControl (page3, new gin::Knob (proc.fireAmpParams.gain));
            addControl (page3, new gin::Knob (proc.fireAmpParams.tone));
            addControl (page3, new gin::Knob (proc.fireAmpParams.output));
            addControl (page3, new gin::Knob (proc.fireAmpParams.mix));
        }

        {
            page4 = new juce::Component ("page4");
            page4->setInterceptsMouseClicks (false, true);

            addControl (page4);

            addControl (page4, new gin::Knob (proc.grindAmpParams.gain));
            addControl (page4, new gin::Knob (proc.grindAmpParams.tone));
            addControl (page4, new gin::Knob (proc.grindAmpParams.output));
            addControl (page4, new gin::Knob (proc.grindAmpParams.mix));
        }

        auto& h = getHeader();
        h.addAndMakeVisible (modeButton);
        modeButton.onClick = [this]
        {
            juce::PopupMenu m;
            m.setLookAndFeel (&getLookAndFeel());

            m.addItem ("Simple", true, proc.fxParams.distMode->getUserValueInt() == 0, [this]
            {
                proc.fxParams.distMode->setUserValue (0.0f);
            });
            m.addItem ("Bitcrusher", true, proc.fxParams.distMode->getUserValueInt() == 1, [this]
            {
                proc.fxParams.distMode->setUserValue (1.0f);
            });
            m.addItem ("Fire Amp", true, proc.fxParams.distMode->getUserValueInt() == 2, [this]
            {
                proc.fxParams.distMode->setUserValue (2.0f);
            });
            m.addItem ("Grind Amp", true, proc.fxParams.distMode->getUserValueInt() == 3, [this]
            {
                proc.fxParams.distMode->setUserValue (3.0f);
            });

            m.showMenuAsync ({});
        };

        watchParam (proc.fxParams.distMode);
        paramChanged();
    }

    void paramChanged() override
    {
        gin::ParamBox::paramChanged();

        auto mode = proc.fxParams.distMode->getUserValueInt();

        page1->setVisible (mode == 0);
        page2->setVisible (mode == 1);
        page3->setVisible (mode == 2);
        page4->setVisible (mode == 3);

        auto& h = getHeader();
        if (mode == 0) h.setTitle ("Dist");
        if (mode == 1) h.setTitle ("Crush");
        if (mode == 2) h.setTitle ("Fire");
        if (mode == 3) h.setTitle ("Grind");
    }

    juce::Component* page1;
    juce::Component* page2;
    juce::Component* page3;
    juce::Component* page4;

    WavetableAudioProcessor& proc;

    gin::SVGButton modeButton { "mode", gin::Assets::caretDown, 6 };
};

//==============================================================================
class DelayBox : public gin::ParamBox
{
public:
    DelayBox (WavetableAudioProcessor& proc_)
        : gin::ParamBox ("Delay"), proc (proc_)
    {
        setName ("delay");
        getProperties().set ("fxId", fxDelay);

        addEnable (proc.delayParams.enable);
        getHeader().setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

        addControl (t = new gin::Knob (proc.delayParams.time), 0, 0);
        addControl (b = new gin::Select (proc.delayParams.beat), 0, 0);
        addControl (new gin::Knob (proc.delayParams.fb), 1, 0);
        addControl (new gin::Knob (proc.delayParams.cf), 2, 0);

        addControl (new gin::Switch (proc.delayParams.sync), 0, 1);
        addControl (new gin::Knob (proc.delayParams.mix), 1.5f, 1.0f);

        t->setName ("Delay1");
        b->setName ("Delay2");

        watchParam (proc.delayParams.sync);
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
        setName ("reverb");
        getProperties().set ("fxId", fxReverb);

        addEnable (proc.reverbParams.enable);
        getHeader().setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

        addControl (new gin::Knob (proc.reverbParams.size), 0, 0);
        addControl (new gin::Knob (proc.reverbParams.decay), 1, 0);
        addControl (new gin::Knob (proc.reverbParams.lowpass), 2, 0);
        addControl (new gin::Knob (proc.reverbParams.damping), 0, 1);
        addControl (new gin::Knob (proc.reverbParams.predelay), 1, 1);
        addControl (new gin::Knob (proc.reverbParams.mix), 2, 1);
    }

    WavetableAudioProcessor& proc;
};
