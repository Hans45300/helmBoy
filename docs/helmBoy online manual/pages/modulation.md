# Modulation Matrix

## Overview

HelmBoy lets you route modulation sources to supported controls. Select a source to reveal the modulation amount controls available on destinations.

## Modulation Sources

### Envelopes

- Amplitude Envelope
- Filter Envelope
- Modulation Envelopes 1-4

### LFOs

- Four Mono LFOs
- Two Poly LFOs

### Performance

- Mod Wheel
- Pitch Bend
- Channel Aftertouch
- Poly Aftertouch
- Velocity

### Other

- Step Sequencer
- Random
- Note Number (keyboard tracking)

## Modulation Destinations

Nearly any parameter can be modulated:

- Oscillator parameters (pitch, volume, waveform, and Hard Sync)
- Filter parameters (cutoff, resonance, drive)
- Effect parameters (delay time, reverb mix)
- LFO parameters (rate, amplitude)
- Envelope parameters

## Creating Modulations

1. Click a modulation-source button.
2. Adjust the amount control shown on the destination you want to modulate.
3. Use a positive or negative amount to choose the modulation direction.
4. Select another source or clear the active source when you are done.

## Modulation Amount

- **Positive values**: Modulation increases parameter value
- **Negative values**: Modulation decreases parameter value
- **Zero**: No modulation
- Numeric destinations accept a symmetric amount range based on the full
  parameter range. This preserves existing positive depths while allowing the
  same depth in the opposite direction.
- For switch destinations, positive amounts activate outside the selected
  threshold band and negative amounts activate inside it. The threshold
  magnitude determines the band, with hysteresis preventing rapid toggling.
- Modulation amounts are stored with presets. Loading clamps an amount to the
  destination's supported range; legacy positive amounts keep their original
  effective depth.

### Numeric Entry

Right-click a modulation source or destination to access its existing routes.
Each route adds an **Enter Value for [destination]** command on the source,
or **Enter Value for [source]** on the destination. These commands are also
available on the destination's modulation amount control.

The dialog shows the source, destination, current amount, and accepted range.
Enter a numeric value and confirm with **OK**, or dismiss with **Cancel**.
The value uses the amount control's units and is limited to its accepted range;
it changes the modulation amount, not the destination's base value.

Disconnecting a route, or setting its amount to zero, removes its value-entry
command from subsequent menus. Confirming a dialog after its route has been
removed does not recreate the route.

## See Also

- [Mono LFO](mono-lfo.md)
- [Poly LFO](poly-lfo.md)
- [Mod Envelope](mod-envelope.md)
