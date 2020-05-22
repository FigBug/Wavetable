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

	void setWavetable (int idx, OwnedArray<gin::BandLimitedLookupTable>& table);

    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override    {}
    void noteKeyStateChanged() override     {}
    
    void setCurrentSampleRate (double newRate) override;

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    bool isVoiceActive() override;

    float getFilterCutoffNormalized (int idx);

private:
    void updateParams (int blockSize);

    WavetableAudioProcessor& proc;
    gin::BandLimitedLookupTables& bandLimitedLookupTables;

	gin::WTVoicedStereoOscillator	wtOscillators[Cfg::numWTs];
    gin::BLLTVoicedStereoOscillator oscillators[Cfg::numOSCs] =
    {
        bandLimitedLookupTables,
    };

    gin::Filter filters[Cfg::numFilters];
    gin::ADSR filterADSRs[Cfg::numFilters];
    
    gin::ADSR modADSRs[Cfg::numENVs];
    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    gin::AnalogADSR adsr;

    float currentMidiNotes[Cfg::numWTs + Cfg::numOSCs];

	gin::WTVoicedStereoOscillator::Params wtParams[Cfg::numWTs];
    gin::BLLTVoicedStereoOscillator::Params oscParams[Cfg::numOSCs];
    
    gin::EasedValueSmoother<float> noteSmoother;
    
    float ampKeyTrack = 1.0f;
};
