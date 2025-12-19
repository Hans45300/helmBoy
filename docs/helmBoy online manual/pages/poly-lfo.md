# Poly LFO

## Overview

![Poly LFO section example](../pictures/Poly%20LFO.png)

Poly LFOs (Low-Frequency Oscillators) provide independent modulation for each voice, creating complex and evolving textures.

## Parameters

### Frequency/Rate

- **Range**: 0.01 Hz to 20 Hz (or tempo-synced)
- **Per-voice**: Each voice can have different LFO phase

### Waveform

- Sine: Smooth, cyclical modulation
- Triangle: Linear ramps
- Square: On/off switching
- Sawtooth: Ramp up or down
- Random: Sample-and-hold random values

### Amplitude

Modulation depth from the LFO.

### Phase

Starting position of the LFO cycle for the first voice.

### Retrigger

- **Free**: LFO continues from previous position
- **Trigger**: Resets phase on each new note

## Mono vs Poly

**Poly LFOs**:

- Independent LFO per voice
- Each note has unique modulation phase
- Creates chorus-like effects
- More organic, evolving sounds

**Mono LFOs**:

- Single LFO shared by all voices
- All notes modulated identically
- More predictable, synchronized

## Applications

- Chorus effects (slight pitch modulation)
- Evolving pad sounds
- Organic texture on sustained notes
- Per-note vibrato with different phases
- Complex filter movements

## See Also

- [Mono LFO](mono-lfo.md)
- [Modulation](modulation.md)
