# Filter Types

## Overview

![Filter Types example](../pictures/Filter%20types.png)

The Type selector chooses the filter family. For the 12 dB and 24 dB families, Blend continuously morphs the response from low-pass through band-pass to high-pass. Blend does not select the family.

## Filter Families

### 12 dB and 24 dB

- **12 dB**: A gentler filter slope
- **24 dB**: A steeper filter slope

Use **Blend** to move between low-pass, band-pass, and high-pass responses. The Blend range and preset values are unchanged.

### Shelf (SH)

Select a low, band, or high shelf response with the shelf selector. Blend is not used in this family.

### Notch (N)

Attenuates frequencies around the cutoff while retaining frequencies above and below the notch.

### Comb (C)

Uses a cutoff-related delay with feedback to create a series of regularly spaced resonances. Resonance controls the feedback amount; the delay period is limited to 100 ms.

## Response Display

The response display follows the selected family and its relevant controls. Drag it to adjust cutoff and resonance.

## See Also

- [Filter](filter.md)
- [Filter Envelope](filter-envelope.md)
