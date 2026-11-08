# Architecture du logiciel

@page architecture Architecture du logiciel

HelmBoy partage son moteur sonore et ses contrôles entre deux intégrations JUCE : le plug-in audio et l’application autonome. Le code spécifique à l’hôte fournit les événements, prépare les blocs audio et expose les paramètres; le moteur `mopo::HelmBoyEngine` calcule les voix et la chaîne d’effets.

## Couches du dépôt

| Répertoire | Responsabilité |
| --- | --- |
| `src/plugin` | Adaptateur `AudioProcessor`, paramètres d’automation, programmes, état d’hôte et création de l’éditeur. |
| `src/standalone` | Fenêtre, gestionnaire de périphériques audio/MIDI et adaptation `AudioSource`. |
| `src/common` | `SynthBase`, façade commune, contrôles, MIDI, sauvegarde, configuration et transport des mises à jour GUI. |
| `src/synthesis` | `HelmBoyEngine`, routage des paramètres, modulation, oscillateurs et construction des voix. |
| `src/editor_sections` | Sections fonctionnelles de l’interface, navigateur de patches et orchestration de l’éditeur. |
| `src/editor_components` | Contrôles, claviers et visualisations JUCE CPU. |
| `src/look_and_feel` | Palette, typographie et primitives de rendu JUCE. |
| `mopo/src` | Primitives DSP, graphe de processeurs, routeurs, paramètres et filtres/effets. |

`HelmPlugin` et `HelmBoyEditor` dérivent tous deux de `SynthBase`. Cela partage l’API du synthétiseur, pas leurs règles de synchronisation : le plug-in utilise le verrou de callback JUCE, tandis que le standalone possède son propre `CriticalSection` et son propre cycle de périphériques.

Le rendu de l’interface utilise les composants CPU JUCE et `Graphics`. Les
visualisations consomment les snapshots publiés par `SynthBase`; la qualité
visuelle est validée sur les plateformes et hôtes pris en charge.

## Cycle audio

### Plug-in

`HelmPlugin::prepareToPlay` transmet la fréquence d’échantillonnage et la taille de bloc au moteur et au gestionnaire MIDI. `processBlock` lit les informations de transport disponibles, applique les changements de contrôles et de modulations en attente, puis traite les événements MIDI à leurs offsets. Les blocs dépassant `MAX_BUFFER_PROCESS` sont découpés en segments d’au plus 256 échantillons. Chaque segment appelle `SynthBase::processAudioSafe`.

### Application autonome

`HelmBoyEditor::prepareToPlay` prépare le moteur après l’ouverture du périphérique. `getNextAudioBlock` protège la section de traitement par le verrou de l’éditeur, vérifie l’état de démarrage et la taille du buffer, récupère les événements du périphérique et du clavier, puis traite le buffer par segments d’au plus 256 échantillons. Si l’audio n’est pas prêt, la région active est effacée.

### Moteur et signal

`HelmBoyEngine` dérive de `HelmBoyModule`, lui-même fondé sur `mopo::ProcessorRouter`. Le routeur ordonne les processeurs d’après leurs dépendances; les processeurs de feedback représentent les cycles explicites. Les modules enregistrent leurs contrôles et sources/destinations de modulation. Le `HelmBoyVoiceHandler` contient les éléments polyphoniques et duplique les processeurs nécessaires selon le nombre de voix; les sources monophoniques (LFO, séquenceur, arpégiateur) sont construites au niveau du moteur.

La chaîne de sortie raccordée dans `HelmBoyEngine::init` est :

1. sommation des voix;
2. distorsion;
3. chorus stéréo;
4. délai stéréo et bypass;
5. un filtre DC par canal;
6. réverbération stéréo et bypass;
7. volume lissé;
8. balance stéréo Pan;
9. mesure de crête;
10. limiteur stéréo lié;
11. clamp final et sorties gauche/droite.

Le limiteur est après le mesureur de crête : l’indication de crête exposée par
ce point du graphe n’est donc pas une mesure post-limiteur. Il calcule un pic
commun gauche/droite sur le bloc, réduit immédiatement le gain si ce pic dépasse
le plafond demandé, puis lisse le retour vers le gain unité selon le temps de
relâchement. Le limiteur n’ajoute pas de buffer de lookahead.

Reverb utilise exclusivement `juce::Reverb` et reçoit les entrées gauche et
droite séparément; l’entrée droite non connectée retombe sur la gauche pour
conserver la compatibilité mono. Le réseau mopo legacy comb/all-pass et son
option de compilation ont été supprimés. Les paramètres, leur ordre et le
mélange Wet/Dry du chemin JUCE restent inchangés.

Dans l’interface, le panneau Limiter est placé à droite de l’oscilloscope, à sa
même hauteur et avec la largeur du panneau Distortion. Son activator est dans
la barre de titre; Ceiling et Release sont les deux contrôles du panneau.

`SynthBase::processAudioSafe` appelle le graphe, copie les sorties du moteur dans les canaux audio en alternant gauche/droite, vérifie les échantillons finis avec les assertions actives, puis publie les données destinées aux visualisations. Ce chemin produit un signal de synthèse; il ne mélange pas un flux audio d’entrée de l’hôte.

Le contrôle `pan` agit après le volume et avant la mesure de crête. Au centre,
les deux canaux conservent leur gain; vers la gauche ou la droite, seul le canal
opposé est atténué. Le traitement ne somme pas les canaux et préserve leur
contenu stéréo.

## Transitions de patch et état des effets

Les chargements de patch, changements de programme, snapshots Init/A-B et
changements de patch MIDI demandent un reset des effets avant l'application du
nouvel état. La demande est atomique; le thread audio la consomme dans
`SynthBase::processAudioSafe`, applique un fondu de sortie de 10 ms, remet les
états persistants à zéro à la frontière silencieuse d'un bloc, puis applique un
fondu d'entrée de 10 ms. Aucun thread UI ne modifie les buffers d'effet.

`HelmBoyEngine::resetEffectState` cible chorus, delay, filtres DC, reverb et
limiteur; `ProcessorRouter::resetState` propage le reset aux sous-processeurs
et aux buffers Feedback. `Memory::reset` invalide en O(1) l'historique logique
d'une ligne circulaire, sans parcourir sa capacité maximale. Reverb appelle
le reset de son moteur JUCE lorsqu'il a déjà été initialisé.
Le traitement reste dans le thread audio et ne crée ni processeur ni buffer.

La durée et le comportement de la transition doivent être vérifiés par écoute
dans plugin et standalone, notamment avec delay/reverb actifs, feedback ou
freeze élevés, et lors de changements répétés de patch. Vérifier l'absence de
clicks/résidus et la réactivité du thread audio; aucune mesure runtime n'est
implicite dans la seule présence de ces chemins.

### Reset hôte immédiat (21.3)

`HelmPlugin::reset` implémente le contrat JUCE d'arrêt des voix et des queues,
sans charger Init ni changer de patch. Le wrapper VST3 appelle notamment ce
contrat à l'arrêt du traitement (`setProcessing(false)`). Un appel normalement
sérialisé réalise le reset avant de retourner, même sans bloc audio ultérieur,
avant préparation ou après arrêt. Les paramètres, routes, valeurs d'automation
en attente et l'historique Undo/Redo ne sont pas effacés.

`SynthBase::resetProcessingState` réutilise `HelmBoyEngine::resetState`, annule
la demande et la phase de fondu des effets, remet le gain de transition à un et
efface les compteurs transitoires de notes/crêtes. Le moteur vide l'arpégiateur,
propage le reset aux processeurs/effets et efface ses sorties. VoiceHandler
retire immédiatement les voix, le sustain et leurs événements, réutilise ses
réserves et vide ses sorties accumulées. HelmBoyVoiceHandler efface également
son trigger de redéclenchement. Les hooks existants des processeurs sont appelés;
ce reset ne prétend pas remettre chaque oscillateur/LFO en phase Init.

La table privée des vélocités de l'arpégiateur est désormais un vecteur de
paires réservé pour les notes MIDI, plutôt qu'une map libérant ses nœuds au
reset. Les tableaux de motifs sont également réservés; le reset vide leur
contenu sans libérer les réserves. Les mêmes clés fractionnaires, vélocités,
déduplication et tris de motifs sont conservés. Les recherches sont linéaires
dans la table des notes; vérifier leur coût et tous les motifs avant/après.
Ce changement local ne remplace ni l'allocateur de voix ni l'architecture DSP.

Le plugin possède un drapeau atomique d'appartenance pour préparation, traitement
et reset. `reset` ne bloque pas : si un callback possède déjà le traitement,
une demande atomique est laissée à son entrée/sortie; le callback termine le
reset et efface son buffer de sortie s'il a reçu une demande pendant le bloc.
Un traitement réentrant sans propriété rend du silence, et une préparation
concurrente est rejetée avec assertion. La remise à zéro synchrone est garantie
pour les appels sérialisés normaux, pas pour un reset arrivant au milieu d'un
appel concurrent illégal. La vérification à l'entrée empêche de reprendre un
bloc avec un reset en attente. Aucun verrou ni attente n'est ajouté au reset.
Les opérations de cycle de vie, chargement de preset et restauration d'état
doivent rester sérialisées selon les contrats JUCE/hôte; ce drapeau ne remplace
pas leurs protections existantes ni ne rend les mutations du graphe concurrentes.

Le clavier JUCE possède un verrou dans `MidiKeyboardState::reset`. Le reset
audio invalide donc son injection par génération atomique, sans appeler ce
reset verrouillé. Le timer de notification existant efface l'état clavier sur
le message thread puis acquitte la génération. Une demande plus récente reste
visible. Jusqu'à cet acquittement, les événements clavier indirects ne sont
pas injectés; les nouveaux événements MIDI reçus directement de l'hôte restent
traités. Sans éditeur/timer actif, les événements invalidés restent inutilisés
jusqu'au prochain démarrage des notifications. Le collecteur de périphérique
standalone et le décodeur UMP expérimental ne sont pas reconfigurés par ce
contrat plugin. Aucun reset automatique du standalone n'est introduit.

## Paramètres et modulation

Les contrôles sont créés par les modules à partir des définitions de `mopo::Parameters`. Les tables des modules servent à exposer les contrôles à `SynthBase` et à l’interface. Une connexion de modulation associe une source, une destination et une valeur d’intensité; `HelmBoyEngine` raccorde la source à l’entrée du processeur de destination et active les sélecteurs mono/poly appropriés.

Dans le plug-in, `ValueBridge` expose les contrôles JUCE à l’hôte et convertit les valeurs entre l’échelle du paramètre interne et celle du paramètre plugin. Les paramètres existants doivent conserver leurs identifiants et leur ordre d’enregistrement; l’ajout de paramètres est effectué en fin de liste afin de préserver les indices d’automation déjà publiés.

`ValueBridge::getText` formate uniquement son argument normalisé, sans lire le
contrôle DSP. Les conversions numériques partagent avec la saisie GUI les
fonctions `mopo::tryInternalToDisplay` et `mopo::tryDisplayToInternal`, selon
la courbe, le décalage, l'inversion et le multiplicateur de `ValueDetails`.
Les choix hôte réutilisent les tables de libellés existantes; les unités sont
exposées séparément par `getLabel`. Un suffixe d'unité correspondant est accepté
à la saisie. Les nombres utilisent une locale classique explicite; une virgule
décimale unique est également acceptée. Texte invalide, unité incompatible,
valeur non finie ou inverse indéfini ramènent la valeur normalisée par défaut.
Les valeurs finies convertibles sont bornées et arrondies selon les étapes
existantes. Les libellés tronqués ne garantissent pas un aller-retour exact.
Les limites de longueur positives tronquent le texte; zéro ou une valeur
négative n'imposent pas de limite. Ce contrat de présentation ne modifie ni
les identifiants, ni les plages, ni les conversions numériques d'automation.
Le formatage et le parsing créent des chaînes; ils ne constituent pas une
nouvelle primitive temps réel et ne sont pas ajoutés au traitement audio.

### Notifications d'automation (21.2)

`ValueBridge` transmet une référence au nom possédé par ses métadonnées, sans
conversion de chaîne dans `setValue`. `HelmPlugin` utilise des recherches
transparentes pour ses notifications. Sa lecture `getValue` utilise le snapshot
atomique des valeurs demandées plutôt que le contrôle DSP mutable; ce snapshot
peut précéder l'application audio, comme les demandes UI déjà mémorisées.
La garde anti-récursion est une pile locale au thread, portée par les appels,
qui bloque le retour vers le même bridge sans supprimer l'entrée d'un autre
thread. Elle ne modifie pas les paires begin/end des gestes.

`SynthBase::valueChangedExternal` ne fait ni notification GUI directe, ni
enregistrement Undo/Redo. Il cherche le paramètre dans la table immuable,
publie sa valeur et marque les atomiques de rafraîchissement existantes. Le
timer GUI continue la livraison sur le message thread, éditeur ouvert ou non;
une nouvelle ouverture lit le snapshot courant. Les anciennes tables restent
possédées jusqu'à la destruction de SynthBase. Les slots et pointeurs de
contrôles ne sont pas invalidés à la fermeture de l'éditeur. Le plugin ne
reconstruit pas cette table en cours de traitement; le préflight standalone
reste une opération préalable à l'activité audio. L'hôte doit avoir cessé les
callbacks avant la destruction du processeur, comme pour les wrappers JUCE.

`ParameterChangeQueue` conserve ConcurrentQueue comme transport commun. Elle
préalloue 4096 places initiales et huit producteurs explicites à la construction.
Chaque producteur est réservé par un test atomique, utilisé par un seul appel
à la fois puis libéré; un producteur occupé est sauté sans attente. Le chemin
hôte utilise uniquement `try_enqueue(token, ...)`, car la variante implicite
peut allouer lors de la création d'un producteur. Le chemin UI conserve
`enqueue`, qui peut allouer et n'est pas présenté comme temps réel. Les 4096
places sont une réserve initiale, pas une limite globale après les enqueues UI.

Si aucun producteur préparé ne peut accepter l'événement, un slot atomique
64 bits conserve ensemble sa présence, son numéro d'ordre et les bits du float
issu de la conversion ValueBridge existante. Les valeurs UI ordinaires restent
en précision mopo native dans la file. Le consommateur audio draine celle-ci
au point existant, puis les secours si le drapeau global le demande. Les
atomiques utilisées par le transport doivent être toujours lock-free sur la
cible, faute de quoi la compilation est refusée plutôt que d'utiliser un
verrou caché. Les réservations n'attendent pas; les CAS du secours et les
opérations de ConcurrentQueue restent non wait-free sous contention.

Les numéros d'ordre sont propres à chaque contrôle, sur 31 bits avec comparaison
modulaire; moins de 2^30 changements peuvent rester non consommés pour un même
contrôle. Un ancien paquet arrivé tardivement ne remplace pas une valeur plus
récente. Comme ConcurrentQueue, le transport ne promet pas d'ordre total entre
producteurs sur des paramètres distincts. En saturation ou contention des huit
producteurs, les valeurs hôte intermédiaires du secours peuvent être fusionnées;
la dernière demande reste représentée pour la prochaine consommation, sans
blocage ni abandon silencieux. Ce n'est pas une nouvelle garantie d'automation
sample-accurate. L'ordre est défini par la réservation de séquence pour chaque
contrôle, pas par l'heure murale de callbacks concurrents.

Cette réduction des allocations ne certifie pas le traitement complet : les
queues de modulation, le routage, la sérialisation, les notifications JUCE et
la première utilisation du stockage local au thread doivent conserver leurs
audits et mesures propres. Aucun changement de structure DSP n'est introduit.

Le traitement exact des changements, de l’état sérialisé et des migrations est détaillé dans @ref state_and_parameters. Les chemins MIDI sont décrits dans @ref midi_input.

### Déclaration des queues à l'hôte (21.4)

`HelmPlugin::getTailLengthSeconds` consulte un ensemble fixe de pointeurs vers
les snapshots atomiques, résolus à la construction. Le masque des destinations
de modulation pertinentes est publié lors des modifications du routage, sous
sa protection déjà existante; aucun parcours de route ni lecture DSP n'est
effectué par la requête hôte. Les paramètres se lisent indépendamment : ce n'est
pas une transaction globale de preset, et les transitions doivent être testées.

`TailLength` estime une décroissance relative d'amplitude à 10^-5 (-100 dB),
pas une durée exacte jusqu'au silence numérique. L'enveloppe utilise le carré
du release et la table `SampleDecayLookup`. Chorus utilise les bornes actuelles
de délai (111 ms) et feedback (0,95), avec la marge de lissage existante.
Le délai utilise sa capacité physique partagée `mopo::MAX_DELAY_SAMPLES`
(300000, inchangée), le module de son feedback et une marge de stabilisation
du passe-bas Butterworth au cutoff minimal de 20 Hz. Réverbération utilise les
constantes de la version JUCE embarquée : plus long comb stéréo, longueurs
all-pass, feedback 0,7 + 0,28 * roomSize et lissage. Les durées des étapes
audibles en série sont additionnées. Ces approximations doivent être vérifiées
par rendu, particulièrement aux limites et après une mise à jour JUCE/DSP.

Bypass et mix nul excluent les effets inactifs du nouveau calcul, sauf si leurs
destinations sont modulées. Chorus reste borné par son clamp de feedback.
Feedback délai de module >= 1, Freeze audible, modulation du release ou du
feedback délai rendent l'estimation non bornée. Une modulation du feedback
Reverb prend sa borne JUCE normale maximale, sans la confondre avec Freeze.
Le modèle utilise la capacité maximale du délai indépendamment du tempo et de
la durée demandée : il évite une sous-estimation lors de synchronisations et
modulations temporelles, mais peut annoncer beaucoup plus que la queue audible.

Le callback audio garde un historique atomique de l'estimation précédente et
le fait vieillir du temps effectivement traité; la requête prend le maximum
entre cette valeur et les nouveaux réglages. Ainsi, réduire le mix ou bypasser
un effet ne supprime pas instantanément sa durée annoncée. Sortir d'un réglage
infini reprend la durée finie courante complète; c'est une estimation, pas une
analyse du contenu des buffers. Le reset hôte efface l'historique, mais les
réglages courants restent déclarés pour les prochaines notes. Sans nouveaux
blocs, l'historique ne vieillit pas : vérifier la suspension et la réactivation
dans chaque hôte. Aucun détecteur de silence DSP ni timer de polling n'est ajouté.

Le wrapper VST3 JUCE 9.0.5 reconnaît `std::isinf` et renvoie `kInfiniteTail`.
Les wrappers AU/AUv3 transmettent directement la durée; les autres conversions
et conventions ne sont pas certifiées ici. Seul VST3 reçoit donc l'infini natif.
Pour les wrappers non vérifiés, et pour les durées finies trop grandes, le
plafond de conversion est (2^30 - 1) / fréquence d'échantillonnage; VST3 reçoit
alors son infini reconnu, les autres une durée finie de repli. La marge protège
les conversions signées/arrondies, mais ce plafond n'est pas une borne acoustique
de Freeze et ne garantit pas qu'un format exploite la déclaration de queue.
La latence de traitement reste inchangée. Un futur wrapper CLAP devra choisir
explicitement son adaptation plutôt que convertir directement un double infini.

Cette politique peut prolonger les exports automatiques et maintenir le plugin
actif plus longtemps, notamment avec un délai court mais un feedback élevé.
Freeze peut légitimement produire une queue infinie; utiliser une longueur
d'export explicite ou désactiver Freeze. Ne pas annoncer une validation des
exports ni une borne mathématique de toute modulation à partir de ces seules
formules. Les vérifications hôte/rendu restent indispensables.

## Initialisation et portée des garanties

Le graphe global d’effets est construit par `HelmBoyEngine::init`. Le plug-in initialise aussi les modules pendant la construction de son moteur; le standalone peut différer l’initialisation de certains modules jusqu’au préflight de démarrage. Cette différence concerne la séquence de lancement, pas l’ordre de la chaîne sonore.

Cette page décrit l’architecture lue dans le code. Elle ne certifie ni la compatibilité de chaque hôte/formats, ni la stabilité en temps réel, ni le comportement audible pour toutes les configurations. Les tests disponibles et leurs options sont recensés dans @ref development_and_tests.
