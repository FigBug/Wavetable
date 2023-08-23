#pragma once

#include <JuceHeader.h>

#include "WavetableVoice.h"

//==============================================================================
class WavetableAudioProcessor : public gin::Processor,
                                public gin::Synthesiser
{
public:
    //==============================================================================
    WavetableAudioProcessor();
    ~WavetableAudioProcessor() override;

    void stateUpdated() override;
    void updateState() override;

    //==============================================================================
    void reset() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void updateParams (int blockSize);
    void setupModMatrix();

    gin::BandLimitedLookupTables bandLimitedLookupTables;

    //==============================================================================
    void handleMidiEvent (const juce::MidiMessage& m) override;
    void handleController (int ch, int num, int val) override;
    //==============================================================================
    juce::Array<float> getLiveFilterCutoff (int idx);

    void applyEffects (juce::AudioSampleBuffer& buffer);

    // Voice Params
    struct OSCParams
    {
        OSCParams() = default;

        gin::Parameter::Ptr enable , wave, voices, voicesTrns, tune, finetune,
                            level, pulsewidth, detune, spread, pan;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (OSCParams)
    };

    struct FilterParams
    {
        FilterParams() = default;

        gin::Parameter::Ptr enable, type, keyTracking, velocityTracking,
                            frequency, resonance, amount,
                            attack, decay, sustain, release;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (FilterParams)
    };

    struct EnvParams
    {
        EnvParams() = default;

        gin::Parameter::Ptr enable, attack, decay, sustain, release;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (EnvParams)
    };

    struct LFOParams
    {
        LFOParams() = default;

        gin::Parameter::Ptr enable, sync, wave, rate, beat, depth, phase, offset, fade, delay;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (LFOParams)
    };

    struct StepLFOParams
    {
        StepLFOParams() = default;

        gin::Parameter::Ptr enable, beat, length;
        gin::Parameter::Ptr level[32] = { nullptr };

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (StepLFOParams)
    };

    struct ADSRParams
    {
        ADSRParams() = default;

        gin::Parameter::Ptr attack, decay, sustain, release, velocityTracking;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (ADSRParams)
    };

    // Global Params
    struct GlobalParams
    {
        GlobalParams() = default;

        gin::Parameter::Ptr mono, glideMode, glideRate, legato, level, voices, mpe;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (GlobalParams)
    };

    struct GateParams
    {
        GateParams() = default;

        gin::Parameter::Ptr enable, beat, length, attack, release;
        gin::Parameter::Ptr l[32] = { nullptr };
        gin::Parameter::Ptr r[32] = { nullptr };

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (GateParams)
    };

    struct ChorusParams
    {
        ChorusParams() = default;

        gin::Parameter::Ptr enable, delay, rate, depth, width, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (ChorusParams)
    };

    struct DistortionParams
    {
        DistortionParams() = default;

        gin::Parameter::Ptr enable, amount, highpass, output, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (DistortionParams)
    };

    struct EQParams
    {
        EQParams() = default;

        gin::Parameter::Ptr enable,
                            loFreq, loGain, loQ,
                            mid1Freq, mid1Gain, mid1Q,
                            mid2Freq, mid2Gain, mid2Q,
                            hiFreq, hiGain, hiQ;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (EQParams)
    };

    struct CompressorParams
    {
        CompressorParams() = default;

        gin::Parameter::Ptr enable, attack, release, ratio, threshold, gain;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (CompressorParams)
    };

    struct DelayParams
    {
        DelayParams() = default;

        gin::Parameter::Ptr enable, sync, time, beat, fb, cf, mix, delay;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (DelayParams)
    };

    struct ReverbParams
    {
        ReverbParams() = default;

        gin::Parameter::Ptr enable, damping, freezeMode, roomSize, width, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (ReverbParams)
    };

    struct LimiterParams
    {
        LimiterParams() = default;

        gin::Parameter::Ptr enable, attack, release, threshold, gain;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (LimiterParams)
    };

    //==============================================================================
    gin::ModSrcId modSrcPressure, modSrcTimbre, modScrPitchBend,
                  modSrcNote, modSrcVelocity, modSrcStep, modSrcMonoStep;

    juce::Array<gin::ModSrcId> modSrcCC, modSrcMonoLFO, modSrcLFO, modSrcFilter, modSrcEnv;

    //==============================================================================

    OSCParams oscParams[Cfg::numOSCs];
    FilterParams filterParams[Cfg::numFilters];
    EnvParams envParams[Cfg::numENVs];
    LFOParams lfoParams[Cfg::numLFOs];
    StepLFOParams stepLfoParams;

    ADSRParams adsrParams;

    GlobalParams globalParams;
    GateParams gateParams;
    ChorusParams chorusParams;
    DistortionParams distortionParams;
    EQParams eqParams;
    CompressorParams compressorParams;
    DelayParams delayParams;
    ReverbParams reverbParams;
    LimiterParams limiterParams;

    //==============================================================================
    gin::GateEffect gate;
    gin::Modulation chorus { 0.5f };
    gin::Distortion distortion;
    gin::StereoDelay stereoDelay { 120.1 };
    gin::Dynamics compressor;
    gin::Dynamics limiter;
    gin::EQ eq {4};
    juce::Reverb reverb;
    gin::GainProcessor outputGain;
    gin::AudioFifo fifo { 2, 44100 };

    //==============================================================================
    gin::ModMatrix modMatrix;

    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    juce::AudioPlayHead* playhead = nullptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessor)
};
