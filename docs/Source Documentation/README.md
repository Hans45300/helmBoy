# Documentation du source

Cette documentation réunit les pages développeur écrites à la main et les
références extraites des commentaires Doxygen. La configuration se trouve dans
[`../../Doxyfile`](../../Doxyfile); l’API est générée à partir de `src`, `mopo`
et de ce répertoire. `externals`, `build` et `concurrentqueue` sont exclus.

## Guides développeur

- [Architecture du logiciel](developer/architecture.md)
- [Paramètres, automation et état](developer/state-and-parameters.md)
- [MIDI 1.0 et décodeur UMP](developer/midi.md)
- [Interface, visualisations et threads](developer/gui-and-threads.md)
- [Cibles de développement et tests](developer/development-and-tests.md)

La [page principale Doxygen](mainpage.dox) est la porte d’entrée de la référence.
Les groupes source sont déclarés dans `src/doxygen_groups.h` et
`mopo_modules.dox`.

## Générer la référence

Prérequis : Doxygen; Graphviz est facultatif pour les graphes.
Depuis la racine du dépôt :

```powershell
doxygen Doxyfile
```

La sortie configurée par `OUTPUT_DIRECTORY` et `HTML_OUTPUT` est
`docs/Source Documentation/html/`. Ouvrir ensuite `index.html` dans ce dossier.
Les fichiers de ce répertoire `html/` sont générés; modifier les sources
Markdown, `.dox`, les commentaires du code ou `Doxyfile`, puis régénérer si
nécessaire. Ne pas corriger directement les pages générées.

## Maintenir la documentation

Quand un flux ou une API change, actualiser le guide concerné et les commentaires
Doxygen des contrats publics concernés. Décrire séparément ce qui est raccordé
aux chemins plugin/standalone et les API présentes mais non intégrées. Vérifier
les références `@ref`, les liens relatifs et les chemins `INPUT`/`EXCLUDE_PATTERNS`
avant de générer la sortie.
