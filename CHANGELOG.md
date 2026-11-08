# Changelog

## English

Notable changes to HelmBoy are listed here, based on the Git history. Entries
are grouped by version and feature.

## [1.1.2] - 2026-11-06

### Completed Features (2026-11-06)

- Expanded modulation destinations to include tempo divisions and synchronization
  modes for frequency controls, plus retrigger modes for LFOs and the step
  sequencer. Discrete mode selections are rounded before routing so modulation
  can select each available mode. Added polyphonic Legato modulation support.
- Improved modulation feedback across controls: destination highlighting is
  consistent for switches and buttons, modulated buttons show a translucent
  green active-state overlay, and tempo/retrigger selectors display their
  effective modulated modes. Corrected modulation meter sizing and scaling for
  frequency controls, and fixed the doubled right edge on section switches.
- Updated the volume context menu to list its active modulation routes and
  allow individual or complete disconnection.
- The project owner confirms successful compilation and behavioural validation
  of these modulation changes (2026-11-06).

### Completed Features (2026-11-04)

- Implemented host tail reporting (21.4) from atomic parameter snapshots and
  relevant modulation flags, with a documented -100 dB release/effect model,
  retained previous finite estimates and explicit non-decaying cases. VST3
  receives its verified infinite-tail convention; other wrappers use a safe
  finite sample-count ceiling. DSP behavior, parameters and latency are unchanged.
  Added formula and plugin snapshot tests. Conservative reports may extend
  automatic exports. The project owner confirms successful Windows compilation
  and Windows standalone tests (2026-11-04). Automated tests, other platforms
  and host/render validation remain unconfirmed.
- Implemented the JUCE host reset contract (21.3): serialized plugin reset
  immediately retires voices, clears arp scheduling and effect tails, and
  cancels an active patch fade without changing parameters, routes, preset
  identity or queued automation. Existing processors and voice reserves are
  reused; arp velocity storage now retains its allocation during reset.
  Keyboard events are invalidated immediately with message-thread cleanup.
  Overlapping audio callbacks use a pending boundary reset without waiting.
  Added focused host reset tests. The project owner confirms successful Windows
  compilation and standalone tests (2026-11-04). Automated HostResetTests,
  allocation checks, other platforms and plugin/host lifecycle validation remain
  unconfirmed; standalone tests do not validate the host reset callback itself.
- Implemented automation notification improvements (21.2): stable parameter
  names, thread-local feedback guards, atomic snapshot reads and prepared
  explicit queue producers for host changes. Per-control sequences prevent
  stale packets from reverting newer values; atomic overflow retains the last
  host value, with intermediate updates coalesced only on saturation/contention.
  GUI delivery and Undo/Redo bookkeeping stay outside host callbacks. Added
  focused notification/concurrency tests. The project owner confirms successful
  Windows compilation and Windows standalone behaviour tests (2026-11-04).
  Owner-supplied CTest output also confirms 50/50 consecutive notification-test
  passes in Windows Release, with zero failures and 10.40 s total wall time.
  Plugin/host validation, other platforms and runtime allocation/contention
  measurements remain unconfirmed; test-process time is not audio callback
  latency or a whole-engine real-time safety guarantee.
- Corrected host parameter text conversion (21.1): format the supplied
  normalized value without reading DSP state, expose declared units and
  existing choice labels, and invert the display transforms on text entry.
  Invalid text returns the normalized default; valid values are clamped and
  stepped without changing automation mappings or parameter definitions.
  Numeric entry shares metadata-only conversions with the GUI. Added focused
  parameter-text tests. The project owner confirms successful compilation and
  tests for 21.1 (2026-11-04); the platform, host and test matrix was not specified.
- Removed the unused legacy mopo comb/all-pass reverb, its helper sources and
  build switch. Reverb now always uses JUCE, retaining the existing JUCE
  parameter mappings, stereo handling, wet/dry mix and reset behaviour.
  Replaced the legacy-only test with a JUCE-reference comparison across sample
  rates, block sizes, control changes and reset. Build and runtime validation
  of this removal remain pending owner verification.
- Completed section 2 (Patch Switching) with an effect reset transition for
  Init and patch changes: a 10 ms output fade-out, effect-state reset at a
  silent block boundary, then a 10 ms fade-in. The reset targets the active
  chorus, delay, DC filters, reverb and limiter through the existing engine
  and processor reset hooks. The project owner confirms successful Windows
  compilation and behavioural tests for Init and patch changes (2026-11-04).
- Simplified standalone startup (section 17): removed exponential startup
  retries and the forced watchdog while retaining peer/audio-device checks,
  saved-device fallback and callback lifetime protections. GUI failures are
  reported without closing the application; headless failures exit cleanly.
  The project owner reports successful runtime validation, including device
  fallback, slow initialization and shutdown during startup (2026-11-04).
- Added a fixed 256-step linear Undo/Redo history for UI parameter edits. Each
  completed gesture stores the parameter index and its start/end values;
  composite controls aggregate changes per parameter. Undo/Redo availability
  follows the history cursor, new edits discard the redo tail, and history is
  cleared when a patch is loaded or Init is restored. Replay uses the existing
  parameter and host-notification path; history bookkeeping stays outside the
  audio thread. Added focused tests for ordering, boundaries, redo truncation,
  reset and capacity.
- Replaced the patch selector's Save, Export and Browse labels with embedded
  SVG icons, and added Save As, Undo and Redo controls plus an Edit state icon.
  Save confirms before overwriting the active patch; Save As reuses the existing
  save dialog. The six actions span the existing selector width with adaptive
  equal button widths and unchanged bar height. Undo/Redo controls reflect the
  section 14 history state.
- The project owner reports successful compilation and usage testing for
  sections 14 and 20 (2026-11-04).

### Completed Features (2026-11-03)

- Added oscillator, Sub and LFO phase controls from -180° to +180°, the
  SuperSaw oscillator waveform, and the Smooth Random LFO waveform. Waveform
  previews now reflect phase and use the same random-waveform model as the DSP.
- Added Init reset and A/B comparison controls. Each A/B slot stores its own
  synthesizer parameter snapshot and modulation routes; Init restores defaults
  for the active slot without changing the other slot.
- Added Mod Envelopes 2–4 as DAHDSR modulation sources with the existing
  envelope editing and routing behaviour. Reorganised the envelope layout and
  added defaults for the 18 new parameters to all 1,322 tracked presets.
- Added a continuous Noise Color control spanning Brown, Pink, White, Blue and
  Violet noise, with smoothed spectral transitions and an exact neutral default
  that preserves legacy noise output. Integrated modulation and automation,
  updated all 1,322 bundled patches, documented the model and added spectral and
  neutral-compatibility tests.
- Extended synchronized LFO divisions from 64/1 through 1/128. Existing LFO
  divisions retain their ordering and are remapped in bundled presets and
  migrated when loading older presets; the normalized host-automation mapping
  change was explicitly accepted.
- Added a modulatable and automatable Delay LP Cutoff with a fully-open default
  that preserves the legacy delay path. A cascaded fourth-order Butterworth
  low-pass filters the delay tap and feedback path, with coefficient smoothing
  over 10 ms; focused tests cover open-path equivalence, attenuation, dry signal
  preservation and cutoff/sample-rate stability.
- Expanded bank export to multiple selected folders and presets while retaining
  hierarchy. Import supports `.helm`, `.helmboy`, `.helmbank` and
  `.helmBoyBank`; legacy conversion reuses active defaults and metadata rules,
  while versioned migrations remain centralized in `LoadSave::prepareState`.
  Focused tests cover conversion, deterministic export, conflicts, unsafe paths
  and export/import round trips.
- Added modulation-aware waveform target feedback and effective waveform
  previews for Osc, Sub and LFO selectors. Target geometry follows the
  selector's rows and coordinate system while preserving Osc/Sub rounding and
  LFO truncation. The user confirmed compilation and standalone/VST3 usage
  tests for this feature.
- Separated oscillator and LFO waveform selector families, label tables and
  shape generation, with explicit family-specific mappings. Added Saw Möbius
  for oscillator and Sub using `y = sin(2*pi*phi + m*sin(4*pi*phi))`, where
  `m = 2*phi - 1`, through a band-limited lookup. Its selector and viewer use
  matching phase and phase-stretch behavior, and waveform-selection modulation
  uses the existing path. Existing waveform indices and raw preset values are
  preserved; extending the oscillator range changes normalized host-automation
  mapping. The user confirmed compilation and tests.
- Replaced the modulation-source bitmap with directly drawn JUCE CPU circles.
  The filled disk reflects whether the source has active routes, the outline
  uses the established modulation color, and the central symbol and interactions
  are preserved. The user confirmed compilation and tests.

### UI and Compatibility Changes (2026-11-03)

- Envelope and filter-envelope displays now show the effective modulated
  contour while retaining the base contour and live position marker. The
  filter response graph follows effective cutoff and resonance, and the formant
  pad shows its effective position using thread-safe telemetry at 30 Hz.
- Enabled bipolar modulation for eligible destinations and updated 46 legacy
  Poly LFO destination aliases in 43 presets without changing their modulation
  amounts.
- Completed the CPU rendering migration and removed the unused GPU-renderer
  components, shader sources, build definitions and graphics-library links.

### Validation Report (2026-11-03)

- The user confirmed Windows and Linux compilation and standalone/VST3
  validation for all completed sections except the cancelled Patch Switching
  section, in Ableton Live, Cubase, Bitwig and Tracktion (2026-11-03).

## [1.1.1] - 2026-10-28

### Added

- Load a preset by double-clicking it in the patch browser; the selection
  window closes after loading.
- New presets: Large Pad and Agressive Organ.

### Changed

- Corrected patch names.
- Optimized linking and code generation on MSVC with `/LTCG`.

## [1.1.0] - 2026-10-20

### Added - 2026-10-20

- Modulateable hard sync between oscillators and independent feedback paths
  for each oscillator.
- AHDSR and DAHDSR envelope stages.
- Notch and Comb filters, separate aftertouch controls, and LFOs and a
  sequencer extended to 128 steps.
- Stereo chorus, stereo effect parameters, stereo width and Freeze mode for
  reverb, and a stereo limiter.
- Direct entry of modulation values and improved modulation displays,
  including oscillator and LFO phase stretch.
- Audio device persistence and MIDI profile save/load in the standalone
  application.
- Many factory presets, as well as imported and converted presets.
- Linux DEB and RPM packages, with presets distributed separately. Linux and
  macOS build workflows were also added; the macOS workflow had not yet been
  validated according to the history.

### Changed - 2026-10-20

- Biquad and SVF filters migrated to JUCE DSP paths; JUCE reverb integrated
  with support for stereo parameters.
- Optimized standalone startup and headless mode.
- Modernized JUCE APIs and adapted build configurations for Linux, LV2, AU,
  and AUv3.

### Fixed

- Freezes when switching presets and issues loading legacy presets.
- Standalone control synchronization and some modulation targets.
- Linux/LV2 builds with GCC and LTO, along with several compilation and
  integration issues.

## Français

Les changements notables de HelmBoy sont répertoriés ici, d’après l’historique
Git. Les entrées regroupent les commits par version et par fonctionnalité.

## [1.1.2] - 2026-11-06 - FR

### Fonctionnalités terminées (2026-11-06)

- Extension des destinations de modulation aux divisions de tempo et aux modes
  de synchronisation des contrôles de fréquence, ainsi qu'aux modes de retrigger
  des LFO et du séquenceur. Les choix discrets sont arrondis avant routage afin
  que la modulation puisse sélectionner chacun des modes disponibles. Ajout
  de la modulation polyphonique de Legato.
- Amélioration du retour visuel de modulation : surlignage cohérent des switches
  et boutons, voile vert translucide sur les boutons activés par modulation et
  affichage des modes effectifs modulés dans les sélecteurs de tempo/retrigger.
  Correction des dimensions et de l'échelle des compteurs sur les contrôles de
  fréquence, ainsi que du bord droit doublé des switches de section.
- Mise à jour du menu contextuel du volume pour afficher ses routes de
  modulation actives et permettre leur déconnexion individuelle ou groupée.
- Le propriétaire confirme la réussite de la compilation et la validation du
  comportement de ces modifications de modulation (2026-11-06).

### Fonctionnalités terminées (2026-11-04)

- Implémentation de la déclaration des queues hôte (21.4) à partir des snapshots
  atomiques et des destinations de modulation pertinentes, avec modèle documenté
  à -100 dB, maintien des estimations finies précédentes et cas non décroissants.
  VST3 reçoit sa convention d'infini vérifiée; les autres wrappers un plafond
  fini de conversion sûr. DSP, paramètres et latence restent inchangés. Ajout
  de tests du calcul et des snapshots plugin. Les estimations prudentes peuvent
  prolonger les exports. Le propriétaire confirme la compilation Windows et les
  tests standalone Windows (2026-11-04). Tests automatisés, autres plateformes
  et essais hôte/rendu restent à confirmer.
- Implémentation du reset hôte JUCE (21.3) : arrêt immédiat des voix, de
  l'arpégiateur et des queues d'effets pour les appels plugin sérialisés, avec
  annulation du fondu de patch sans modifier paramètres, routes, identité du
  preset ni automation en attente. Les processeurs et réserves de voix sont
  réutilisés; le stockage des vélocités arp conserve sa réserve au reset.
  Les événements clavier sont invalidés immédiatement puis nettoyés sur le
  message thread. Un callback audio concurrent reprend la demande à sa frontière
  sans attente. Ajout de tests ciblés. Le propriétaire confirme la compilation
  Windows et les tests standalone (2026-11-04). Les HostResetTests automatisés,
  contrôles d'allocations, autres plateformes et essais du cycle de vie plugin/hôte
  restent à confirmer; le standalone ne valide pas le callback de reset hôte.
- Implémentation des notifications d'automation (21.2) : noms stables, gardes
  de récursion locales au thread, lecture du snapshot atomique et producteurs
  explicites préparés pour les changements hôte. Les séquences par contrôle
  empêchent les anciens messages d'annuler une valeur plus récente; le secours
  atomique conserve la dernière valeur hôte, avec coalescence des intermédiaires
  uniquement en saturation/contention. La livraison GUI et l'historique restent
  hors callbacks hôte. Ajout de tests ciblés de notifications et de concurrence.
  Le propriétaire confirme la compilation Windows et les tests de comportement
  standalone sous Windows (2026-11-04). La sortie CTest fournie confirme aussi
  50/50 exécutions consécutives du test de notifications sous Windows Release,
  sans échec, en 10,40 s au total. Validation plugin/hôte, autres plateformes et
  mesures runtime d'allocations/contention restent à confirmer; ce temps de
  processus de test ne mesure pas la latence des callbacks audio et ne certifie
  pas la sûreté temps réel globale du moteur.
- Correction des conversions texte des paramètres hôte (21.1) : formatage de
  l'argument normalisé sans lecture DSP, unités déclarées et libellés existants,
  avec transformation inverse à la saisie. Le texte invalide renvoie la valeur
  normalisée par défaut; les valeurs valides sont bornées et arrondies selon
  les étapes, sans modification des mappings d'automation ni des définitions
  de paramètres. La saisie numérique partage ses conversions avec la GUI.
  Ajout de tests ciblés. Le propriétaire confirme la compilation et les tests
  du point 21.1 (2026-11-04), sans préciser la matrice de plateformes, d'hôtes
  et de tests couverte.
- Suppression de la réverbération mopo legacy comb/all-pass inutilisée, de ses
  fichiers auxiliaires et de son option de compilation. Reverb utilise
  désormais toujours JUCE, avec les mêmes correspondances de paramètres,
  traitement stéréo, mélange Wet/Dry et reset que le chemin JUCE existant.
  Remplacement du test legacy par une comparaison à JUCE selon la fréquence,
  la taille de bloc, les changements de contrôles et le reset. La compilation
  et la validation runtime de cette suppression restent à vérifier par le
  propriétaire du projet.
- Clôture du point 2 (Patch Switching) avec une transition de reset des effets
  pour Init et les changements de patchs : fondu de sortie de 10 ms, reset des
  états à une frontière de bloc silencieuse, puis fondu de retour de 10 ms.
  Le reset cible les instances actives du chorus, du délai, des filtres DC,
  de la réverbération et du limiteur via les mécanismes existants du moteur
  et des processeurs. Le propriétaire confirme la compilation Windows et les
  tests de comportement sur Init et les changements de patchs (2026-11-04).
- Simplification du démarrage standalone (section 17) : suppression des
  tentatives exponentielles et du watchdog forcé, avec conservation des
  contrôles de fenêtre native et de périphérique audio, du repli sur la
  configuration audio et des protections de durée de vie des callbacks.
  Les erreurs GUI sont signalées sans fermer l’application; les erreurs
  headless provoquent une sortie propre. Le propriétaire confirme les tests
  de fonctionnement, notamment le repli audio, l’initialisation lente et la
  fermeture pendant le démarrage (2026-11-04).
- Ajout d’un historique linéaire Undo/Redo de 256 étapes pour les modifications
  de paramètres faites dans l’interface. Chaque geste terminé conserve
  l’index du paramètre et ses valeurs initiale/finale; les contrôles composites
  agrègent les changements par paramètre. L’état actif des boutons suit le
  curseur, une nouvelle modification supprime la branche Redo et l’historique
  est vidé au chargement d’un patch ou au retour à Init. Le replay réutilise le
  chemin existant de paramètres et de notification hôte; la gestion de
  l’historique reste hors du thread audio. Des tests ciblés couvrent l’ordre,
  les bornes, la branche Redo, le reset et la capacité.
- Remplacement des libellés Save, Export et Browse du sélecteur par des icônes
  SVG intégrées; ajout de Save As, Undo et Redo, avec une icône Edit pour l’état
  du navigateur. Save confirme avant d’écraser le patch actif; Save As réutilise
  la boîte de sauvegarde existante. Les six actions occupent toute la largeur
  du sélecteur, avec des largeurs égales adaptatives et une hauteur de barre
  inchangée. L’état des boutons Undo/Redo suit l’historique de la section 14.
- Le propriétaire confirme la compilation et les tests d’utilisation des
  sections 14 et 20 (2026-11-04).

### Fonctionnalités terminées (2026-11-03)

- Ajout de commandes de phase de -180° à +180° pour les oscillateurs, le Sub et
  les LFO, de la forme SuperSaw pour l’oscillateur et de Smooth Random pour le
  LFO. Les aperçus reflètent désormais la phase et reprennent le modèle DSP des
  formes aléatoires.
- Ajout des commandes Init et A/B. Chaque état conserve son propre instantané
  des paramètres de synthèse et de ses routes de modulation; Init restaure les
  valeurs par défaut de l’état actif sans modifier l’autre.
- Ajout des enveloppes de modulation 2 à 4, de type DAHDSR, avec le même éditeur
  et le même routage que l’enveloppe existante. Réorganisation de leur
  disposition et ajout des valeurs par défaut des 18 nouveaux paramètres dans
  les 1 322 presets suivis.
- Ajout d’un contrôle continu Noise Color couvrant les bruits brun, rose, blanc,
  bleu et violet, avec transitions spectrales lissées et une valeur neutre qui
  préserve exactement le bruit historique. Intégration à la modulation et à
  l’automation, mise à jour des 1 322 presets suivis, documentation du modèle et
  tests de compatibilité neutre et de réponse spectrale.
- Extension des divisions LFO synchronisées de 64/1 à 1/128. L’ordre des
  divisions existantes est conservé; les presets suivis sont remappés et les
  anciens presets migrés au chargement. La modification de la correspondance
  normalisée de l’automation hôte a été acceptée.
- Ajout d’un cutoff Delay LP modulable et automatisable, ouvert par défaut afin
  de préserver le chemin historique du délai. Un passe-bas Butterworth du
  quatrième ordre filtre le tap et le feedback; ses coefficients sont lissés
  sur 10 ms. Les tests ciblés couvrent l’équivalence à pleine ouverture,
  l’atténuation, la préservation du signal sec et la stabilité selon cutoff et
  fréquence d’échantillonnage.
- Extension de l’export aux dossiers et presets sélectionnés multiples, avec
  conservation de la hiérarchie. L’import prend en charge `.helm`, `.helmboy`,
  `.helmbank` et `.helmBoyBank`; la conversion historique réutilise les valeurs
  par défaut et règles de métadonnées actives, et les migrations versionnées
  restent centralisées dans `LoadSave::prepareState`. Les tests ciblés couvrent
  conversion, export déterministe, conflits, chemins dangereux et aller-retour.
- Ajout de l’indicateur de cible de modulation et des aperçus waveform effectifs
  pour les sélecteurs Osc, Sub et LFO. La cible suit les rangées et le repère du
  sélecteur, en conservant l’arrondi Osc/Sub et la troncature LFO. L’utilisateur
  confirme la compilation et les tests d’utilisation en standalone/VST3.
- Séparation des familles de formes d’onde Osc/LFO, de leurs tables de libellés
  et de la génération des tracés, avec des correspondances explicites par
  famille. Ajout de Saw Möbius pour l’oscillateur et le Sub selon
  `y = sin(2*pi*phi + m*sin(4*pi*phi))`, avec `m = 2*phi - 1`, au moyen d’une
  table band-limitée. Le sélecteur et le viewer appliquent les mêmes phase et
  phase stretch; la modulation de sélection conserve le chemin existant. Les
  index de formes d’onde et valeurs brutes des presets sont préservés; l’extension
  de la plage de l’oscillateur modifie la correspondance normalisée de
  l’automation hôte. L’utilisateur confirme la compilation et les tests.
- Remplacement de l’image bitmap des sources de modulation par des cercles JUCE
  CPU dessinés directement. Le disque plein reflète l’usage de la source, le
  contour reprend la couleur de modulation existante, et le symbole central
  ainsi que les interactions sont conservés. L’utilisateur confirme la
  compilation et les tests.

### Modifications d’interface et de compatibilité (2026-11-03)

- Les enveloppes et le filtre affichent maintenant leur contour modulé effectif
  tout en conservant le contour de base et le marqueur de position en direct.
  La réponse du filtre suit cutoff et résonance effectifs; le pad formant montre
  sa position effective à partir de télémétrie thread-safe, à 30 Hz.
- Activation de la modulation bipolaire pour les destinations compatibles et
  mise à jour de 46 anciens alias de destinations Poly LFO dans 43 presets, sans
  modifier les quantités de modulation.
- Finalisation de la migration vers le rendu CPU et suppression des composants
  de rendu GPU inutilisés, des sources de shaders, des définitions de build et
  des liens vers les bibliothèques graphiques associées.

### Rapport de validation (2026-11-03)

- L’utilisateur confirme la compilation Windows et Linux ainsi que la validation
  standalone et VST3 de toutes les sections terminées, à l’exception de la
  section Patch Switching annulée, dans Ableton Live, Cubase, Bitwig et
  Tracktion.

## [1.1.1] - 2026-10-28 - FR

### Ajouté - 2026-10-28

- Chargement d’un preset par double-clic dans le navigateur de patches, avec
  fermeture de la fenêtre de sélection.
- Nouveaux presets : Large Pad et Agressive Organ.

### Modifié

- Correction de noms de patches.
- Optimisation de l’édition de liens et de la génération de code sous MSVC avec
  `/LTCG`.

## [1.1.0] - 2026-10-20 - FR

### Ajouté

- Synchronisation dure modulable entre oscillateurs et chemins de feedback
  indépendants pour chaque oscillateur.
- Phases AHDSR et DAHDSR pour les enveloppes.
- Filtres Notch et Comb, aftertouch différenciés, et LFO et séquenceur étendus
  jusqu’à 128 pas.
- Chorus stéréo, paramètres d’effets stéréo, largeur stéréo et mode Freeze pour
  la réverbération, ainsi qu’un limiteur stéréo.
- Saisie directe des valeurs de modulation et améliorations de leur affichage,
  notamment pour le phase stretch des oscillateurs et des LFO.
- Mémorisation des périphériques audio et sauvegarde/chargement des profils
  MIDI dans l’application autonome.
- Nombreux presets d’usine et presets importés ou convertis.
- Paquets Linux DEB et RPM, avec les presets distribués séparément. Des workflows
  de génération pour Linux et macOS ont également été ajoutés ; le workflow
  macOS n’était pas encore validé dans l’historique.

### Modifié - 2026-10-20

- Filtres biquad et SVF migrés vers les chemins DSP JUCE ; réverbération JUCE
  intégrée avec prise en charge des paramètres stéréo.
- Démarrage autonome et mode sans interface optimisés.
- Modernisation des API JUCE et adaptation des configurations de build pour
  Linux, LV2, AU et AUv3.

### Corrigé

- Blocages lors du changement de preset et problèmes de chargement des anciens
  presets.
- Synchronisation des contrôles autonomes et de certaines cibles de modulation.
- Compatibilité des builds Linux/LV2 avec GCC et LTO, ainsi que plusieurs
  problèmes de compilation et d’intégration.
