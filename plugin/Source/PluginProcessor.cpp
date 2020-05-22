#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WavetableVoice.h"

using namespace gin;

static String waveTextFunction (const Parameter&, float v)
{
    switch ((Wave)int (v))
    {
        case Wave::sine:        return "Sine";
        case Wave::triangle:    return "Triangle";
        case Wave::sawUp:       return "Saw (Up)";
        case Wave::sawDown:     return "Saw (Down)";
        case Wave::pulse:       return "Pulse";
        case Wave::square:      return "Square";
        case Wave::noise:       return "Noise";
        default:
            jassertfalse;
            return {};
    }
}

static String lfoTextFunction (const Parameter&, float v)
{
    switch ((LFO::WaveShape)int (v))
    {
        case LFO::WaveShape::none:          return "None";
        case LFO::WaveShape::sine:          return "Sine";
        case LFO::WaveShape::triangle:      return "Triangle";
        case LFO::WaveShape::sawUp:         return "Saw Up";
        case LFO::WaveShape::sawDown:       return "Saw Down";
        case LFO::WaveShape::square:        return "Square";
        case LFO::WaveShape::squarePos:     return "Square+";
        case LFO::WaveShape::sampleAndHold: return "S&H";
        case LFO::WaveShape::noise:         return "Noise";
        case LFO::WaveShape::stepUp3:       return "Step Up 3";
        case LFO::WaveShape::stepUp4:       return "Step Up 4";
        case LFO::WaveShape::stepup8:       return "Step Up 8";
        case LFO::WaveShape::stepDown3:     return "Step Down 3";
        case LFO::WaveShape::stepDown4:     return "Step Down 4";
        case LFO::WaveShape::stepDown8:     return "Step Down 8";
        case LFO::WaveShape::pyramid3:      return "Pyramid 3";
        case LFO::WaveShape::pyramid5:      return "Pyramid 5";
        case LFO::WaveShape::pyramid9:      return "Pyramid 9";
        default:
            jassertfalse;
            return {};
    }
}

static String enableTextFunction (const Parameter&, float v)
{
    return v > 0.0f ? "On" : "Off";
}

static String durationTextFunction (const Parameter&, float v)
{
    return NoteDuration::getNoteDurations()[size_t (v)].getName();
}

static String distortionAmountTextFunction (const Parameter&, float v)
{
    return String (v * 5.0f - 1.0f, 1);
}

static String filterTextFunction (const Parameter&, float v)
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

static String freqTextFunction (const Parameter&, float v)
{
    return String (int (getMidiNoteInHertz (v)));
}

static String glideModeTextFunction (const Parameter&, float v)
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
void WavetableAudioProcessor::WTParams::setup (WavetableAudioProcessor& p, int idx)
{
    String id = "wt" + String (idx + 1);
    String nm = "WT" + String (idx + 1) + " ";

    enable     = p.addIntParam (id + "enable",     nm + "Enable",      "Enable",    "", { 0.0, 1.0, 1.0, 1.0 }, idx == 0 ? 1.0f : 0.0f, 0.0f);
	table      = p.addExtParam (id + "table",      nm + "Table",       "Table",     "", { 0.0, 1.0, 0.0, 1.0 }, 0.0f, 0.0f);
    voices     = p.addIntParam (id + "unison",     nm + "Unison",      "Unison",    "", { 1.0, 8.0, 1.0, 1.0 }, 1.0, 0.0f);
    voicesTrns = p.addExtParam (id + "unisontrns", nm + "Unison Trns", "LTrans",    "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    tune       = p.addExtParam (id + "tune",       nm + "Tune",        "Tune",      "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    finetune   = p.addExtParam (id + "finetune",   nm + "Fine Tune",   "Fine",      "ct", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    level      = p.addExtParam (id + "level",      nm + "Level",       "Level",     "db", { -100.0, 0.0, 1.0, 4.0 }, 0.0, 0.0f);
    detune     = p.addExtParam (id + "detune",     nm + "Detune",      "Detune",    "", { 0.0, 0.5, 0.0, 1.0 }, 0.0, 0.0f);
    spread     = p.addExtParam (id + "spread",     nm + "Spread",      "Spread",    "%", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    pan        = p.addExtParam (id + "pan",        nm + "Pan",         "Pan",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);

    level->conversionFunction = [] (float in)   { return Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::OSCParams::setup (WavetableAudioProcessor& p, int idx)
{
    String id = "osc" + String (idx + 1);
    String nm = "OSC" + String (idx + 1) + " ";

    enable     = p.addIntParam (id + "enable",     nm + "Enable",      "Enable",    "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f);
    wave       = p.addIntParam (id + "wave",       nm + "Wave",        "Wave",      "", { 1.0, 7.0, 1.0, 1.0 }, 1.0, 0.0f, waveTextFunction);
    voices     = p.addIntParam (id + "unison",     nm + "Unison",      "Unison",    "", { 1.0, 8.0, 1.0, 1.0 }, 1.0, 0.0f);
    voicesTrns = p.addExtParam (id + "unisontrns", nm + "Unison Trns", "LTrans",    "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    tune       = p.addExtParam (id + "tune",       nm + "Tune",        "Tune",      "st", { -36.0, 36.0, 1.0, 1.0 }, 0.0, 0.0f);
    finetune   = p.addExtParam (id + "finetune",   nm + "Fine Tune",   "Fine",      "ct", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    level      = p.addExtParam (id + "level",      nm + "Level",       "Level",     "db", { -100.0, 0.0, 1.0, 4.0 }, 0.0, 0.0f);
    pulsewidth = p.addExtParam (id + "pulsewidth", nm + "Pulse Width", "PW",        "%", { 1.0, 99.0, 0.0, 1.0 }, 50.0, 0.0f);
    detune     = p.addExtParam (id + "detune",     nm + "Detune",      "Detune",    "", { 0.0, 0.5, 0.0, 1.0 }, 0.0, 0.0f);
    spread     = p.addExtParam (id + "spread",     nm + "Spread",      "Spread",    "%", { -100.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    pan        = p.addExtParam (id + "pan",        nm + "Pan",         "Pan",       "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);

    level->conversionFunction = [] (float in)   { return Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::FilterParams::setup (WavetableAudioProcessor& p, int idx)
{
    String id = "flt" + String (idx + 1);
    String nm = "FLT" + String (idx + 1) + " ";

    float maxFreq = float (getMidiNoteFromHertz (20000.0));

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "",      "", { 0.0, 1.0, 1.0, 1.0 }, idx == 0 ? 1.0f : 0.0f, 0.0f);
    type             = p.addIntParam (id + "type",    nm + "Type",    "Type",  "", { 0.0, 7.0, 1.0, 1.0 }, 0.0, 0.0f, filterTextFunction);
    keyTracking      = p.addExtParam (id + "key",     nm + "Key",     "Key",   "%", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    velocityTracking = p.addExtParam (id + "vel",     nm + "Vel",     "Vel",   "%", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    frequency        = p.addExtParam (id + "freq",    nm + "Freq",    "Freq",  "Hz", { 0.0, maxFreq, 0.0, 1.0 }, 64.0, 0.0f, freqTextFunction);
    resonance        = p.addExtParam (id + "res",     nm + "Res",     "Res",   "", { 0.0, 100.0, 0.0, 1.0 }, 0.0, 0.0f);
    amount           = p.addExtParam (id + "amount",  nm + "Amount",  "Amnt",  "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    decay            = p.addExtParam (id + "decay",   nm + "Decay",   "D",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    sustain          = p.addExtParam (id + "sustain", nm + "Sustain", "S",     "%", { 0.0, 100.0, 0.0, 1.0 }, 80.0f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);

    sustain->conversionFunction          = [] (float in) { return in / 100.0f; };
    velocityTracking->conversionFunction = [] (float in) { return in / 100.0f; };
    keyTracking->conversionFunction      = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::EnvParams::setup (WavetableAudioProcessor& p, int idx)
{
    String id = "env" + String (idx + 1);
    String nm = "ENV" + String (idx + 1) + " ";

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    decay            = p.addExtParam (id + "decay",   nm + "Decay",   "D",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    sustain          = p.addExtParam (id + "sustain", nm + "Sustain", "S",     "%", { 0.0, 100.0, 0.0, 1.0 }, 80.0f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);

    sustain->conversionFunction = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::LFOParams::setup (WavetableAudioProcessor& p, int idx)
{
    String id = "lfo" + String (idx + 1);
    String nm = "LFO" + String (idx + 1) + " ";

    auto notes = NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    sync             = p.addIntParam (id + "sync",    nm + "Sync",    "Sync",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    wave             = p.addIntParam (id + "wave",    nm + "Wave",    "Wave",   "", { 1.0, 17.0, 1.0, 1.0 }, 1.0, 0.0f, lfoTextFunction);
    rate             = p.addExtParam (id + "rate",    nm + "Rate",    "Rate",   "Hz", { 0.0, 50.0, 0.0, 0.3f }, 10.0, 0.0f);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 13.0, 0.0f, durationTextFunction);
    depth            = p.addExtParam (id + "depth",   nm + "Depth",   "Depth",  "", { -1.0, 1.0, 0.0, 1.0 }, 1.0, 0.0f);
    phase            = p.addExtParam (id + "phase",   nm + "Phase",   "Phase",  "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    offset           = p.addExtParam (id + "offset",  nm + "Offset",  "Offset", "", { -1.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    fade             = p.addExtParam (id + "fade",    nm + "Fade",    "Fade",   "s", { -60.0, 60.0, 0.0, 0.2f, true }, 0.1f, 0.0f);
    delay            = p.addExtParam (id + "delay",   nm + "Delay",   "Delay",  "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::StepLFOParams::setup (WavetableAudioProcessor& p)
{
    String id = "slfo";
    String nm = "Step LFO";

    auto notes = NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 13.0, 0.0f, durationTextFunction);
    length           = p.addIntParam (id + "length",  nm + "Length",   "Length", "", { 2.0, 32.0, 1.0, 1.0f }, 8.0f, 0.0f);

    for (int i = 0; i < 32; i++)
    {
        auto num = String (i + 1);
        level[i]     = p.addIntParam (id + "step" + num,  nm + "Step " + num, "", "", { -1.0, 1.0, 0.0, 1.0f }, 0.0f, 0.0f);
    }
}

//==============================================================================
void WavetableAudioProcessor::GateParams::setup (WavetableAudioProcessor& p)
{
    String id = "gate";
    String nm = "Gate";

    auto notes = NoteDuration::getNoteDurations();

    enable           = p.addIntParam (id + "enable",  nm + "Enable",  "Enable", "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);
    beat             = p.addIntParam (id + "beat",    nm + "Beat",    "Beat",   "", { 0.0, float (notes.size() - 1), 1.0, 1.0 }, 7.0, 0.0f, durationTextFunction);
    length           = p.addIntParam (id + "length",  nm + "Length",  "Length", "", { 2.0, 32.0, 1.0, 1.0f }, 8.0f, 0.0f);
    attack           = p.addExtParam (id + "attack",  nm + "Attack",  "A",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);
    release          = p.addExtParam (id + "release", nm + "Release", "R",     "s", { 0.0, 60.0, 0.0, 0.2f }, 0.1f, 0.0f);

    for (int i = 0; i < 32; i++)
    {
        auto num = String (i + 1);
        l[i]     = p.addIntParam (id + "l" + num,  nm + "L " + num, "", "", { 0.0, 1.0, 1.0, 1.0f }, (i % 2 == 0 || i % 5 == 0) ? 1.0f : 0.0f, 0.0f);
        r[i]     = p.addIntParam (id + "r" + num,  nm + "R " + num, "", "", { 0.0, 1.0, 1.0, 1.0f }, (i % 2 == 0 || i % 5 == 0) ? 1.0f : 0.0f, 0.0f);
    }
}

//==============================================================================
void WavetableAudioProcessor::ADSRParams::setup (WavetableAudioProcessor& p)
{
    velocityTracking = p.addExtParam ("vel",     "Vel",     "Vel",   "", { 0.0, 100.0, 0.0, 1.0 }, 100.0, 0.0f);
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
    mono        = p.addIntParam ("mono",    "Mono",       "",      "",   { 0.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    glideMode   = p.addIntParam ("gMode",   "Glide Mode", "Glide", "",   { 0.0, 2.0, 0.0, 1.0 }, 0.0f, 0.0f, glideModeTextFunction);
    glideRate   = p.addExtParam ("gRate",   "Glide Rate", "Rate",  "s",   { 0.001f, 20.0, 0.0, 0.2f }, 0.3f, 0.0f);
    legato      = p.addIntParam ("legato",  "Legato",     "",      "",   { 0.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f, enableTextFunction);
    level       = p.addExtParam ("level",   "Level",      "",      "db", { -100.0, 0.0, 1.0, 4.0f }, 0.0, 0.0f);
    voices      = p.addIntParam ("voices",  "Voices",     "",      "",   { 2.0, 40.0, 1.0, 1.0 }, 40.0f, 0.0f);
    mpe         = p.addIntParam ("mpe",     "MPE",        "",      "",   { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);

    level->conversionFunction     = [] (float in) { return Decibels::decibelsToGain (in); };
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
    highpass = p.addExtParam ("dsHighpass", "Highpass",   "",   "", { 0.0, 1.0, 0.0, 1.0 }, 0.0, 0.0f);
    output   = p.addExtParam ("dsOutput",   "Output",     "",   "", { 0.0, 1.0, 0.0, 1.0 }, 1.0, 0.0f);
    mix      = p.addExtParam ("dsMix",      "Mix",        "",   "", { 0.0, 1.0, 0.0, 1.0 }, 1.0, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::EQParams::setup (WavetableAudioProcessor& p)
{
    float maxFreq = float (getMidiNoteFromHertz (20000.0));
    float d1 = float (getMidiNoteFromHertz (80.0));
    float d2 = float (getMidiNoteFromHertz (3000.0));
    float d3 = float (getMidiNoteFromHertz (5000.0));
    float d4 = float (getMidiNoteFromHertz (17000.0));

    enable   = p.addIntParam ("eqEnable",    "Enable",      "",     "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);

    loFreq   = p.addExtParam ("eqLoFreq",    "Lo Freq",     "Freq", "Hz", { 0.0, maxFreq, 0.0, 1.0 }, d1, 0.0f, freqTextFunction);
    loQ      = p.addExtParam ("eqLoQ",       "Lo Q",        "Q",    "", { 0.025f, 40.0f, 0.0, 0.2f }, 1.0f, 0.0f);
    loGain   = p.addExtParam ("eqLoGain",    "Lo Gain",     "Gain", "dB", { -20.0f, 20.0f, 0.0, 1.0 }, 0.0f, 0.0f);

    mid1Freq = p.addExtParam ("eqM1Freq",    "Min 1 Freq",  "Freq", "Hz", { 0.0, maxFreq, 0.0, 1.0 }, d2, 0.0f, freqTextFunction);
    mid1Q    = p.addExtParam ("eqM1Q",       "Min 1 Q",     "Q",    "", { 0.025f, 40.0f, 0.0, 0.2f }, 1.0f, 0.0f);
    mid1Gain = p.addExtParam ("eqM1Gain",    "Min 1 Gain",  "Gain", "dB", { -20.0f, 20.0f, 0.0, 1.0 }, 0.0f, 0.0f);

    mid2Freq = p.addExtParam ("eqM2Freq",    "Mid 2 Freq",  "Freq", "Hz", { 0.0, maxFreq, 0.0, 1.0 }, d3, 0.0f, freqTextFunction);
    mid2Q    = p.addExtParam ("eqM2Q",       "Mid 2 Q",     "Q",    "", { 0.025f, 40.0f, 0.0, 0.2f }, 1.0f, 0.0f);
    mid2Gain = p.addExtParam ("eqM2Gain",    "Mid 2 Gain",  "Gain", "dB", { -20.0f, 20.0f, 0.0, 1.0 }, 0.0f, 0.0f);

    hiFreq   = p.addExtParam ("eqHiFreq",    "Hi Freq",     "Freq", "Hz", { 0.0, maxFreq, 0.0, 1.0 }, d4, 0.0f, freqTextFunction);
    hiQ      = p.addExtParam ("eqHiQ",       "Hi Q",        "Q",    "", { 0.025f, 40.0f, 0.0, 0.2f }, 1.0f, 0.0f);
    hiGain   = p.addExtParam ("eqHiGain",    "Hi Gain",     "Gain", "dB", { -20.0f, 20.0f, 0.0, 1.0 }, 0.0f, 0.0f);

    loFreq->conversionFunction   = [] (float in) { return getMidiNoteInHertz (in); };
    mid1Freq->conversionFunction = [] (float in) { return getMidiNoteInHertz (in); };
    mid2Freq->conversionFunction = [] (float in) { return getMidiNoteInHertz (in); };
    hiFreq->conversionFunction   = [] (float in) { return getMidiNoteInHertz (in); };

    loGain->conversionFunction   = [] (float in) { return Decibels::decibelsToGain (in); };
    mid1Gain->conversionFunction = [] (float in) { return Decibels::decibelsToGain (in); };
    mid2Gain->conversionFunction = [] (float in) { return Decibels::decibelsToGain (in); };
    hiGain->conversionFunction   = [] (float in) { return Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::CompressorParams::setup (WavetableAudioProcessor& p)
{
    enable    = p.addIntParam ("cpEnable",    "Enable",    "", "",     { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);

    attack    = p.addExtParam ("cpAttack",    "Attack",    "", "ms",   { 1.0f,   200.0f, 0.0f, 0.4f},    1.0f, 0.1f);
    release   = p.addExtParam ("cpRelease",   "Release",   "", "ms",   { 1.0f,  2000.0f, 0.0f, 0.4f},    5.0f, 0.1f);
    ratio     = p.addExtParam ("cpRatio",     "Ratio",     "", "",     { 1.0f,    30.0f, 0.0f, 0.4f},    5.0f, 0.1f);
    threshold = p.addExtParam ("cpThreshold", "Thresh",    "", "dB",     { -60.0f,   0.0f, 0.0f, 1.0f},  -30.0f, 0.1f);
    gain      = p.addExtParam ("cpGain",      "Gain",      "", "dB",     { -30.0f,  30.0f, 0.0f, 1.0f},    0.0f, 0.1f);

    attack->conversionFunction  = [] (float in) { return in / 1000.0f; };
    release->conversionFunction = [] (float in) { return in / 1000.0f; };
    gain->conversionFunction    = [] (float in) { return Decibels::decibelsToGain (in); };
}

//==============================================================================
void WavetableAudioProcessor::DelayParams::setup (WavetableAudioProcessor& p)
{
    enable = p.addIntParam ("dlEnable",    "Enable",     "",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0f, 0.0f, enableTextFunction);

    float mxd = float (NoteDuration::getNoteDurations().size()) - 1.0f;

    sync  = p.addExtParam ("dlSync",  "Sync",      "", "",   {   0.0f,   1.0f, 1.0f, 1.0f},    0.0f, 0.0f, enableTextFunction);
    time  = p.addExtParam ("dlTime",  "Delay",     "", "",   {   0.0f, 120.0f, 0.0f, 0.3f},    1.0f, 0.0f);
    beat  = p.addExtParam ("dlBeat",  "Delay",     "", "",   {   0.0f,    mxd, 1.0f, 1.0f},   13.0f, 0.0f, durationTextFunction);
    fb    = p.addExtParam ("dlFb",    "FB",        "", "dB", {-100.0f,   0.0f, 0.0f, 5.0f},  -10.0f, 0.1f);
    cf    = p.addExtParam ("dlCf",    "CF",        "", "dB", {-100.0f,   0.0f, 0.0f, 5.0f}, -100.0f, 0.1f);
    mix   = p.addExtParam ("dlMix",   "Mix",       "", "%",  {   0.0f, 100.0f, 0.0f, 1.0f},    0.5f, 0.1f);

    delay = p.addIntParam ("dlDelay", "Delay",     "", "",   {   0.0f, 120.0f, 0.0f, 1.0f},    1.0f, {0.2f, SmoothingType::eased});

    fb->conversionFunction  = [] (float in) { return Decibels::decibelsToGain (in); };
    cf->conversionFunction  = [] (float in) { return Decibels::decibelsToGain (in); };
    mix->conversionFunction = [] (float in) { return in / 100.0f; };
}

//==============================================================================
void WavetableAudioProcessor::ReverbParams::setup (WavetableAudioProcessor& p)
{
    enable     = p.addIntParam ("rvEnable",   "Enable",  "",   "", { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f);

    damping    = p.addExtParam ("rvbDamping", "Damping", "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
    freezeMode = p.addExtParam ("rvbFreeze",  "Freeze",  "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
    roomSize   = p.addExtParam ("rvbSize",    "Size",    "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
    width      = p.addExtParam ("rvbWidth",   "Width",   "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
    mix        = p.addExtParam ("rvbMix",     "Mix",     "",   "", {0.0f, 1.0f,    0.0f, 1.0f}, 0.0f, 0.0f);
}

//==============================================================================
void WavetableAudioProcessor::LimiterParams::setup (WavetableAudioProcessor& p)
{
    enable    = p.addIntParam ("lmEnable",    "Enable",    "", "",     { 0.0, 1.0, 1.0, 1.0 }, 0.0, 0.0f);

    attack    = p.addExtParam ("lmAttack",    "Attack",    "", "ms",   { 1.0f,     5.0f, 0.0f, 0.4f},    1.0f, 0.1f);
    release   = p.addExtParam ("lmRelease",   "Release",   "", "ms",   { 1.0f,   100.0f, 0.0f, 0.4f},    5.0f, 0.1f);
    threshold = p.addExtParam ("lmThreshold", "Ceil",      "", "dB",   { -60.0f,   0.0f, 0.0f, 1.0f},  -30.0f, 0.1f);
    gain      = p.addExtParam ("lmGain",      "Gain",      "", "dB",   { -30.0f,  30.0f, 0.0f, 1.0f},    0.0f, 0.1f);

    attack->conversionFunction  = [] (float in) { return in / 1000.0f; };
    release->conversionFunction = [] (float in) { return in / 1000.0f; };
    gain->conversionFunction    = [] (float in) { return Decibels::decibelsToGain (in); };
}

//==============================================================================
WavetableAudioProcessor::WavetableAudioProcessor()
{
	formatManager->registerBasicFormats();

    enableLegacyMode();
    setVoiceStealingEnabled (true);

    for (int i = 0; i < numElementsInArray (wtParams); i++)
        wtParams[i].setup (*this, i);

    for (int i = 0; i < numElementsInArray (oscParams); i++)
        oscParams[i].setup (*this, i);

    for (int i = 0; i < numElementsInArray (filterParams); i++)
        filterParams[i].setup (*this, i);

    for (int i = 0; i < numElementsInArray (envParams); i++)
        envParams[i].setup (*this, i);

    for (int i = 0; i < numElementsInArray (lfoParams); i++)
        lfoParams[i].setup (*this, i);

    stepLfoParams.setup (*this);
    adsrParams.setup (*this);

    globalParams.setup (*this);
    gateParams.setup (*this);
    chorusParams.setup (*this);
    distortionParams.setup (*this);
    eqParams.setup (*this);
    compressorParams.setup (*this);
    delayParams.setup (*this);
    reverbParams.setup (*this);
    limiterParams.setup (*this);

    eq.setNumChannels (2);
    compressor.setNumChannels (2);
    limiter.setNumChannels (2);

    for (int i = 0; i < 50; i++)
    {
        auto voice = new WavetableVoice (*this, bandLimitedLookupTables);
        modMatrix.addVoice (voice);
        addVoice (voice);
    }

    setupModMatrix();

	MemoryBlock mb (BinaryData::WINDOW_S_WAV, BinaryData::WINDOW_S_WAVSize);
	updateWavetable (0, mb, 256);
}

WavetableAudioProcessor::~WavetableAudioProcessor()
{
}

//==============================================================================
void WavetableAudioProcessor::stateUpdated()
{
    modMatrix.stateUpdated (state);
}

void WavetableAudioProcessor::updateState()
{
    modMatrix.updateState (state);
}

//==============================================================================
void WavetableAudioProcessor::setupModMatrix()
{
    modSrcPressure  = modMatrix.addPolyModSource ("mpep", "MPE Pressure", false);
    modSrcTimbre    = modMatrix.addPolyModSource ("mpet", "MPE Timbre", false);

    modScrPitchBend = modMatrix.addMonoModSource ("pb", "Pitch Bend", true);

    modSrcNote      = modMatrix.addPolyModSource ("note", "MIDI Note Number", false);
    modSrcVelocity  = modMatrix.addPolyModSource ("vel", "MIDI Velocity", false);

    for (int i = 0; i <= 119; i++)
    {
        String name = MidiMessage::getControllerName (i);
        if (name.isEmpty())
            modSrcCC.add (modMatrix.addMonoModSource (String::formatted ("cc%d", i), String::formatted ("CC %d", i), false));
        else
            modSrcCC.add (modMatrix.addMonoModSource (String::formatted ("cc%d", i), String::formatted ("CC %d ", i) + name, false));
    }

    for (int i = 0; i < Cfg::numLFOs; i++)
        modSrcMonoLFO.add (modMatrix.addMonoModSource (String::formatted ("mlfo%d", i + 1), String::formatted ("LFO %d (Mono)", i + 1), true));

    for (int i = 0; i < Cfg::numLFOs; i++)
        modSrcLFO.add (modMatrix.addPolyModSource (String::formatted ("lfo%d", i + 1), String::formatted ("LFO %d", i + 1), true));

    modSrcMonoStep = modMatrix.addMonoModSource ("mstep", "Step LFO (Mono)", true);
    modSrcStep     = modMatrix.addPolyModSource ("step", "Step LFO", true);

    for (int i = 0; i < Cfg::numFilters; i++)
        modSrcFilter.add (modMatrix.addPolyModSource (String::formatted ("fenv%d", i + 1), String::formatted ("Filter Envelope %d", i + 1), false));

    for (int i = 0; i < Cfg::numENVs; i++)
        modSrcEnv.add (modMatrix.addPolyModSource (String::formatted ("env%d", i + 1), String::formatted ("Envelope %d", i + 1), false));

    auto firstMonoParam = globalParams.mono;
    bool polyParam = true;
    for (auto pp : getPluginParameters())
    {
        if (pp == firstMonoParam)
            polyParam = false;

        if (! pp->isInternal())
        {
            modMatrix.addParameter (pp, polyParam);
        }
    }

    modMatrix.build();
}

void WavetableAudioProcessor::reset()
{
    Processor::reset();

    gate.reset();
    chorus.reset();
    distortion.reset();
    stereoDelay.reset();
    compressor.reset();
    limiter.reset();

    eq.reset();

    reverb.reset();

    for (auto& l : modLFOs)
        l.reset();

    modStepLFO.reset();
}

void WavetableAudioProcessor::prepareToPlay (double newSampleRate, int newSamplesPerBlock)
{
    Processor::prepareToPlay (newSampleRate, newSamplesPerBlock);

    bandLimitedLookupTables.setSampleRate (newSampleRate);
    setCurrentPlaybackSampleRate (newSampleRate);

    modMatrix.setSampleRate (newSampleRate);

    gate.setSampleRate (newSampleRate);
    chorus.setSampleRate (newSampleRate);
    distortion.setSampleRate (newSampleRate);
    stereoDelay.setSampleRate (newSampleRate);
    compressor.setSampleRate (newSampleRate);
    limiter.setSampleRate (newSampleRate);

    eq.setSampleRate (newSampleRate);

    reverb.setSampleRate (newSampleRate);

    for (auto& l : modLFOs)
        l.setSampleRate (newSampleRate);

    modStepLFO.setSampleRate (newSampleRate);
}

void WavetableAudioProcessor::releaseResources()
{
}

void WavetableAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    startBlock();
    setMPE (globalParams.mpe->isOn());

    playhead = getPlayHead();

    int pos = 0;
    int todo = buffer.getNumSamples();

    buffer.clear();

    setMono (globalParams.mono->isOn());
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

        auto slice = sliceBuffer (buffer, pos, thisBlock);
        applyEffects (slice);

        modMatrix.finishBlock (thisBlock);

        pos += thisBlock;
        todo -= thisBlock;
    }

    playHead = nullptr;

    fifo.write (buffer);
    endBlock (buffer.getNumSamples());
}

Array<float> WavetableAudioProcessor::getLiveFilterCutoff (int i)
{
    Array<float> values;

    for (auto v : voices)
    {
        if (v->isActive())
        {
            auto vav = dynamic_cast<WavetableVoice*>(v);
            values.add (vav->getFilterCutoffNormalized (i));
        }
    }
    return values;
}

void WavetableAudioProcessor::applyEffects (AudioSampleBuffer& buffer)
{
    // Apply gate
    if (gateParams.enable->isOn())
        gate.process (buffer, noteOnIndex, noteOffIndex);

    // Apply Chorus
    if (chorusParams.enable->isOn())
        chorus.process (buffer);

    // Apply Distortion
    if (distortionParams.enable->isOn())
        distortion.process (buffer);

    // Apply EQ
    if (eqParams.enable->isOn())
        eq.process (buffer);

    // Apply Compressor
    if (compressorParams.enable->isOn())
        compressor.process (buffer);

    // Apply Delay
    if (delayParams.enable->isOn())
        stereoDelay.process (buffer);

    // Apply Reverb
    if (reverbParams.enable->isOn())
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples ());

    // Apply Limiter
    if (limiterParams.enable->isOn())
        limiter.process (buffer);

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
            LFO::Parameters params;

            float freq = 0;
            if (lfoParams[i].sync->getProcValue() > 0.0f)
                freq = 1.0f / NoteDuration::getNoteDurations()[size_t (lfoParams[i].beat->getProcValue())].toSeconds (playhead);
            else
                freq = modMatrix.getValue (lfoParams[i].rate);

            params.waveShape = (LFO::WaveShape) int (lfoParams[i].wave->getProcValue());
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
        float freq = 1.0f / NoteDuration::getNoteDurations()[size_t (stepLfoParams.beat->getProcValue())].toSeconds (playhead);

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
        float freq = 1.0f / NoteDuration::getNoteDurations()[size_t (gateParams.beat->getProcValue())].toSeconds (playhead);

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
        distortion.setParams (modMatrix.getValue (distortionParams.amount),
                              modMatrix.getValue (distortionParams.highpass),
                              modMatrix.getValue (distortionParams.output),
                              modMatrix.getValue (distortionParams.mix));
    }


    // Update EQ
    if (eqParams.enable->isOn())
    {
        eq.setParams (0, EQ::lowshelf,
                      modMatrix.getValue (eqParams.loFreq),
                      modMatrix.getValue (eqParams.loQ),
                      modMatrix.getValue (eqParams.loGain));

        eq.setParams (1, EQ::peak,
                      modMatrix.getValue (eqParams.mid1Freq),
                      modMatrix.getValue (eqParams.mid1Q),
                      modMatrix.getValue (eqParams.mid1Gain));

        eq.setParams (2, EQ::peak,
                      modMatrix.getValue (eqParams.mid2Freq),
                      modMatrix.getValue (eqParams.mid2Q),
                      modMatrix.getValue (eqParams.mid2Gain));

        eq.setParams (3, EQ::highshelf,
                      modMatrix.getValue (eqParams.hiFreq),
                      modMatrix.getValue (eqParams.hiQ),
                      modMatrix.getValue (eqParams.hiGain));
    }

    // Update Compressor
    if (compressorParams.enable->isOn())
    {
        compressor.setInputGain (1.0f);
        compressor.setOutputGain (modMatrix.getValue (compressorParams.gain));
        compressor.setParams (modMatrix.getValue (compressorParams.attack),
                              modMatrix.getValue (compressorParams.release),
                              modMatrix.getValue (compressorParams.threshold),
                              modMatrix.getValue (compressorParams.ratio),
                              6);
    }

    // Update Delay
    if (delayParams.enable->isOn())
    {
        if (delayParams.sync->isOn())
        {
            auto& duration = NoteDuration::getNoteDurations()[(size_t)delayParams.beat->getUserValueInt()];
            delayParams.delay->setUserValue (duration.toSeconds (getPlayHead()));
        }
        else
        {
            delayParams.delay->setUserValue (delayParams.time->getUserValue());
        }

        stereoDelay.setParams (delayParams.delay->getUserValue(),
                               modMatrix.getValue (delayParams.mix),
                               modMatrix.getValue (delayParams.fb),
                               modMatrix.getValue (delayParams.cf));
    }


    // Update Reverb
    if (reverbParams.enable->isOn())
    {
        Reverb::Parameters p;

        auto mix = modMatrix.getValue (reverbParams.mix);
        WetDryMix wetDry (mix);

        p.damping    = modMatrix.getValue (reverbParams.damping);
        p.freezeMode = modMatrix.getValue (reverbParams.freezeMode);
        p.roomSize   = modMatrix.getValue (reverbParams.roomSize);
        p.width      = modMatrix.getValue (reverbParams.width);
        p.dryLevel   = wetDry.dryGain;
        p.wetLevel   = wetDry.wetGain;

        reverb.setParameters (p);
    }

    // Update Limiter
    if (limiterParams.enable->isOn())
    {
        limiter.setInputGain (1.0f);
        limiter.setOutputGain (modMatrix.getValue (limiterParams.gain));
        limiter.setParams (modMatrix.getValue (limiterParams.attack),
                           modMatrix.getValue (limiterParams.release),
                           modMatrix.getValue (limiterParams.threshold),
                           100, 6);
    }

    // Output gain
    outputGain.setGain (modMatrix.getValue (globalParams.level));
}

void WavetableAudioProcessor::handleMidiEvent (const MidiMessage& m)
{
    MPESynthesiser::handleMidiEvent (m);

    if (m.isPitchWheel())
        modMatrix.setMonoValue (modScrPitchBend, float (m.getPitchWheelValue()) / 0x2000 - 1.0f);
}

void WavetableAudioProcessor::handleController ([[maybe_unused]] int ch, int num, int val)
{
    modMatrix.setMonoValue (modSrcCC[num], val / 127.0f);
}

void WavetableAudioProcessor::updateWavetable (int idx, MemoryBlock& src, int tableSize)
{
	if (auto reader = std::unique_ptr<AudioFormatReader> (formatManager->createReaderFor (std::make_unique<MemoryInputStream> (src, false))))
	{
		AudioSampleBuffer buffer;
		buffer.setSize (1, int (reader->lengthInSamples));
		reader->read (&buffer, 0, int (reader->lengthInSamples), 0, true, false);

		loadWavetables (waveTables[idx], buffer, reader->sampleRate, tableSize);

		for (auto v : voices)
			if (auto wtv = dynamic_cast<WavetableVoice*>(v))
				wtv->setWavetable (idx, waveTables[idx]);
	}
}

//==============================================================================
bool WavetableAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* WavetableAudioProcessor::createEditor()
{
    return new WavetableAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WavetableAudioProcessor();
}
