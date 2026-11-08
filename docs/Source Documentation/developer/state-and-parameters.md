# Paramètres et état

@page state_and_parameters Paramètres et état

## Définition et exposition des paramètres

Les définitions centrales sont dans `mopo::Parameters`. Chaque `ValueDetails`
associe un nom de contrôle à sa plage et à sa valeur par défaut. Les modules
créent les valeurs DSP avec `HelmBoyModule`; `SynthBase::controls_` expose la
table commune aux widgets, à la sauvegarde et au pont de paramètres du plug-in.

`HelmPlugin` crée un `ValueBridge` pour chaque contrôle et l’enregistre auprès
de JUCE. Le pont convertit entre l’échelle normalisée de l’hôte et l’échelle
interne décrite par les métadonnées du paramètre. Il notifie l’hôte des gestes
et changements issus de l’interface. Depuis le point 21.5, chaque contrôle
utilise l’ID explicite stable `helmboy_` suivi de son nom mopo; les wrappers
peuvent dériver leur identifiant ou symbole de format de cet ID. L’ordre
d’enregistrement, les plages et les conversions restent inchangés. L’utilisateur
confirme l’absence d’historique d’utilisation antérieur : aucune migration
d’anciens projets n’est requise. La validation porte sur l’unicité des IDs
après adaptation par wrapper et le rappel stable des nouveaux projets sauvegardés.

## Changements de valeur

- Un changement GUI utilise `SynthBase::valueChanged` et entre dans la file
  concurrente de contrôles; `processControlChanges` applique les valeurs au
  graphe pendant le traitement audio.
- Une modification de l’hôte passe par `HelmPlugin::parameterChanged`, puis
  `valueChangedExternal`; la valeur est mise en file pour le moteur et une
  notification GUI est préparée.
- Une valeur de modulation est un état de connexion distinct. Le changement
  est mis en file, puis `processModulationChanges` met à jour l’intensité et
  connecte/déconnecte la route dans `HelmBoyEngine`.
- Les gestes GUI appellent `beginChangeGesture`/`endChangeGesture`; ce sont des
  notifications d’automation, pas le calcul DSP lui-même.

Les files évitent de faire muter directement le graphe à partir de tous les
contextes appelants, mais cela ne signifie pas que toute l’interface de
`SynthBase` est lock-free. La sérialisation et les mutations d’état utilisent
également les sections critiques fournies par chaque hôte.

## Format des patches

Les contrôles de décalage de phase ajoutés pour OSC 1/2, Sub et les huit LFO
ont des identifiants `*_phase_offset`, une plage de `-180..180` degrés et une
valeur par défaut de `0`. Les anciens patches qui ne contiennent pas ces clés
conservent ainsi leur phase existante. Les nouveaux paramètres hôte sont
enregistrés après la séquence historique afin de préserver les indices déjà
publiés.

Les indices des formes LFO existantes restent inchangés : S&H vaut 11 et S&G
vaut 12; Smooth Random est ajouté à l’indice 13. Les trois formes aléatoires
utilisent un état PRNG local par instance et avancent selon les cycles franchis,
indépendamment du découpage en blocs. La nouvelle séquence aléatoire n’est pas
identique aux anciennes séquences issues de `std::rand()`.

Les formes d’oscillateur conservent leurs index bruts 0 à 10; Saw Möbius est
ajouté à l’index 11 pour Osc 1/2 et Sub. Les valeurs des presets JSON existants
restent donc inchangées. En revanche, `ValueBridge` normalise les automations
hôte à partir des bornes du paramètre : l’extension de 0..10 à 0..11 change
cette correspondance normalisée pour ces contrôles. La compatibilité des
automations existantes doit être vérifiée dans les hôtes concernés.

S&H conserve sa valeur sur les 90 premiers pour cent du cycle puis utilise un
fondu cosinus vers la valeur suivante sur les 10 derniers pour cent. Cette
valeur suivante est pré-tirée afin que le signal soit continu au wrap. S&G
raccorde sa valeur cible précédente à la suivante; Smooth Random reporte le
dernier point du cycle comme premier point du cycle suivant. Les points exacts
utilisés sont publiés au viewer via la télémétrie UI dédiée, sans les ajouter
aux sources de modulation.

La suite facultative `HELMBOY_BUILD_LFO_WAVEFORM_TESTS` couvre les indices,
les plages et valeurs par défaut de phase, les interpolations et la stabilité
des sorties aléatoires selon la taille de bloc. Elle nécessite également
`HELMBOY_RUN_TESTS=ON`.

Un patch est un objet JSON versionné. Les métadonnées de premier niveau
comprennent notamment `synth_version`, `patch_name`, `folder_name`, `author` et
`license`. L’objet `settings` contient les valeurs nommées des contrôles et la
liste `modulations`, chaque route décrivant sa source, sa destination et son
intensité. Les champs absents ne signifient pas qu’un contrôle est invalide :
le chargeur fournit sa valeur par défaut déclarée.

Les paramètres du limiteur sont `limiter_ceiling` (défaut `0`, plage `-12..0`
dB) et `limiter_release` (défaut `100`, plage `5..500` ms); `limiter_on` reste
son interrupteur indépendant. Les 1?220 patches suivis contiennent ces deux
clés à leurs valeurs par défaut, placées alphabétiquement autour de
`limiter_on`. Leur ajout ne modifie pas les autres valeurs ni les modulations.

Le paramètre `pan` va de `-1` (gauche) à `+1` (droite), avec `0` au centre.
Son défaut `0` figure dans les 1?220 patches, entre `osc_mix` et
`pitch_bend_range` dans `settings`.

Le chargement suit deux étapes :

1. `LoadSave::prepareState` valide et transforme une copie de l’objet avant la
section critique. Il traite les anciennes enveloppes de format, les aliases de
contrôles, les migrations de routes, les nombres non finis, les contrôles
inconnus et les valeurs hors plage.
2. `LoadSave::applyPreparedState` applique les valeurs reconnues, reconstruit
les connexions de modulation et restaure les métadonnées. Cette fonction ne
prend pas elle-même le verrou; son appelant protège les mutations. `SynthBase`
prépare d’abord l’état, puis applique le résultat dans sa section critique.

Des exemples de migrations versionnées dans l’implémentation convertissent les
anciens patches (par exemple le format antérieur à 0.4.1, le mélangeur ajouté
après 0.5.0 et les changements de niveau/unisson jusqu’à 0.8.6). Ajouter une
migration seulement pour une rupture réelle de schéma et conserver une valeur
par défaut sûre pour les nouveaux contrôles.

## Presets et configuration

`LoadSave` gère séparément l’état d’un patch et la configuration locale : racine
de patches personnalisée, mappings MIDI, périphériques, taille de fenêtre et
préférences. Le navigateur de patches est une vue de ces fichiers et banques;
les programmes hôte du plug-in ont leur propre liste chargée depuis les patches
disponibles. Ne pas confondre un numéro de programme d’hôte avec un index global
permanent du format JSON.

Les points d’entrée à consulter avant de modifier ce flux sont
`SynthBase::loadFromVar`, `HelmPlugin::getStateInformation`,
`HelmPlugin::setStateInformation`, `LoadSave::stateToVar`,
`LoadSave::prepareState` et `LoadSave::applyPreparedState`.
