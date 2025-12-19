# Keyboard Modulation

## Overview

![Keyboard modulation section example](../pictures/Keyboard%20Mod.png)

Keyboard modulation uses note number and velocity as modulation sources for dynamic, expressive control.

## Modulation Sources

### Note Number (Key Tracking)

- **Range**: MIDI note 0 to 127
- **Usage**: Higher notes = higher modulation value
- **Applications**:
  - Filter cutoff tracking (brighter notes higher up keyboard)
  - Volume adjustment (louder higher notes)
  - Envelope timing (faster envelopes higher up)

### Velocity

- **Range**: 0 to 127
- **Usage**: Harder hits = higher modulation value
- **Applications**:
  - Filter cutoff (harder hits open filter more)
  - Volume dynamics (harder hits = louder)
  - Envelope amount (harder hits = more envelope)

### Aftertouch (Channel Pressure)

- **Range**: 0 to 127
- **Usage**: Pressure after note is pressed
- **Applications**:
  - Vibrato depth
  - Filter modulation
  - Volume swells

### Polyphonic Aftertouch

Individual pressure per note (if controller supports it).

## Setting Up Keyboard Modulation

1. Open modulation matrix
2. Select "Note" or "Velocity" as source
3. Choose destination parameter
4. Adjust modulation amount
5. Positive or negative polarity

## Common Routings

### Natural Filter Tracking

- Source: Note Number
- Destination: Filter Cutoff
- Amount: +30% to +50%

### Velocity to Volume

- Source: Velocity
- Destination: Volume
- Amount: +50% to +100%

### Velocity to Filter

- Source: Velocity
- Destination: Filter Cutoff
- Amount: +40% to +70%

### Aftertouch Vibrato

- Source: Aftertouch
- Destination: LFO ? Pitch Amount
- Amount: +50% to +100%

## See Also

- [Modulation](modulation.md)
- [Keyboard](keyboard.md)
- [Filter](filter.md)
