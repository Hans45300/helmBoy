# HelmBoy Oscillators - Technical Documentation

## Overview

HelmBoy features a sophisticated dual-oscillator engine that serves as the primary sound source for the synthesizer. Each oscillator provides multiple waveforms, advanced unison capabilities, and various modulation options including cross-modulation, frequency modulation (FM), and ring modulation.

## Architecture

### Core Components

The oscillator system consists of several key components:

- **Dual Oscillator Engine** (`HelmBoyOscillators`): Manages two independent oscillators with cross-modulation
- **Waveform Generator** (`Oscillator`): Produces band-limited waveforms using lookup tables
- **Unison Engine**: Generates up to 15 detuned voices per oscillator
- **Fixed-Point Wave System**: Optimized wavetable synthesis with precomputed buffers

### Signal Flow

```text
OSC1 and OSC2 -> oscillator modulation and sync -> oscillator mix -> filter -> effects -> output
```

## Oscillator Features

### Waveform Types

output volume is automatically reduced to prevent clipping. This is calculated as:

```text

| Index | Waveform | Description |
| ------- | ---------- | ------------- |
| 0 | Sine | Pure sine wave - fundamental harmonic only |
| 1 | Triangle | Triangular waveform - odd harmonics with 1/n² falloff |
| 2 | Square | Square wave - odd harmonics only |
| 3 | Down Saw | Downward sawtooth - all harmonics with 1/n falloff |
| 4 | Up Saw | Upward sawtooth - all harmonics with 1/n falloff |
| 5 | Three Step | 3-step staircase waveform |
| 6 | Four Step | 4-step staircase waveform |
| 7 | Eight Step | 8-step staircase waveform |
| 8 | Three Pyramid | 3-level pyramid waveform |
| 9 | Five Pyramid | 5-level pyramid waveform |
| 10 | Nine Pyramid | 9-level pyramid waveform |

**Note**: White noise is available but not selectable as a standard oscillator waveform.

### Band-Limited Synthesis

All waveforms (except sine and noise) use **band-limited synthesis** to prevent aliasing:

- **Dynamic Harmonic Limiting**: Number of harmonics automatically adjusted based on frequency
- **Lookup Tables**: Pre-computed harmonic series for efficient real-time synthesis
- **Frequency-Dependent**: Maximum harmonics = `20000 Hz / frequency - 1`
- **Quality Threshold**: If frequency allows ?100 harmonics, uses mathematical waveform directly

The band-limited algorithm ensures that:

- No aliasing artifacts occur at high frequencies
- Waveforms remain rich and full at low frequencies
- CPU usage is optimized for real-time performance

### Hard Sync

Hard Sync is a toggle parameter, `osc_hard_sync`, and is off by default. When enabled, Oscillator 1 is the master and resets Oscillator 2's phase at each OSC1 cycle. The reset also applies to Oscillator 2's unison voices. Hard Sync is available as a modulation destination.

### Pitch Control

Each oscillator has independent pitch control with three parameters:

#### Transpose (osc_1_transpose, osc_2_transpose)

- **Range**: -48 to +48 semitones
- **Default**: 0 (no transposition)
- **Units**: Semitones
- **Purpose**: Coarse pitch adjustment, useful for stacking oscillators at octave intervals

#### Tune (osc_1_tune, osc_2_tune)

- **Range**: -100 to +100 cents
- **Default**: 0 cents
- **Units**: Cents (1/100 of a semitone)
- **Purpose**: Fine-tuning and slight detuning effects

#### Phase Stretch (osc_1_phase_stretch, osc_2_phase_stretch)

- **Range**: 0.01 to 0.99 (1% to 99%)
- **Default**: 0.5 (50% - no stretching)
- **Units**: Percentage
- **Purpose**: Alters waveform shape by remapping phase:
  - Values < 0.5: First half of waveform compressed, second half stretched
  - Values > 0.5: First half stretched, second half compressed
  - Creates asymmetric waveforms and harmonic variations

### Volume Control

Each oscillator has independent volume control:

#### Volume (osc_1_volume, osc_2_volume)

- **Range**: 0.0 to 1.0 (linear)
- **Default**: 0.5477225575 (?-5.23 dB, accounting for unison voices)
- **Scaling**: Quadratic response for better perception
- **Purpose**: Balance between oscillators and prevent clipping with unison

## Unison System

The unison engine creates rich, wide sounds by generating multiple slightly detuned copies of each oscillator.

### Unison Voices

#### Voice Count (osc_1_unison_voices, osc_2_unison_voices)

- **Range**: 1 to 15 voices
- **Default**: 1 (no unison)
- **Automatic Scaling**: Output level automatically adjusted by `1/?(voices)` to prevent clipping
- **Performance**: More voices = higher CPU usage

**Voice Scaling Table**:
| Voices | Scale Factor | Level (dB) |

|--------|--------------|------------|
| 1 | 1.000 | 0.00 |
| 2 | 0.707 | -3.01 |
| 3 | 0.577 | -4.77 |
| 4 | 0.500 | -6.02 |
| 5 | 0.447 | -6.99 |
| 8 | 0.354 | -9.03 |
| 15 | 0.258 | -11.76 |

### Unison Detune

#### Detune Amount (osc_1_unison_detune, osc_2_unison_detune)

- **Range**: 0 to 100 cents
- **Default**: 10 cents
- **Units**: Cents
- **Distribution**: Voices detuned symmetrically around base pitch
  - Odd voices: Detuned upward
  - Even voices: Detuned downward
  - Amount increases with voice number
- **Formula**: `detune_amount = (base_detune × (voice_number + 1) / 2) / ((total_voices + 1) / 2)`

**Example** (8 voices, 10 cents detune):

- Voice 1 (center): 0 cents
- Voice 2: -5 cents
- Voice 3: +10 cents
- Voice 4: -10 cents
- Voice 5: +15 cents
- Voice 6: -15 cents
- Voice 7: +20 cents
- Voice 8: -20 cents

### Harmonize Mode

#### Harmonize (osc_1_unison_harmonize, osc_2_unison_harmonize)

- **Type**: Toggle button (on/off)
- **Default**: Off
- **Effect**: When enabled, unison voices are tuned to harmonic intervals instead of detuned cents
- **Voice Tuning**: Voice N tuned to (N × fundamental frequency)
- **Use Cases**: Creating organ-like sounds, rich harmonic stacks, formant-like effects

## Cross-Modulation

Cross-modulation allows oscillators to modulate each other's phase, creating complex harmonic interactions.

### Cross Modulation Amount (cross_modulation)

- **Range**: 0.0 to 0.5
- **Default**: 0.0 (no modulation)
- **Units**: Percentage (0-200% displayed)
- **Algorithm**:
  - Oscillator 1 and 2 modulate each other's phase
  - Creates mutual phase modulation (both oscillators affect each other)
  - Generates complex, evolving timbres with intermodulation products
  - Similar to feedback FM synthesis

### Implementation Details

- **Phase Modulation**: Output phase of each oscillator affects the other's phase input
- **Bidirectional**: Both oscillators modulate each other simultaneously
- **Non-linear**: Creates sum and difference frequencies
- **Harmonic Content**: Adds inharmonic partials at higher settings

## Frequency Modulation (FM)

FM synthesis uses one oscillator to modulate the frequency of another, creating rich harmonic spectra.

### FM Amount (FM_amount)

- **Range**: 0.0 to 1.0
- **Default**: 0.0 (no FM)
- **Units**: Percentage (0-200% displayed)
- **Modulation Routing**: Oscillator 2 frequency modulates Oscillator 1
- **Characteristics**:
  - **Low settings** (0.0-0.3): Subtle harmonic enrichment
  - **Medium settings** (0.3-0.6): Bell-like and metallic tones
  - **High settings** (0.6-1.0): Complex inharmonic spectra, aggressive timbres

### FM Theory

- **Sidebands**: Creates sidebands at `carrier_freq ± (n × modulator_freq)`
- **Harmonic Series**: When carrier/modulator ratio is simple (1:1, 2:1, etc.), sidebands are harmonic
- **Inharmonic Series**: When ratio is complex (e.g., 1:1.3), sidebands are inharmonic
- **Modulation Index**: Controlled by FM_amount parameter, determines number and intensity of sidebands

## Ring Modulation

Ring modulation multiplies two oscillator signals, creating sum and difference frequencies.

### Ring Mod Amount (Not documented in provided files but exists in UI)

- **Range**: Typically 0.0 to 1.0
- **Effect**: Multiplies Oscillator 1 and Oscillator 2 outputs
- **Output Frequencies**:
  - Sum: `f1 + f2`
  - Difference: `|f1 - f2|`
- **Characteristics**:
  - Generally creates inharmonic, metallic sounds
  - When oscillators are tuned to simple ratios, can create harmonic content
  - Useful for bell and metallic percussion sounds

## Oscillator Feedback

The oscillator section includes a feedback path for creating self-modulating, resonant timbres.

### Feedback Amount (osc_feedback_amount)

- **Range**: -1.0 to 1.0 (-100% to +100%)
- **Default**: 0.0 (no feedback)
- **Effect**: Routes oscillator output back to its phase input
- **Polarity**:
  - Positive values: Normal feedback
  - Negative values: Inverted feedback
- **Sonic Result**: Creates resonances, formants, and aggressive timbres

### Feedback Transpose (osc_feedback_transpose)

- **Range**: -24 to +24 semitones
- **Default**: 0 semitones
- **Purpose**: Shifts feedback signal pitch before feeding back

### Feedback Tune (osc_feedback_tune)

- **Range**: -100 to +100 cents
- **Default**: 0 cents
- **Purpose**: Fine-tunes feedback pitch for precise control of resonances

## Technical Implementation

### Phase Accumulation

The oscillators use **fixed-point phase accumulation** for efficiency:

- **Phase Range**: 0 to UINT_MAX (unsigned 32-bit integer)
- **Wrap-around**: Automatic modulo operation when phase exceeds maximum
- **Precision**: 32-bit resolution provides excellent pitch accuracy
- **Sample-accurate**: Phase calculated per-sample for precise modulation

### Reset Behavior

Oscillators support sample-accurate reset via the `kReset` input:

- **Trigger Detection**: Checks for `kVoiceReset` trigger value
- **Phase Reset**: Resets oscillator phase to 0
- **Unison Reset**:
  - Voice 1 (center) resets to phase 0
  - Additional voices reset to random phases to avoid phase cancellation
- **Cross-mod Reset**: Clears cross-modulation and FM buffers

### SIMD Optimization

The oscillator implementation uses SIMD (Single Instruction, Multiple Data) operations for performance:

- **Library**: xsimd library for cross-platform SIMD
- **Batch Processing**: Processes multiple samples simultaneously
- **Frequency Calculation**: Vectorized frequency-to-phase conversion
- **Limitations**: Waveform lookup and modulation remain scalar for accuracy

### Buffer Management

Each unison voice maintains separate buffers:

- **Wave Buffers**: Pre-computed waveform samples for each voice
- **Phase Buffers**: Independent phase accumulation per voice
- **Detune Buffers**: Pre-calculated detune offsets
- **Optimization**: Buffers prepared once per audio block for efficiency

## Parameter Relationships

### Unison and Volume

When unison voice count increases, the output volume is automatically reduced to prevent clipping. This is calculated as:

```text
output_scale = 1.0 / sqrt(num_voices)
```

### FM and Oscillator Frequency

FM intensity is frequency-dependent. The same FM_amount setting produces different timbral results at different pitches:

- **Low frequencies**: Slower modulation, warmer tones
- **High frequencies**: Faster modulation, brighter, more aggressive tones

### Phase Stretch and Waveform

Phase stretch has different effects on different waveforms:

- **Sine**: Minimal effect (sine remains sine)
- **Square/Saw/Triangle**: Alters duty cycle and harmonic balance
- **Complex waveforms**: Creates unique asymmetric variations

## Best Practices

### Creating Rich Sounds

1. **Start with unison**: Use 4-8 voices with 10-20 cent detune for instant width
2. **Layer oscillators**: Detune Osc 2 by 5-10 cents from Osc 1
3. **Add octaves**: Use transpose to stack oscillators at different octaves
4. **Blend waveforms**: Mix different waveforms between oscillators for complexity

### Avoiding Phase Issues

1. **Random phases**: Unison automatically randomizes phases for voices 2+
2. **Detune slightly**: Even small detune amounts (1-2 cents) prevent phase cancellation
3. **Vary waveforms**: Use different waveforms per oscillator to minimize comb filtering

### CPU Optimization

1. **Limit unison voices**: Use only as many voices as needed (4-8 typically sufficient)
2. **Disable unused modulation**: Set cross_modulation and FM_amount to 0 when not needed
3. **Consider waveform complexity**: Simple waveforms (sine, triangle) are more efficient than complex ones

### FM Synthesis Tips

1. **Carrier/Modulator**: Oscillator 1 acts as carrier, Oscillator 2 as modulator
2. **Harmonic FM**: Tune oscillators to harmonic ratios (1:1, 2:1, 3:2) for musical tones
3. **Inharmonic FM**: Use detuned ratios for bells and metallic sounds
4. **Envelope modulation**: Modulate FM_amount with envelopes for evolving timbres

## Performance Characteristics

### CPU Usage (Approximate)

- Single oscillator, 1 voice: ~2% CPU (modern processor)
- Dual oscillators, 1 voice each: ~4% CPU
- Dual oscillators, 8 voices each: ~15% CPU
- Dual oscillators, 15 voices each: ~30% CPU

### Latency

- **Processing**: Zero-latency (real-time sample generation)
- **Modulation**: Sample-accurate (no block-rate smoothing on frequency input)

### Quality Settings

- **Sample Rate**: Oscillators adapt to host sample rate automatically
- **Anti-aliasing**: Band-limited synthesis quality depends on harmonic count
- **Interpolation**: Linear interpolation between waveform samples

## Modulation Targets

The following oscillator parameters can be modulated via the modulation matrix:

- **Oscillator 1/2 Volume**: Dynamic level control
- **Oscillator 1/2 Transpose**: Vibrato and pitch effects
- **Oscillator 1/2 Tune**: Micro-pitch variations
- **Oscillator 1/2 Unison Detune**: Dynamic width control
- **Cross Modulation Amount**: Evolving timbral modulation
- **FM Amount**: Dynamic FM synthesis
- **Phase Stretch**: Waveform shape modulation
- **Feedback Amount**: Dynamic feedback intensity

## Common Use Cases

### 1. Super Saw (Trance Lead)

- Waveform: Up Saw (both oscillators)
- Unison Voices: 7-8 per oscillator
- Unison Detune: 15-20 cents
- Oscillator 2 Detune: +7 cents
- Cross Modulation: 0.0
- FM Amount: 0.0

### 2. Warm Analog Pad

- Waveform: Triangle (Osc 1), Square (Osc 2)
- Unison Voices: 4 per oscillator
- Unison Detune: 8-12 cents
- Oscillator 2 Transpose: -12 semitones (one octave down)
- Cross Modulation: 0.05-0.1 (adds movement)

### 3. FM Bell

- Waveform: Sine (both oscillators)
- Unison Voices: 1
- Oscillator 2 Transpose: +19 semitones (harmonic ratio)
- Cross Modulation: 0.0
- FM Amount: 0.4-0.7
- Envelope: Fast attack, long decay on FM_amount

### 4. Aggressive Bass

- Waveform: Square (Osc 1), Up Saw (Osc 2)
- Unison Voices: 2-3
- Unison Detune: 5 cents
- Cross Modulation: 0.2-0.3
- Phase Stretch: 0.3 or 0.7 (asymmetric waveform)
- Filter: Low-pass with resonance

### 5. Harmonic Stack (Organ-like)

- Waveform: Sine or Triangle
- Unison Voices: 6-8
- Unison Detune: 0 cents
- Harmonize: ON
- Volume: Reduced to prevent clipping

## Troubleshooting

### Problem: Thin Sound

- **Solution**: Increase unison voices or detune amount
- **Alternative**: Add slight cross-modulation (0.05-0.1)

### Problem: Harsh/Aliased Sound

- **Cause**: Very high frequencies with complex waveforms
- **Solution**: Use simpler waveforms (sine, triangle) at high registers
- **Note**: Band-limiting should prevent this; may indicate buffer issues

### Problem: Volume Drops with Unison

- **Cause**: Automatic scaling to prevent clipping
- **Solution**: This is by design; increase post-oscillator gain if needed
- **Alternative**: Use fewer voices with higher detune for similar effect

### Problem: Phasing/Hollow Sound

- **Cause**: Phase cancellation between oscillators
- **Solution**: Detune oscillators slightly (2-5 cents)
- **Alternative**: Use different waveforms per oscillator

### Problem: CPU Overload

- **Solution 1**: Reduce unison voice count
- **Solution 2**: Disable unused modulation (cross-mod, FM)
- **Solution 3**: Use simpler waveforms

## Related Documentation

- **Filters**: `FILTERS.md` (when available) - Post-oscillator filtering
- **Modulation**: `MODULATION.md` (when available) - LFO and envelope sources
- **Voice Architecture**: `VOICE_ARCHITECTURE.md` (when available) - Voice allocation and routing

## References

### Source Files

- `mopo/src/oscillator.h` - Base oscillator implementation
- `mopo/src/oscillator.cpp` - Oscillator processing logic
- `mopo/src/wave.h` - Waveform generation and lookup tables
- `src/synthesis/helmBoy_oscillators.h` - Dual oscillator engine interface
- `src/synthesis/helmBoy_oscillators.cpp` - Dual oscillator implementation
- `src/synthesis/fixed_point_wave.h` - Fixed-point wavetable system
- `src/synthesis/detune_lookup.h` - Detune calculation tables
- `src/editor_sections/oscillator_section.h` - UI implementation
- `src/common/helmBoy_common.cpp` - Parameter definitions

### Algorithm References

- **Band-limited Synthesis**: Based on additive synthesis with limited harmonics
- **FM Synthesis**: Classic frequency modulation (Chowning method)
- **Phase Modulation**: Digital implementation of through-zero FM
- **Unison**: Based on Roland SuperSaw and modern synthesizer techniques

---

*This documentation describes the oscillator system as implemented in helmBoy. Parameters and behavior may be subject to change in future versions.*
