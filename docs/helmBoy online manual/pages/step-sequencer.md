# Step Sequencer

## Overview

![Step Sequencer example](../pictures/Step%20Sequencer.png)

The step sequencer is a modulation source that steps through a pattern of values, creating rhythmic and melodic modulation.

## Parameters

### Number of Steps

- **Range**: 1 to 32 steps
- **Effect**: Length of the sequence pattern

### Step Values

Each step has an adjustable value that defines the modulation amount at that step.

### Frequency/Rate

- **Tempo Sync**: Locks to musical divisions (1/4, 1/8, 1/16, etc.)
- **Free**: Independent timing

### Smoothing

- **Off**: Steps change instantly (stepped)
- **On**: Steps interpolate smoothly (glide)

### Gate

Each step can be on or off, creating rhythmic patterns.

## Applications

### Melodic Sequences

Route to oscillator pitch for arpeggiated melodies.

### Rhythmic Filter

Route to filter cutoff for rhythmic timbral changes.

### Rhythmic Volume

Route to volume for gating effects.

### Modulation Rhythms

Route to LFO rate or other modulators for complex patterns.

## Editing

- Click steps to adjust values
- Drag to draw patterns
- Right-click for step options
- Copy/paste patterns

## See Also

- [Modulation](modulation.md)
- [Arp](arp.md)
- [Stutter](stutter.md)
