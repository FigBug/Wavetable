# Wavetable

![Build](https://github.com/FigBug/Wavetable/workflows/Build/badge.svg)

A 2 oscillator wavetable synthesizer with flexible modulation options.

![Screenshot](Screenshots/Screenshot1.png)

[Download](https://github.com/FigBug/Wavetable/releases) | [Product Page](https://socalabs.com/synths/Wavetable/)

## Features

### Oscillators
- 2 wavetable oscillators with position, tune, fine tune, level, and pan controls
- Unison with up to 8 voices per oscillator, detune, and stereo spread
- Formant and bend controls for wavetable manipulation
- Load custom wavetables from WAV files (supports 256, 512, 1024, 2048 sample sizes)
- Sub oscillator with selectable waveforms
- Noise generator with multiple noise types

### Filter
- Multi-mode filter (LP, HP, BP, Notch, etc.)
- Dedicated filter ADSR envelope
- Key tracking and velocity tracking
- Per-source routing (OSC1, OSC2, Sub, Noise)

### Modulation
- 3 LFOs with sync-to-tempo option
- 3 modulation envelopes
- 32-step sequencer
- Comprehensive modulation matrix
- MPE support

### Effects
- Gate (16-step pattern sequencer)
- Chorus
- Distortion (Simple, Bitcrusher, Fire Amp, Grind Amp modes)
- Delay (free or tempo-synced)
- Reverb

### Global
- Mono/Poly modes
- Glide with legato option
- MTS-ESP microtuning support

## Formats

- VST3
- VST2 (requires VST2 SDK)
- AU (macOS)
- LV2
- Standalone

## Building

Requirements:
- CMake 3.16+
- C++20 compatible compiler

```bash
git clone --recursive https://github.com/FigBug/Wavetable.git
cd Wavetable
cmake -B build
cmake --build build --config Release
```

## License

The synth is BSD licensed. However, it depends on JUCE. To use in a commercial application, you must have a JUCE license. Wavetables have their own license.

## Contact

Need additional features or help integrating? Contact me for consulting services: https://rabiensoftware.com/
