# Mono LFO

## Overview

![Mono LFO section example](../pictures/Mono%20LFO.png)

Mono LFOs (Low-Frequency Oscillators) provide modulation that is shared across all voices, creating synchronized movement.

## Parameters

### Frequency/Rate

- **Range**: 0.01 Hz to 20 Hz (or tempo-synced)
- **Sync**: Can lock to tempo for rhythmic modulation

### Waveform

- Sine: Smooth, cyclical modulation
- Triangle: Linear ramps
- Square: On/off switching
- Sawtooth: Ramp up or down
- Random: Sample-and-hold random values

### Amplitude

Modulation depth from the LFO.

### Phase

Starting position of the LFO cycle.

### Retrigger

- **Free**: LFO runs continuously
- **Trigger**: Resets phase on each new note
- **Sync**: Resets to tempo grid

## Mono vs Poly

**Mono LFOs**:

- Single LFO shared by all voices
- All notes modulated identically
- Good for synchronized effects

**Poly LFOs**:

- Independent LFO per voice
- Each note has its own modulation
- Good for evolving, complex textures

## Applications

- Vibrato (pitch modulation)
- Tremolo (volume modulation)
- Filter sweeps
- Panning effects
- Rhythmic modulation

## See Also

- [Poly LFO](poly-lfo.md)
- [Modulation](modulation.md)
- [Step Sequencer](step-sequencer.md)
