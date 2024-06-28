#include "WavetableVoice.h"
#include "PluginProcessor.h"

//==============================================================================
WavetableVoice::WavetableVoice (WavetableAudioProcessor& p)
    : proc (p)
    , noise (proc.analogTables)
    , sub (proc.analogTables)
{
    filter.setNumChannels (2);
}

void WavetableVoice::noteStarted()
{
    oscillators[0].setWavetable (&proc.osc1Tables);
    oscillators[1].setWavetable (&proc.osc2Tables);

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
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.timbre.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPitchbend, note.pitchbend.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());

    juce::ScopedValueSetter<bool> svs (disableSmoothing, true);

    filter.reset();
    filterADSR.reset();

    for (auto& a : modADSRs)
         a.reset();

    for (auto& l : modLFOs)
        l.reset();

    updateParams (0);
    snapParams();
    updateParams (0);
    snapParams();
    
    for (auto idx = 0; auto& osc : oscillators)
    {
        auto& params = proc.oscParams[idx++];
        auto retrig = params.retrig->getBoolValue();
        auto voices = params.voices->getUserValue();

        if (voices == 1)
        {
            osc.noteOn (retrig ? 0.0f : proc.rng.nextFloat());
        }
        else if (retrig)
        {
            float phases[8] = { 0.0f };

            for (auto i = 0; i < voices; i++)
                phases[i] = 1.0f / voices * i;

            osc.noteOn (phases);
        }
        else
        {
            float phases[8];
            for (auto& p : phases)
                p = proc.rng.nextFloat();

            osc.noteOn (phases);
        }
    }

    noise.noteOn();
    sub.noteOn (proc.subParams.retrig->getBoolValue() ? 0.0f : proc.rng.nextFloat());

    filterADSR.noteOn();

    for (auto& a : modADSRs)
        a.noteOn();

    for (auto idx = 0; auto& l : modLFOs)
        l.noteOn (proc.lfoParams[idx++].retrig->getBoolValue() ? -1 : proc.rng.nextFloat());

    modStepLFO.reset();
    modStepLFO.noteOn (proc.stepLfoParams.retrig->getBoolValue() ? -1 : proc.rng.nextFloat());

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
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.timbre.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());
    proc.modMatrix.setPolyValue (*this, proc.modSrcPitchbend, note.pitchbend.asUnsignedFloat());
    
    updateParams (0);

    if (proc.filterParams.retrig->getBoolValue())
        filterADSR.noteOn();

    for (auto idx = 0; auto& a : modADSRs)
        if (proc.envParams[idx++].retrig->getBoolValue())
             a.noteOn();
    
    if (proc.adsrParams.retrig->getBoolValue())
        adsr.noteOn();
}

void WavetableVoice::noteStopped (bool allowTailOff)
{
    adsr.noteOff();
    filterADSR.noteOff();

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
    proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.timbre.asUnsignedFloat());
}

void WavetableVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();
    proc.modMatrix.setPolyValue (*this, proc.modSrcPitchbend, note.pitchbend.asUnsignedFloat());
}

void WavetableVoice::setCurrentSampleRate (double newRate)
{
    MPESynthesiserVoice::setCurrentSampleRate (newRate);

    for (auto& osc : oscillators)
        osc.setSampleRate (newRate);
    noise.setSampleRate (newRate);
    sub.setSampleRate (newRate);

    filter.setSampleRate (newRate);

    filterADSR.setSampleRate (newRate);
    
    for (auto& l : modLFOs)
        l.setSampleRate (newRate);

    for (auto& a : modADSRs)
        a.setSampleRate (newRate);

    modStepLFO.setSampleRate (newRate);
    noteSmoother.setSampleRate (newRate);
    adsr.setSampleRate (newRate);
}

void WavetableVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    updateParams (numSamples);

    // Run OSC
    gin::ScratchBuffer preFilter (2, numSamples);
    gin::ScratchBuffer postFilter (2, numSamples);

    if (proc.oscParams[0].enable->isOn())
        oscillators[0].processAdding (currentMidiNotes[0], oscParams[0], proc.filterParams.wt1->isOn() ? preFilter : postFilter);

    if (proc.oscParams[1].enable->isOn())
        oscillators[1].processAdding (currentMidiNotes[1], oscParams[1], proc.filterParams.wt2->isOn() ? preFilter : postFilter);

    if (proc.subParams.enable->isOn())
        sub.processAdding (subNote, subParams, proc.filterParams.sub->isOn() ? preFilter : postFilter);

    if (proc.noiseParams.enable->isOn())
        noise.processAdding (60.0f, noiseParams, proc.filterParams.noise->isOn() ? preFilter : postFilter);

    // Apply velocity
    float velocity = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat();
    preFilter.applyGain (gin::velocityToGain (velocity, ampKeyTrack));
    postFilter.applyGain (gin::velocityToGain (velocity, ampKeyTrack));

    // Apply filter
    if (proc.filterParams.enable->isOn())
        filter.process (preFilter);

    postFilter.addFrom (0, 0, preFilter, 0, 0, numSamples);
    postFilter.addFrom (1, 0, preFilter, 1, 0, numSamples);
    
    // Run ADSR
    adsr.processMultiplying (postFilter);
    
    if (adsr.getState() == gin::AnalogADSR::State::idle)
    {
        clearCurrentNote();
        stopVoice();
    }

    // Copy output to synth
    outputBuffer.addFrom (0, startSample, postFilter, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, postFilter, 1, 0, numSamples);
    
    finishBlock (numSamples);
}

void WavetableVoice::updateParams (int blockSize)
{
    auto note = getCurrentlyPlayingNote();
    
    proc.modMatrix.setPolyValue (*this, proc.modSrcNote, note.initialNote / 127.0f);

    for (int i = 0; i < Cfg::numOSCs; i++)
    {
        if (! proc.oscParams[i].enable->isOn()) continue;
        
        currentMidiNotes[i] = noteSmoother.getCurrentValue() * 127.0f;
        if (glideInfo.glissando) currentMidiNotes[i] = (float) juce::roundToInt (currentMidiNotes[i]);
        currentMidiNotes[i] += float (note.totalPitchbendInSemitones);
        currentMidiNotes[i] += getValue (proc.oscParams[i].tune) + getValue (proc.oscParams[i].finetune) / 100.0f;

        oscParams[i].voices     = int (proc.oscParams[i].voices->getProcValue());
        oscParams[i].position   = getValue (proc.oscParams[i].pos) / 100.0f;
        oscParams[i].pan        = getValue (proc.oscParams[i].pan);
        oscParams[i].spread     = getValue (proc.oscParams[i].spread) / 100.0f;
        oscParams[i].detune     = getValue (proc.oscParams[i].detune);
        oscParams[i].gain       = getValue (proc.oscParams[i].level);
        oscParams[i].formant    = getValue (proc.oscParams[i].formant);
        oscParams[i].bend       = getValue (proc.oscParams[i].bend);
    }

    if (proc.subParams.enable->isOn())
    {
        subNote = noteSmoother.getCurrentValue() * 127.0f;
        if (glideInfo.glissando) subNote = (float) juce::roundToInt (subNote);
        subNote += float (note.totalPitchbendInSemitones);
        subNote += getValue (proc.subParams.tune);

        switch (proc.subParams.wave->getUserValueInt())
        {
            case 0: 
                subParams.wave = gin::Wave::sine;
                break;
            case 1: 
                subParams.wave = gin::Wave::triangle;
                break;
            case 2: 
                subParams.wave = gin::Wave::sawUp;
                break;
            case 3: 
                subParams.wave = gin::Wave::pulse;
                subParams.pw   = 0.50f;
                break;
            case 4:
                subParams.wave = gin::Wave::pulse;
                subParams.pw   = 0.25f;
                break;
            case 5:
                subParams.wave = gin::Wave::pulse;
                subParams.pw   = 0.125f;
                break;
        }

        subParams.leftGain  = getValue (proc.subParams.level) * (1.0f - getValue (proc.subParams.pan));
        subParams.rightGain = getValue (proc.subParams.level) * (1.0f + getValue (proc.subParams.pan));
    }

    if (proc.noiseParams.enable->isOn())
    {
        noiseParams.wave = proc.noiseParams.type->getUserValueInt() == 0 ? gin::Wave::whiteNoise : gin::Wave::pinkNoise;

        noiseParams.leftGain  = getValue (proc.noiseParams.level) * (1.0f - getValue (proc.noiseParams.pan));
        noiseParams.rightGain = getValue (proc.noiseParams.level) * (1.0f + getValue (proc.noiseParams.pan));
    }
    
    ampKeyTrack = getValue (proc.adsrParams.velocityTracking);

    if (! proc.filterParams.enable->isOn())
    {
        proc.modMatrix.setPolyValue (*this, proc.modSrcFilter, 0);
    }
    else
    {
        filterADSR.setAttack (getValue (proc.filterParams.attack));
        filterADSR.setSustainLevel (getValue (proc.filterParams.sustain));
        filterADSR.setDecay (getValue (proc.filterParams.decay));
        filterADSR.setRelease (getValue (proc.filterParams.release));

        filterADSR.process (blockSize);

        float filterWidth = float (gin::getMidiNoteFromHertz (20000.0));
        float filterEnv   = filterADSR.getOutput();
        float filterSens = getValue (proc.filterParams.velocityTracking);
        filterSens = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat() * filterSens + 1.0f - filterSens;

        float n = getValue (proc.filterParams.frequency);
        n += (currentlyPlayingNote.initialNote - 60) * getValue (proc.filterParams.keyTracking);
        n += filterEnv * filterSens * getValue (proc.filterParams.amount) * filterWidth;

        float f = gin::getMidiNoteInHertz (n);
        float maxFreq = std::min (20000.0f, float (getSampleRate() / 2));
        f = juce::jlimit (4.0f, maxFreq, f);

        float q = gin::Q / (1.0f - (getValue (proc.filterParams.resonance) / 100.0f) * 0.99f);

        switch (int (proc.filterParams.type->getProcValue()))
        {
            case 0:
                filter.setType (gin::Filter::lowpass);
                filter.setSlope (gin::Filter::db12);
                break;
            case 1:
                filter.setType (gin::Filter::lowpass);
                filter.setSlope (gin::Filter::db24);
                break;
            case 2:
                filter.setType (gin::Filter::highpass);
                filter.setSlope (gin::Filter::db12);
                break;
            case 3:
                filter.setType (gin::Filter::highpass);
                filter.setSlope (gin::Filter::db24);
                break;
            case 4:
                filter.setType (gin::Filter::bandpass);
                filter.setSlope (gin::Filter::db12);
                break;
            case 5:
                filter.setType (gin::Filter::bandpass);
                filter.setSlope (gin::Filter::db24);
                break;
            case 6:
                filter.setType (gin::Filter::notch);
                filter.setSlope (gin::Filter::db12);
                break;
            case 7:
                filter.setType (gin::Filter::notch);
                filter.setSlope (gin::Filter::db24);
                break;
        }

        filter.setParams (f, q);

        proc.modMatrix.setPolyValue (*this, proc.modSrcFilter, filterADSR.getOutput());
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
            gin::LFO::Parameters params;

            float freq = 0;
            if (proc.lfoParams[i].sync->getProcValue() > 0.0f)
                freq = 1.0f / gin::NoteDuration::getNoteDurations()[size_t (proc.lfoParams[i].beat->getProcValue())].toSeconds (proc.playhead);
            else
                freq = getValue (proc.lfoParams[i].rate);

            params.waveShape = (gin::LFO::WaveShape) int (proc.lfoParams[i].wave->getProcValue());
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
        float freq = 1.0f / gin::NoteDuration::getNoteDurations()[size_t (proc.stepLfoParams.beat->getProcValue())].toSeconds (proc.playhead);

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

float WavetableVoice::getFilterCutoffNormalized()
{
    float freq = filter.getFrequency();
    auto range = proc.filterParams.frequency->getUserRange();
    return range.convertTo0to1 (juce::jlimit (range.start, range.end, gin::getMidiNoteFromHertz (freq)));
}

gin::WTOscillator::Params WavetableVoice::getLiveWTParams (int osc)
{
    gin::WTOscillator::Params p;
    p.position = getValue (proc.oscParams[osc].pos) / 100.0f;
    p.formant = getValue (proc.oscParams[osc].formant);
    p.bend = getValue (proc.oscParams[osc].bend);
    return p;
}

float WavetableVoice::getCurrentNote()
{
	return noteSmoother.getCurrentValue() * 127.0f;
}
