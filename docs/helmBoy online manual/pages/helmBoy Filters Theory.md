# HelmBoy Filters - Technical Documentation

> **Scope:** This supplementary DSP document is not a reference to the current filter controls. Some modes, parameter names, and internal ranges below describe implementation concepts or legacy behavior and are not selectable in the current interface. For user-accessible controls, see [Filter](filter.md) and [Filter Types](filter-types.md).

## Overview

HelmBoy features a sophisticated multi-mode filter system that shapes the frequency content of synthesized sounds. The filter engine provides both traditional subtractive filtering and advanced formant synthesis capabilities, with extensive modulation options for dynamic timbral control.

## Architecture

### Core Filter Components

The filter system consists of three main components:

- **State Variable Filter (SVF)**: Primary multi-mode filter with 12dB/24dB slopes
- **Formant Filter**: Vocal synthesis using parallel bandpass filters
- **Biquad Filters**: High-quality RBJ biquad implementations for various filter types

### Signal Flow

```cpp

Oscillators + Sub + Noise
         ?
    [Mixer]
         ?
  [State Variable Filter] ?? [Formant Filter]
    (12dB/24dB/Shelf)        (Vowel Synthesis)
         ?                           ?
         ????????[Blend]??????????????
                   ?
             [Effects]
                   ?
              [Output]
```

## State Variable Filter (SVF)

The primary filter in helmBoy uses a state variable topology, providing high-quality filtering with multiple modes and slopes.

### Filter Styles

#### 12dB/octave Mode

- **Slope**: -12 dB per octave beyond cutoff
- **Character**: Gentle, smooth filtering
- **Use Cases**:
  - Subtle tone shaping
  - Vintage synthesizer emulation
  - Natural-sounding filtering
  - Preserving more harmonics

#### 24dB/octave Mode

- **Slope**: -24 dB per octave beyond cutoff
- **Character**: Steep, aggressive filtering
- **Use Cases**:
  - Classic synthesizer sounds
  - Pronounced filter sweeps
  - Emphasizing resonance peaks
  - Removing unwanted frequencies completely

#### Shelf Mode

- **Character**: Boost or cut specific frequency regions
- **Types**: Low Shelf, Band Shelf, High Shelf
- **Use Cases**:
  - EQ-style tone shaping
  - Adding brightness or warmth
  - Frequency balance adjustments
  - Corrective filtering

### Filter Parameters

#### Cutoff Frequency (filter_cutoff)

- **Range**: 1 Hz to Nyquist frequency (sample rate / 2)
- **Default**: Varies by preset
- **Units**: Hertz (Hz)
- **Function**: Sets the corner frequency where filtering begins
- **Modulation**: Primary target for envelopes and LFOs
- **Display**: Typically shown in Hz or MIDI note number

**Frequency Response**:

- Frequencies below cutoff (lowpass): Pass through
- Frequencies at cutoff: 3dB attenuation
- Frequencies above cutoff: Attenuated by slope (12dB or 24dB/octave)

#### Resonance (filter_resonance)

- **Range**: 0.1 to 16.0 (internal units)
- **Default**: Varies by preset
- **Function**: Emphasizes frequencies around the cutoff
- **Effect**:
  - **Low values** (0.1-2.0): Minimal resonance, natural sound
  - **Medium values** (2.0-8.0): Noticeable peak, character boost
  - **High values** (8.0-16.0): Strong peak, can self-oscillate
- **Self-Oscillation**: At maximum resonance with high Q, the filter oscillates on its own, generating a sine wave at the cutoff frequency

**Technical Note**: In 24dB mode, resonance is internally adjusted (`sqrt(resonance)`) to maintain consistent character across filter styles.

#### Filter Drive (filter_drive)

- **Range**: -12 dB to +20 dB
- **Default**: 0 dB (unity gain)
- **Function**: Adds saturation and harmonics before filtering
- **Characteristics**:
  - **Negative values**: Clean signal, reduced input
  - **0 dB**: Unity gain, transparent
  - **Positive values**: Adds warmth and harmonics
  - **High values** (+15 to +20 dB): Aggressive saturation

**Saturation Characteristics**:

- Uses `tanh()` function for smooth, analog-like clipping
- In 12dB mode: Applied once before filter
- In 24dB mode: Applied before first stage and between stages
- Creates additional harmonics that interact with resonance

#### Pass Blend (filter_pass_blend)

- **Range**: 0.0 to 2.0
- **Values**:
  - **0.0**: Pure high-pass filter
  - **1.0**: Pure band-pass filter
  - **2.0**: Pure low-pass filter
- **Function**: Morphs between filter types continuously
- **Intermediate Values**: Blend between adjacent types
- **Use Cases**:
  - Smooth filter type transitions
  - Creating hybrid filter responses
  - Dynamic filter morphing via modulation

**Blend Calculations**:

```cpp
low_pass_amount = sqrt(clamp(1.0 - blend, 0.0, 1.0))
band_pass_amount = sqrt(clamp(1.0 - abs(blend - 1.0), 0.0, 1.0))
high_pass_amount = sqrt(clamp(blend - 1.0, 0.0, 1.0))
```

Square root scaling provides perceptually linear blending.

#### Shelf Choice (filter_shelf)

Used only in Shelf mode, selects the shelf type:

##### Low Shelf

- **Function**: Boosts or cuts low frequencies
- **Pivot Point**: At cutoff frequency
- **Effect**:
  - Gain > 1.0: Bass boost
  - Gain < 1.0: Bass cut
- **Use Cases**: Adding warmth, removing mud

##### Band Shelf

- **Function**: Boosts or cuts mid-range frequencies
- **Pivot Point**: Centered around cutoff
- **Bandwidth**: Controlled by resonance parameter
- **Use Cases**: Presence boost, vocal-range emphasis

##### High Shelf

- **Function**: Boosts or cuts high frequencies
- **Pivot Point**: At cutoff frequency
- **Effect**:
  - Gain > 1.0: Brightness boost
  - Gain < 1.0: Darkness/warmth
- **Use Cases**: Air addition, de-essing

#### Gain (filter_gain)

- **Range**: 0.0 to 10.0 (linear)
- **Used in**: Shelf mode only
- **Function**: Amount of boost or cut in shelf filters
- **Display**: Typically shown in dB
- **Default**: 1.0 (0 dB, no change)

### Filter Types in Detail

#### Low-Pass Filter

Allows frequencies below the cutoff to pass through while attenuating higher frequencies.

**Frequency Response**:

- **Passband**: 0 Hz to cutoff frequency
- **Transition**: At cutoff (-3 dB point)
- **Stopband**: Above cutoff, attenuated by slope

**Applications**:

- Removing harshness and brightness
- Warm, dark tones
- Classic synthesizer bass sounds
- Filter sweeps and evolving pads
- Emulating analog equipment

**With Resonance**:

- Creates a peak at cutoff frequency
- Emphasizes harmonics near cutoff
- At high resonance: distinctive "vocal" quality
- Self-oscillation produces pure tone

#### High-Pass Filter

Allows frequencies above the cutoff to pass through while attenuating lower frequencies.

**Frequency Response**:

- **Stopband**: 0 Hz to cutoff frequency
- **Transition**: At cutoff (-3 dB point)
- **Passband**: Above cutoff

**Applications**:

- Removing bass and rumble
- Thin, bright tones
- Clearing low-end mud
- Creating space in a mix
- Telephone/radio effects

**With Resonance**:

- Emphasis on frequencies just above cutoff
- Can create "hollow" sound at high Q
- Less commonly modulated than low-pass
- Useful for special effects

#### Band-Pass Filter

Allows only frequencies around the cutoff to pass through, attenuating both higher and lower frequencies.

**Frequency Response**:

- **Lower Stopband**: Below cutoff
- **Passband**: Centered at cutoff, width controlled by resonance
- **Upper Stopband**: Above cutoff

**Applications**:

- Vocal/formant-like sounds
- Resonant, "ringing" tones
- Simulating acoustic resonances
- Telephone/radio effects
- Removing both bass and treble

**Bandwidth**:

- Narrow at high resonance values
- Wide at low resonance values
- Always centered on cutoff frequency

### Implementation Details

#### State Variable Topology

The SVF uses an integrator-based topology with these advantages:

- Simultaneous low-pass, band-pass, and high-pass outputs
- Independent control of cutoff and resonance
- Low noise and excellent stability
- Sample-accurate parameter changes

**Internal Structure**:

```cpp
Input ? [a1×IC1 + a2×V3] ? V1 (band-pass output)
              ?
        [IC2 + a2×IC1 + a3×V3] ? V2 (low-pass output)
              ?
        [Input - IC2] ? V3
```

Where:

- `IC1`, `IC2`: Integrator states
- `a1`, `a2`, `a3`: Coefficients derived from cutoff and resonance
- `V1`, `V2`, `V3`: Intermediate filter outputs

**Coefficient Calculation**:

```cpp
g = tan(? × cutoff / sample_rate)  // Frequency warping
k = 1.0 / resonance                 // Damping coefficient
a1 = 1.0 / (1.0 + g × (g + k))     // Normalized coefficient
a2 = g × a1                         // Scaled by frequency
a3 = g × a2                         // Second order term
```

#### 12dB vs 24dB Mode

**12dB Mode**:

- Single SVF stage
- Applies drive via `tanh()` once
- Output: `m0×audio + m1×V1 + m2×V2`
- Coefficients `m0`, `m1`, `m2` set by PassBlend

**24dB Mode**:

- Two cascaded SVF stages
- Drive applied before first stage
- Additional `tanh()` saturation between stages
- Steeper rolloff, more aggressive character
- Resonance pre-adjusted: `resonance = sqrt(resonance)`

#### Sample-Accurate Modulation

Filter parameters are interpolated per-sample to prevent zipper noise:

```cpp
delta_m0 = (target_m0 - m0) / buffer_size
// For each sample:
m0 += delta_m0
```

This ensures smooth parameter changes even with fast modulation.

#### Reset Behavior

When a voice is triggered (kVoiceReset):

- Integrator states (`IC1`, `IC2`) reset to 0
- Removes DC offset and clicks
- Ensures consistent behavior per note
- Sample-accurate reset at trigger_offset

## Formant Filter System

The formant filter creates vowel-like sounds using parallel bandpass filters tuned to speech formants.

### Formant Theory

Speech sounds contain characteristic resonances called formants:

- **F1**: First formant (lowest frequency)
- **F2**: Second formant
- **F3**: Third formant
- **F4**: Fourth formant (highest frequency)

Different vowels have different formant frequency patterns.

### Formant Implementation

HelmBoy uses **4 parallel bandpass filters** (`GainedBandPass` biquad type), each tuned to a specific formant frequency with individual gain and Q (resonance).

#### Vowel Formant Tables

##### Vowel "A" (as in "father")

```cpp
Formant  Frequency   Gain (dB)  Q
F1       ~800 Hz     24 dB      10
F2       ~1150 Hz    18 dB      12
F3       ~2800 Hz    17 dB      16
F4       ~3500 Hz    16 dB      16
```

##### Vowel "E" (as in "bed")

```cpp
Formant  Frequency   Gain (dB)  Q
F1       ~400 Hz     24 dB      10
F2       ~1700 Hz    10 dB      12
F3       ~2600 Hz    12 dB      16
F4       ~3200 Hz    10 dB      16
```

##### Vowel "I" (as in "see")

```cpp
Formant  Frequency   Gain (dB)  Q
F1       ~240 Hz     24 dB      13
F2       ~2200 Hz    9 dB       12
F3       ~3000 Hz    6 dB       16
F4       ~3500 Hz    4 dB       16
```

##### Vowel "O" (as in "home")

```cpp
Formant  Frequency   Gain (dB)  Q
F1       ~400 Hz     24 dB      11
F2       ~800 Hz     14 dB      12
F3       ~2600 Hz    12 dB      16
F4       ~2900 Hz    12 dB      16
```

##### Vowel "U" (as in "you")

```cpp
Formant  Frequency   Gain (dB)  Q
F1       ~350 Hz     24 dB      11
F2       ~600 Hz     4 dB       12
F3       ~2700 Hz    7 dB       16
F4       ~3500 Hz    10 dB      16
```

### Formant Parameters

#### Formant X (formant_x)

- **Function**: Horizontal position in formant space
- **Range**: Typically 0.0 to 1.0
- **Effect**: Morphs between different vowel sounds
- **Interpolation**: Smooth interpolation between vowel tables

#### Formant Y (formant_y)

- **Function**: Vertical position in formant space
- **Range**: Typically 0.0 to 1.0
- **Effect**: Adjusts formant character and brightness
- **Behavior**: May adjust Q values or overall gain

#### Formant Blend (filter_blend)

- **Range**: 0.0 to 2.0
- **Values**:
  - **0.0**: Pure main filter (no formant)
  - **1.0**: Equal blend
  - **2.0**: Pure formant filter (no main filter)
- **Function**: Crossfades between main SVF and formant filter

### Formant Filter Characteristics

**Advantages**:

- Realistic vocal timbres
- Complex harmonic emphasis
- Musical, speech-like quality
- Suitable for leads, pads, and bass

**Limitations**:

- CPU-intensive (4 biquad filters)
- Fixed formant structure
- Less drastic than main filter sweeps
- Best with harmonic-rich sources

## Biquad Filter Types

helmBoy's formant system and shelf modes use RBJ (Robert Bristow-Johnson) biquad filters.

### Biquad Types Available

#### LowPass

Standard second-order low-pass filter.

#### HighPass

Standard second-order high-pass filter.

#### BandPass

Standard second-order band-pass filter.

#### LowShelf

Boosts or cuts low frequencies.

#### HighShelf

Boosts or cuts high frequencies.

#### BandShelf

Boosts or cuts a band of frequencies.

#### AllPass

Passes all frequencies but affects phase.

- **Use Cases**: Phase effects, phaser-like sounds

#### Notch

Removes frequencies around cutoff.

- **Use Cases**: Removing specific resonances, comb filtering

#### GainedBandPass

Band-pass with additional gain parameter.

- **Used for**: Formant filters (emphasize formant peaks)

### Biquad Coefficient Calculation

All biquad filters follow the standard difference equation:

```cpp
y[n] = b0×x[n] + b1×x[n-1] + b2×x[n-2] - a1×y[n-1] - a2×y[n-2]
```

Where:

- `x[n]`: Input samples
- `y[n]`: Output samples
- `b0, b1, b2`: Feedforward coefficients (input)
- `a1, a2`: Feedback coefficients (output)

  **Example: Low-Pass Biquad**

```cpp
omega = 2? × cutoff / sample_rate
alpha = sin(omega) / (2 × resonance)
norm = 1 + alpha

b0 = (1 - cos(omega)) / (2 × norm)
b1 = (1 - cos(omega)) / norm
b2 = b0
a1 = -2 × cos(omega) / norm
a2 = (1 - alpha) / norm
```

### Biquad Stability

Biquads are numerically stable when:

- Cutoff frequency < Nyquist (sample_rate / 2)
- Resonance > 0
- Coefficients updated smoothly

helmBoy ensures stability by:

- Clamping cutoff: `1 Hz to sample_rate`
- Clamping resonance: `0.1 to 16.0`
- Per-sample coefficient interpolation

## Filter Modulation

### Key Tracking (Keyboard Follow)

#### Parameter: filter_keytrack

- **Range**: -100% to +100%
- **Default**: Varies by preset
- **Function**: Makes cutoff frequency follow played notes

**Calculation**:

```cpp
cutoff_adjustment = keytrack × (note_number - center_note) × semitones_to_hz
final_cutoff = base_cutoff × cutoff_adjustment
```

**Common Settings**:

- **0%**: No tracking (constant cutoff regardless of pitch)
- **50%**: Partial tracking (cutoff rises with pitch)
- **100%**: Full tracking (cutoff tracks pitch 1:1)
- **Negative values**: Cutoff drops as pitch rises (inverse tracking)

**Use Cases**:

- **100% tracking**: Maintain consistent timbre across keyboard
- **50% tracking**: Natural brightness variation with pitch
- **0% tracking**: Fixed filter character (classic synth bass)
- **Negative tracking**: Darker high notes, brighter low notes

### Filter Envelope

The filter envelope (separate from amplitude envelope) modulates cutoff frequency over time.

#### Envelope Parameters

**Attack**: Time to reach peak cutoff after note trigger
**Decay**: Time to fall from peak to sustain level
**Sustain**: Cutoff level maintained while note held
**Release**: Time to return to base cutoff after note release

#### Envelope Amount (filter_env_depth)

- **Range**: -100% to +100%
- **Positive values**: Envelope opens filter (raises cutoff)
- **Negative values**: Envelope closes filter (lowers cutoff)
- **Zero**: No envelope modulation

**Typical Settings**:

*Classic Filter Sweep*:

- Attack: Fast (5-50 ms)
- Decay: Medium (200-800 ms)
- Sustain: Low (10-30%)
- Release: Short (50-200 ms)
- Amount: +60% to +100%

*Reverse Sweep*:

- Attack: Medium (100-500 ms)
- Decay: Fast (50-200 ms)
- Sustain: Medium (40-70%)
- Release: Fast (50-150 ms)
- Amount: -40% to -70%

*Percussive*:

- Attack: Fast (1-10 ms)
- Decay: Fast (50-200 ms)
- Sustain: 0%
- Release: 0 ms
- Amount: +80% to +100%

### LFO Modulation

LFOs can modulate cutoff frequency for cyclic filter movement.

**Common Routings**:

*Slow Filter Sweep*:

- Source: Mono LFO 1
- Waveform: Sine or Triangle
- Rate: 0.1 - 2 Hz
- Destination: Filter Cutoff
- Amount: ±30% to ±60%

*Rhythmic Wobble*:

- Source: Mono LFO (tempo-synced)
- Waveform: Sine
- Rate: 1/4 or 1/8 note
- Destination: Filter Cutoff
- Amount: ±50% to ±80%

*Random Variation*:

- Source: Poly LFO (per-voice)
- Waveform: Random (S&H)
- Rate: Slow (0.5 - 3 Hz)
- Destination: Filter Cutoff
- Amount: ±10% to ±30%

## Advanced Filtering Techniques

### Filter Self-Oscillation

At maximum resonance, the filter self-oscillates, generating a sine wave tone.

**Using Self-Oscillation**:

1. Set resonance to maximum (16.0)
2. Adjust cutoff to desired pitch
3. Reduce or silence oscillators
4. Use filter envelope and key tracking for melodic playing

**Applications**:

- Sine wave bass lines
- Whistling lead sounds
- Resonant percussion (kick drums, toms)
- Special effects

**Tips**:

- Use 24dB mode for stronger self-oscillation
- Moderate drive to maintain clean tone
- Key tracking 100% for melodic playing
- Use filter envelope for pitch slides

### Parallel Filter Configuration

HelmBoy's formant blend allows parallel filtering:

- Main filter processes one copy of signal
- Formant filter processes another copy
- Blend parameter mixes both outputs

**Creative Uses**:

- Main filter: Low-pass for warmth
- Formant filter: Vowel character
- Blend: Mix to taste

### Filter Feedback Loops

Oscillator feedback can be processed by the filter, then fed back to oscillators, creating complex interactions:

1. Oscillators ? Filter
2. Filter ? Feedback section
3. Feedback ? Oscillator phase modulation
4. Result: Resonant, evolving timbres

**Tips**:

- Use moderate feedback amounts (20-40%)
- Combine with filter envelope for dynamic tones
- Add resonance for metallic/bell-like sounds

### Dynamic Filter Control

**Velocity to Filter**:

- Harder hits open filter more
- Typical amount: +40% to +70%
- Creates expressive, dynamic playing

**Aftertouch to Filter**:

- Pressure control of cutoff during notes
- Typical amount: +30% to +60%
- Allows expressive filter sweeps

**Mod Wheel to Resonance**:

- Real-time resonance control
- Amount: +30% to +100%
- Useful for live performance emphasis

## Filter Response Visualization

HelmBoy includes a real-time filter response display showing:

- Frequency response curve
- Cutoff frequency position
- Resonance peak
- Main filter and formant filter overlays
- Blend between filters

**Interpreting the Display**:

- **X-axis**: Frequency (logarithmic scale, 20 Hz to 20 kHz)
- **Y-axis**: Amplitude (dB)
- **Yellow curve**: Main SVF response
- **Blue curve**: Formant filter response (if active)
- **Green vertical line**: Current cutoff frequency
- **Peak height**: Resonance amount

## Performance Considerations

### CPU Usage

**Approximate CPU Load** (per voice, modern processor):

- 12dB SVF: ~1-2%
- 24dB SVF: ~2-3%
- Formant filter (4 biquads): ~3-4%
- Combined (SVF + Formant): ~5-7%

**Optimization Tips**:

- Use 12dB mode when 24dB not necessary
- Disable formant filter (blend = 0.0) when not in use
- Reduce polyphony if CPU limited
- Use lower sample rates if quality acceptable

### Latency

- **Processing Latency**: Zero (real-time sample generation)
- **Modulation Latency**: Sample-accurate (no block delay)
- **Parameter Changes**: Interpolated smoothly over buffer

## Common Filter Settings

### Subtractive Synthesis Classics

**Moog-Style Bass**:

- Style: 24dB
- Pass Blend: 2.0 (low-pass)
- Cutoff: Low (200-800 Hz)
- Resonance: Medium-High (5-10)
- Drive: Moderate (+3 to +8 dB)
- Envelope: Classic sweep (fast attack, medium decay)

**Roland-Style Pad**:

- Style: 12dB
- Pass Blend: 2.0 (low-pass)
- Cutoff: Medium (1000-3000 Hz)
- Resonance: Low-Medium (1-4)
- Drive: Low (0 to +3 dB)
- LFO: Slow sine wave modulation

**Oberheim-Style Lead**:

- Style: 24dB
- Pass Blend: 2.0 (low-pass)
- Cutoff: Medium-High (1500-5000 Hz)
- Resonance: Medium (4-8)
- Drive: Medium (+5 to +12 dB)
- Envelope: Percussive (fast attack/decay, low sustain)

### Formant-Based Sounds

**Vowel Bass**:

- Blend: 2.0 (formant only)
- Formant X: Animate between vowels
- Formant Y: Medium
- LFO: Slow modulation of Formant X

**Talking Lead**:

- Blend: 1.0 (mix main and formant)
- Main Filter: 12dB, band-pass
- Formant: Modulate X with LFO or step sequencer
- Result: Speech-like melodic line

### Special Effects

**Telephone/Radio**:

- Style: 12dB
- Pass Blend: 1.0 (band-pass)
- Cutoff: 800-2000 Hz
- Resonance: Medium (3-6)
- Drive: High (+10 to +15 dB)

**Resonant Percussion**:

- Style: 24dB
- Resonance: Maximum (16.0)
- Cutoff: Tune to desired pitch
- Key Tracking: 100%
- Envelope: Fast attack, fast decay, zero sustain
- Self-oscillation for tone generation

**Wind/Breath**:

- Main filter off (on = 0.0)
- Oscillators: Minimal or off
- Noise: High level
- Band-pass or formant filter
- LFO: Random modulation of cutoff
- Creates organic breath sounds

## Troubleshooting

### Problem: Filter Sounds Harsh

**Causes**:

- Excessive drive
- High resonance with bright input signal
- 24dB mode too aggressive

**Solutions**:

- Reduce drive to 0 dB or lower
- Lower resonance
- Switch to 12dB mode
- Reduce oscillator volume before filter

### Problem: Filter Has No Effect

**Causes**:

- Filter off (on = 0.0)
- Cutoff frequency too high (above audio range)
- Blend set to wrong mode
- Resonance too low

**Solutions**:

- Enable filter (on = 1.0)
- Lower cutoff to audible range (20 Hz - 10 kHz)
- Adjust pass blend to desired type (2.0 for low-pass)
- Increase resonance for more character

### Problem: Volume Drops with Resonance

**Cause**: High resonance emphasizes narrow frequency band, reducing overall level

**Solutions**:

- Increase gain after filter (mixer/output volume)
- Use moderate resonance (4-8) for balance
- Add drive to compensate for level loss
- Expected behavior; part of filter character

### Problem: Formant Filter Sounds Muddy

**Causes**:

- Formant blend too high
- Input signal too harmonically rich
- Formant frequencies too low

**Solutions**:

- Reduce blend (try 0.5 - 1.5)
- Use simpler oscillator waveforms
- Use main filter in combination (high-pass or band-pass)
- Adjust Formant Y parameter

### Problem: Filter Self-Oscillates Unexpectedly

**Cause**: Resonance at or near maximum (16.0)

**Solutions**:

- Reduce resonance to 8-12 for strong character without self-oscillation
- If intentional, use key tracking and envelope for control
- Lower input signal level (drive and oscillator volumes)

## Related Documentation

- [Filter Section UI](filter.md) - User interface controls
- [Filter Types](filter-types.md) - Quick reference for filter modes
- [Filter Envelope](filter-envelope.md) - Envelope routing and settings
- [Formant Filter](formant.md) - Formant synthesis details
- [Modulation](modulation.md) - Routing LFOs and envelopes to filters

## References

### Source Files

- `mopo/src/state_variable_filter.h` - Main SVF filter interface
- `mopo/src/state_variable_filter.cpp` - SVF implementation
- `mopo/src/biquad_filter.h` - Biquad filter interface
- `mopo/src/biquad_filter.cpp` - Biquad implementations
- `mopo/src/formant_manager.h` - Formant filter coordination
- `mopo/src/formant_manager.cpp` - Formant filter implementation
- `src/synthesis/helmBoy_voice_handler.cpp` - Voice and filter routing
- `src/editor_sections/filter_section.cpp` - UI implementation
- `src/editor_components/filter_response.cpp` - Response visualization
- `src/common/helmBoy_common.cpp` - Parameter definitions

### Algorithm References

- **State Variable Filter**: Hal Chamberlin, "Musical Applications of Microprocessors"
- **RBJ Biquad**: Robert Bristow-Johnson, "Cookbook formulae for audio EQ biquad filter coefficients"
- **Formant Synthesis**: Based on speech synthesis research and vocal tract modeling
- **Topology**: Inspired by classic analog synthesizers (Moog, Oberheim, Roland)

### Filter Design Resources

- Hal Chamberlin: State variable filter topology
- Julius O. Smith III: Digital filter theory and implementation
- Will Pirkle: Designing Software Synthesizer Plugins in cpp
- Robert Bristow-Johnson: Audio EQ Cookbook

---

*This documentation describes the filter system as implemented in helmBoy. Filter algorithms and behavior are subject to refinement in future versions.*
