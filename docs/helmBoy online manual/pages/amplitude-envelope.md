# Amplitude Envelope

## Overview

![Amplitude envelope example](../pictures/Amplitude%20Envelope.png)

The amplitude envelope controls each note's volume with five stages: Attack, Hold, Decay, Sustain, and Release (AHDSR).

## Parameters

### Attack

- **Range**: 0 to 4 seconds
- **Effect**: Time for volume to reach maximum after note is triggered
- **Fast**: Percussive, immediate sound
- **Slow**: Soft, gradual fade-in

### Hold

- **Range**: 0 to 4 seconds
- **Effect**: Time the envelope stays at its peak after Attack and before Decay

### Decay

- **Range**: 0 to 4 seconds
- **Effect**: Time to fall from peak to sustain level
- **Fast**: Plucked sound character
- **Slow**: Sustained sound character

### Sustain

- **Range**: 0% to 100%
- **Effect**: Volume level maintained while note is held
- **High**: Pad and organ-like sounds
- **Low**: Plucked and percussive sounds

### Release

- **Range**: 0 to 4 seconds
- **Effect**: Time for volume to fade after note is released
- **Short**: Abrupt ending
- **Long**: Gradual fade-out

The envelope shape, including the Hold plateau, is shown in the envelope display. There are no separate attack, decay, or release curve selectors.

## Common Settings

### Plucked/Struck

- Attack: Fast (1-10 ms)
- Decay: Medium (100-500 ms)
- Sustain: Low (0-30%)
- Release: Short (50-200 ms)

### Pad/String

- Attack: Medium to slow (50-500 ms)
- Decay: Medium (200-800 ms)
- Sustain: High (70-100%)
- Release: Long (500-2000 ms)

### Organ

- Attack: Fast (1-5 ms)
- Decay: Fast (10-50 ms)
- Sustain: Full (100%)
- Release: Short (10-100 ms)

### Percussion

- Attack: Fast (1-5 ms)
- Decay: Medium (100-500 ms)
- Sustain: Zero (0%)
- Release: Short (10-100 ms)

## See Also

- [Filter Envelope](filter-envelope.md)
- [Mod Envelope](mod-envelope.md)
- [Volume](volume.md)
