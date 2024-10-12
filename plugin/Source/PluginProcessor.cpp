#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WavetableVoice.h"

static juce::String subTextFunction (const gin::Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "Sine";
        case 1: return "Triangle";
        case 2: return "Saw";
        case 3: return "Pulse 50%";
        case 4: return "Pulse 25%";
        case 5: return "Pulse 12%";
        default:
            jassertfalse;
            return {};
    }
}

static juce::String noiseTextFunction (const gin::Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "White";
        case 1: return "Pink";
        default:
            jassertfalse;
            return {};
    }
}

static juce::String lfoTextFunction (const gin::Parameter&, float v)
{
    switch ((gin::LFO::WaveShape)int (v))
    {
        case gin::LFO::WaveShape::none:          return "None";
        case gin::LFO::WaveShape::sine:          return "Sine";
        case gin::LFO::WaveShape::triangle:      return "Triangle";
        case gin::LFO::WaveShape::sawUp:         return "Saw Up";
        case gin::LFO::WaveShape::sawDown:       return "Saw Down";
        case gin::LFO::WaveShape::square:        return "Square";
        case gin::LFO::WaveShape::squarePos:     return "Square+";
        case gin::LFO::WaveShape::sampleAndHold: return "S&H";
        case gin::LFO::WaveShape::noise:         return "Noise";
        case gin::LFO::WaveShape::stepUp3:       return "Step Up 3";
        case gin::LFO::WaveShape::stepUp4:       return "Step Up 4";
        case gin::LFO::WaveShape::stepup8:       return "Step Up 8";
        case gin::LFO::WaveShape::stepDown3:     return "Step Down 3";
        case gin::LFO::WaveShape::stepDown4:     return "Step Down 4";
        case gin::LFO::WaveShape::stepDown8:     return "Step Down 8";
        case gin::LFO::WaveShape::pyramid3:      return "Pyramid 3";
        case gin::LFO::WaveShape::pyramid5:      return "Pyramid 5";
        case gin::LFO::WaveShape::pyramid9:      return "Pyramid 9";
        default:
            jassertfalse;
            return {};
    }
}

static juce::String enableTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "On" : "Off";
}

static juce::String durationTextFunction (const gin::Parameter&, float v)
{
    return gin::NoteDuration::getNoteDurations()[size_t (v)].getName();
}

static juce::String distortionAmountTextFunction (const gin::Parameter&, float v)
{
    return juce::String (v * 5.0f - 1.0f, 1);
}

static juce::String filterTextFunction (const gin::Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "LP 12";
        case 1: return "LP 24";
        case 2: return "HP 12";
        case 3: return "HP 24";
        case 4: return "BP 12";
        case 5: return "BP 24";
        case 6: return "NT 12";
        case 7: return "NT 24";
        default:
            jassertfalse;
            return {};
    }
}

static juce::String freqTextFunction (const gin::Parameter&, float v)
{
    return juce::String (int (gin::getMidiNoteInHertz (v)));
}

static juce::String glideModeTextFunction (const gin::Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "Off";
        case 1: return "Glissando";
        case 2: return "Portamento";
        default:
            jassertfalse;
            return {};
    }
}

//==============================================================================
void WavetableAudioProcessor::OSCParams::setup (WavetableAudioProcessor& p, int idx)
{
    juce::String id = "osc" + juce::String (idx + 1);
    juce::String nm = "OSC" + juce::String (idx + 1) + " ";

    enable     = p.addIntParam (id + "enable",     nm + "Enable",      "Enable",    "", { 0.0, 1.0, 1.0, 1.0 }, idx == 0 ? 1.0f : 0.0f, 0.0f);
    retrig     = p.addIntParam (id + "retrig",     nm + "Retrig",      "Retrig",    "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    voices     = p.addIntParam (id + "unison",     nm + "Unison",      "Unison",    "", { 1.0, 8.0, 1.0, 1.0 }, 1.0, 0.0f);
    tune       = p.addExtParam (id + "tune",       nm + "Tune",        "Tune",      "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    finetune   = p.addExtParam (id + "finetune",   nm + "Fine Tune",   "Fine",      "ct", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    level      = p.addExtParam (id + "level",      nm + "Level",       "Level",     "db", { -100.0, 0.0, 1.0, 4.0 }, 0.0, 0.0f);
    pos        = p.addExtParam (id + "pos",        nm + "Pos",         "Pos",       "%", { 0.0, 100.0, 0.0, 1.0 }, 00.0, 0.0f);
    detune     = p.addExtParam (id + "detune",     nm + "Detune",      "Detune",    "", { 0.0, 0.5, 0.0, 1.0 }, 0.0, 0.0f);
    spread     = p.addExtParam (id + "spread",     nm + "Spread",      "Spread",    "%", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    pan        = p.addExtParam (id + "pan",        nm + "Pan",         "Pan",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    formant    = p.addExtParam (id + "formant",    nm + "Formant",     "Formant",   "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    bend       = p.addExtParam (id + "bend",       nm + "Bend",        "Bend",      "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);

    level->conversionFunction = [] (float in)   { return juce::Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::SubParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "sub";
    juce::String nm = "SUB ";

    enable     = p.addIntParam (id + "enable",     nm + "Enable",      "Enable",    "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    retrig     = p.addIntParam (id + "retrig",     nm + "Retrig",      "Retrig",    "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    wave       = p.addIntParam (id + "wave",       nm + "Wave",        "Wave",      "", { 0.0, 5.0, 1.0, 1.0 }, 1.0, 0.0f, subTextFunction);
    tune       = p.addExtParam (id + "tune",       nm + "Tune",        "Tune",      "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    level      = p.addExtParam (id + "level",      nm + "Level",       "Level",     "db", { -100.0, 0.0, 1.0, 4.0 }, 0.0, 0.0f);
    pan        = p.addExtParam (id + "pan",        nm + "Pan",         "Pan",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);

    level->conversionFunction = [] (float in)   { return juce::Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::NoiseParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "noise";
    juce::String nm = "Noise ";

    enable     = p.addIntParam (id + "enable",     nm + "Enable",      "Enable",    "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f);
    type       = p.addIntParam (id + "type",       nm + "Type",        "Type",      "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, noiseTextFunction);
    level      = p.addExtParam (id + "level",      nm + "Level",       "Level",     "db", { -100.0, 0.0, 1.0, 4.0 }, 0.0, 0.0f);
    pan        = p.addExtParam (id + "pan",        nm + "Pan",         "Pan",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);

    level->conversionFunction = [] (float in)   { return juce::Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::FilterParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "flt";
    juce::String nm = "FLT ";

    float maxFreq = float (gin::getMidiNoteFromHertz (20000.0));

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "",           "", { 0.0, 1.0, 1.0, 1.0 }, 1.0f, 0.0f);
    retrig           = p.addIntParam (id + "retrig",  nm + "Retrig",   "Retrig",    "", { 0.0, 1.0, 1.0, 1.0 }, 1.0, 0.0f, enableTextFunction);
    type             = p.addIntParam (id + "type",    nm + "Type",    "Type",       "", { 0.0, 7.0, 1.0, 1.0 }, 0.0, 0.0f, filterTextFunction);
    keyTracking      = p.addExtParam (id + "key",     nm + "Key",     "Key",        "%", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    velocityTracking = p.addExtParam (id + "vel",     nm + "Vel",     "Vel",        "%", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    frequency        = p.addExtParam (id + "freq",    nm + "Freq",    "Freq",       "Hz", { 0.0, maxFreq, 0.0, 1.0 }, 64.0, 0.0f, freqTextFunction);
    resonance        = p.addExtParam (id + "res",     nm + "Res",     "Res",        "", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    amount           = p.addExtParam (id + "amount",  nm + "Amount",  "Amnt",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",          "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    decay            = p.addExtParam (id + "decay",   nm + "Decay",   "D",          "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    sustain          = p.addExtParam (id + "sustain", nm + "Sustain", "S",          "%", { 0.0, 100.0, 0.0, 1.0 }, 80.0f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",          "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    wt1              = p.addIntParam (id + "wt1",     nm + "WT1",     "",           "", { 0.0, 1.0, 1.0, 1.0f }, 1.0f, 0.0f);
    wt2              = p.addIntParam (id + "wt2",     nm + "WT2",     "",           "", { 0.0, 1.0, 1.0, 1.0f }, 1.0f, 0.0f);
    sub              = p.addIntParam (id + "sub",     nm + "sub",     "",           "", { 0.0, 1.0, 1.0, 1.0f }, 1.0f, 0.0f);
    noise            = p.addIntParam (id + "noise",   nm + "noise",   "",           "", { 0.0, 1.0, 1.0, 1.0f }, 1.0f, 0.0f);

    sustain->conversionFunction          = [] (float in) { return in / 100.0f; };
    velocityTracking->conversionFunction = [] (float in) { return in / 100.0f; };
    keyTracking->conversionFunction      = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::EnvParams::setup (WavetableAudioProcessor& p, int idx)
{
    juce::String id = "env" + juce::String (idx + 1);
    juce::String nm = "ENV" + juce::String (idx + 1) + " ";

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    retrig           = p.addIntParam (id + "retrig",  nm + "Retrig",  "Retrig", "", { 0.0, 1.0, 1.0, 1.0 }, 1.0, 0.0f, enableTextFunction);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    decay            = p.addExtParam (id + "decay",   nm + "Decay",   "D",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    sustain          = p.addExtParam (id + "sustain", nm + "Sustain", "S",     "%", { 0.0, 100.0, 0.0, 1.0 }, 80.0f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);

    sustain->conversionFunction = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::LFOParams::setup (WavetableAudioProcessor& p, int idx)
{
    juce::String id = "lfo" + juce::String (idx + 1);
    juce::String nm = "LFO" + juce::String (idx + 1) + " ";

    auto notes = gin::NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    sync             = p.addIntParam (id + "sync",    nm + "Sync",    "Sync",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    retrig           = p.addIntParam (id + "retrig",  nm + "Retrig",  "Retrig", "", { 0.0, 1.0, 1.0, 1.0 }, 1.0, 0.0f, enableTextFunction);
    wave             = p.addIntParam (id + "wave",    nm + "Wave",    "Wave",   "", { 1.0, 17.0, 1.0, 1.0 }, 1.0, 0.0f, lfoTextFunction);
    rate             = p.addExtParam (id + "rate",    nm + "Rate",    "Rate",   "Hz", { 0.0, 50.0, 0.0, 0.3f }, 10.0, 0.0f);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 13.0, 0.0f, durationTextFunction);
    depth            = p.addExtParam (id + "depth",   nm + "Depth",   "Depth",  "", { -1.0, 1.0, 0.0, 1.0 }, 1.0, 0.0f);
    phase            = p.addExtParam (id + "phase",   nm + "Phase",   "Phase",  "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    offset           = p.addExtParam (id + "offset",  nm + "Offset",  "Offset", "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    fade             = p.addExtParam (id + "fade",    nm + "Fade",    "Fade",   "s", { -60.0, 60.0, 0.0, 0.2f, true }, 0.0f, 0.0f);
    delay            = p.addExtParam (id + "delay",   nm + "Delay",   "Delay",  "s", { 0.0, 60.0, 0.0, 0.2f }, 0.0f, 0.0f);
    xgrid            = p.addIntParam (id + "xgrid",   nm + "XGrid",   "XGrid",   "", { 2.0, 32.0, 1.0, 1.0 }, 8.0, 0.0f);
    ygrid            = p.addIntParam (id + "ygrid",   nm + "YGrid",   "YGrid",   "", { 2.0, 32.0, 1.0, 1.0 }, 2.0, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::StepLFOParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "slfo";
    juce::String nm = "Step LFO";

    auto notes = gin::NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 13.0, 0.0f, durationTextFunction);
    length           = p.addIntParam (id + "length",  nm + "Length",  "Length", "", { 2.0, 32.0, 1.0, 1.0f }, 8.0f, 0.0f);
    retrig           = p.addIntParam (id + "retrig",  nm + "Retrig",  "Retrig", "", { 0.0, 1.0, 1.0, 1.0 }, 1.0, 0.0f, enableTextFunction);

    for (int i = 0; i < Cfg::numStepLFOSteps; i++)
    {
        auto num = juce::String (i + 1);
        level[i] = p.addIntParam (id + "step" + num,  nm + "Step " + num, "", "", { -1.0, 1.0, 0.0, 1.0f }, 0.0f, 0.0f);
    }
}

//==============================================================================
void WavetableAudioProcessor::GateParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "gate";
    juce::String nm = "Gate";

    auto notes = gin::NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 7.0, 0.0f, durationTextFunction);
    length           = p.addIntParam (id + "length",  nm + "Length",  "Length", "", { 2.0, Cfg::numGateSteps, 1.0, 1.0f }, 8.0f, 0.0f);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",     "s", { 0.0, 1.0, 0.0, 0.2f }, 0.1f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",     "s", { 0.0, 1.0, 0.0, 0.2f }, 0.1f, 0.0f);

    for (int i = 0; i < Cfg::numGateSteps; i++)
    {
        auto num = juce::String (i + 1);
        l[i]     = p.addIntParam (id + "l" + num,  nm + "L " + num, "", "", { 0.0, 1.0, 1.0, 1.0f }, (i % 2 == 0 || i % 5 == 0) ? 1.0f : 0.0f, 0.0f);
        r[i]     = p.addIntParam (id + "r" + num,  nm + "R " + num, "", "", { 0.0, 1.0, 1.0, 1.0f }, (i % 2 == 0 || i % 5 == 0) ? 1.0f : 0.0f, 0.0f);
    }
}

//==============================================================================
void WavetableAudioProcessor::ADSRParams::setup (WavetableAudioProcessor& p)
{
    retrig           = p.addIntParam ("retrig",  "Retrig",  "Retrig","", { 0.0, 1.0, 1.0, 1.0 },     1.0f, 0.0f, enableTextFunction);
    velocityTracking = p.addExtParam ("vel",     "Vel",     "Vel",   "", { 0.0, 100.0, 0.0, 1.0 }, 100.0f, 0.0f);
    attack           = p.addExtParam ("attack",  "Attack",  "A",     "s", { 0.0, 60.0, 0.0, 0.2f },  0.1f, 0.0f);
    decay            = p.addExtParam ("decay",   "Decay",   "D",     "s", { 0.0, 60.0, 0.0, 0.2f },  0.1f, 0.0f);
    sustain          = p.addExtParam ("sustain", "Sustain", "S",     "%", { 0.0, 100.0, 0.0, 1.0 }, 80.0f, 0.0f);
    release          = p.addExtParam ("release", "Release", "R",     "s", { 0.0, 60.0, 0.0, 0.2f },  0.1f, 0.0f);

    sustain->conversionFunction          = [] (float in) { return in / 100.0f; };
    velocityTracking->conversionFunction = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::GlobalParams::setup (WavetableAudioProcessor& p)
{
    mono        = p.addIntParam ("mono",        "Mono",       "",         "",   { 0.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    glideMode   = p.addIntParam ("gMode",       "Glide Mode", "Glide",    "",   { 0.0, 2.0, 0.0, 1.0 }, 0.0f, 0.0f, glideModeTextFunction);
    glideRate   = p.addExtParam ("gRate",       "Glide Time", "Time",     "s",  { 0.001f, 20.0, 0.0, 0.2f }, 0.3f, 0.0f);
    legato      = p.addIntParam ("legato",      "Legato",     "",         "",   { 0.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    level       = p.addExtParam ("level",       "Level",      "",         "db", { -100.0, 0.0, 1.0, 4.0f }, 0.0, 0.0f);
    voices      = p.addIntParam ("voices",      "Voices",     "",         "",   { 2.0, 40.0, 1.0, 1.0 }, 40.0f, 0.0f);
    mpe         = p.addIntParam ("mpe",         "MPE",        "",         "",   { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    pitchBend   = p.addIntParam ("pitchbend",   "Pitch Bend", "PB Range", "",   { 0.0, 48.0, 1.0, 1.0 }, 2.0f, 0.0f);

    level->conversionFunction     = [] (float in) { return juce::Decibels::decibelsToGain (in); };

	if (auto props = p.getSettings())
		mpe->setUserValue (props->getBoolValue ("mpe", false) ? 1.0f : 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::UIParams::setup (WavetableAudioProcessor& p)
{
    activeLFO   = p.addIntParam ("uiLFO",   "LFO", "", "",   { 0.0, 2.0, 0.0, 1.0 }, 0.0, 0.0f);
    activeENV   = p.addIntParam ("uiENV",   "ENV", "", "",   { 0.0, 2.0, 0.0, 1.0 }, 0.0f, 0.0f);
    activeMOD   = p.addIntParam ("uiMOD",   "MOD", "", "",   { 0.0, 1.0, 0.0, 1.0 }, 0.0f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::ChorusParams::setup (WavetableAudioProcessor& p)
{
    enable = p.addIntParam ("chEnable",    "Enable",  "",   "",   { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    delay  = p.addExtParam ("chDelay",     "Delay",   "",   "ms", {0.1f, 30.0f, 0.0f, 1.0f}, 1.0f, 0.0f);
    depth  = p.addExtParam ("chDepth",     "Depth",   "",   "ms", {0.1f, 20.0f, 0.0f, 1.0f}, 1.0f, 0.0f);
    rate   = p.addExtParam ("chSpeed",     "Speed",   "",   "Hz", {0.1f, 10.0f, 0.0f, 1.0f}, 3.0f, 0.0f);
    width  = p.addExtParam ("chWidth",     "Width",   "",   "",   {0.0f, 1.0f,  0.0f, 1.0f}, 0.5f, 0.0f);
    mix    = p.addExtParam ("chMix",       "Mix",     "",   "",   {0.0f, 1.0f,  0.0f, 1.0f}, 0.5f, 0.0f);

    delay->conversionFunction = [] (float in) { return in / 1000.0f; };
    depth->conversionFunction = [] (float in) { return in / 1000.0f; };
}

//==============================================================================
void WavetableAudioProcessor::DistortionParams::setup (WavetableAudioProcessor& p)
{
    enable   = p.addIntParam ("dsEnable",   "Enable",     "",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    amount   = p.addExtParam ("dsAmount",   "Amount",     "",   "", { 0.0, 1.0, 0.0, 1.0 }, 0.2f, 0.0f, distortionAmountTextFunction);
}

//==============================================================================
void WavetableAudioProcessor::DelayParams::setup (WavetableAudioProcessor& p)
{
    enable = p.addIntParam ("dlEnable",    "Enable",     "",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);

    float mxd = float (gin::NoteDuration::getNoteDurations().size()) - 1.0f;

    sync  = p.addExtParam ("dlSync",  "Sync",      "", "",   {   0.0f,   1.0f, 1.0f, 1.0f},    0.0f, 0.0f, enableTextFunction);
    time  = p.addExtParam ("dlTime",  "Delay",     "", "",   {   0.0f, 120.0f, 0.0f, 0.3f},    1.0f, 0.0f);
    beat  = p.addExtParam ("dlBeat",  "Delay",     "", "",   {   0.0f,    mxd, 1.0f, 1.0f},   13.0f, 0.0f, durationTextFunction);
    fb    = p.addExtParam ("dlFb",    "FB",        "", "dB", {-100.0f,   0.0f, 0.0f, 5.0f},  -10.0f, 0.0f);
    cf    = p.addExtParam ("dlCf",    "CF",        "", "dB", {-100.0f,   0.0f, 0.0f, 5.0f}, -100.0f, 0.0f);
    mix   = p.addExtParam ("dlMix",   "Mix",       "", "%",  {   0.0f, 100.0f, 0.0f, 1.0f},    0.5f, 0.0f);

    delay = p.addIntParam ("dlDelay", "Delay",     "", "",   {   0.0f, 120.0f, 0.0f, 1.0f},    1.0f, 0.0f);

    fb->conversionFunction  = [] (float in) { return juce::Decibels::decibelsToGain (in); };
    cf->conversionFunction  = [] (float in) { return juce::Decibels::decibelsToGain (in); };
    mix->conversionFunction = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::ReverbParams::setup (WavetableAudioProcessor& p)
{
    enable     = p.addIntParam ("rvEnable",   "Enable",  "",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f);

    size        = p.addExtParam ("rvbSize",     "Size",     "",   "", {0.0f, 1.0f,    0.0f, 2.0f}, 0.0f, 0.0f);
    decay       = p.addExtParam ("rvbDecay",    "Decay",    "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
    lowpass     = p.addExtParam ("rvbLowpass",  "Lowpass",  "",   "", {16.0f, 20000.0f, 0.0f, 0.3f}, 10000.0f, 0.0f);
    damping     = p.addExtParam ("rvbDamping",  "Damping",  "",   "", {16.0f, 20000.0f, 0.0f, 0.3f}, 10000.0f, 0.0f);
    predelay    = p.addExtParam ("rvbPredelay", "Predelay", "",   "", {0.0f, 0.1f,    0.0f, 1.0f}, 0.0f, 0.0f);
    mix         = p.addExtParam ("rvbMix",      "Mix",      "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::FXParams::setup (WavetableAudioProcessor& p)
{
    fx1         = p.addIntParam ("fxOrder1",   "FX1",       "",   "", { 0.0, 4.0, 1.0, 1.0 }, fxGate,    0.0f);
    fx2         = p.addIntParam ("fxOrder2",   "FX2",       "",   "", { 0.0, 4.0, 1.0, 1.0 }, fxChorus,  0.0f);
    fx3         = p.addIntParam ("fxOrder3",   "FX3",       "",   "", { 0.0, 4.0, 1.0, 1.0 }, fxDistort, 0.0f);
    fx4         = p.addIntParam ("fxOrder4",   "FX4",       "",   "", { 0.0, 4.0, 1.0, 1.0 }, fxDelay,   0.0f);
    fx5         = p.addIntParam ("fxOrder5",   "FX5",       "",   "", { 0.0, 4.0, 1.0, 1.0 }, fxReverb,  0.0f);
    distMode    = p.addIntParam ("distMode",   "Dist Mode", "",   "", { 0.0, 3.0, 1.0, 1.0 }, 0,  0.0f);
}

//==============================================================================
void WavetableAudioProcessor::BitCrusherParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "crush";
    juce::String nm = "Crush ";

    rate = p.addExtParam (id + "rate",  nm + "Rate",  "Rate",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    rez  = p.addExtParam (id + "rez",   nm + "Rez",   "Rez",     "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    hard = p.addExtParam (id + "hard",  nm + "Hard",  "Hard",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.8f, 0.0f);
    mix  = p.addExtParam (id + "mix",   nm + "Mix",   "Mix",     "", { 0.0, 1.0, 0.0, 1.0f }, 1.0f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::FireAmpParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "fire";
    juce::String nm = "Fire ";

    gain   = p.addExtParam (id + "gain",   nm + "Gain",   "Gain",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    tone   = p.addExtParam (id + "tone",   nm + "Tone",   "Tone",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    output = p.addExtParam (id + "output", nm + "Output", "Output",  "", { 0.0, 1.0, 0.0, 1.0f }, 0.8f, 0.0f);
    mix    = p.addExtParam (id + "mix",    nm + "Mix",    "Mix",     "", { 0.0, 1.0, 0.0, 1.0f }, 1.0f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::GrindAmpParams::setup (WavetableAudioProcessor& p)
{
    juce::String id = "grind";
    juce::String nm = "Grind ";

    gain   = p.addExtParam (id + "gain",   nm + "Gain",   "Gain",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    tone   = p.addExtParam (id + "tone",   nm + "Tone",   "Tone",    "", { 0.0, 1.0, 0.0, 1.0f }, 0.5f, 0.0f);
    output = p.addExtParam (id + "output", nm + "Output", "Output",  "", { 0.0, 1.0, 0.0, 1.0f }, 0.8f, 0.0f);
    mix    = p.addExtParam (id + "mix",    nm + "Mix",    "Mix",     "", { 0.0, 1.0, 0.0, 1.0f }, 1.0f, 0.0f);
}

#if 0
void convertWavetables()
{
    auto src = juce::File (__FILE__).getChildFile ("../../Resources/Wavetables");
    if (! src.isDirectory())
        return;

    auto files = src.findChildFiles (juce::File::findFiles, true, "*.wav");
    for (auto srcFile : files)
    {
        int sz = gin::getWavetableSize (srcFile);
        jassert (sz > 0);

        auto path = srcFile.withFileExtension (".wt" + juce::String (sz)).getFullPathName().replace ("Wavetables", "WavetablesFLAC");
        auto dstFile = juce::File (path);

        dstFile.deleteFile();
        dstFile.getParentDirectory().createDirectory();

        if (auto is = srcFile.createInputStream())
        {
            if (auto reader = std::unique_ptr<juce::AudioFormatReader> (juce::WavAudioFormat().createReaderFor (is.release(), true)))
            {
                juce::AudioSampleBuffer buffer (1, int (reader->lengthInSamples));
                reader->read (&buffer, 0, int (reader->lengthInSamples), 0, true, false);

                if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (juce::FlacAudioFormat().createWriterFor (dstFile.createOutputStream().release(), reader->sampleRate, 1, 16, {}, 8)))
                {
                    writer->writeFromAudioReader (*reader, 0, reader->lengthInSamples);
                }
            }
        }
    }
}

void extractWavetables()
{
    auto save = [] (const juce::File& dst, const juce::String& name, const juce::MemoryBlock& mb)
    {
        auto sz = gin::getWavetableSize (mb);
        jassert (sz > 0);
        if (sz <= 0)
            return;

        auto f = dst.getChildFile (name + ".wav");

        f.replaceWithData (mb.getData(), mb.getSize());
    };

	auto src = juce::File ("/Users/rrabien/Downloads/HavenSteps");
    auto dst = juce::File ("/Users/rrabien/Downloads/HavenSteps_out");

    for (auto f : src.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        auto preset = juce::ValueTree::fromXml (f.loadFileAsString());
        auto state = preset.getChildWithName ("state");

        auto name1 = state.getProperty ("wt1");
        auto name2 = state.getProperty ("wt2");

        juce::MemoryBlock mb1;
        juce::MemoryBlock mb2;

        mb1.fromBase64Encoding (state.getProperty ("wt1Data").toString());
        mb2.fromBase64Encoding (state.getProperty ("wt1Data").toString());

        auto changed = false;
        if (mb1.getSize() > 0)
        {
            save (dst, name1, mb1);

            mb1.reset();
            state.setProperty ("wt1Data", mb1.toBase64Encoding(), nullptr);
            changed = true;
        }

        if (mb2.getSize() > 0)
        {
            save (dst, name2, mb2);

            mb2.reset();
            state.setProperty ("wt2Data", mb2.toBase64Encoding(), nullptr);
            changed = true;
        }

        if (changed)
            dst.getChildFile (f.getFileName()).replaceWithText (preset.toXmlString());
        else
            f.copyFileTo (dst.getChildFile (f.getFileName()));
    }
}
#endif

//==============================================================================
WavetableAudioProcessor::WavetableAudioProcessor()
  : gin::Processor (juce::AudioProcessor::BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), false),
    bitcrusher (FXBaseCallback ([this] { return gin::Processor::getSampleRate(); })),
    fireAmp (FXBaseCallback ([this] { return gin::Processor::getSampleRate(); })),
    grindAmp (FXBaseCallback ([this] { return gin::Processor::getSampleRate(); }))
{
   {
        auto sz = 0;
        for (auto i = 0; i < BinaryData::namedResourceListSize; i++)
            if (juce::String (BinaryData::originalFilenames[i]).endsWith (".xml"))
                if (auto data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], sz))
                    extractProgram (BinaryData::originalFilenames[i], data, sz);
    }

	mtsClient = MTS_RegisterClient();
    enableLegacyMode();
    setVoiceStealingEnabled (true);

    osc1Table = "Analog PWM Square 01";
    osc2Table = "Analog PWM Saw 01";

    reloadWavetables();

    for (int i = 0; i < juce::numElementsInArray (oscParams); i++)
        oscParams[i].setup (*this, i);

    subParams.setup (*this);
    noiseParams.setup (*this);
    filterParams.setup (*this);

    for (int i = 0; i < juce::numElementsInArray (envParams); i++)
        envParams[i].setup (*this, i);

    for (int i = 0; i < juce::numElementsInArray (lfoParams); i++)
        lfoParams[i].setup (*this, i);

    stepLfoParams.setup (*this);
    adsrParams.setup (*this);

    globalParams.setup (*this);
    uiParams.setup (*this);
    gateParams.setup (*this);
    chorusParams.setup (*this);
    distortionParams.setup (*this);
    delayParams.setup (*this);
    reverbParams.setup (*this);
    fxParams.setup (*this);

    versionHint++;
    bitcrusherParams.setup (*this);
    fireAmpParams.setup (*this);
    grindAmpParams.setup (*this);

    for (int i = 0; i < 50; i++)
    {
        auto voice = new WavetableVoice (*this);
        modMatrix.addVoice (voice);
        addVoice (voice);
    }

    setupModMatrix();
    init();

    lastMono = globalParams.mono->isOn();

	reset();
}

WavetableAudioProcessor::~WavetableAudioProcessor()
{
	MTS_DeregisterClient (mtsClient);
	mtsClient = nullptr;
}

void WavetableAudioProcessor::reloadWavetables()
{
    auto loadMemory = [&] (const juce::String& name) -> juce::MemoryBlock
    {
        for (auto i = 0; i < BinaryData::namedResourceListSize; i++)
        {
            if ((name + ".wt2048").equalsIgnoreCase (BinaryData::originalFilenames[i]))
            {
                int sz = 0;
                if (auto data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], sz))
                    return juce::MemoryBlock (data, sz);
            }
        }
        return {};
    };

    auto shouldLoad = [&] (int osc, const juce::String& name, double sr)
    {
        auto& v = curTables[osc];
        if (v.name == name && juce::exactlyEqual (v.sampleRate, sr))
            return false;

        v.name = name;
        v.sampleRate = sr;
        return true;
    };

    auto sr = gin::Processor::getSampleRate();
    if (sr == 0)
        return;

    if (userTable1.getSize() > 0)
    {
        if (shouldLoad (0, osc1Table.toString(), sr))
            loadWaveTable (osc1Tables, sr, userTable1, "wav", osc1Size);
    }
    else if (auto mb = loadMemory (osc1Table.toString()); mb.getSize() > 0)
    {
        if (shouldLoad (0, osc1Table.toString(), sr))
            loadWaveTable (osc1Tables, sr, mb, "flac", osc1Size);
    }

    if (userTable2.getSize() > 0)
    {
        if (shouldLoad (1, osc2Table.toString(), sr))
            loadWaveTable (osc2Tables, sr, userTable2, "wav", osc2Size);
    }
    else if (auto mb = loadMemory (osc2Table.toString()); mb.getSize() > 0)
    {
        if (shouldLoad (1, osc2Table.toString(), sr))
            loadWaveTable (osc2Tables, sr, mb, "flac", osc2Size);
    }
}

void WavetableAudioProcessor::incWavetable (int osc, int delta)
{
    auto& table = osc == 0 ? osc1Table : osc2Table;

    if (osc == 0)
        userTable1.reset();
    else
        userTable2.reset();

    juce::StringArray tables;
    for (auto i = 0; i < BinaryData::namedResourceListSize; i++)
        if (juce::String (BinaryData::originalFilenames[i]).endsWith (".wt2048"))
            tables.add (juce::String (BinaryData::originalFilenames[i]).upToLastOccurrenceOf (".wt2048", false, false));

    tables.sortNatural();

    auto idx = tables.indexOf (table.toString());

    idx += delta;
    if (idx < 0) idx = tables.size() - 1;
    if (idx >= tables.size()) idx = 0;

    table = tables[idx];

    reloadWavetables();
}

bool WavetableAudioProcessor::loadUserWavetable (int osc, const juce::File& f, int sz)
{
    auto& table = osc == 0 ? osc1Tables : osc2Tables;
    auto& mb    = osc == 0 ? userTable1 : userTable2;
    auto& name  = osc == 0 ? osc1Table  : osc2Table;
    auto& size  = osc == 0 ? osc1Size   : osc2Size;

    juce::MemoryBlock raw;
    f.loadFileAsData (raw);

    if (loadWaveTable (table, gin::Processor::getSampleRate(), raw, "wav", sz))
    {
        mb = raw;
        name = f.getFileNameWithoutExtension();
        size = sz;
        return true;
    }
    return false;
}

//==============================================================================
void WavetableAudioProcessor::stateUpdated()
{
    modMatrix.stateUpdated (state);

    osc1Table = state.getProperty ("wt1");
    osc2Table = state.getProperty ("wt2");

    osc1Size = state.getProperty ("wt1Size", -1);
    osc2Size = state.getProperty ("wt2Size", -1);

    userTable1.reset();
    userTable2.reset();
    
    userTable1.fromBase64Encoding (state.getProperty ("wt1Data").toString());
    userTable2.fromBase64Encoding (state.getProperty ("wt2Data").toString());

    reloadWavetables();
    presetLoaded = true;
    lastMono = globalParams.mono->isOn();
}

void WavetableAudioProcessor::updateState()
{
    modMatrix.updateState (state);

    state.setProperty ("wt1", osc1Table, nullptr);
    state.setProperty ("wt2", osc2Table, nullptr);

    state.setProperty ("wt1Size", osc1Size, nullptr);
    state.setProperty ("wt2Size", osc2Size, nullptr);

    state.setProperty ("wt1Data", userTable1.toBase64Encoding(), nullptr);
    state.setProperty ("wt2Data", userTable2.toBase64Encoding(), nullptr);
}

//==============================================================================
void WavetableAudioProcessor::setupModMatrix()
{
    modSrcPressure  = modMatrix.addPolyModSource ("mpep", "MPE Pressure", false);
    modSrcTimbre    = modMatrix.addPolyModSource ("mpet", "MPE Timbre", false);
    modSrcPitchbend = modMatrix.addPolyModSource ("mpebp", "MPE Pitch Bend", true);

    modScrPitchBend = modMatrix.addMonoModSource ("pb", "Pitch Bend", true);

    modSrcNote      = modMatrix.addPolyModSource ("note", "MIDI Note Number", false);
    modSrcVelocity  = modMatrix.addPolyModSource ("vel", "MIDI Velocity", false);

    for (int i = 0; i < Cfg::numLFOs; i++)
        modSrcMonoLFO.add (modMatrix.addMonoModSource (juce::String::formatted ("mlfo%d", i + 1), juce::String::formatted ("LFO %d (Mono)", i + 1), true));

    for (int i = 0; i < Cfg::numLFOs; i++)
        modSrcLFO.add (modMatrix.addPolyModSource (juce::String::formatted ("lfo%d", i + 1), juce::String::formatted ("LFO %d", i + 1), true));

    modSrcMonoStep = modMatrix.addMonoModSource ("mstep", "Step LFO (Mono)", true);
    modSrcStep     = modMatrix.addPolyModSource ("step", "Step LFO", true);
    modSrcFilter   = modMatrix.addPolyModSource ("fenv", "Filter Envelope", false);

    for (int i = 0; i < Cfg::numENVs; i++)
        modSrcEnv.add (modMatrix.addPolyModSource (juce::String::formatted ("env%d", i + 1), juce::String::formatted ("Envelope %d", i + 1), false));

    for (int i = 0; i <= 119; i++)
    {
        juce::String name = juce::MidiMessage::getControllerName (i);
        if (name.isEmpty())
            modSrcCC.add (modMatrix.addMonoModSource (juce::String::formatted ("cc%d", i), juce::String::formatted ("CC %d", i), false));
        else
            modSrcCC.add (modMatrix.addMonoModSource (juce::String::formatted ("cc%d", i), juce::String::formatted ("CC %d ", i) + name, false));
    }
    
    auto firstMonoParam = globalParams.mono;
    bool polyParam = true;
    for (auto pp : getPluginParameters())
    {
        if (pp == firstMonoParam)
            polyParam = false;

        if (! pp->isInternal() || pp == delayParams.delay)
            modMatrix.addParameter (pp, polyParam, getSmoothingTime (pp));
    }

    modMatrix.build();
}

bool WavetableAudioProcessor::isParamLocked (gin::Parameter* p)
{
    if (p == uiParams.activeMOD) return true;
    if (p == uiParams.activeLFO) return true;
    if (p == uiParams.activeENV) return true;

    return false;
}

float WavetableAudioProcessor::getSmoothingTime (gin::Parameter*)
{
    return 0.02f;
}

void WavetableAudioProcessor::reset()
{
    Processor::reset();

    gate.reset();
    chorus.reset();
    stereoDelay.reset();
    reverb.reset();
	bitcrusher.reset();
	fireAmp.reset();
	grindAmp.reset();

    for (auto& l : modLFOs)
        l.reset();

    modStepLFO.reset();

    reloadWavetables();
}

void WavetableAudioProcessor::prepareToPlay (double newSampleRate, int newSamplesPerBlock)
{
    Processor::prepareToPlay (newSampleRate, newSamplesPerBlock);

    setCurrentPlaybackSampleRate (newSampleRate);

    modMatrix.setSampleRate (newSampleRate);

    gate.setSampleRate (newSampleRate);
    chorus.setSampleRate (newSampleRate);
    stereoDelay.setSampleRate (newSampleRate);
    reverb.setSampleRate (float (newSampleRate));

    for (auto& l : modLFOs)
        l.setSampleRate (newSampleRate);

    modStepLFO.setSampleRate (newSampleRate);

    reloadWavetables();
    analogTables.setSampleRate (newSampleRate);
}

void WavetableAudioProcessor::releaseResources()
{
}

bool WavetableAudioProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    if (layout.inputBuses.size() != 0 || layout.outputBuses.size() != 1)
        return false;
    
    return layout.outputBuses[0].size() == 2;
}

void WavetableAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (! dspLock.tryEnter())
    {
        blockMissed = true;
        return;
    }
    
    if (buffer.getNumChannels() != 2)
        return;

	if (mtsClient)
		for (auto itr : midi)
			if (auto m = itr.getMessage(); m.isSysEx())
				MTS_ParseMIDIDataU (mtsClient, itr.data, itr.numBytes);

    if (blockMissed || presetLoaded || lastMono != globalParams.mono->isOn())
    {
        blockMissed = presetLoaded = false;
        lastMono = globalParams.mono->isOn();
        stereoDelay.reset();
        reverb.reset();
        turnOffAllVoices (false);
    }

    startBlock();
    setMPE (globalParams.mpe->isOn());
    setPitchBendRange (globalParams.pitchBend->getUserValueInt());

    playhead = getPlayHead();

    int pos = 0;
    int todo = buffer.getNumSamples();

    buffer.clear();

    setMono (lastMono);
    setLegato (globalParams.legato->isOn());
    setGlissando (globalParams.glideMode->getProcValue() == 1.0f);
    setPortamento (globalParams.glideMode->getProcValue() == 2.0f);
    setGlideRate (globalParams.glideRate->getProcValue());
    setNumVoices (int (globalParams.voices->getProcValue()));

    while (todo > 0)
    {
        int thisBlock = std::min (todo, 32);

        updateParams (thisBlock);

        renderNextBlock (buffer, midi, pos, thisBlock);

        auto bufferSlice = gin::sliceBuffer (buffer, pos, thisBlock);
        applyEffects (bufferSlice);

        modMatrix.finishBlock (thisBlock);

        pos += thisBlock;
        todo -= thisBlock;
    }

    playhead = nullptr;

    if (buffer.getNumSamples() <= scopeFifo.getFreeSpace() && buffer.getNumChannels() == scopeFifo.getNumChannels())
        scopeFifo.write (buffer);

    endBlock (buffer.getNumSamples());
       
    dspLock.exit();
}

juce::Array<float> WavetableAudioProcessor::getLiveFilterCutoff()
{
    juce::Array<float> values;

    for (auto v : voices)
    {
        if (v->isActive())
        {
            auto vav = dynamic_cast<WavetableVoice*>(v);
            values.add (vav->getFilterCutoffNormalized());
        }
    }
    return values;
}

gin::WTOscillator::Params WavetableAudioProcessor::getLiveWTParams (int osc)
{
    for (auto v : voices)
        if (v->isActive())
            if (auto vav = dynamic_cast<WavetableVoice*>(v))
                return vav->getLiveWTParams (osc);

    gin::WTOscillator::Params p;
    p.position  = oscParams[osc].pos->getUserValue() / 100.0f;
    p.formant   = oscParams[osc].formant->getUserValue();
    p.bend      = oscParams[osc].bend->getUserValue();
    return p;
}

void WavetableAudioProcessor::applyEffect (juce::AudioSampleBuffer& buffer, int fxId)
{
    // Apply gate
    if (fxId == fxGate && gateParams.enable->isOn())
        gate.process (buffer, noteOnIndex, noteOffIndex);

    // Apply Chorus
    if (fxId == fxChorus && chorusParams.enable->isOn())
        chorus.process (buffer);

    // Apply Distortion
    if (fxId == fxDistort && distortionParams.enable->isOn())
    {
        auto mode = fxParams.distMode->getUserValueInt();
        if (mode == 0)
        {
            auto clip = 1.0f / (2.0f * distortionVal);
            gin::Distortion::processBlock (buffer, distortionVal, -clip, clip);
        }
        else if (mode == 1)
        {
            bitcrusher.processReplacing ((float**)buffer.getArrayOfWritePointers(), (float**)buffer.getArrayOfWritePointers(), buffer.getNumSamples());
        }
        else if (mode == 2)
        {
            fireAmp.processReplacing ((float**)buffer.getArrayOfWritePointers(), (float**)buffer.getArrayOfWritePointers(), buffer.getNumSamples());
        }
        else if (mode == 3)
        {
            grindAmp.processReplacing ((float**)buffer.getArrayOfWritePointers(), (float**)buffer.getArrayOfWritePointers(), buffer.getNumSamples());
        }
    }

    // Apply Delay
    if (fxId == fxDelay && delayParams.enable->isOn())
    {
        if (delayParams.sync->isOn())
            stereoDelay.process (buffer);
        else
            stereoDelay.processSmoothed (buffer);
    }

    // Apply Reverb
    if (fxId == fxReverb && reverbParams.enable->isOn())
        reverb.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples ());
}

void WavetableAudioProcessor::applyEffects (juce::AudioSampleBuffer& buffer)
{
    applyEffect (buffer, fxParams.fx1->getUserValueInt());
    applyEffect (buffer, fxParams.fx2->getUserValueInt());
    applyEffect (buffer, fxParams.fx3->getUserValueInt());
    applyEffect (buffer, fxParams.fx4->getUserValueInt());
    applyEffect (buffer, fxParams.fx5->getUserValueInt());

    // Output gain
    outputGain.process (buffer);
}

void WavetableAudioProcessor::updateParams (int newBlockSize)
{
    // Update Mono LFOs
    for (int i = 0; i < Cfg::numLFOs; i++)
    {
        if (lfoParams[i].enable->isOn())
        {
            gin::LFO::Parameters params;

            float freq = 0;
            if (lfoParams[i].sync->getProcValue() > 0.0f)
                freq = 1.0f / gin::NoteDuration::getNoteDurations()[size_t (lfoParams[i].beat->getProcValue())].toSeconds (playhead);
            else
                freq = modMatrix.getValue (lfoParams[i].rate);

            params.waveShape = (gin::LFO::WaveShape) int (lfoParams[i].wave->getProcValue());
            params.frequency = freq;
            params.phase     = modMatrix.getValue (lfoParams[i].phase);
            params.offset    = modMatrix.getValue (lfoParams[i].offset);
            params.depth     = modMatrix.getValue (lfoParams[i].depth);
            params.delay     = 0;
            params.fade      = 0;

            modLFOs[i].setParameters (params);
            modLFOs[i].process (newBlockSize);

            modMatrix.setMonoValue (modSrcMonoLFO[i], modLFOs[i].getOutput());
        }
        else
        {
            modMatrix.setMonoValue (modSrcMonoLFO[i], 0);
        }
    }

    // Update Mono Step LFO
    if (stepLfoParams.enable->isOn())
    {
        float freq = 1.0f / gin::NoteDuration::getNoteDurations()[size_t (stepLfoParams.beat->getProcValue())].toSeconds (playhead);

        modStepLFO.setFreq (freq);

        int n = int (stepLfoParams.length->getProcValue());
        modStepLFO.setNumPoints (n);
        for (int i = n; --i >= 0;)
            modStepLFO.setPoint (i, stepLfoParams.level[i]->getProcValue());

        modStepLFO.process (newBlockSize);

        modMatrix.setMonoValue (modSrcMonoStep, modStepLFO.getOutput());
    }
    else
    {
        modMatrix.setMonoValue (modSrcMonoStep, 0);
    }

    // Update Gate
    if (gateParams.enable->isOn())
    {
        float freq = 1.0f / gin::NoteDuration::getNoteDurations()[size_t (gateParams.beat->getProcValue())].toSeconds (playhead);

        int n = int (gateParams.length->getProcValue());

        gate.setLength (n);

        for (int i = 0; i < n; i++)
            gate.setStep (i, gateParams.l[i]->isOn(), gateParams.r[i]->isOn());

        gate.setFrequency (freq);
        gate.setAttack (modMatrix.getValue (gateParams.attack));
        gate.setRelease (modMatrix.getValue (gateParams.release));
    }

    // Update Chorus
    if (chorusParams.enable->isOn())
    {
        chorus.setParams (modMatrix.getValue (chorusParams.delay),
                          modMatrix.getValue (chorusParams.rate),
                          modMatrix.getValue (chorusParams.depth),
                          modMatrix.getValue (chorusParams.width),
                          modMatrix.getValue (chorusParams.mix));
    }

    // Update Distortion
    if (distortionParams.enable->isOn())
    {
        auto mode = fxParams.distMode->getUserValueInt();
        if (mode == 0)
        {
            distortionVal = modMatrix.getValue (distortionParams.amount);
        }
        else if (mode == 1)
        {
            bitcrusher.setParameter (0, modMatrix.getValue (bitcrusherParams.rez));
            bitcrusher.setParameter (1, modMatrix.getValue (bitcrusherParams.rate));
            bitcrusher.setParameter (2, modMatrix.getValue (bitcrusherParams.hard));
            bitcrusher.setParameter (3, modMatrix.getValue (bitcrusherParams.mix));
        }
        else if (mode == 2)
        {
            fireAmp.setParameter (0, modMatrix.getValue (fireAmpParams.gain));
            fireAmp.setParameter (1, modMatrix.getValue (fireAmpParams.tone));
            fireAmp.setParameter (2, modMatrix.getValue (fireAmpParams.output));
            fireAmp.setParameter (3, modMatrix.getValue (fireAmpParams.mix));
        }
        else if (mode == 3)
        {
            grindAmp.setParameter (0, modMatrix.getValue (grindAmpParams.gain));
            grindAmp.setParameter (1, modMatrix.getValue (grindAmpParams.tone));
            grindAmp.setParameter (2, modMatrix.getValue (grindAmpParams.output));
            grindAmp.setParameter (3, modMatrix.getValue (grindAmpParams.mix));
        }
    }

    // Update Delay
    if (delayParams.enable->isOn())
    {
        if (delayParams.sync->isOn())
        {
            auto& duration = gin::NoteDuration::getNoteDurations()[(size_t)modMatrix.getValue (delayParams.beat)];
            delayParams.delay->setUserValue (duration.toSeconds (getPlayHead()));
            stereoDelay.setParams (delayParams.delay->getUserValue(),
                                   modMatrix.getValue (delayParams.mix),
                                   modMatrix.getValue (delayParams.fb),
                                   modMatrix.getValue (delayParams.cf));
        }
        else
        {
            delayParams.delay->setUserValue (modMatrix.getValue (delayParams.time));
            stereoDelay.setParams (modMatrix.getValue (delayParams.delay),
                                   modMatrix.getValue (delayParams.mix),
                                   modMatrix.getValue (delayParams.fb),
                                   modMatrix.getValue (delayParams.cf));
        }
    }

    // Update Reverb
    if (reverbParams.enable->isOn())
    {
        reverb.setSize (modMatrix.getValue (reverbParams.size));
        reverb.setDecay (modMatrix.getValue (reverbParams.decay));
        reverb.setLowpass (modMatrix.getValue (reverbParams.lowpass));
        reverb.setDamping (modMatrix.getValue (reverbParams.damping));
        reverb.setPredelay (modMatrix.getValue (reverbParams.predelay));
        reverb.setMix (modMatrix.getValue (reverbParams.mix));
    }

    // Output gain
    outputGain.setGain (modMatrix.getValue (globalParams.level));
}

bool WavetableAudioProcessor::loadWaveTable (juce::OwnedArray<gin::BandLimitedLookupTable>& table, double sr, const juce::MemoryBlock& wav, const juce::String& format, int size)
{
    auto is = new juce::MemoryInputStream (wav, false);

    if (format == "wav")
    {
        if (auto reader = std::unique_ptr<juce::AudioFormatReader> (juce::WavAudioFormat().createReaderFor (is, true)))
        {
            if (size <= 0)
                size = gin::getWavetableSize (wav);

            if (size > 0)
            {
                int samplesToUse = int (reader->lengthInSamples);
                int frames = samplesToUse / size;

                samplesToUse = frames * size;

                juce::AudioSampleBuffer buf (1, samplesToUse);
                reader->read (&buf, 0, samplesToUse, 0, true, false);

                juce::OwnedArray<gin::BandLimitedLookupTable> t;
                loadWavetables (t, sr, buf, reader->sampleRate, size);

                juce::ScopedLock sl (dspLock);
                std::swap (t, table);

                return true;
            }
        }
    }
    else if (format == "flac")
    {
        if (auto reader = std::unique_ptr<juce::AudioFormatReader> (juce::FlacAudioFormat().createReaderFor (is, true)))
        {
            juce::AudioSampleBuffer buf (1, int (reader->lengthInSamples));
            reader->read (&buf, 0, int (reader->lengthInSamples), 0, true, false);

            juce::OwnedArray<gin::BandLimitedLookupTable> t;
            loadWavetables (t, sr, buf, reader->sampleRate, 2048);
            
            juce::ScopedLock sl (dspLock);
            std::swap (t, table);
            
            return true;
        }
    }

    return false;
}

void WavetableAudioProcessor::handleMidiEvent (const juce::MidiMessage& m)
{
    gin::Synthesiser::handleMidiEvent (m);

    if (m.isPitchWheel())
        modMatrix.setMonoValue (modScrPitchBend, float (m.getPitchWheelValue()) / 0x2000 - 1.0f);
}

void WavetableAudioProcessor::handleController ([[maybe_unused]] int ch, int num, int val)
{
	if (num >= 0 && num <= 119)
		modMatrix.setMonoValue (modSrcCC[num], val / 127.0f);
}

//==============================================================================
bool WavetableAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* WavetableAudioProcessor::createEditor()
{
    return new gin::ScaledPluginEditor (new WavetableAudioProcessorEditor (*this), state);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WavetableAudioProcessor();
}
