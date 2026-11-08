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

### Channel Aftertouch (Channel Pressure)

- **Range**: 0 to 127
- **Usage**: Pressure applied after notes are pressed; the value is shared by active voices on that MIDI channel
- **Applications**:
  - Vibrato depth
  - Filter modulation
  - Volume swells

### Poly Aftertouch (Polyphonic Key Pressure)

Individual pressure per note (if the controller supports polyphonic key pressure). HelmBoy applies it to the matching active note on the matching MIDI channel.

## Setting Up Keyboard Modulation

1. Open modulation matrix
2. Select a source such as "Note", "Velocity", "Channel Aftertouch", or "Poly Aftertouch"
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

### Channel Aftertouch Vibrato

- Source: Channel Aftertouch
- Destination: LFO ? Pitch Amount
- Amount: +50% to +100%

### Poly Aftertouch Filter

- Source: Poly Aftertouch
- Destination: Filter Cutoff
- Amount: Adjust to taste for per-note timbre control

## See Also

- [Modulation](modulation.md)
- [Keyboard](keyboard.md)
- [Filter](filter.md)
