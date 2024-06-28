#pragma once

#include <JuceHeader.h>
#include "Cfg.h"

class WavetableAudioProcessor;

//==============================================================================
class WavetableVoice : public gin::SynthesiserVoice,
                       public gin::ModVoice
{
public:
    WavetableVoice (WavetableAudioProcessor& p);
    
    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override;
    void noteKeyStateChanged() override     {}

	float getCurrentNote() override;

    void setCurrentSampleRate (double newRate) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    bool isVoiceActive() override;

    float getFilterCutoffNormalized();
    gin::WTOscillator::Params getLiveWTParams (int osc);

    void updateParams (int blockSize);

    WavetableAudioProcessor& proc;

    gin::WTVoicedStereoOscillator oscillators[Cfg::numOSCs];
    gin::StereoOscillator noise;
    gin::StereoOscillator sub;

    gin::Filter filter;
    gin::ADSR filterADSR;
    
    gin::ADSR modADSRs[Cfg::numENVs];
    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    gin::AnalogADSR adsr;

    float currentMidiNotes[Cfg::numOSCs];
    gin::WTVoicedStereoOscillatorParams oscParams[Cfg::numOSCs];

    float subNote = 0.0f;
    gin::StereoOscillator::Params subParams;
    gin::StereoOscillator::Params noiseParams;
    
    gin::EasedValueSmoother<float> noteSmoother;
    
    float ampKeyTrack = 1.0f;
};
