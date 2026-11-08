# Modulation Envelope

## Overview

![Modulation envelope section example](../pictures/Mod%20Envelope.png)

HelmBoy provides four independent DAHDSR modulation envelopes. Each can modulate supported parameters through the modulation system. Their stages are Delay, Attack, Hold, Decay, Sustain, and Release, and all four share the same note-trigger behaviour.

## Parameters

### Delay

Time from the note trigger to the start of Attack.

### Attack

Time to reach peak value after note trigger.

### Hold

Time the envelope stays at its peak after Attack and before Decay.

### Decay

Time to fall from peak to sustain level.

### Sustain

Level maintained while note is held.

### Release

Time to return to zero after note release.

## Routing

Each modulation envelope can be routed independently to control:

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
