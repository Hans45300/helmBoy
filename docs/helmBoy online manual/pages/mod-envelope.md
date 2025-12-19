# Modulation Envelope

## Overview

![Modulation envelope section example](../pictures/Mod%20Envelope.png)

The modulation envelope is a flexible envelope generator that can modulate any parameter through the modulation matrix.

## Parameters

### Attack

Time to reach peak value after note trigger.

### Decay

Time to fall from peak to sustain level.

### Sustain

Level maintained while note is held.

### Release

Time to return to zero after note release.

## Routing

The modulation envelope can be routed to control:

- Filter cutoff and resonance
- Oscillator pitch and volume
- LFO rate and amplitude
- Effect parameters
- Any other modulatable parameter

## Common Applications

### Filter Sweep

Route to filter cutoff for classic synthesizer sounds.

### Pitch Envelope

Route to oscillator pitch for percussive attacks or drops.

### LFO Amount

Route to LFO amplitude for evolving modulation.

### Volume Ducking

Route to volume with negative amount for sidechain-like effects.

## See Also

- [Filter Envelope](filter-envelope.md)
- [Amplitude Envelope](amplitude-envelope.md)
- [Modulation](modulation.md)
