# Wavetable Synthesizer Manual

## Overview

Wavetable is a polyphonic wavetable synthesizer featuring two wavetable oscillators, a sub-oscillator, noise generator, filter, multiple modulation sources, a flexible mod matrix, and a comprehensive effects section.

---

## Oscillators

### Wavetable Oscillators (OSC1 & OSC2)

Two identical wavetable oscillators provide the main sound source.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable the oscillator |
| Wavetable | - | Select from built-in wavetables or load custom WAV files |
| Tune | -36 to +36 semitones | Coarse pitch adjustment |
| Fine | -100 to +100 cents | Fine pitch adjustment |
| Level | -100 to 0 dB | Oscillator volume |
| Pos | 0-100% | Wavetable position - morphs through the wavetable frames |
| Pan | -100% to +100% | Stereo panning |
| Formant | -1.0 to +1.0 | Formant shifting for tonal variation |
| Bend | -1.0 to +1.0 | Pitch bend modulation amount |
| Voices | 1-8 | Number of unison voices |
| Detune | 0-0.5 | Pitch spread between unison voices (enabled when Voices > 1) |
| Spread | -100% to +100% | Stereo spread of unison voices (enabled when Voices > 1) |
| Retrig | On/Off | Retrigger oscillator phase on each new note |

**Tip:** Drag on the wavetable display to adjust the position in real-time.

### Sub-Oscillator

A simple oscillator typically used one octave below the main oscillators.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable the sub-oscillator |
| Wave | Sine, Triangle, Saw Up, Pulse 50%, Pulse 25%, Pulse 12% | Waveform selection |
| Tune | -36 to +36 semitones | Pitch adjustment |
| Level | -100 to 0 dB | Volume |
| Pan | -100% to +100% | Stereo panning |
| Retrig | On/Off | Retrigger phase on new notes |

### Noise Generator

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable noise |
| Type | White, Pink | Noise color |
| Level | -100 to 0 dB | Volume |
| Pan | -100% to +100% | Stereo panning |

---

## Filter

A multi-mode filter with envelope control and key/velocity tracking.

### Filter Types

- **LP 12** - Low-pass 12 dB/octave
- **LP 24** - Low-pass 24 dB/octave
- **HP 12** - High-pass 12 dB/octave
- **HP 24** - High-pass 24 dB/octave
- **BP 12** - Band-pass 12 dB/octave
- **BP 24** - Band-pass 24 dB/octave
- **NT 12** - Notch 12 dB/octave
- **NT 24** - Notch 24 dB/octave

### Filter Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable the filter |
| Type | See above | Filter type selection |
| Frequency | 20 Hz - 20 kHz | Cutoff frequency |
| Resonance | 0-100% | Filter resonance/Q |
| Key | 0-100% | Keyboard tracking amount |
| Velocity | 0-100% | Velocity to filter frequency modulation |
| Amount | -1.0 to +1.0 | Filter envelope modulation depth |

### Filter Envelope

| Parameter | Range | Description |
|-----------|-------|-------------|
| Attack | 0-60 seconds | Attack time |
| Decay | 0-60 seconds | Decay time |
| Sustain | 0-100% | Sustain level |
| Release | 0-60 seconds | Release time |
| Retrig | On/Off | Retrigger envelope on new notes |

### Filter Routing

Use the routing buttons (WT1, WT2, Sub, Noise) to select which sound sources are processed by the filter.

---

## Amplitude Envelope

The main volume envelope applied to all sound sources.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Attack | 0-60 seconds | Attack time |
| Decay | 0-60 seconds | Decay time |
| Sustain | 0-100% | Sustain level |
| Release | 0-60 seconds | Release time |
| Velocity | 0-100% | Velocity sensitivity |
| Retrig | On/Off | Retrigger on new notes (mono mode with glide only) |

---

## Modulation Sources

### Modulation Envelopes (ENV 1, 2, 3)

Three independent ADSR envelopes for modulation purposes.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable the envelope |
| Attack | 0-60 seconds | Attack time |
| Decay | 0-60 seconds | Decay time |
| Sustain | 0-100% | Sustain level |
| Release | 0-60 seconds | Release time |
| Retrig | On/Off | Retrigger on new notes |

### LFOs (LFO 1, 2, 3)

Three independent LFOs with multiple waveforms.

#### LFO Waveforms

- None, Sine, Triangle
- Saw Up, Saw Down
- Square, Square+
- Sample & Hold, Noise
- Step Up (3, 4, 8 steps)
- Step Down (3, 4, 8 steps)
- Pyramid (3, 5, 9 steps)

#### LFO Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable the LFO |
| Wave | See above | Waveform selection |
| Sync | On/Off | Sync to host tempo |
| Rate | 0-50 Hz | LFO speed (when Sync is off) |
| Beat | Note values | LFO speed (when Sync is on) |
| Depth | -1.0 to +1.0 | Modulation amount |
| Phase | -1.0 to +1.0 | Starting phase offset |
| Offset | -1.0 to +1.0 | DC offset/center bias |
| Fade | -60 to +60 seconds | Fade in (positive) or fade out (negative) time |
| Delay | 0-60 seconds | Delay before LFO starts |
| Retrig | On/Off | Retrigger phase on new notes |

### Step LFO

A 32-step sequencer for rhythmic modulation.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable or disable |
| Beat | Note values | Step clock speed (tempo-synced) |
| Length | 2-32 steps | Number of active steps |
| Retrig | On/Off | Retrigger sequence on new notes |
| Steps 1-32 | -1.0 to +1.0 | Individual step values |

---

## Modulation Matrix

The mod matrix allows routing any modulation source to any parameter with flexible depth, curve shaping, and polarity controls.

### Modulation Source Icons

Modulation sources are displayed with ring icons that indicate their type:

- **Single Ring** - Mono (global) modulation source. The modulation value is the same for all voices.
- **Double Ring** - Poly (per-voice) modulation source. Each voice has its own independent modulation value.

### Available Modulation Sources

**MIDI:**
- Pitch Bend (mono)
- Note Number (poly)
- Velocity (poly)
- All MIDI CC controllers CC 1-120 (mono)

**MPE (when enabled):**
- MPE Pressure (poly) - per-note aftertouch
- MPE Timbre (poly) - per-note slide/CC74
- MPE Pitch Bend (poly) - per-note pitch bend

**Internal:**
- LFO 1, 2, 3 Poly (per-voice LFO)
- LFO 1, 2, 3 Mono (global LFO)
- Step LFO Poly (per-voice)
- Step LFO Mono (global)
- Filter Envelope (poly)
- Mod Envelopes 1, 2, 3 (poly)

### Assigning Modulation

**Method 1: Drag and Drop**
1. Find the modulation source in the mod source list (left panel)
2. Click and drag the source icon (single or double ring)
3. Drop it onto any parameter knob you want to modulate
4. A small modulation depth indicator will appear on the knob

**Method 2: Learn Mode**
1. Click on a modulation source icon to enter learn mode
2. The source name appears in the title bar with a pulsing indicator
3. Click on any parameter knob to assign the modulation
4. Click elsewhere or press the source again to cancel learn mode

### Adjusting Modulation

Once a modulation is assigned, a **modulation depth slider** appears on the parameter. This small circular indicator shows the current modulation depth:

- Drag the depth slider to adjust modulation amount
- The filled portion shows positive depth, extending clockwise
- Negative depth extends counter-clockwise
- Double-click to reset to zero
- The popup shows the resulting parameter value range

### Modulation Matrix Panel

The mod matrix panel shows all active modulation assignments in a list:

| Control | Description |
|---------|-------------|
| Enable (power icon) | Toggle this modulation on/off without deleting it |
| Depth Slider | Adjust modulation depth (-1.0 to +1.0) |
| Bipolar Button | Toggle unipolar/bipolar mapping mode |
| Curve Button | Select modulation curve/shape |
| Source | Name of the modulation source |
| Destination | Name of the modulated parameter |
| Delete (X icon) | Remove this modulation assignment |

### Polarity Modes

Click the **bipolar button** to toggle between:

- **Unipolar** - Modulation ranges from 0 to 1 (positive only). Source value of 0 = no modulation, 1 = full modulation.
- **Bipolar** - Modulation ranges from -1 to +1. Source value of 0 = center, negative values modulate downward.

### Modulation Curves

Click the **curve button** to select how modulation is shaped:

**Linear:**
- Linear - Direct 1:1 mapping (default)

**Quadratic (smooth acceleration):**
- Quadratic In - Slow start, fast end
- Quadratic Out - Fast start, slow end
- Quadratic In/Out - Smooth both ends

**Sine (S-curve):**
- Sine In - Slow start, fast end
- Sine Out - Fast start, slow end
- Sine In/Out - Smooth both ends

**Exponential (rapid acceleration):**
- Exponential In - Very slow start, very fast end
- Exponential Out - Very fast start, very slow end
- Exponential In/Out - Smooth both ends

**Inverted versions** of all curves are also available (Inv Linear, Inv Quadratic In, etc.) which flip the response.

### Tips for Modulation

1. **Subtle movement**: Use LFO with low depth on wavetable position for evolving pads
2. **Velocity response**: Map Velocity to filter cutoff and amp envelope for dynamic playing
3. **Per-voice variation**: Use poly mod sources for each voice to respond independently
4. **Complex modulation**: Stack multiple sources on the same parameter with different curves
5. **Disable without deleting**: Use the enable button to A/B compare with/without modulation

---

## Effects

Effects can be reordered by dragging. The signal flows through effects in the displayed order.

### Gate

A rhythmic gate/tremolo effect.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable the gate |
| Beat | Note values | Gate clock speed |
| Length | 2-16 steps | Pattern length |
| Attack | 0-1 second | Gate opening time |
| Release | 0-1 second | Gate closing time |
| Steps | On/Off per step | L/R channel gate pattern |

### Chorus

A stereo chorus effect.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable chorus |
| Delay | 0.1-30 ms | Base delay time |
| Rate | 0.1-10 Hz | Modulation speed |
| Depth | 0.1-20 ms | Modulation depth |
| Width | 0-100% | Stereo width |
| Mix | 0-100% | Dry/wet balance |

### Distortion

Four distortion modes available:

**Simple Distortion:**
- Amount (0-100%)

**Bitcrusher (DeRez):**
- Rate - Sample rate reduction
- Rez - Bit depth reduction
- Hard - Drive amount
- Mix - Dry/wet balance

**Fire Amp:**
- Gain - Input drive
- Tone - Tone shaping
- Output - Output level
- Mix - Dry/wet balance

**Grind Amp:**
- Gain - Input drive
- Tone - Tone control
- Output - Output level
- Mix - Dry/wet balance

### Delay

A stereo delay with tempo sync.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable delay |
| Sync | On/Off | Sync to host tempo |
| Time | 0-120 seconds | Delay time (when Sync is off) |
| Beat | Note values | Delay time (when Sync is on) |
| Feedback | -100 to 0 dB | Delay repeats |
| Crossfeed | -100 to 0 dB | Channel crossfeed |
| Mix | 0-100% | Dry/wet balance |

### Reverb

A studio-quality reverb.

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enable | On/Off | Enable reverb |
| Size | 0-100% | Room size |
| Decay | 0-100% | Reverb tail length |
| Lowpass | 16 Hz - 20 kHz | High frequency damping |
| Damping | 16 Hz - 20 kHz | Additional damping |
| Predelay | 0-100 ms | Delay before reverb |
| Mix | 0-100% | Dry/wet balance |

---

## Global Settings

| Parameter | Range | Description |
|-----------|-------|-------------|
| Level | -100 to 0 dB | Master output volume |
| Voices | 2-40 | Maximum polyphony |
| Mono | On/Off | Monophonic mode |
| Glide | Off, Glissando, Portamento | Pitch glide mode |
| Glide Time | 0.001-20 seconds | Glide duration |
| Legato | On/Off | Legato mode (mono only) |
| Pitch Bend | 0-48 semitones | MIDI pitch bend range |
| MPE | On/Off | Enable MPE support |

---

## MPE Support

When MPE (MIDI Polyphonic Expression) is enabled:

- Each voice responds to its own MIDI channel
- Per-note pitch bend is available
- Per-note pressure (aftertouch) is available
- Per-note timbre (slide/CC74) is available

These can be used as modulation sources in the mod matrix for expressive per-voice control.

---

## MIDI Learn

Right-click on any parameter to access MIDI Learn:

- **Start Learning** - Move a MIDI CC controller to assign it
- **Cancel Learning** - Cancel the current learn operation
- **Clear CC** - Remove the current CC assignment

MIDI mappings are saved globally and persist across sessions.

### MIDI Map Files

From the menu:
- **Load MIDI Map...** - Load a saved MIDI mapping file (.midimap)
- **Save MIDI Map...** - Save current MIDI mappings to a file

---

## Preset Management

### Saving Presets

1. Click the **+** button in the title bar
2. Enter a preset name
3. Optionally add author and tags (when using the preset browser)
4. Click OK

### Loading Presets

- Use the dropdown menu to select a preset
- Use the **<** and **>** buttons to step through presets
- Click the browse button to open the full preset browser

### Preset Browser

The preset browser allows:
- Browsing presets by category/tags
- Searching for presets
- Managing user presets
- Deleting presets (right-click menu)

---

## Wavetable Import

To load custom wavetables:

1. Click the **+** button on an oscillator
2. Select a WAV file containing wavetable data
3. The plugin will auto-detect the wavetable frame size, or you can specify it manually

Supported frame sizes: 256, 512, 1024, 2048 samples

Custom wavetables are embedded in presets when saved.

---

## Tips & Tricks

1. **Rich Pads**: Use both oscillators with different wavetables, slight detune, and long filter envelope
2. **Punchy Basses**: Enable the sub-oscillator, use LP24 filter with short decay envelope
3. **Movement**: Modulate wavetable position with an LFO for evolving textures
4. **Rhythmic Effects**: Use the Step LFO or Gate effect for pulsing sounds
5. **Expression**: Enable MPE and map pressure/timbre to filter cutoff and wavetable position
6. **Layering**: Route oscillators differently through the filter for complex timbres

---

## System Requirements

- macOS 10.13 or later
- Windows 10 or later
- Linux (Ubuntu 20.04 or compatible)

Available formats: VST3, AU, Standalone

---

## Credits

Wavetable Synthesizer by SocaLabs
