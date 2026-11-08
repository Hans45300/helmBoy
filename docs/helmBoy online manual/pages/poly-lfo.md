# Poly LFO

## Overview

![Poly LFO section example](../pictures/Poly%20LFO.png)

HelmBoy has two Poly LFOs. Each voice has an independent LFO phase, allowing notes to move differently from one another.

## Parameters

### Frequency/Rate

- Choose a free-running rate or a tempo-synchronized division from 64/1
  through 1/128, including dotted and triplet values.

### Waveform

- Sine, triangle, and square
- Up and down sawtooth
- 3-, 4-, and 8-step shapes
- 3-, 5-, and 9-level pyramid shapes
- Sample and hold, sample and glide, and Smooth Random

### Amplitude

Modulation depth from the LFO.

### Phase

**PH** offsets the waveform cycle from -180° to +180°. At 0°, the LFO retains
its default phase. The waveform viewer previews the offset.

### Phase Stretch

The **PS** control changes the LFO waveform's phase shape independently of PH.
The waveform viewer previews both controls.

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
