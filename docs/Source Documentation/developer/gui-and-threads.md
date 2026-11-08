# Interface et frontières de threads

@page gui_and_threads Interface, visualisations et threads

## Composition de l’interface

`HelmEditor` (plug-in) et `HelmBoyEditor` (standalone) fournissent le contexte
JUCE et implémentent `SynthGuiInterface`. `FullInterface` assemble les sections
de synthèse, le navigateur de patches, les contrôles globaux, les visualisations
et le gestionnaire de modulation. `SynthesisInterface` compose les sections
fonctionnelles; les composants réutilisables (slider, clavier, enveloppe,
oscilloscope, compteurs) vivent dans `src/editor_components`.

Les contrôles d’interface correspondent aux noms du registre moteur. Le
gestionnaire de modulation crée les commandes et compteurs de modulation à
partir des sources et destinations exposées par `HelmBoyModule`.
Le rendu des visualisations est effectué par JUCE `Graphics` sur le thread
message. Les contrôles et les composants de visualisation utilisent les
chemins CPU JUCE.

## Transport des changements vers l’interface

`SynthBase` reçoit plusieurs catégories de données avec des durées de vie et
des rythmes différents :

- Les changements de paramètres issus du traitement audio sont publiés dans
  une table de contrôles préallouée contenant valeur atomique et indicateur
  `dirty`. Un timer JUCE sur le thread message délivre les notifications GUI
  toutes les 30 ms environ; les composants ne sont pas mutés depuis la boucle
  audio.
- `captureGuiStateSnapshot` copie valeurs et routes de modulation sous la
  section critique. Le code GUI applique ensuite cette copie aux widgets et au
  gestionnaire de modulation.
- Les tables publiées restent vivantes tant que le synthétiseur existe; lors
  d’un rebuild, le pointeur atomique bascule vers une nouvelle table au lieu de
  libérer une table qu’un lecteur audio pourrait encore observer.

Ces mécanismes ne rendent pas chaque méthode de l’interface thread-safe. Les
fonctions d’état, le graphe de modulation et les callbacks d’hôte ont leurs
propres règles de verrouillage; consulter les implémentations avant de déplacer
un appel entre threads.

## Télémétrie et visualisations

Après le calcul audio, `SynthBase` publie une télémétrie sélectionnée et des
niveaux de crête atomiques. Les tables de télémétrie utilisent un indicateur
`atomic_flag` non bloquant; si un lecteur occupe la table, la publication du
bloc courant est sautée. Les lecteurs doivent garder leur dernier snapshot
complet plutôt que d’attendre dans un chemin temps réel.

L’oscilloscope utilise `copyOutputMemory` pour copier une fenêtre cohérente du
signal. La méthode échoue sans modifier la destination quand le snapshot est
occupé ou trop petit. Les visualiseurs d’enveloppe, d’onde et les compteurs de
modulation s’appuient sur les sorties sélectionnées et leur télémétrie; ils ne
doivent pas lire directement un buffer DSP mutable depuis le thread de rendu.

Pour la validation manuelle, moduler Attack ou Sustain d'une enveloppe pendant
une note et vérifier que le contour coloré et le marqueur suivent la voix
active sans déplacer les contrôles de base. Moduler ensuite X et Y du formant
avec un LFO, puis avec deux sources simultanées; l'anneau doit suivre leur
valeur effective tout en laissant le curseur de base éditable. Répéter avec
l'automation hôte, plusieurs notes, les animations désactivées et les modes
plug-in/standalone.

Les chemins CPU mettent en cache leurs images de grille et reconstruisent les
tracés sur le thread message; les niveaux, formes d’onde et enveloppes lisent
uniquement des snapshots publiés par `SynthBase`. Les snapshots occupés
conservent la dernière image complète au lieu d’attendre. Les visualisations
doivent éviter les lectures directes des buffers DSP et borner les allocations
et repaint au thread graphique.

Les sélecteurs de formes d’onde partagent leur géométrie de cellule avec le
meter de modulation. Celui-ci convertit les bounds du sélecteur dans ses propres
coordonnées et lit la valeur effective depuis la télémétrie publiée, sans
reconstruire la modulation ni accéder aux buffers DSP. Le viewer CPU relit cette
valeur sur le thread message et n’affiche un tracé superposé que si son index
quantifié diffère de la valeur de base. L’indexation conserve l’arrondi
Osc/Sub et la troncature LFO du runtime.

Chaque `WaveSelector` déclare explicitement sa famille Osc/Sub ou LFO; la
géométrie des cellules reste partagée, mais le rendu et les libellés de leurs
formes spécifiques sont distincts. Saw Möbius est calculée côté audio dans la
table band-limitée de l’oscillateur et côté UI par la même fonction mathématique
pure, après les mêmes transformations de phase. Le viewer utilise l’index
effectif publié par la télémétrie existante; aucun accès UI au DSP mutable n’est
introduit. La sélection Saw Möbius suit la modulation du paramètre waveform par
le chemin existant.

Les boutons de source de modulation dessinent directement leurs cercles sur le
thread message avec `Graphics` : le disque et son contour reflètent l’usage de
la source, tandis que le symbole central conserve son état de sélection et est
peint au premier plan. Le rendu ne modifie ni les routes ni le traitement audio.

Validation manuelle : pour Osc 1/2, Sub et chaque LFO, parcourir toutes les
cellules et leurs deux rangées, puis moduler le waveform avec chaque source
supportée. Faire varier la valeur de base et la profondeur pendant la lecture,
avec les animations activées puis désactivées; vérifier l’alignement de la cible,
la forme affichée, l’absence d’état périmé et l’absence de changement audible.
Répéter dans le plug-in et le standalone.

Vérifier également Saw Möbius sur Osc 1/2 et Sub : comparer le tracé du viewer à
la forme entendue en faisant varier PH/PS, puis moduler la sélection waveform et
confirmer que la forme effective et son aperçu suivent le même index.

## Saisie des quantites de modulation

`SynthSlider::showModulationPopup` ajoute aux commandes existantes une entree
`Enter Value for ...` par liaison active, depuis la source, la destination ou
son controle de quantite. Le callback conserve les noms source/destination,
pas des pointeurs de connexions ni des indices a recalculer pour ces entrees.

`FullInterface` delegue la saisie a `ModulationManager`, qui utilise la
plage, les unites et la conversion du controle de modulation concerne.
La boite reutilise la validation de `Enter Value...` avec un controle temporaire
dont la duree de vie couvre le dialogue asynchrone. Des `SafePointer` protegent
les composants GUI; la liaison est reverifiee avant validation pour ne pas
recreer une route supprimee pendant la saisie.

La confirmation appelle `setModulationAmount` sur le thread message, conserve
la source selectionnee, rafraichit les indicateurs et marque le patch modifie.
Le moteur recoit la quantite par la file de changements de modulation existante.
Une quantite nulle supprime la liaison et ses entrees dans les menus suivants.

## Verrous propres aux hôtes

Le plug-in retourne le verrou de callback JUCE à `SynthBase`; l’application
autonome utilise le `CriticalSection` de `HelmBoyEditor`. Le traitement audio
standalone tient ce verrou pendant la préparation des changements, la lecture
MIDI et le rendu du bloc. Une modification d’état ne doit donc pas supposer que
les règles de verrouillage sont identiques entre ces deux adaptateurs.
