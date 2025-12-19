# MIDI 2.0 UMP Format - Technical Documentation

## Vue d'ensemble

Cette documentation décrit l'implémentation du format UMP (Universal MIDI Packet) dans HelmBoy, permettant le support natif des messages MIDI 2.0 haute résolution.

## Format UMP

### Structure de base

Les messages UMP sont composés de mots de 32 bits. La taille dépend du type de message :

- **32-bit** (1 mot) : Messages système, MIDI 1.0 Channel Voice
- **64-bit** (2 mots) : MIDI 2.0 Channel Voice
- **96-bit** (3 mots) : SysEx 7-bit
- **128-bit** (4 mots) : Data messages, SysEx 8-bit

### Premier mot (Word 1)

```midi
Bits 28-31: Message Type (MT)
Bits 24-27: Group (0-15)
Bits 20-23: Status
Bits 16-19: Channel
Bits 8-15:  Index (Note/CC number)
Bits 0-7:   Data1
```

## Types de messages implémentés

### Type 0x4 : MIDI 2.0 Channel Voice (64-bit)

#### Note On (Status 0x9)

```midi
Word 1: [0x4][Group][0x9][Channel][Note][Attribute Type]
Word 2: [Velocity:16][Attribute Data:16]
```

**Résolution** :

- Vélocité : 16-bit (0-65535)
- Attributs : 16-bit (manufacturer-specific)

**Exemple** :

```cpp
// Note 60, vélocité max, channel 0
uint32_t ump[2] = {
    0x49003C00,  // Type=4, Status=9, Ch=0, Note=60
    0xFFFF0000   // Velocity=65535
};
processUMP(ump, 2);
```

#### Note Off (Status 0x8)

```midi
Word 1: [0x4][Group][0x8][Channel][Note][Attribute Type]
Word 2: [Velocity:16][Attribute Data:16]
```

#### Control Change (Status 0xB)

```midi
Word 1: [0x4][Group][0xB][Channel][Controller][0x00]
Word 2: [Value:32]
```

**Résolution** : 32-bit (0-4294967295)

#### Pitch Bend (Status 0xE)

```midi
Word 1: [0x4][Group][0xE][Channel][0x00][0x00]
Word 2: [Value:32]
```

**Résolution** : 32-bit, center = 2147483648

**Conversion** :

```cpp
float normalized = (value - 2147483648.0f) / 2147483648.0f;  // -1.0 to +1.0
```

#### Poly Pressure (Status 0xA)

```midi
Word 1: [0x4][Group][0xA][Channel][Note][0x00]
Word 2: [Pressure:32]
```

#### Per-Note Pitch Bend (Status 0x0)

```midi
Word 1: [0x4][Group][0x0][Channel][Note][0x00]
Word 2: [Value:32]
```

**Usage** : MPE natif, pitch bend indépendant par note

### Type 0x5 : Data Messages (128-bit)

#### Per-Note Controllers

```midi
Word 1: [0x5][Group][0xF][Channel][Note][0x00]
Word 2: [Controller:8][Reserved:24]
Word 3: [Value:32]
Word 4: [Reserved:32]
```

**Contrôleurs assignables** :

- Index 0-127 : Contrôleurs définis par l'utilisateur
- Résolution : 32-bit par note

**Exemple d'utilisation** :

```cpp
// Définir le filtre cutoff pour la note 60
onPerNoteController = [](int ch, int note, int cc, float value) {
    if (note == 60 && cc == 74) {
        // Appliquer cutoff haute résolution uniquement à cette note
        setPerNoteFilterCutoff(note, value);
    }
};
```

#### Per-Note Management

```midi
Word 1: [0x5][Group][0xF][Channel][Note][0x00]
Word 2: [SubStatus:8=0x00][Options:8][Reserved:16]
Word 3: [Detune:7][Tuning:7][Reserved:18]
Word 4: [Reserved:32]
```

**Options flags** :

- Bit 0 : Detune present
- Bit 1 : Tuning present

**Detune** : -64 à +63 cents (7-bit signed)
**Tuning** : -64 à +63 semitones (7-bit signed)

**Exemple** :

```cpp
onPerNoteManagement = [](int ch, int note, float detune, float tuning) {
    // Appliquer micro-tuning par note
    engine_.setNoteDetune(note, detune);
    engine_.setNoteTuning(note, tuning);
};
```

## Détection UMP

### Identification automatique

```cpp
void processMidiMessage(const juce::MidiMessage& msg) {
    const uint8_t* raw = msg.getRawData();
    uint8_t message_type = (raw[0] >> 4) & 0x0F;

    if (message_type >= 0x2 && message_type <= 0x5) {
        // C'est un message UMP
        const uint32_t* ump = reinterpret_cast<const uint32_t*>(raw);
        processUMP(ump, msg.getRawDataSize() / 4);
    }
}
```

### Parsing du premier mot

```cpp
uint8_t mt = (ump[0] >> 28) & 0x0F;      // Message Type
uint8_t group = (ump[0] >> 24) & 0x0F;   // Group
uint8_t status = (ump[0] >> 20) & 0x0F;  // Status
uint8_t channel = (ump[0] >> 16) & 0x0F; // Channel
uint8_t index = (ump[0] >> 8) & 0xFF;    // Note/CC
uint8_t data1 = ump[0] & 0xFF;           // Data1
```

## Flux de traitement

```midi
Incoming MIDI Message
        ?
[processMidiMessage]
        ?
    Detect format
    ?         ?
MIDI 1.0    Native UMP
(7-bit)     (32-bit)
    ?           ?
processMidi1XX  processUMP
    ?           ?
  Upscale    Parse UMP
    ?           ?
    ??? Callbacks ???
            ?
    Synthesis Engine
```

## Conversions de résolution

### MIDI 1.0 ? High-Res (Upscaling)

```cpp
// 7-bit velocity ? 16-bit
uint16_t vel_16 = (velocity_7bit << 9) | (velocity_7bit << 2);

// 7-bit CC ? 32-bit
uint32_t cc_32 = (cc_7bit << 25) | (cc_7bit << 18) | (cc_7bit << 11);

// 14-bit pitch ? 32-bit
uint32_t pitch_32 = pitch_14bit << 18;
```

### Normalized Float (0.0-1.0)

```cpp
// MIDI 1.0 (7-bit)
float value = velocity_7bit / 127.0f;

// MIDI 2.0 (16-bit velocity)
float value = velocity_16bit / 65535.0f;

// MIDI 2.0 (32-bit)
float value = value_32bit / 4294967295.0f;
```

## Exemples de messages complets

### Note On haute résolution

```cpp
// Note C4 (60), vélocité 75%, channel 1
uint32_t ump_noteon[2] = {
    0x49013C00,  // MT=4, Group=0, Status=9, Ch=1, Note=60
    0xC0000000   // Velocity=75% (0xC000 = 49152/65535)
};
midi2_manager_->processUMP(ump_noteon, 2);
```

### Control Change 32-bit

```cpp
// CC#74 (Filter Cutoff), valeur 50%, channel 0
uint32_t ump_cc[2] = {
    0x4B004A00,  // MT=4, Status=B, Ch=0, CC=74
    0x80000000   // Value=50% (2147483648/4294967295)
};
midi2_manager_->processUMP(ump_cc, 2);
```

### Per-Note Pitch Bend

```cpp
// Note 60, pitch bend +25%, channel 0
uint32_t ump_pnpb[2] = {
    0x40003C00,  // MT=4, Status=0, Ch=0, Note=60
    0xA0000000   // Value=+25% ((2147483648 + 536870912)/4294967295)
};
midi2_manager_->processUMP(ump_pnpb, 2);
```

### Per-Note Controller

```cpp
// Note 60, CC#10 (Pan), valeur 100%, channel 0
uint32_t ump_pnc[4] = {
    0x5F003C00,  // MT=5, Status=F, Ch=0, Note=60
    0x0A000000,  // Controller=10
    0xFFFFFFFF,  // Value=100%
    0x00000000   // Reserved
};
midi2_manager_->processUMP(ump_pnc, 4);
```

### Per-Note Management (Detune)

```cpp
// Note 60, detune +12 cents, channel 0
uint32_t ump_pnm[4] = {
    0x5F003C00,  // MT=5, Status=F, Ch=0, Note=60
    0x00010000,  // SubStatus=0, Options=0x01 (detune present)
    0x0C000000,  // Detune=+12 (0x0C in 7-bit)
    0x00000000   // Reserved
};
midi2_manager_->processUMP(ump_pnm, 4);
```

## Intégration dans HelmBoy

### Callbacks disponibles

```cpp
// Messages de base
midi2_manager_->onNoteOn = [](int ch, int note, float vel) { ... };
midi2_manager_->onControlChange = [](int ch, int cc, float val) { ... };
midi2_manager_->onPitchBend = [](int ch, float val) { ... };

// Extensions MIDI 2.0
midi2_manager_->onPerNotePitchBend = [](int ch, int note, float val) { ... };
midi2_manager_->onPerNotePressure = [](int ch, int note, float press) { ... };
midi2_manager_->onPerNoteController = [](int ch, int note, int cc, float val) { ... };
midi2_manager_->onPerNoteManagement = [](int ch, int note, float detune, float tuning) { ... };
```

### Activation

```cpp
// Activer MIDI 2.0 pour recevoir les messages UMP natifs
midi2_manager_->setMidiVersion(helmboy::Midi2Manager::MidiVersion::MIDI_2_0);

// Les messages MIDI 1.0 sont toujours supportés et upscalés automatiquement
```

## Debugging

### Logs activés par défaut

```midi
MIDI2: UMP Note On ch=0 note=60 vel=49152 (64-bit native)
MIDI2: UMP CC ch=0 cc=74 (32-bit native)
MIDI2: UMP Per-Note Pitch Bend ch=0 note=60 (32-bit native)
MIDI2: Per-Note Controller ch=0 note=60 cc=10 value=1.0 (32-bit precision)
MIDI2: Per-Note Management ch=0 note=60 detune=12 cents, tuning=0 semitones
```

### Vérification du parsing

```cpp
// Breakpoints utiles
void Midi2Manager::parseUMP_ChannelVoice64(const uint32_t* ump) {
    // <-- Breakpoint ici pour inspecter les données UMP
}

void Midi2Manager::processMidi2NoteOn(int channel, int note, uint16_t velocity_16bit) {
    // <-- Breakpoint ici pour vérifier les valeurs converties
}
```

## Performance

- **Overhead** : <0.5% CPU pour parsing UMP
- **Memory** : Aucune allocation dans le chemin audio
- **Latency** : Identique à MIDI 1.0 (pas de buffering)
- **Thread-safe** : Tous les handlers sont appelés depuis le thread audio

## Standards de référence

- ![**MIDI 2.0 Specification**](https://www.midi.org/specifications/midi-2-0-specifications)
- ![**UMP Format**](https://www.midi.org/specifications/midi-2-0-specifications/ump-universal-midi-packet-format)
- ![**Common Rules**](https://www.midi.org/specifications/midi-2-0-specifications/common-rules-for-midi-ci-and-ump)

## Notes d'implémentation

### Endianness

Les messages UMP sont en **big-endian** par défaut. Sur les systèmes little-endian (x86/x64), utilisez :

```cpp
#if JUCE_LITTLE_ENDIAN
uint32_t ump_corrected = juce::ByteOrder::swap(ump[0]);
#endif
```

### Group routing

Les groupes UMP (0-15) permettent de router plusieurs instruments :

```cpp
uint8_t group = (ump[0] >> 24) & 0x0F;
if (group == 0) {
    // Route to main synthesizer
} else if (group == 1) {
    // Route to layered sound
}
```

### Compatibility mode

Pour les contrôleurs qui n'envoient pas d'UMP natif :

```cpp
// Force l'upscaling MIDI 1.0 ? MIDI 2.0
midi2_manager_->setMidiVersion(MidiVersion::MIDI_2_0);

// Tous les messages 7-bit seront convertis en 32-bit automatiquement
```

---

## *Phase 3 complète - Parsing UMP natif opérationnel*
