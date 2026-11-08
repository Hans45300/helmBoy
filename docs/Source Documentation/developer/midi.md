# MIDI 1.0 et UMP

@page midi_input MIDI 1.0 et décodeur UMP

## Chemin MIDI utilisé par l’application

`MidiManager` est le chemin MIDI 1.0 commun du plug-in et du standalone. Il
reçoit les messages JUCE, les place dans les buffers de traitement, puis
`SynthBase::processMidi` les transmet au gestionnaire avec leur position
d’échantillon. Le moteur traite les notes, le pitch bend, la molette de
modulation et les événements de pression; les CC peuvent alimenter MIDI Learn
et les mappings de paramètres. Le gestionnaire gère aussi les sélections de
banque/dossier et de patch utilisées par l’interface.

Les numéros de canal ne sont pas uniformes dans toutes les interfaces : JUCE
représente ses canaux MIDI 1.0 par 1 à 16; les callbacks de `Midi2Manager` et
les champs UMP utilisent 0 à 15. Les clés de mapping de `MidiManager` combinent
le canal et le numéro de contrôleur; consulter `createMidiId` plutôt que
déduire le codage de l’index du CC seul.

## Adaptateur de callbacks et UMP

`Midi2Manager::processMidiMessage` traite exclusivement des messages MIDI 1.0
`juce::MidiMessage`. Les valeurs qu’il transmet sont normalisées à partir de
la résolution MIDI 1.0 (CC/pression 7 bits, pitch bend 14 bits). Basculer son
`MidiVersion` ne transforme pas ces messages en messages haute résolution.

Le décodeur UMP est une API distincte :

- `decodeUMP` vérifie un paquet de 8 octets, le type de message 0x4 et les
  données réservées/attributs supportés; il n’a pas d’effet de bord.
- `processUMP` refuse les paquets quand le mode MIDI 2.0 est désactivé. Quand
  il est activé, il décode puis appelle uniquement `onUMPEvent`.
- Les statuts décodés sont Note Off, Note On (un Note On de vélocité zéro
  devient Note Off), Control Change et Pitch Bend.
- L’événement conserve le groupe, le canal 0-based, l’index, la valeur brute
  16/32 bits et l’offset d’échantillon. Le décodeur ne normalise pas ces valeurs
  et ne les convertit pas vers les callbacks MIDI 1.0.
- Les types de message, attributs de note et statuts non pris en charge sont
  rejetés avec un résultat `UMPDecodeResult` explicite.

`HelmPlugin` et `HelmBoyEditor` créent un `Midi2Manager` et lui assignent des
callbacks MIDI 1.0, mais les buffers MIDI natifs des hôtes et périphériques ne
sont pas envoyés à `processUMP`. L’événement UMP décodé n’est pas raccordé au
gestionnaire de voix dans le flux audio actuel. Le réglage sauvegardé active
donc le décodeur explicite; il ne rend pas disponible une entrée UMP native.

## API expérimentales séparées

`midi_2_phase4.cpp` contient des API anciennes pour profils, propriétés et
feedback avec périphérique de sortie. Elles ne sont pas appelées par les
chemins plugin/standalone actuels. Leur présence dans le binaire ne constitue
pas une implémentation complète ou une certification MIDI 2.0/MIDI-CI; ne pas
les présenter comme une négociation de capacités ou un feedback hôte opérationnel.

## Couverture de test

`tests/midi_2_manager_tests.cpp` couvre les paquets acceptés/rejetés, le
maintien du groupe et de l’offset, le mode désactivé et la résolution des
callbacks MIDI 1.0. Il ne valide ni l’ingestion UMP native par un hôte, ni le
raccordement des événements décodés au synthétiseur.
