# Portamento

## Overview

![Portamento section example](../pictures/Portamento.png)

Portamento (also called glide) smoothly transitions pitch between notes rather than jumping instantly.

## Parameters

### Portamento Time

- **Range**: 0 ms to 2 seconds
- **0**: Instant pitch change (no portamento)
- **Higher values**: Slower, more gradual pitch glide

### Portamento Mode

#### Always

Portamento applied between all notes.

#### Legato

Portamento only when playing legato (overlapping notes).

- No portamento when playing staccato
- Natural, expressive playing

#### Retrigger

- **Off**: Envelopes don't retrigger during portamento
- **On**: Envelopes retrigger for each new note

### Portamento Type

#### Linear

Constant rate pitch change.

- Equal speed throughout glide

#### Exponential

Natural, curved pitch transition.

- Fast start, slows toward target
- More musical sounding

## Applications

### Smooth Leads

Medium portamento time (50-200 ms) for smooth melodic lines.

### Bass Slides

Longer portamento (200-500 ms) for bass glides.

### Pitch Effects

Very long portamento (500+ ms) for special effects.

### Expressive Playing

Legato mode for natural musical phrasing.

## Usage Tips

- **Monophonic sounds**: Portamento most effective in mono mode
- **Legato playing**: Use legato mode for natural expression
- **Avoid on chords**: Disable for polyphonic/chord playing
- **Envelope retriggering**: Off for smooth legato, on for percussive

## See Also

- [Voices](voices.md)
- [Keyboard](keyboard.md)
