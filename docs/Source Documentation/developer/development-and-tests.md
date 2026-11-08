# Développement et tests ciblés

@page development_and_tests Développement et tests ciblés

## Cibles principales

Le projet CMake configure les cibles JUCE `HelmBoyPlugin` et
`HelmBoyStandalone`. Les formats du plug-in sont contrôlés par `BUILD_VST3`,
`BUILD_AU`, `BUILD_LV2` et `BUILD_AAX`; VST3, AU et LV2 sont activés par défaut,
AAX ne l’est pas. Les formats effectivement produits dépendent des SDK et de la
plateforme. Le standalone dépend des pilotes activés et de la disponibilité
optionnelle du SDK ASIO sur Windows.

## Traces de démarrage standalone

Les jalons détaillés du démarrage standalone sont désactivés par défaut pour
éviter des écritures synchrones dans le journal pendant l’initialisation. Pour
les activer lors d’un diagnostic, définir `HELMBOY_ENABLE_STARTUP_TRACE=1` à la
compilation. Les traces sont écrites dans le fichier `startup_trace.log` du
répertoire de journaux de l’application et incluent les horodatages des phases
de démarrage.

Le démarrage standalone crée maintenant l’éditeur et tente de démarrer l’audio
une seule fois, sans reprises temporisées ni watchdog qui force la fermeture.
En cas d’échec, l’application GUI affiche un message et reste ouverte; en mode
headless, elle termine proprement. Les contrôles de readiness du peer et du
périphérique, le repli vers le périphérique par défaut après un état audio
sauvegardé invalide, ainsi que les protections de durée de vie des callbacks
restent actifs.

Validation manuelle recommandée : démarrer et fermer plusieurs fois le GUI
avec un périphérique disponible; essayer un périphérique indisponible et un état
audio sauvegardé invalide; vérifier que le GUI reste utilisable après un échec,
que le mode headless termine proprement en cas d’échec et que les deux modes
démarrent normalement avec une configuration valide.

## Tests optionnels

Les options de tests ciblés sont désactivées par défaut :

- `HELMBOY_BUILD_MIDI_TESTS` ajoute `HelmBoyMidiTests` et le test CTest
  `Midi2ManagerTests`.
- `HELMBOY_BUILD_EFFECT_TESTS` ajoute `HelmBoyEffectTests` et le test CTest
  `StereoEffectsTests`.
- `HELMBOY_BUILD_SVF_TESTS` ajoute `HelmBoySVFTests` et le test CTest
  `StateVariableFilterTests`, couvrant l'atténuation passe-bas, le bypass et la
  stabilité des sorties 12/24 dB sur plusieurs taux d'échantillonnage, tailles
  de bloc et plages de coupure/résonance.
- `HELMBOY_BUILD_UNDO_HISTORY_TESTS` ajoute `HelmBoyUndoHistoryTests` et le test
  CTest `UndoHistoryTests`, couvrant les bornes, l'ordre undo/redo, la
  suppression de la branche redo, l'éviction au-delà de la capacité et le reset.
- `HELMBOY_BUILD_WAVEFORM_SELECTION_TESTS` ajoute `HelmBoyWaveformSelectionTests`
  et le test CTest `WaveformSelectionTests`, couvrant la formule et la table
  Saw Möbius, les règles d’indexation et de labels Osc/LFO, les plages de
  paramètres et la géométrie des cellules.

La cible Undo/Redo se valide avec les commandes suivantes depuis la racine :

```powershell
cmake -S . -B build -DHELMBOY_RUN_TESTS=ON -DHELMBOY_BUILD_UNDO_HISTORY_TESTS=ON
cmake --build build --config Debug --target HelmBoyUndoHistoryTests
ctest --test-dir build -C Debug -R UndoHistoryTests --output-on-failure
```

Le ring d'historique est borné à 256 étapes. Chaque étape contient un index de
contrôle et deux valeurs `mopo_float`; avec la précision double actuelle, son
stockage fixe est de 6 KiB maximum. Le suivi des gestes est préalloué une fois
par contrôle; append, Undo et Redo sont en O(1). Le coût runtime et la
réactivité en hôte restent à mesurer par le propriétaire.

Le script `utilities/Invoke-SVFTests.ps1` configure un répertoire isolé,
compile la cible SVF en Release, lance uniquement `StateVariableFilterTests`
et analyse le rapport JUnit. Il conserve le journal, le rapport et un résumé
dans `build/svf-tests/test-reports/`. Depuis la racine du dépôt :

```powershell
.\utilities\Invoke-SVFTests.ps1
```

Le script accepte `-BuildDirectory`, `-Generator`, `-Configuration`,
`-Parallelism`, `-SkipConfigure` et `-SkipBuild`. Les options de saut supposent
que le répertoire contient déjà une configuration ou un binaire utilisable.
Le script ne supprime pas le répertoire de build. Une erreur CMake, de
compilation, CTest ou du rapport JUnit produit un code de sortie non nul.

Exemple PowerShell à lancer localement depuis la racine pour les cibles MIDI
et effets, si un build Debug existe ou doit être configuré :

```powershell
cmake -S . -B build -DHELMBOY_BUILD_MIDI_TESTS=ON -DHELMBOY_BUILD_EFFECT_TESTS=ON
cmake --build build --config Debug --target HelmBoyMidiTests HelmBoyEffectTests
ctest --test-dir build -C Debug --output-on-failure
```

Les tests MIDI vérifient le décodeur UMP et certains callbacks isolés. Les tests
d’effets couvrent le reset logique de `Memory`, la suppression d'une queue de
feedback du délai, le ping-pong stéréo, le reset du moteur Reverb, le plafond du limiteur,
son relâchement, son bypass, le lien stéréo et les positions centre/gauche/droite
du Pan. Ils ne remplacent pas un essai audio, une validation
de preset, ni un test dans chaque hôte/formats.

La demande de reset des effets est exécutée au thread audio avec fondu de 10 ms
avant et après le reset silencieux au bloc. Vérification manuelle propriétaire
recommandée : changer de patch et revenir à Init avec delay/reverb actifs,
feedback élevé et freeze activé, répéter dans plugin et standalone, puis
écouter les transitions et surveiller la charge CPU. Cette écoute n'est pas
remplacée par le test ciblé de mémoire/délai.

Le test SVF ne fixe pas de seuil de performance, car les mesures de temps de
traitement dépendent de la machine et de la charge. Pour comparer le coût d'une
modification du filtre, mesurer en Release dans le même hôte avec le même preset,
la même polyphonie, le même taux d'échantillonnage, la même taille de bloc et les
mêmes réglages de modulation. Cette caractérisation n'implique pas que le rendu
ou l'implémentation du filtre ait été modifié.

Ces assertions SVF sont des garde-fous techniques de réponse, de bypass et de
stabilité; elles ne déterminent pas si une variation du filtre est plus musicale.
Pour juger une amélioration sonore, comparer la référence et la variante à
niveau égal, avec les mêmes presets, réglages, modulation et automation, puis
consigner l'écoute et les mesures avant/après. Un défaut de calcul n'est pas
requis pour qu'une expérimentation musicale soit pertinente.

Les options `HELMBOY_ENABLE_BIQUAD_JUCE_PATH` et
`HELMBOY_ENABLE_SVF_JUCE_PATH` sont activées par défaut dans `CMakeLists.txt`.
Reverb utilise toujours JUCE : son ancien réseau comb/all-pass et l'option
`HELMBOY_ENABLE_REVERB_JUCE_PATH` ont été supprimés; une ancienne entrée de
cache ne sélectionne plus d'autre implémentation. `juce_audio_basics` est une
dépendance publique de `mopo`, requise par l'en-tête de Reverb. Les options
`HELMBOY_DEBUG_*` et les cibles de tests ont des valeurs distinctes; vérifier
le cache CMake utilisé avant de conclure quel chemin de filtre a été compilé.

Le test stéréo compare Reverb à une instance JUCE de référence à 44,1, 48 et
96 kHz, avec des blocs de 64 et 128 échantillons, des changements de Wet,
Width et Freeze et un reset. Les vérifications du signal sec et de l'effacement
des queues sont conservées. Ces tests sont à exécuter par le propriétaire du
projet; leur ajout ne constitue pas un résultat de validation. Après recompilation,
comparer aussi les presets existants dans les cibles plugin et standalone,
notamment avec Freeze, modulation, changement de patch et retour à Init.

## Texte des paramètres hôte (21.1)

La cible opt-in `HelmBoyParameterTextTests` vérifie le contrat de `ValueBridge`
sans pointeur DSP : argument fourni, unités, libellés, conversions non linéaires,
valeurs négatives, étapes discrètes, erreurs de saisie, limites de longueur,
locale et allers-retours sur les définitions de paramètres. Elle vérifie aussi
la conservation des formules de normalisation et des métadonnées. Elle ne
remplace pas des tests de rappel de session ni une écoute de comparaison.

Commandes PowerShell proposées au propriétaire, non exécutées par l'assistant :

```powershell
cmake -S . -B build -DHELMBOY_RUN_TESTS=ON -DHELMBOY_BUILD_PARAMETER_TEXT_TESTS=ON
cmake --build build --config Release --target HelmBoyParameterTextTests
ctest --test-dir build -C Release -R '^ParameterTextTests$' --output-on-failure
```

Compiler aussi les cibles plugin et standalone concernées avec MSVC, GCC et
Apple Clang. Dans les éditeurs génériques des hôtes maintenus, vérifier les
pourcentages signés, durées, fréquences, choix et toggles, puis la saisie avec
et sans unité. Vérifier les gestes, l'automation et le rappel d'une session
existante avec l'éditeur ouvert puis fermé. Dans HelmBoy, vérifier la saisie
numérique GUI et comparer les mêmes presets avant/après sans changement de son.
Les affichages hôte utilisent désormais les valeurs présentées et les unités
déclarées; leurs valeurs numériques d'automation restent normalisées comme
auparavant. Le propriétaire confirme la compilation et les tests du point 21.1
le 2026-11-04. Cette validation n'a pas été répétée par l'assistant. La sortie
CTest Windows Release fournie par le propriétaire confirme `ParameterTextTests`
réussi; les hôtes et scénarios de validation ne sont pas précisés.

## Notifications des paramètres hôte (21.2)

La cible opt-in `HelmBoyParameterNotificationTests` vérifie les noms stables,
le snapshot atomique, la récursion, les gestes hôte, deux callbacks simultanés,
les producteurs concurrents, le secours de saturation, l'ordre UI/hôte et le
rebouclage des numéros d'ordre. Elle affiche un temps indicatif pour 40000
publications concurrentes, sans seuil universel de performance. Ses attentes
et synchronisations de test ne font pas partie du code audio de production.

Le compteur de tests intercepte uniquement les appels C++ ordinaires à `new`
du thread observé. Il ne prouve pas l'absence de malloc direct, d'allocations
alignées, d'allocations dans le runtime TLS ou dans les wrappers. Le contrôle
des producteurs explicites de ConcurrentQueue complète cet audit de source,
sans remplacer un profileur d'allocations dans l'hôte réel.

Commandes PowerShell proposées au propriétaire, non exécutées par l'assistant :

```powershell
cmake -S . -B build -DHELMBOY_RUN_TESTS=ON -DHELMBOY_BUILD_PARAMETER_TEXT_TESTS=ON -DHELMBOY_BUILD_PARAMETER_NOTIFICATION_TESTS=ON -DHELMBOY_BUILD_UNDO_HISTORY_TESTS=ON
cmake --build build --config Release --target HelmBoyParameterTextTests HelmBoyParameterNotificationTests HelmBoyUndoHistoryTests
ctest --test-dir build -C Release -R '^(ParameterTextTests|ParameterNotificationTests|UndoHistoryTests)$' --output-on-failure
```

Compiler les produits plugin et standalone avec MSVC, GCC et Apple Clang.
Mesurer les allocations dès la première automation d'un thread hôte, pas
seulement après échauffement, puis la charge et la contention sous automation
dense. Comparer à une référence avant modification aux mêmes fréquences,
tailles de bloc, polyphonie et presets, avec l'éditeur ouvert puis fermé.
Vérifier valeur finale, rappel de session, MIDI Learn, gestes, absence de boucle
hôte, absence d'enregistrement de l'automation dans Undo/Redo, changements de
patch et Init. Sous forte saturation, vérifier la politique de coalescence
documentée et l'absence de réversion par un message plus ancien. Répéter
l'ouverture/fermeture de l'éditeur et les cycles de destruction de l'instance.
Utiliser un détecteur de courses sur une cible compatible, sans remplacer
l'écoute et la validation hôte. Le propriétaire confirme la compilation Windows
et les tests de comportement standalone sous Windows le 2026-11-04; cette
validation n'a pas été répétée par l'assistant. Deux sorties CTest fournies par
le propriétaire confirment chacune 50 exécutions consécutives réussies de
`ParameterNotificationTests` sous Windows Release (`--repeat until-fail:50`),
sans échec : 10,40 s pour le premier run et 3,68 s pour le second. Lors du
second run, les durées affichées par exécution sont de 0,07 à 0,09 s. Ces temps
incluent le processus et son initialisation; ils ne mesurent ni la latence des
callbacks audio ni leur pire temps d'exécution. Ces résultats valident les
assertions de cette cible sur ces essais, sans prouver l'absence de toute course
ni remplacer le profilage en hôte.
La validation plugin/hôte, les autres plateformes et les mesures runtime
d'allocations/contention restent à confirmer.
Le point 21.4 est implémenté : compilation et tests standalone Windows confirmés
par le propriétaire le 2026-11-04, validation hôte/export encore en attente.
Le point 21.5 est implémenté : les IDs explicites sont ajoutés; tests automatisés,
build et rappel de session avec cette version restent à valider par le propriétaire.

## Reset hôte du plugin (21.3)

La cible opt-in `HelmBoyHostResetTests` instancie le vrai plugin sans ouvrir son
éditeur. Elle couvre reset avant préparation, répétition, reset après arrêt,
voix maintenues/sustain, queues d'effets/Freeze, état sérialisé et routes conservés,
paramètres en attente, invalidation des événements clavier et reprise des notes.
Les fréquences 44,1/48/96 kHz et blocs de 32/128/256 sont exercés. Un cas
arpégiateur vérifie vélocité, déduplication, notes fractionnaires, sustain et
reprise; un autre couvre son activation dans le plugin. Le compteur d'allocations
ne voit que le `new` C++ ordinaire du thread observé, pas toutes les allocations
du runtime ni les libérations; il ne constitue pas un certificat temps réel.
L'initialisation JUCE GUI nécessaire au plugin peut exiger une session graphique
ou un affichage virtuel dans un environnement Linux sans écran.

Commandes PowerShell proposées au propriétaire, non exécutées par l'assistant :

```powershell
cmake -S . -B build -DHELMBOY_RUN_TESTS=ON -DHELMBOY_BUILD_HOST_RESET_TESTS=ON -DHELMBOY_BUILD_EFFECT_TESTS=ON
cmake --build build --config Release --target HelmBoyHostResetTests HelmBoyEffectTests
ctest --test-dir build -C Release -R '^(HostResetTests|StereoEffectsTests)$' --output-on-failure
```

Compiler aussi plugin et standalone avec MSVC, GCC et Apple Clang. En hôte,
vérifier arrêt/reprise, désactivation/réactivation, export hors ligne, répétition
du reset, changement de fréquence et de taille de bloc, éditeur ouvert/fermé et
fermeture de session. Tester arp, sustain, longues releases, feedback élevé et
Freeze : silence sans ancien résidu après reset, puis nouvelles notes normales,
sans changement des paramètres ni des routes. Vérifier également l'absence de
régression des motifs d'arpégiateur et de leur coût sur des accords chargés.
Le reset est une coupure hôte, non le fondu musical de changement de patch;
ne pas exiger une continuité sonore d'une commande demandant le silence.

Avec un profileur, contrôler allocations et libérations dans le reset, ainsi
que l'absence de nouvelle attente. Contrôler sous détecteur de courses les
frontières de callbacks et le nettoyage clavier, en respectant les exigences
de sérialisation des opérations d'état/cycle de vie. Si un reset arrive pendant
un bloc, vérifier le reset à sa frontière et la sortie silencieuse; aucun test
ajouté ne certifie tous les scénarios de concurrence hôte. Vérifier le nettoyage
différé du clavier à la réouverture de l'éditeur et les nouvelles notes MIDI.
Le propriétaire confirme la compilation Windows et les tests standalone du
point 21.3 le 2026-11-04. Cette validation n'a pas été répétée par l'assistant.
Les `HostResetTests` automatisés passent dans l'exécution CTest ciblée Windows
Release rapportée ci-dessous. Les contrôles d'allocations, compilations sur les
autres plateformes et essais plugin/hôte restent à confirmer.
Le standalone ne déclenchant pas ce contrat plugin, ses tests ne valident pas
à eux seuls le callback de reset hôte.

## Durée des queues hôte (21.4)

`HelmBoyTailTests` couvre release, bypass/mix, chorus, feedback délai signé,
limites +/-1, réverbération normale/Freeze, modulation pertinente, entrées
invalides, historique et protection des conversions wrapper jusqu'à 384 kHz.
`HelmBoyHostResetTests` couvre aussi les snapshots avant consommation audio,
les modifications de routage, le maintien d'un ancien rapport après bypass,
son effacement au reset et l'absence de mutation d'état/latence par la requête.
`TailLengthTests` et `StereoEffectsTests` passent dans l'exécution CTest ciblée
Windows Release rapportée ci-dessous; l'assistant ne les a pas exécutés.

Commandes PowerShell proposées au propriétaire :

```powershell
cmake -S . -B build -DHELMBOY_RUN_TESTS=ON -DHELMBOY_BUILD_TAIL_TESTS=ON -DHELMBOY_BUILD_HOST_RESET_TESTS=ON
cmake --build build --config Release --target HelmBoyTailTests HelmBoyHostResetTests
ctest --test-dir build -C Release -R '^(TailLengthTests|HostResetTests)$' --output-on-failure
```

Compiler les produits affectés avec MSVC, GCC et Apple Clang. Dans les hôtes
VST3 puis les autres formats maintenus, comparer rendus à longueur explicite et
automatique : effets bypassés, mix nul, release long, chorus, délai court/long,
tempo synchronisé très lent, feedback négatif/proche de +/-1, réverbération et
Freeze. Mesurer le temps jusqu'à -100 dB et inspecter les résidus, sans confondre
seuil relatif et silence absolu. Répéter après automation, modulation de mix,
bypass/feedback/release, changement de preset, arrêt/reprise et reset.

Vérifier suspension et CPU après silence, vieillissement des rapports, rappel
de session et absence de modification de latence ou d'automation. Tester les
requêtes avant préparation, pendant les modifications et à haute fréquence
audio, avec profileur d'allocations et détecteur de courses si disponible.
Contrôler qu'une queue infinie n'est pas convertie en compteur négatif ou petit;
les formats sans infini vérifié reçoivent un plafond fini de repli, pas une
garantie de couvrir Freeze. Un export automatique peut être très long : prévoir
une longueur explicite pour les réglages non décroissants. Consigner ce compromis
et le comportement de chaque format; compilation seule et succès des tests de
formule ne prouvent pas que les hôtes respectent la déclaration. Le propriétaire
confirme la compilation Windows et les tests standalone Windows du point 21.4
le 2026-11-04, sans répétition par l'assistant. Les tests automatisés du calcul
et des snapshots passent dans l'exécution CTest ciblée Windows Release rapportée
ci-dessous. Les autres plateformes, les essais d'export/suspension et les
mesures de queues longues/Freeze restent à confirmer. Les essais standalone
ne valident pas à eux seuls la déclaration ni son interprétation par les hôtes.

## Documentation API

La référence Doxygen est configurée dans `Doxyfile`; les pages Markdown du
répertoire sont aussi ses entrées. La procédure et le chemin de sortie sont
décrits dans [le README Doxygen](../README.md). La sortie HTML n’est pas une
source : ne pas modifier ni régénérer ses fichiers dans une tâche documentaire
sans besoin explicite.

## Discipline de validation

Point 21.5 implémenté : `ValueBridge` dérive maintenant de
`AudioProcessorParameterWithID` et utilise l'ID stable `helmboy_` suivi du nom
existant de contrôle mopo. L'ordre d'enregistrement, les plages, defaults et
conversions restent inchangés. Les wrappers JUCE de la révision d60b7a7803
exposent des mappings différents : hash positif 31 bits en VST3, hash UInt32 et
tri par version hint puis hash en AU, ID explicite en AUv3, symbole URI en LV2.
Le test `ParameterTextTests` vérifie les IDs explicites de tous les contrôles,
leur unicité, l'absence de collision des hashes VST3/AU et AAX et les caractères
compatibles avec les symboles LV2. Le propriétaire a compilé la cible et fourni le résultat CTest Windows Release
du 2026-11-04 : test réussi (1/1, zéro échec, 0,18 s). Il confirme
également une compilation MSVC Release sans erreur avec la commande
`./utilities/buildAndCompile.ps1 -BuildMode All -Config Release` le
2026-11-04. Le mode All compile le plugin VST3, le plugin LV2 et le
standalone.

L'utilisateur confirme l'absence d'historique d'utilisation : aucun rappel d'anciens
projets ni migration depuis les IDs d'index n'est requis. Procédure de validation
future : sauvegarder puis rouvrir, avec cette même version, un projet contenant
les paramètres et automations représentatifs dans chaque format/hôte disponible;
les IDs et valeurs doivent rester stables. VST3 : Ableton/Cubase; LV2 : hôte Linux.
AU/AUv3 n'ont jamais été disponibles : le workflow Apple n'a jamais fonctionné.

Résultat CTest ciblé Windows Release fourni par le propriétaire le 2026-11-04 :
`ParameterTextTests`, `ParameterNotificationTests`, `HostResetTests`,
`TailLengthTests` et `StereoEffectsTests` passent (5/5, zéro échec, 1,01 s).
Le propriétaire confirme également une compilation Linux avec les dernières
modifications; le compilateur et les cibles exactes ne sont pas précisés. Ces
résultats n'ont pas été reproduits par l'assistant et ne valident pas les essais
d'hôte/export, les mesures d'allocation ou les autres plateformes.

Pour un changement documentaire, vérifier les liens relatifs, les chemins
mentionnés, les identifiants de paramètres et les déclarations d’options contre
le source. Pour un changement DSP ou d’hôte, les vérifications locales
pertinentes sont compilation des cibles touchées, tests CTest ciblés, écoute et
validation dans l’hôte visé. Les commandes de ce guide sont destinées à
l’utilisateur; aucun résultat de compilation ou d’écoute n’est implicite.
