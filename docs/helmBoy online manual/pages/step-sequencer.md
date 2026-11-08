# Step Sequencer

## Overview

![Step Sequencer example](../pictures/Step%20Sequencer.png)

The step sequencer is a modulation source that steps through a pattern of values, creating rhythmic and melodic modulation.

## Parameters

### Number of Steps

- **Range**: 1 to 64 steps
- **Effect**: Length of the sequence pattern

### Step Values

Each step has an adjustable value that defines the modulation amount at that step.

### Frequency, Sync, and Retrigger

- Choose free-running time or tempo divisions, including straight, dotted, and triplet values.
- The retrigger selector offers **Free**, **Retrigger**, and **Sync to Playhead**.

### Slide

- The Slide control smooths transitions between step values.

Each of the 64 step controls sets that step's modulation value. The **Steps** control selects how many are used in the pattern.

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

Edit the graphical step controls directly. The sequencer displays its current position and values during playback.

## Keyboard Shortcuts

Click a step in the graph to select it and give the sequencer keyboard focus.

- **Left / Right Arrow**: Select the previous or next step. Selection wraps at the ends; if no step is selected, either key selects the first step.
- **Up / Down Arrow**: Increase or decrease the selected step by `0.01`.
- **Ctrl + Up / Down Arrow**: Change the selected step by `0.1`.
- **Shift + Up / Down Arrow**: Change the selected step by one semitone (`1/48`).
- **Ctrl + Shift + Up / Down Arrow**: Change the selected step by six semitones (`6/48`).

Step values are limited to `-1` through `+1`.

## See Also

- [Modulation](modulation.md)
- [Arp](arp.md)
- [Stutter](stutter.md)
