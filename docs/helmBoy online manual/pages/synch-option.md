# Synchronization Options

## Overview

![Sync block example](../pictures/Synch%20Option.png)

HelmBoy provides separate controls for tempo-based rates, note retriggering, and oscillator hard synchronization. These controls do not use an external MIDI clock.

## Sync Types

### Tempo-Based Rates

The LFOs, delay, arpeggiator, step sequencer, and stutter provide rate or time selectors. Depending on the module, choose free-running seconds or a host/internal tempo division: straight, dotted, or triplet.

Both the tempo division (`*_tempo`) and the synchronization mode (`*_sync`) can be modulation destinations. The synchronization mode follows the nearest available choice and displays the effective mode in green while modulation changes it; tempo-division controls show the existing modulation amount indicator.

### LFO and Sequencer Retriggering

Where a retrigger selector is present, its modes are **Free**, **Retrigger**, and **Sync to Playhead**. These control how the modulation cycle responds to notes or the host playhead; they are distinct from tempo-rate selection. Retrigger modes can also be modulation destinations: modulation selects the nearest mode, and the selector highlights the effective mode when it changes.

### Oscillator Hard Sync

The **Hard Sync** button in the oscillator section is off by default. When enabled, Oscillator 1 resets Oscillator 2's phase at every master cycle. It is not a gradual or soft-sync mode, and it can also be controlled by modulation.
