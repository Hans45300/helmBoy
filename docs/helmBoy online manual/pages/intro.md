# HelmBoy Synthesizer - Introduction

## Overview

**HelmBoy** is a polyphonic synthesizer for building sounds from two oscillators, a filter, envelopes, modulation sources, and effects. It is available as a standalone application and as a VST3 plug-in.

### Host Parameter Text

In the host's generic parameter editor, numeric values use the parameter's
display scale and declared units; choices use the existing selector labels.
Numeric entry accepts a decimal point or a single decimal comma, optionally
followed by the matching unit. Choice labels are case-insensitive. Invalid
text returns the parameter's default value; valid out-of-range numbers are
clamped. Automation remains normalized as before. Short host text fields may
truncate labels or numeric precision.

### Host Reset

A host reset stops playing voices, arpeggiator notes and effect tails without
loading Init or changing the patch, parameters or modulation assignments.
Unlike a patch transition, it is an immediate cut rather than a musical fade.
New host MIDI notes can start normally afterward. Pending on-screen keyboard
events are discarded; its visual state is cleared on the message thread.

### Effect Tails in Host Exports

The plug-in reports a conservative release/effect-tail estimate to the host,
separate from processing latency. This may extend automatic exports beyond the
audible tail, especially with delay feedback. Freeze and non-decaying feedback
can sustain sound indefinitely. Use an explicit export length for these settings;
formats without a verified infinite-tail convention use a finite reporting
ceiling, not a guarantee that a frozen tail will end within that time.

---

## Core Features

### Oscillator Section

HelmBoy features a **dual oscillator architecture** with extensive sound design capabilities:

#### Dual Oscillators

- **Two independent oscillators** with individual waveform selection
- **11 selectable waveforms**, including sine, triangle, square, saw, stepped, and pyramid shapes
- **Independent transpose and fine-tune controls** for each oscillator
- **Unison** up to **15 voices per oscillator**, with detune and harmonize controls
- **Sub-oscillator** for additional low-frequency content

#### Advanced Modulation

- **Cross-Modulation**: Bidirectional phase modulation between OSC1 ? OSC2
- **Frequency Modulation (FM)**: Unidirectional FM where OSC1 modulates OSC2
- **Ring Modulation**: Multiplies OSC1 × OSC2 for metallic, bell-like timbres
- **Hard Sync**: Resets OSC2's phase at each OSC1 cycle; OSC1 is the master and OSC2 the slave
- **Phase Stretching**: Variable waveform symmetry control (1%-99%) - works like PWM but applies to all waveform types, not just square waves

#### Oscillator Mixing

- Independent amplitude control for each oscillator
- Smooth blending between oscillators
- Sub-oscillator level control

---

### Filter Section

Sophisticated filtering with multiple topologies:

- **Filter Responses**: Low-pass, high-pass, and shelf responses
- **Filter Styles**: 12 dB, 24 dB, and shelf
- **Resonance Control**: Emphasizes the area around the cutoff
- **Filter Drive**: Adds saturation and harmonic richness
- **Key Tracking**: Makes filter cutoff follow note pitch
- **Filter Envelope**: Dedicated ADSR envelope for dynamic filter movement
- **Filter Blending**: Mix between different filter configurations

---

### Modulation System

Flexible and powerful modulation routing:

#### Envelopes

- **Amplitude Envelope**: AHDSR for volume shaping
- **Filter Envelope**: AHDSR for filter cutoff modulation
- **Modulation Envelope**: DAHDSR for general modulation purposes

#### LFOs (Low Frequency Oscillators)

- **4 Mono LFOs**: Shared modulation sources
- **2 Poly LFOs**: Per-voice modulation for evolving textures
- **Multiple waveforms**: Sine, triangle, saw, square, random, and more
- **Tempo Sync**: Lock LFO rates to host tempo
- **Free-running or retriggerable** operation

#### Step Sequencer

- **Graphical step sequencer** with real-time visual feedback
- **Up to 64 steps** per sequence
- **Adjustable step frequency** with tempo sync
- **Smooth interpolation** between steps
- **Step smoothing** for transitions between values

---

### Effects Section

Professional-grade effects processing:

#### Distortion

- **Four distortion types**: Soft Clip, Hard Clip, Linear Fold, and Sine Fold
- **Drive control**: From subtle warmth to aggressive saturation
- **Mix control**: Blend dry and distorted signals

#### Delay

- **Tempo-synchronized delay time** with free-time and musical-division options
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
- **Voice allocation** for playing chords and layered notes

#### Playability

- **Legato mode**: Smooth note transitions without retriggering envelopes
- **Portamento/Glide**: Smooth pitch transitions between notes
- **Pitch Bend**: Configurable range up to ±2 octaves
- **Velocity sensitivity**: Dynamic response to playing intensity
- **Aftertouch modulation sources**: Independent channel pressure and per-note polyphonic key pressure

#### Arpeggiator

- **Multiple arpeggio patterns**: Up, down, up/down, random, and more
- **Tempo sync**: Lock arpeggiator to host tempo
- **Octave range**: Span up to 4 octaves
- **Gate control**: Adjust note length from staccato to legato

---

## Architecture Highlights

### Signal Flow

Oscillators and additional sources feed the filter and amplifier; effects process the resulting sound before it reaches the output. Modulation sources can control supported parameters throughout the synth.

### Modulation Routing

- **Source selection** reveals modulation amount controls on supported destinations
- **Positive and negative modulation amounts**
- **Modulation depth control** for each connection
- **Visual feedback** of active modulations

### Preset Management

- **Factory presets** covering a wide range of sounds
- **User preset storage** for custom creations
- **Import/Export** functionality for sharing presets
- **Preset browser** with category filtering

The patch selector provides icon buttons for saving, Save As, exporting and
browsing patches. Save asks for confirmation before overwriting the currently
loaded patch; Save As opens the existing patch-saving dialog to choose a name
and destination. The Browse button changes to Edit while the browser is open.
Undo and Redo step through completed parameter-edit gestures. Undo restores the
starting value of the previous edit; Redo reapplies its final value. A new edit
after Undo discards the redo branch. The history is limited to 256 steps and is
cleared when another patch is loaded or the Init state is restored.

---

## Technical Specifications

- **Formats**: Standalone application and VST3 plug-in
- **Audio configuration**: The host or selected audio device determines the sample rate and buffer size

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
2. **Experiment with oscillator controls**, including Hard Sync, cross modulation, FM, and ring modulation
3. **Try the phase stretching** for unique waveform variations
4. **Add movement** with LFOs and envelopes
5. **Shape the tone** with the filter section
6. **Polish with effects** for professional-quality results

---

HelmBoy combines the warmth of classic analog synthesis with the precision and flexibility of modern digital design, providing a comprehensive tool for sound designers, producers, and performers.
