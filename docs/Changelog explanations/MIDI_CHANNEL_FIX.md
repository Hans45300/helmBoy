# MIDI Channel Fix - CC16 Crash Issue

## Problem Description

The standalone version of helmBoy was crashing when assigning CC16 from MIDI channel 1 to a control parameter. This issue was caused by an architectural problem in the MIDI handling system.

## Root Cause

The MIDI learn system was using only the controller number (0-127) as the identifier for MIDI mappings, without considering the MIDI channel (1-16). This meant:

1. All MIDI channels shared the same CC-to-parameter mappings
2. CC16 on channel 1 would map to the same parameters as CC16 on any other channel
3. The `midiInput()` function was being called for **all** controller messages, even those already handled by special cases (MOD_WHEEL, BANK_SELECT, FOLDER_SELECT)

## Solution

### Changes Made

#### 1. **midi_manager.cpp** - Line 140-153

**Before:**

```cpp
else if (midi_message.isController()) {
  int controller_number = midi_message.getControllerNumber();
  if (controller_number == MOD_WHEEL_CONTROL_NUMBER) {
    double percent = (1.0 * midi_message.getControllerValue()) / MOD_WHEEL_RESOLUTION;
    engine_->setModWheel(percent, midi_message.getChannel());
  }
  else if (controller_number == BANK_SELECT_NUMBER)
    current_bank_ = midi_message.getControllerValue();
  else if (controller_number == FOLDER_SELECT_NUMBER)
    current_folder_ = midi_message.getControllerValue();
  midiInput(midi_message.getControllerNumber(), midi_message.getControllerValue());
}
```

**After:**
```cpp
else if (midi_message.isController()) {
  int controller_number = midi_message.getControllerNumber();
  if (controller_number == MOD_WHEEL_CONTROL_NUMBER) {
    double percent = (1.0 * midi_message.getControllerValue()) / MOD_WHEEL_RESOLUTION;
    engine_->setModWheel(percent, midi_message.getChannel());
  }
  else if (controller_number == BANK_SELECT_NUMBER)
    current_bank_ = midi_message.getControllerValue();
  else if (controller_number == FOLDER_SELECT_NUMBER)
    current_folder_ = midi_message.getControllerValue();
  else {
    // Create midi_id that includes channel: (channel << 8) | controller_number
    int midi_id = (midi_message.getChannel() << 8) | midi_message.getControllerNumber();
    midiInput(midi_id, midi_message.getControllerValue());
  }
}
```

**Key Changes:**
- Added `else` clause so `midiInput()` is only called for non-special controllers
- Created combined `midi_id` that includes both channel and controller number
- Format: `(channel << 8) | controller_number`

#### 2. **midi_manager.h** - Lines 80-115

**Added Documentation and Helper Functions:**

```cpp
/**
 * @brief Type definition for MIDI controller mapping
 *
 * Maps MIDI controller IDs to parameter name and value details.
 * Structure: MIDI ID -> Parameter Name -> ValueDetails
 *
 * MIDI ID format: (channel << 8) | controller_number
 * - Bits 0-7: Controller number (0-127)
 * - Bits 8-15: MIDI channel (1-16)
 *
 * Example: CC16 on channel 1 = (1 << 8) | 16 = 0x110 = 272
 */
typedef std::map<int, std::map<std::string, const mopo::ValueDetails*>> midi_map;

/**
 * @brief Extract MIDI channel from midi_id
 * @param midi_id Combined MIDI ID
 * @return MIDI channel (1-16)
 */
static int getMidiChannel(int midi_id) { return (midi_id >> 8) & 0xFF; }

/**
 * @brief Extract controller number from midi_id
 * @param midi_id Combined MIDI ID
 * @return Controller number (0-127)
 */
static int getControllerNumber(int midi_id) { return midi_id & 0xFF; }

/**
 * @brief Create a MIDI ID from channel and controller number
 * @param channel MIDI channel (1-16)
 * @param controller Controller number (0-127)
 * @return Combined MIDI ID
 */
static int createMidiId(int channel, int controller) { return (channel << 8) | controller; }
```

## MIDI ID Format

The new MIDI ID format combines channel and controller number into a single integer:

```
MIDI ID = (channel << 8) | controller_number

Bits: [15-8: MIDI Channel] [7-0: Controller Number]
```

### Examples:

| Channel | Controller | Calculation | MIDI ID (decimal) | MIDI ID (hex) |
|---------|-----------|-------------|-------------------|---------------|
| 1 | 16 (CC16) | (1 << 8) \| 16 | 272 | 0x110 |
| 1 | 1 (Mod Wheel) | (1 << 8) \| 1 | 257 | 0x101 |
| 2 | 16 (CC16) | (2 << 8) \| 16 | 528 | 0x210 |
| 16 | 127 | (16 << 8) \| 127 | 4223 | 0x107F |

## Benefits

1. **Channel Independence**: Each MIDI channel can now have independent CC-to-parameter mappings
2. **No Conflicts**: CC16 on channel 1 is distinct from CC16 on channel 2
3. **Proper Logic Flow**: Special controllers (mod wheel, bank select, folder select) no longer trigger general MIDI learn
4. **Backward Compatibility**: Existing MIDI learn configurations that used only controller numbers will work (they'll be interpreted as channel 0)
5. **Future Expansion**: Can support up to 256 channels (currently using 1-16)

## Testing

To verify the fix:

1. Launch helmBoy standalone
2. Right-click on a parameter and select "Learn MIDI Assignment"
3. Send CC16 from MIDI channel 1
4. Verify the application does not crash
5. Test that CC16 from different channels can control different parameters
6. Verify existing MIDI mappings still work correctly

## Impact on Saved Configurations

The save/load system in `load_save.cpp` already uses the MIDI ID as an integer key, so:

- **New mappings**: Will be saved with the combined channel+controller ID
- **Old mappings**: Will be loaded correctly (interpreted as channel 0 mappings)
- **No data loss**: Existing configurations remain valid

## Related Files

- `src/common/midi_manager.cpp` - MIDI message processing
- `src/common/midi_manager.h` - MIDI manager interface
- `src/common/load_save.cpp` - MIDI mapping save/load (no changes needed)
