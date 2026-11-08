# Noise Generator

## Overview

The noise generator provides a colored-noise source that can be mixed with the oscillators.

## Parameters

### Level

The Noise panel has a single **Amp** control. Its level is also available in the [Mixer](mixer.md).

### Noise Color

The **NC** knob in the **SUB / NOISE** section continuously changes the noise
spectrum from Brown (minimum) through Pink, White, and Blue to Violet (maximum).
White at the centre produces the original noise samples unchanged. Noise Color
supports automation and modulation; changes are smoothed during playback.

The coloration blends a low-pass Brown reference and a high-pass Violet
reference around the unchanged White signal. The filter corner is 1 kHz, and
the blend curve has smooth transitions through the Brown/Pink/White/Blue/Violet
regions.

## Applications

- **Percussion**: Snare drums, hi-hats, cymbals
- **Wind Effects**: Breaths, wind sounds
- **Texture**: Add air and presence to sounds
- **Transients**: Enhance attack phase

## Usage Tips

- Use sparingly for subtle texture
- Filter noise for specific frequency ranges
- Modulate noise volume with envelopes for percussive effects

## See Also

- [Mixer](mixer.md)
- [Filter](filter.md)
