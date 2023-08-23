#pragma once

#include <JuceHeader.h>
#include "Cfg.h"

class WavetableAudioProcessor;

//==============================================================================
class WavetableVoice : public gin::SynthesiserVoice,
                       public gin::ModVoice
{
public:
    WavetableVoice (WavetableAudioProcessor& p, gin::BandLimitedLookupTables& bandLimitedLookupTables);
    
    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override    {}
    void noteKeyStateChanged() override     {}
    
    void setCurrentSampleRate (double newRate) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    bool isVoiceActive() override;

    float getFilterCutoffNormalized (int idx);

private:
    void updateParams (int blockSize);

    WavetableAudioProcessor& proc;
    gin::BandLimitedLookupTables& bandLimitedLookupTables;

    gin::BLLTVoicedStereoOscillator oscillators[Cfg::numOSCs] =
    {
        bandLimitedLookupTables,
        bandLimitedLookupTables,
        bandLimitedLookupTables,
        bandLimitedLookupTables,
    };

    gin::Filter filters[Cfg::numFilters];
    gin::ADSR filterADSRs[Cfg::numFilters];
    
    gin::ADSR modADSRs[Cfg::numENVs];
    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    gin::AnalogADSR adsr;

    float currentMidiNotes[Cfg::numOSCs];
    gin::BLLTVoicedStereoOscillator::Params oscParams[Cfg::numOSCs];
    
    gin::EasedValueSmoother<float> noteSmoother;
    
    float ampKeyTrack = 1.0f;
};
