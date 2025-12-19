# Voice Settings

## Overview

![Voices section example](../pictures/Voices.png)

Voice configuration determines how helmBoy handles multiple simultaneous notes.

## Parameters

### Polyphony

- **Range**: 1 to 32 voices
- **Monophonic**: 1 voice
- **Polyphonic**: Multiple voices for chords

### Voice Mode

- **Poly**: Each note triggers a new voice
- **Mono**: Single voice, legato transitions
- **Unison**: All voices play same note (stacked)

### Voice Stealing

When polyphony limit is reached:

- **Oldest**: Steal oldest playing note
- **Quietest**: Steal quietest voice
- **None**: Ignore new notes when full

## Unison Mode

When in unison mode:

- All voices play the same pitch
- Slight detuning creates wide stereo image
- Voice count determines thickness

## See Also

- [Portamento](portamento.md)
- [Oscillators](oscillators.md)
