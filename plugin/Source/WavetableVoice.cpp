#include "WavetableVoice.h"
#include "PluginProcessor.h"

using namespace gin;

//==============================================================================
WavetableVoice::WavetableVoice (WavetableAudioProcessor& p, BandLimitedLookupTables& bllt)
    : proc (p)
    , bandLimitedLookupTables (bllt)
{
    for (auto& f : filters)
        f.setNumChannels (2);
}

void WavetableVoice::setWavetable (int idx, OwnedArray<BandLimitedLookupTable>& table)
{
	wtOscillators[idx].setWavetable (table);
}

void WavetableVoice::noteStarted()
{
    fastKill = false;
    startVoice();

    auto note = getCurrentlyPlayingNote();
    if (glideInfo.fromNote != -1 && (glideInfo.glissando || glideInfo.portamento))
    {
        noteSmoother.setTime (glideInfo.rate);
        noteSmoother.setValueUnsmoothed (glideInfo.fromNote / 127.0f);
        noteSmoother.setValue (note.initialNote / 127.0f);
    }
    else
    {
        noteSmoother.setValueUnsmoothed (note.initialNote / 127.0f);
    }

    proc.modMatrix.setPolyValue (*this, proc.modSrcVelocity, note.noteOnVelocity.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.initialTimbre.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());

    ScopedValueSetter<bool> svs (disableSmoothing, true);

    for (auto& f : filters)
        f.reset();

    for (auto& a : filterADSRs)
        a.reset();

    for (auto& a : modADSRs)
         a.noteOn();

    for (auto& l : modLFOs)
        l.reset();

    updateParams (0);
    snapParams();
    updateParams (0);
    snapParams();

    for (auto& osc : wtOscillators)
        osc.noteOn();

    for (auto& osc : oscillators)
        osc.noteOn();

    for (auto& a : filterADSRs)
        a.noteOn();

    for (auto& a : modADSRs)
         a.noteOn();

    for (auto& l : modLFOs)
        l.noteOn();

    modStepLFO.reset();
    modStepLFO.noteOn();

    adsr.reset();
    adsr.noteOn();
}

void WavetableVoice::noteRetriggered()
{
    auto note = getCurrentlyPlayingNote();
    
    if (glideInfo.fromNote != -1 && (glideInfo.glissando || glideInfo.portamento))
    {
        noteSmoother.setTime (glideInfo.rate);
        noteSmoother.setValue (note.initialNote / 127.0f);
    }
    else
    {
        noteSmoother.setValueUnsmoothed (note.initialNote / 127.0f);
    }
    
    proc.modMatrix.setPolyValue (*this, proc.modSrcVelocity, note.noteOnVelocity.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.initialTimbre.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());
    
    updateParams (0);

    for (auto& osc : wtOscillators)
        osc.noteOn();

    for (auto& osc : oscillators)
        osc.noteOn();

    for (auto& a : filterADSRs)
        a.noteOn();

    for (auto& a : modADSRs)
         a.noteOn();
    
    modStepLFO.noteOn();
    adsr.noteOn();
}

void WavetableVoice::noteStopped (bool allowTailOff)
{
    adsr.noteOff();

    for (auto& a : filterADSRs)
        a.noteOff();

    for (auto& a : modADSRs)
        a.noteOff();

    if (! allowTailOff)
    {
        clearCurrentNote();
        stopVoice();
    }
}

void WavetableVoice::notePressureChanged()
{
    auto note = getCurrentlyPlayingNote();
    proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());
}

void WavetableVoice::noteTimbreChanged()
{
    auto note = getCurrentlyPlayingNote();
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.initialTimbre.asUnsignedFloat());
}

void WavetableVoice::setCurrentSampleRate (double newRate)
{
    MPESynthesiserVoice::setCurrentSampleRate (newRate);

    for (auto& osc : wtOscillators)
        osc.setSampleRate (newRate);

    for (auto& osc : oscillators)
        osc.setSampleRate (newRate);

    for (auto& f : filters)
        f.setSampleRate (newRate);

    for (auto& a : filterADSRs)
        a.setSampleRate (newRate);
    
    for (auto& l : modLFOs)
        l.setSampleRate (newRate);

    modStepLFO.setSampleRate (newRate);
    noteSmoother.setSampleRate (newRate);
    adsr.setSampleRate (newRate);
}

void WavetableVoice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    updateParams (numSamples);

    // Run OSC
    ScratchBuffer buffer (2, numSamples);

    for (int i = 0; i < Cfg::numWTs; i++)
        if (proc.wtParams[i].enable->isOn())
            wtOscillators[i].processAdding (currentMidiNotes[i], wtParams[i], buffer);

    for (int i = 0; i < Cfg::numOSCs; i++)
        if (proc.oscParams[i].enable->isOn())
            oscillators[i].processAdding (currentMidiNotes[Cfg::numWTs + i], oscParams[i], buffer);

    // Apply velocity
    float velocity = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat();
    buffer.applyGain (velocityToGain (velocity, ampKeyTrack));

    // Apply filters
    for (int i = 0; i < numElementsInArray (filters); i++)
        if (proc.filterParams[i].enable->isOn())
            filters[i].process (buffer);
    
    // Run ADSR
    adsr.processMultiplying (buffer);
    
    if (adsr.getState() == AnalogADSR::State::idle)
    {
        clearCurrentNote();
        stopVoice();
    }

    // Copy output to synth
    outputBuffer.addFrom (0, startSample, buffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, buffer, 1, 0, numSamples);
    
    finishBlock (numSamples);
}

void WavetableVoice::updateParams (int blockSize)
{
    auto note = getCurrentlyPlayingNote();
    
    proc.modMatrix.setPolyValue (*this, proc.modSrcNote, note.initialNote / 127.0f);

    for (int i = 0; i < Cfg::numWTs; i++)
    {
        if (! proc.wtParams[i].enable->isOn()) continue;

        currentMidiNotes[i] = noteSmoother.getCurrentValue() * 127.0f;
        if (glideInfo.glissando) currentMidiNotes[i] = roundToInt (currentMidiNotes[i]);
        currentMidiNotes[i] += float (note.totalPitchbendInSemitones);
        currentMidiNotes[i] += getValue (proc.wtParams[i].tune) + getValue (proc.wtParams[i].finetune) / 100.0f;

        wtParams[i].voices = int (proc.wtParams[i].voices->getProcValue());
        wtParams[i].vcTrns = int (proc.wtParams[i].voicesTrns->getProcValue());
		wtParams[i].pw     = getValue (proc.wtParams[i].table);
        wtParams[i].pan    = getValue (proc.wtParams[i].pan);
        wtParams[i].spread = getValue (proc.wtParams[i].spread) / 100.0f;
        wtParams[i].detune = getValue (proc.wtParams[i].detune);
        wtParams[i].gain   = getValue (proc.wtParams[i].level);
    }

    for (int i = 0; i < Cfg::numOSCs; i++)
    {
        if (! proc.oscParams[i].enable->isOn()) continue;
        
        currentMidiNotes[Cfg::numWTs + i] = noteSmoother.getCurrentValue() * 127.0f;
        if (glideInfo.glissando) currentMidiNotes[Cfg::numWTs + i] = roundToInt (currentMidiNotes[Cfg::numWTs + i]);
        currentMidiNotes[Cfg::numWTs + i] += float (note.totalPitchbendInSemitones);
        currentMidiNotes[Cfg::numWTs + i] += getValue (proc.oscParams[i].tune) + getValue (proc.oscParams[i].finetune) / 100.0f;

        oscParams[i].wave   = (Wave) int (proc.oscParams[i].wave->getProcValue());
        oscParams[i].voices = int (proc.oscParams[i].voices->getProcValue());
        oscParams[i].vcTrns = int (proc.oscParams[i].voicesTrns->getProcValue());
        oscParams[i].pw     = getValue (proc.oscParams[i].pulsewidth) / 100.0f;
        oscParams[i].pan    = getValue (proc.oscParams[i].pan);
        oscParams[i].spread = getValue (proc.oscParams[i].spread) / 100.0f;
        oscParams[i].detune = getValue (proc.oscParams[i].detune);
        oscParams[i].gain   = getValue (proc.oscParams[i].level);
    }
    
    ampKeyTrack = getValue (proc.adsrParams.velocityTracking);

    for (int i = 0; i < Cfg::numFilters; i++)
    {
        if (! proc.filterParams[i].enable->isOn())
        {
            proc.modMatrix.setPolyValue (*this, proc.modSrcFilter[i], 0);
            continue;
        }
        
        filterADSRs[i].setAttack (getValue (proc.filterParams[i].attack));
        filterADSRs[i].setSustainLevel (getValue (proc.filterParams[i].sustain));
        filterADSRs[i].setDecay (getValue (proc.filterParams[i].decay));
        filterADSRs[i].setRelease (getValue (proc.filterParams[i].release));

        filterADSRs[i].process (blockSize);

        float filterWidth = float (getMidiNoteFromHertz (20000.0));
        float filterEnv   = filterADSRs[i].getOutput();
        float filterSens = getValue (proc.filterParams[i].velocityTracking);
        filterSens = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat() * filterSens + 1.0f - filterSens;

        float n = getValue (proc.filterParams[i].frequency);
        n += (currentlyPlayingNote.initialNote - 60) * getValue (proc.filterParams[i].keyTracking);
        n += filterEnv * filterSens * getValue (proc.filterParams[i].amount) * filterWidth;

        float f = getMidiNoteInHertz (n);
        float maxFreq = std::min (20000.0f, float (getSampleRate() / 2));
        f = jlimit (4.0f, maxFreq, f);

        float q = Q / (1.0f - (getValue (proc.filterParams[i].resonance) / 100.0f) * 0.99f);

        switch (int (proc.filterParams[i].type->getProcValue()))
        {
            case 0:
                filters[i].setType (Filter::lowpass);
                filters[i].setSlope (Filter::db12);
                break;
            case 1:
                filters[i].setType (Filter::lowpass);
                filters[i].setSlope (Filter::db24);
                break;
            case 2:
                filters[i].setType (Filter::highpass);
                filters[i].setSlope (Filter::db12);
                break;
            case 3:
                filters[i].setType (Filter::highpass);
                filters[i].setSlope (Filter::db24);
                break;
            case 4:
                filters[i].setType (Filter::bandpass);
                filters[i].setSlope (Filter::db12);
                break;
            case 5:
                filters[i].setType (Filter::bandpass);
                filters[i].setSlope (Filter::db24);
                break;
            case 6:
                filters[i].setType (Filter::notch);
                filters[i].setSlope (Filter::db12);
                break;
            case 7:
                filters[i].setType (Filter::notch);
                filters[i].setSlope (Filter::db24);
                break;
        }
        
        filters[i].setParams (f, q);

        proc.modMatrix.setPolyValue (*this, proc.modSrcFilter[i], filterADSRs[i].getOutput());
    }

    for (int i = 0; i < Cfg::numENVs; i++)
    {
        if (proc.envParams[i].enable->isOn())
        {
            modADSRs[i].setAttack (getValue (proc.envParams[i].attack));
            modADSRs[i].setSustainLevel (getValue (proc.envParams[i].sustain));
            modADSRs[i].setDecay (getValue (proc.envParams[i].decay));
            modADSRs[i].setRelease (getValue (proc.envParams[i].release));

            proc.modMatrix.setPolyValue (*this, proc.modSrcEnv[i], modADSRs[i].getOutput());

            modADSRs[i].process (blockSize);
        }
        else
        {
            proc.modMatrix.setPolyValue (*this, proc.modSrcEnv[i], 0.0f);
        }
    }
    
    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        if (proc.lfoParams[i].enable->isOn())
        {
            LFO::Parameters params;

            float freq = 0;
            if (proc.lfoParams[i].sync->getProcValue() > 0.0f)
                freq = 1.0f / NoteDuration::getNoteDurations()[size_t (proc.lfoParams[i].beat->getProcValue())].toSeconds (proc.playhead);
            else
                freq = getValue (proc.lfoParams[i].rate);

            params.waveShape = (LFO::WaveShape) int (proc.lfoParams[i].wave->getProcValue());
            params.frequency = freq;
            params.phase     = getValue (proc.lfoParams[i].phase);
            params.offset    = getValue (proc.lfoParams[i].offset);
            params.depth     = getValue (proc.lfoParams[i].depth);
            params.delay     = getValue (proc.lfoParams[i].delay);
            params.fade      = getValue (proc.lfoParams[i].fade);

            modLFOs[i].setParameters (params);
            modLFOs[i].process (blockSize);

            proc.modMatrix.setPolyValue (*this, proc.modSrcLFO[i], modLFOs[i].getOutput());
        }
        else
        {
            proc.modMatrix.setPolyValue (*this, proc.modSrcLFO[i], 0);
        }
    }
    
    // Update Step LFO
    if (proc.stepLfoParams.enable->isOn())
    {
        float freq = 1.0f / NoteDuration::getNoteDurations()[size_t (proc.stepLfoParams.beat->getProcValue())].toSeconds (proc.playhead);

        modStepLFO.setFreq (freq);
        
        int n = int (proc.stepLfoParams.length->getProcValue());
        modStepLFO.setNumPoints (n);
        for (int i = n; --i >= 0;)
            modStepLFO.setPoint (i, proc.stepLfoParams.level[i]->getProcValue());
        
        modStepLFO.process (blockSize);

        proc.modMatrix.setPolyValue (*this, proc.modSrcStep, modStepLFO.getOutput());
    }
    else
    {
        proc.modMatrix.setPolyValue (*this, proc.modSrcStep, 0);
    }

    adsr.setAttack (getValue (proc.adsrParams.attack));
    adsr.setDecay (getValue (proc.adsrParams.decay));
    adsr.setSustainLevel (getValue (proc.adsrParams.sustain));
    adsr.setRelease (fastKill ? 0.01f : getValue (proc.adsrParams.release));
    
    noteSmoother.process (blockSize);
}

bool WavetableVoice::isVoiceActive()
{
    return isActive();
}

float WavetableVoice::getFilterCutoffNormalized (int idx)
{
    float freq = filters[idx].getFrequency();
    return proc.filterParams[idx].frequency->getUserRange().convertTo0to1 (getMidiNoteFromHertz (freq));
}
