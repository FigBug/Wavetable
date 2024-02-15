#pragma once

#include <JuceHeader.h>

#include "WavetableVoice.h"
#include "FX/DeRez2.h"
#include "FX/FireAmp.h"
#include "FX/GrindAmp.h"

constexpr auto fxGate       = 0;
constexpr auto fxChorus     = 1;
constexpr auto fxDistort    = 2;
constexpr auto fxDelay      = 3;
constexpr auto fxReverb     = 4;

//==============================================================================
class WavetableAudioProcessor : public gin::Processor,
                                public gin::Synthesiser
{
public:
    //==============================================================================
    WavetableAudioProcessor();
    ~WavetableAudioProcessor() override;

    bool supportsMPE() const override { return true; }

    void stateUpdated() override;
    void updateState() override;

    //==============================================================================
    void reset() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void updateParams (int blockSize);
    void setupModMatrix();

    //==============================================================================
    void handleMidiEvent (const juce::MidiMessage& m) override;
    void handleController (int ch, int num, int val) override;
    //==============================================================================
    juce::Array<float> getLiveFilterCutoff();
    gin::WTOscillator::Params getLiveWTParams (int osc);

    void reloadWavetables();
    void incWavetable (int osc, int delta);
    bool loadUserWavetable (int osc, const juce::File& f, int sz);

    void applyEffects (juce::AudioSampleBuffer& buffer);
    void applyEffect (juce::AudioSampleBuffer& buffer, int fxId);

    bool loadWaveTable (juce::OwnedArray<gin::BandLimitedLookupTable>& table, double sr, const juce::MemoryBlock& wav, const juce::String& format, int size);

    // Voice Params
    struct OSCParams
    {
        OSCParams() = default;

        gin::Parameter::Ptr enable, voices, tune, finetune, level, pos, detune, spread, pan, bend, formant, retrig;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (OSCParams)
    };

    struct SubParams
    {
        SubParams() = default;

        gin::Parameter::Ptr enable, wave, tune, level, pan, retrig;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (SubParams)
    };

    struct NoiseParams
    {
        NoiseParams() = default;

        gin::Parameter::Ptr enable, type, level, pan;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (NoiseParams)
    };

    struct FilterParams
    {
        FilterParams() = default;

        gin::Parameter::Ptr enable, type, keyTracking, velocityTracking,
                            frequency, resonance, amount, retrig,
                            attack, decay, sustain, release, wt1, wt2, sub, noise;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (FilterParams)
    };

    struct EnvParams
    {
        EnvParams() = default;

        gin::Parameter::Ptr enable, attack, decay, sustain, release, retrig;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (EnvParams)
    };

    struct LFOParams
    {
        LFOParams() = default;

        gin::Parameter::Ptr enable, sync, retrig, wave, rate, beat, depth, phase, offset, fade, delay, xgrid, ygrid;

        void setup (WavetableAudioProcessor& p, int idx);

        JUCE_DECLARE_NON_COPYABLE (LFOParams)
    };

    struct StepLFOParams
    {
        StepLFOParams() = default;

        gin::Parameter::Ptr enable, beat, length, retrig;
        gin::Parameter::Ptr level[Cfg::numStepLFOSteps] = { nullptr };

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (StepLFOParams)
    };

    struct ADSRParams
    {
        ADSRParams() = default;

        gin::Parameter::Ptr attack, decay, sustain, release, velocityTracking, retrig;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (ADSRParams)
    };

    // Global Params
    struct GlobalParams
    {
        GlobalParams() = default;

        gin::Parameter::Ptr mono, glideMode, glideRate, legato, level, voices, mpe, pitchBend;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (GlobalParams)
    };

    // UI Params
    struct UIParams
    {
        UIParams() = default;

        gin::Parameter::Ptr activeLFO, activeENV, activeMOD;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (UIParams)
    };

    struct GateParams
    {
        GateParams() = default;

        gin::Parameter::Ptr enable, beat, length, attack, release;
        gin::Parameter::Ptr l[Cfg::numGateSteps] = { nullptr };
        gin::Parameter::Ptr r[Cfg::numGateSteps] = { nullptr };

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

        gin::Parameter::Ptr enable, amount;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (DistortionParams)
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

        gin::Parameter::Ptr enable, size, decay, lowpass, damping, predelay, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (ReverbParams)
    };

    struct FXParams
    {
        FXParams() = default;

        gin::Parameter::Ptr fx1, fx2, fx3, fx4, fx5, distMode;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (FXParams)
    };

    struct BitCrusherParams
    {
        BitCrusherParams() = default;

        gin::Parameter::Ptr rate, rez, hard, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (BitCrusherParams)
    };

    struct FireAmpParams
    {
        FireAmpParams() = default;

        gin::Parameter::Ptr gain, tone, output, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (FireAmpParams)
    };

    struct GrindAmpParams
    {
        GrindAmpParams() = default;

        gin::Parameter::Ptr gain, tone, output, mix;

        void setup (WavetableAudioProcessor& p);

        JUCE_DECLARE_NON_COPYABLE (GrindAmpParams)
    };

    //==============================================================================
    gin::ModSrcId modSrcPressure, modSrcTimbre, modSrcPitchbend, modScrPitchBend,
                  modSrcFilter, modSrcNote, modSrcVelocity, modSrcStep, modSrcMonoStep;

    juce::Array<gin::ModSrcId> modSrcCC, modSrcMonoLFO, modSrcLFO, modSrcEnv;

    //==============================================================================

    OSCParams oscParams[Cfg::numOSCs];
    SubParams subParams;
    NoiseParams noiseParams;
    FilterParams filterParams;
    EnvParams envParams[Cfg::numENVs];
    LFOParams lfoParams[Cfg::numLFOs];
    StepLFOParams stepLfoParams;
    FXParams fxParams;

    ADSRParams adsrParams;

    GlobalParams globalParams;
    GateParams gateParams;
    ChorusParams chorusParams;
    DistortionParams distortionParams;
    DelayParams delayParams;
    ReverbParams reverbParams;
    BitCrusherParams bitcrusherParams;
    FireAmpParams fireAmpParams;
    GrindAmpParams grindAmpParams;
    UIParams uiParams;

    //==============================================================================
    gin::GateEffect gate;
    gin::Modulation chorus { 0.5f };
    gin::StereoDelay stereoDelay { 120.1 };
    gin::PlateReverb<float, int> reverb;
    gin::GainProcessor outputGain;
    gin::AudioFifo scopeFifo { 2, 44100 };
    float distortionVal = 0.0f;
    DeRez2  bitcrusher;
    FireAmp fireAmp;
    GrindAmp grindAmp;

    juce::OwnedArray<gin::BandLimitedLookupTable> osc1Tables;
    juce::OwnedArray<gin::BandLimitedLookupTable> osc2Tables;

    gin::BandLimitedLookupTables analogTables;

    juce::Value osc1Table, osc2Table;
    juce::MemoryBlock userTable1, userTable2;
    int osc1Size = -1, osc2Size = -1;

    //==============================================================================
    gin::ModMatrix modMatrix;

    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    juce::AudioPlayHead* playhead = nullptr;
    bool blockMissed = false;
    bool presetLoaded = false;
    bool lastMono = false;

    struct CurTable
    {
        juce::String name;
        double sampleRate = 0.0;
    };

    CurTable curTables[Cfg::numOSCs];

    juce::CriticalSection dspLock;
    juce::Random rng;

private:
    bool isParamLocked (gin::Parameter* p) override;
    float getSmoothingTime (gin::Parameter*);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessor)
};
