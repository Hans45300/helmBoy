# HelmBoy Synthesizer - Introduction

## Overview

**HelmBoy** is a powerful polyphonic subtractive synthesizer featuring advanced oscillator modulation, flexible filtering, comprehensive modulation routing, and professional effects processing. Built on a robust synthesis engine with real-time performance optimization, HelmBoy offers both classic analog-style sounds and modern digital synthesis capabilities.

---

## Core Features

### Oscillator Section

HelmBoy features a **dual oscillator architecture** with extensive sound design capabilities:

#### Dual Oscillators

- **Two independent oscillators** with individual waveform selection
- **Multiple waveform types**: Sine, Triangle, Sawtooth, Square, and wavetables
- **Independent transpose and fine-tune controls** for each oscillator
- **Polyphonic unison** up to **15 voices per oscillator** with detuning and stereo spread
- **Sub-oscillator** for additional low-frequency content

#### Advanced Modulation

- **Cross-Modulation**: Bidirectional phase modulation between OSC1 ? OSC2
- **Frequency Modulation (FM)**: Unidirectional FM where OSC1 modulates OSC2
- **Ring Modulation**: Multiplies OSC1 × OSC2 for metallic, bell-like timbres
- **Phase Stretching**: Variable waveform symmetry control (1%-99%) - works like PWM but applies to all waveform types, not just square waves

#### Oscillator Mixing

- Independent amplitude control for each oscillator
- Smooth blending between oscillators
- Sub-oscillator level control

---

### Filter Section

Sophisticated filtering with multiple topologies:

- **Filter Types**: Low-pass, high-pass, band-pass, notch, and more
- **Filter Styles**: Choose between different filter characteristics
- **Resonance Control**: From subtle emphasis to self-oscillation
- **Filter Drive**: Adds saturation and harmonic richness
- **Key Tracking**: Makes filter cutoff follow note pitch
- **Filter Envelope**: Dedicated ADSR envelope for dynamic filter movement
- **Filter Blending**: Mix between different filter configurations

---

### Modulation System

Flexible and powerful modulation routing:

#### Envelopes

- **Amplitude Envelope**: Standard ADSR for volume shaping
- **Filter Envelope**: Dedicated ADSR for filter cutoff modulation
- **Modulation Envelope**: Additional ADSR for general modulation purposes

#### LFOs (Low Frequency Oscillators)

- **3 Mono LFOs**: Global modulation sources
- **2 Poly LFOs**: Per-voice modulation for evolving textures
- **Multiple waveforms**: Sine, triangle, saw, square, random, and more
- **Tempo Sync**: Lock LFO rates to host tempo
- **Free-running or retriggerable** operation

#### Step Sequencer

- **Graphical step sequencer** with real-time visual feedback
- **Up to 32 steps** per sequence
- **Adjustable step frequency** with tempo sync
- **Smooth interpolation** between steps
- **Keyboard navigation** support for precise value editing

---

### Effects Section

Professional-grade effects processing:

#### Distortion

- **Multiple distortion types**: Soft clip, hard clip, bit reduction, and more
- **Drive control**: From subtle warmth to aggressive saturation
- **Mix control**: Blend dry and distorted signals

#### Delay

- **Stereo delay** with independent left/right timing
- **Tempo sync**: Lock delay times to host tempo
- **Feedback control**: From single repeats to infinite loops
- **Dry/Wet mix**: Precise balance of delayed signal

#### Reverb

- **High-quality algorithmic reverb**
- **Damping control**: Adjust high-frequency absorption
- **Feedback/Size control**: From small rooms to vast halls
- **Dry/Wet mix**: Studio-grade reverb blending

#### Formant Filter

- **Vocal formant synthesis**
- **X/Y control**: Navigate through vowel space (A, E, I, O, U)
- **Creates vocal-like characteristics** in any sound

---

### Performance Features

#### Polyphony

- **Up to 32 voices** of polyphony
- **Intelligent voice stealing** for smooth performance
- **Voice allocation** optimized for real-time playing

#### Playability

- **Legato mode**: Smooth note transitions without retriggering envelopes
- **Portamento/Glide**: Smooth pitch transitions between notes
- **Pitch Bend**: Configurable range up to ±2 octaves
- **Velocity sensitivity**: Dynamic response to playing intensity
- **Aftertouch support**: Per-note and channel aftertouch

#### Arpeggiator

- **Multiple arpeggio patterns**: Up, down, up/down, random, and more
- **Tempo sync**: Lock arpeggiator to host tempo
- **Octave range**: Span up to 4 octaves
- **Gate control**: Adjust note length from staccato to legato

---

## Architecture Highlights

### Signal Flow

```cpp
Oscillators ? Cross-Mod/FM/Ring Mod ? Mixer ? Filter ? Amplitude Envelope ? Effects ? Output
     ?                                            ?
     ??????? Modulation Sources (LFOs, Envelopes, Step Seq) ?????
```

### Modulation Routing

- **Drag-and-drop modulation connections** in the interface
- **Bipolar and unipolar modulation** sources
- **Modulation depth control** for each connection
- **Visual feedback** of active modulations

### Preset Management

- **Factory presets** covering a wide range of sounds
- **User preset storage** for custom creations
- **Import/Export** functionality for sharing presets
- **Preset browser** with category filtering

---

## Technical Specifications

- **Sample Rate**: Up to 192 kHz
- **Bit Depth**: 32-bit floating-point processing
- **Latency**: Optimized for real-time performance
- **CPU Usage**: Efficient SIMD-optimized DSP code
- **Platform Support**: VST3 and standalone application

---

## Typical Use Cases

- **Analog-style bass and lead synth** sounds
- **Rich pads and textures** with unison and modulation
- **Percussive and rhythmic sequences** using the step sequencer
- **FM-style bells and metallic tones** with FM and ring modulation
- **Complex evolving soundscapes** with multiple LFO modulations
- **Aggressive distorted sounds** for electronic and industrial music
- **Warm vintage-inspired tones** using filter drive and saturation

---

## Getting Started

1. **Select a preset** from the browser to explore HelmBoy's capabilities
2. **Experiment with oscillator modulation** (cross-mod, FM, ring mod)
3. **Try the phase stretching** for unique waveform variations
4. **Add movement** with LFOs and envelopes
5. **Shape the tone** with the filter section
6. **Polish with effects** for professional-quality results

---

HelmBoy combines the warmth of classic analog synthesis with the precision and flexibility of modern digital design, providing a comprehensive tool for sound designers, producers, and performers.
