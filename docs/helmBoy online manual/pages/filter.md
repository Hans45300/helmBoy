# Filter

## Overview

![Filter section example](../pictures/Filter.png)

The filter section shapes the frequency content of the sound, removing or emphasizing certain frequencies.

## Parameters

### Cutoff Frequency

- **Range**: 20 Hz to 20 kHz
- **Function**: Sets the filter's corner frequency
- **Modulation**: Can be modulated by envelopes, LFOs, and keyboard tracking

### Resonance

- **Range**: 0% to 100%
- **Effect**: Emphasizes frequencies around the cutoff
- **High values**: Creates peaks, self-oscillation

### Filter Drive

- **Range**: 0 to 100%
- **Effect**: Adds saturation and harmonics to the filter
- **Character**: Adds warmth and analog character

### Key Tracking

- **Range**: -100% to +100%
- **Effect**: Makes cutoff follow played note
- **100%**: Filter cutoff tracks keyboard 1:1
- **0%**: No keyboard tracking

## See Also

- [Filter Types](filter-types.md)
- [Filter Envelope](filter-envelope.md)
- [Formant](formant.md)
