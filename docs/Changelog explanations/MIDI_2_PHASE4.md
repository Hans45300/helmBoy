# MIDI 2.0 Phase 4: Profiles, Property Exchange et Feedback

## Vue d'ensemble

La Phase 4 complète l'implémentation MIDI 2.0 avec les fonctionnalités avancées de communication bidirectionnelle et de négociation de capacités.

## Profiles MIDI 2.0

### Concept

Les Profiles MIDI 2.0 définissent des configurations standard pour différents types d'instruments ou modes de contrôle. Ils permettent une négociation automatique des capacités entre le plugin et le contrôleur.

### Structure ProfileId

```cpp
struct ProfileId {
    uint8_t profile_byte_1;  // Standard Profile Bank (0x00 = standard)
    uint8_t profile_byte_2;  // Profile Number
    uint8_t profile_byte_3;  // Profile Version
    uint8_t profile_byte_4;  // Profile Level
    uint8_t profile_byte_5;  // Reserved (0x00)
};
```

### Profiles standards

#### General MIDI 2 (0x00, 0x01)

```cpp
Midi2Manager::ProfileId gm2 = {0x00, 0x01, 0x01, 0x00, 0x00};
midi2_manager_->enableProfile(gm2, 0);  // Channel 0
```

#### Polyphonic Expression (MPE) (0x00, 0x02)

```cpp
Midi2Manager::ProfileId mpe = {0x00, 0x02, 0x01, 0x00, 0x00};
midi2_manager_->enableProfile(mpe, -1);  // All channels
```

#### Drawbar Organ (0x00, 0x10)

```cpp
Midi2Manager::ProfileId drawbar = {0x00, 0x10, 0x01, 0x00, 0x00};
midi2_manager_->enableProfile(drawbar, 1);
```

### Gestion des profiles

#### Activer un profile

```cpp
bool success = midi2_manager_->enableProfile(profile, channel);
// channel: 0-15 pour un canal spécifique, -1 pour tous les canaux
```

#### Désactiver un profile

```cpp
midi2_manager_->disableProfile(profile, channel);
```

#### Vérifier l'état

```cpp
if (midi2_manager_->isProfileEnabled(profile, 0)) {
    DBG("Profile actif sur le channel 0");
}
```

#### Callback de notification

```cpp
midi2_manager_->onProfileChanged = [](const ProfileId& profile,
                                      int channel,
                                      bool enabled) {
    String status = enabled ? "enabled" : "disabled";
    DBG("Profile " << status << " on channel " << channel);

    // Adapter les comportements selon le profile actif
    if (profile.profile_byte_2 == 0x02 && enabled) {
        // MPE activé : activer le per-note pitch bend
        enablePerNotePitchBend();
    }
};
```

### Messages UMP Profile

Format (128-bit Stream Message) :

```midi
Word 1: [0xF][Group=0][0x24/0x25][Reserved]
Word 2: [profile_byte_1][profile_byte_2][profile_byte_3][profile_byte_4]
Word 3: [Channel or 0xFF][Reserved]
Word 4: [Reserved]
```

- Status 0x24 = Profile Enable
- Status 0x25 = Profile Disable

## Property Exchange

### Property Exchange - Concept

Le système Property Exchange permet l'échange bidirectionnel de métadonnées entre le plugin et le contrôleur via JSON encodé en UMP.

### Types de requêtes

```cpp
enum class PropertyRequest {
    GET_PROPERTY,    // Demander une propriété
    SET_PROPERTY,    // Définir une propriété
    SUBSCRIBE,       // S'abonner aux changements
    NOTIFY           // Notification de changement
};
```

### Envoyer une propriété

```cpp
// Propriété simple
midi2_manager_->sendProperty("synthName", "HelmBoy");

// Propriété numérique
midi2_manager_->sendProperty("polyphony", 8);
midi2_manager_->sendProperty("sampleRate", 48000.0);

// Propriété complexe (object)
juce::DynamicObject::Ptr preset = new juce::DynamicObject();
preset->setProperty("name", "Lead 1");
preset->setProperty("category", "Synth");
preset->setProperty("author", "Marc Scheffer");
midi2_manager_->sendProperty("currentPreset", juce::var(preset.get()));
```

### Demander une propriété

```cpp
// Le contrôleur répondra via le callback onPropertyRequest
midi2_manager_->requestProperty("displayBrightness");
midi2_manager_->requestProperty("currentProgram");
```

### S'abonner aux changements

```cpp
// Recevoir des notifications quand la propriété change
midi2_manager_->subscribeToProperty("tempo");
midi2_manager_->subscribeToProperty("currentPreset");
```

### Callback de réception

```cpp
midi2_manager_->onPropertyRequest = [this](PropertyRequest type,
                                           const juce::String& property_id,
                                           const juce::var& value) {
    switch (type) {
        case PropertyRequest::GET_PROPERTY:
            // Le contrôleur demande une propriété
            if (property_id == "polyphony") {
                midi2_manager_->sendProperty("polyphony", engine_.getPolyphony());
            }
            break;

        case PropertyRequest::SET_PROPERTY:
            // Le contrôleur envoie une propriété
            if (property_id == "displayBrightness") {
                float brightness = value;
                updateDisplayBrightness(brightness);
            }
            break;

        case PropertyRequest::NOTIFY:
            // Notification de changement (si abonné)
            if (property_id == "tempo") {
                float new_tempo = value;
                engine_.setBpm(new_tempo);
            }
            break;

        case PropertyRequest::SUBSCRIBE:
            // Le contrôleur s'abonne à une propriété
            // Envoyer la valeur initiale
            if (property_id == "currentPreset") {
                midi2_manager_->sendProperty("currentPreset", getCurrentPresetName());
            }
            break;
    }
};
```

### Messages UMP Property

Format (128-bit Stream Messages, multiple packets pour grandes données) :

```midi
Word 1: [0xF][Group=0][Status][Packet Index]
Words 2-4: [JSON data payload, 12 bytes per packet]
```

- Status 0x30 = GET_PROPERTY
- Status 0x31 = SET_PROPERTY
- Status 0x32 = SUBSCRIBE
- Status 0x33 = NOTIFY

Exemple JSON :

```json
{"synthName": "HelmBoy"}
{"polyphony": 8}
{"currentPreset": {"name": "Lead 1", "category": "Synth"}}
```

## Feedback bidirectionnel

### Feedback bidirectionnel - Concept

Le feedback bidirectionnel permet au plugin d'envoyer les changements de paramètres au contrôleur pour mettre à jour les LEDs, écrans et motorized faders.

### Configuration de la sortie MIDI

```cpp
// Ouvrir un périphérique MIDI de sortie
auto midi_devices = juce::MidiOutput::getAvailableDevices();
for (const auto& device : midi_devices) {
    if (device.name.contains("Controller")) {
        auto output = juce::MidiOutput::openDevice(device.identifier);
        midi2_manager_->setMidiOutput(output.get());
        break;
    }
}
```

### Envoyer du feedback manuel

```cpp
// Feedback CC standard
midi2_manager_->sendParameterFeedback(
    0,      // channel
    74,     // controller number (filter cutoff)
    0.75f   // normalized value (0.0-1.0)
);

// Feedback per-note controller
midi2_manager_->sendParameterFeedback(
    0,      // channel
    10,     // controller (pan)
    0.5f,   // value
    true,   // is_per_note
    60      // note number (C4)
);
```

### Feedback automatique

#### Activation

```cpp
midi2_manager_->setAutoFeedbackEnabled(true);
```

#### Intégration dans les callbacks de paramètres

```cpp
void HelmPlugin::parameterChanged(std::string_view name, mopo::mopo_float value) {
    std::string name_str{name};
    valueChangedExternal(name_str, value);

    // Notifier NKS hardware
    if (nks_support_) {
        nks_support_->notifyParameterChanged(name, value);
    }

    // Feedback MIDI 2.0 automatique
    if (midi2_manager_ && midi2_manager_->isAutoFeedbackEnabled()) {
        int cc = parameterNameToCC(name);
        midi2_manager_->sendParameterFeedback(0, cc, value);
    }
}
```

### Mapping paramètres ? CC

```cpp
int parameterNameToCC(std::string_view name) {
    static const std::map<std::string, int> mapping = {
        {"cutoff", 74},
        {"resonance", 71},
        {"osc_1_volume", 7},
        {"filter_env_depth", 91},
        {"lfo_1_frequency", 76},
        {"reverb_mix", 93}
    };

    auto it = mapping.find(std::string(name));
    return (it != mapping.end()) ? it->second : -1;
}
```

### Messages UMP Feedback

Format (64-bit Channel Voice) :

```midi
Word 1: [0x4][Group][0xB][Channel][Controller][0x00]
Word 2: [32-bit value]
```

Pour per-note :

```midi
Word 1: [0x4][Group][0x0][Channel][Note][0x00]
Word 2: [32-bit value]
```

## Exemples complets

### Exemple 1 : Configuration MPE avec feedback

```cpp
// Activer le profile MPE
Midi2Manager::ProfileId mpe = {0x00, 0x02, 0x01, 0x00, 0x00};
midi2_manager_->enableProfile(mpe, -1);

// Configurer la sortie
auto output = juce::MidiOutput::openDevice(0);
midi2_manager_->setMidiOutput(output.get());

// Activer le feedback automatique
midi2_manager_->setAutoFeedbackEnabled(true);

// Callback per-note pitch bend
midi2_manager_->onPerNotePitchBend = [this](int ch, int note, float value) {
    engine_.setPerNotePitchBend(note, value);

    // Feedback automatique pour visual feedback
    if (midi2_manager_->isAutoFeedbackEnabled()) {
        midi2_manager_->sendParameterFeedback(ch, 0, value, true, note);
    }
};
```

### Exemple 2 : Property Exchange pour presets

```cpp
// Publier le preset actuel
void publishCurrentPreset() {
    juce::DynamicObject::Ptr preset = new juce::DynamicObject();
    preset->setProperty("name", getCurrentPresetName());
    preset->setProperty("category", getCurrentCategory());
    preset->setProperty("author", getAuthor());
    preset->setProperty("tags", getCurrentTags());

    midi2_manager_->sendProperty("currentPreset", juce::var(preset.get()));
}

// Callback de changement de preset
midi2_manager_->onPropertyRequest = [this](PropertyRequest type,
                                           const String& id,
                                           const var& value) {
    if (id == "loadPreset" && type == PropertyRequest::SET_PROPERTY) {
        String preset_name = value["name"].toString();
        loadPresetByName(preset_name);

        // Notifier le contrôleur du changement
        publishCurrentPreset();
    }
};

// S'abonner aux demandes de liste de presets
midi2_manager_->subscribeToProperty("presetList");
```

### Exemple 3 : Feedback visuel complet

```cpp
class ParameterFeedbackManager {
public:
    void initialize(Midi2Manager* midi2) {
        midi2_ = midi2;

        // Activer le feedback
        midi2_->setAutoFeedbackEnabled(true);

        // Timer pour les paramètres animés
        startTimer(50);  // 20 Hz update rate
    }

    void parameterChanged(const std::string& name, float value) {
        int cc = getControllerNumber(name);
        if (cc >= 0) {
            midi2_->sendParameterFeedback(0, cc, value);
        }
    }

    void timerCallback() override {
        // Feedback pour les paramètres dynamiques (LFOs, enveloppes, etc.)
        updateLFOFeedback();
        updateEnvelopeFeedback();
        updateMeterFeedback();
    }

private:
    void updateLFOFeedback() {
        for (int lfo = 0; lfo < 4; ++lfo) {
            float value = engine_->getLFOOutput(lfo);
            midi2_->sendParameterFeedback(0, 70 + lfo, value);
        }
    }

    void updateMeterFeedback() {
        float left_peak = engine_->getOutputPeak(0);
        float right_peak = engine_->getOutputPeak(1);

        midi2_->sendParameterFeedback(0, 120, left_peak);
        midi2_->sendParameterFeedback(0, 121, right_peak);
    }

    Midi2Manager* midi2_;
};
```

## Performance et optimisations

### Throttling du feedback

Pour éviter la surcharge MIDI :

```cpp
class FeedbackThrottler {
public:
    void sendFeedback(int channel, int cc, float value) {
        auto now = Time::getMillisecondCounterHiRes();
        String key = String(channel) + "_" + String(cc);

        auto last_time = last_send_time_[key];
        if (now - last_time < 50.0) {  // Max 20 Hz par paramètre
            return;
        }

        last_send_time_[key] = now;
        midi2_manager_->sendParameterFeedback(channel, cc, value);
    }

private:
    std::map<String, double> last_send_time_;
    Midi2Manager* midi2_manager_;
};
```

### Batch processing

Pour les changements multiples :

```cpp
void updateMultipleParameters(const std::vector<ParameterChange>& changes) {
    for (const auto& change : changes) {
        // Les messages UMP sont envoyés de manière optimisée
        midi2_manager_->sendParameterFeedback(
            change.channel,
            change.controller,
            change.value
        );
    }
}
```

## Thread safety

Toutes les méthodes Phase 4 sont thread-safe :

- `profiles_lock_` protège la gestion des profiles
- `properties_lock_` protège le cache des propriétés
- `output_lock_` protège l'accès à MidiOutput

```cpp
// Safe depuis n'importe quel thread
juce::MessageManager::callAsync([this]() {
    midi2_manager_->sendParameterFeedback(0, 74, 0.5f);
});
```

## Debugging

### Logs Phase 4

```midi
MIDI2: Profile enabled - Bank=0 Number=2 Channel=All
MIDI2: Property sent - ID=synthName Value="HelmBoy"
MIDI2: Property requested - ID=displayBrightness
MIDI2: Subscribed to property - ID=tempo
MIDI2: Feedback sent - Ch=0 CC=74 Value=0.75
MIDI2: Per-Note Feedback sent - Ch=0 Note=60 CC=10 Value=0.5
MIDI2: MIDI output connected
```

## Standards de référence

- ![**MIDI 2.0 Profile Configuration**](https://www.midi.org/specifications/midi-2-0-specifications/profile-configuration)
- ![**MIDI 2.0 Property Exchange**](https://www.midi.org/specifications/midi-2-0-specifications/property-exchange)
- ![**Bidirectional Communication**](https://www.midi.org/specifications/midi-2-0-specifications/common-rules-for-midi-ci-and-ump)

---

## *Phase 4 complète - MIDI 2.0 implémentation finale*
