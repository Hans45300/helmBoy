# Filter Types

## Overview

![Filter Types example](../pictures/Filter%20types.png)

helmBoy offers multiple filter types, each with unique frequency response characteristics.

## Available Types

### Low Pass (LP)

Allows frequencies below the cutoff to pass through.

- **12 dB/oct**: Gentle slope, smooth sound
- **24 dB/oct**: Steep slope, more pronounced filtering

### High Pass (HP)

Allows frequencies above the cutoff to pass through.

- **12 dB/oct**: Gentle high-pass filtering
- **24 dB/oct**: Steep high-pass filtering

### Band Pass (BP)

Allows frequencies around the cutoff to pass through.

- **12 dB/oct**: Wide band
- **24 dB/oct**: Narrow band

### Notch

Removes frequencies around the cutoff.

- Creates phaser-like effects
- Useful for removing specific frequency ranges

### Comb

Creates peaks at harmonic intervals.

- Metallic, resonant character
- Physical modeling applications

### Low Shelf

Boosts or cuts frequencies below the cutoff.

### High Shelf

Boosts or cuts frequencies above the cutoff.

### All Pass

Affects phase without changing amplitude.

- Creates phase effects
- Useful for sound design

## See Also

- [Filter](filter.md)
- [Filter Envelope](filter-envelope.md)
