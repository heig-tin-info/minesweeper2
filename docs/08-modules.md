# Découper en modules

> **Objectif.** Traduire les couches en dossiers et en règles d'inclusion, et
> faire vérifier ces règles par la machine plutôt que par la bonne volonté.
>
> **Code :** [src/](../src/), [tests/check_layers.cmake](../tests/check_layers.cmake),
> [CMakeLists.txt](../CMakeLists.txt)

## Le besoin

Les étapes précédentes ont dessiné trois cercles : les règles, ce qui les
entoure, le monde. Un dessin ne contraint personne. Le jour où quelqu'un
ajoute `#include <SDL2/SDL.h>` dans `Game` « juste pour afficher un message »,
l'architecture est morte sans que personne ne s'en aperçoive.

Il faut donc : des **dossiers** qui matérialisent les couches, une **règle**
qui dit ce que chaque dossier a le droit d'inclure, et un **test** qui échoue
quand la règle est violée.

## Les options

| Option | Ce qu'elle garantit | Ce qu'elle laisse passer |
| --- | --- | --- |
| Un seul dossier, des commentaires | rien | tout |
| Des dossiers par couche | la lisibilité | un `#include` vers le mauvais dossier compile très bien |
| Une bibliothèque CMake par couche | l'ordre de **liaison** : `adapters → app → core` | un `#include <iostream>` dans le cœur, qui ne lie rien |
| Les deux, plus un test qui lit les `#include` | la règle complète, à chaque `make test` | — |

## La décision

### L'arborescence

```text
src/
├── core/        le domaine : les règles du démineur, sans aucune entrée/sortie
├── app/         l'application : les ports et la session de jeu
├── adapters/    la console et la fenêtre SDL
│   ├── console/
│   └── sdl/
└── launcher/    le point de composition commun aux deux exécutables
tests/           tests unitaires, doublures de test, test d'architecture
```

| Module | Espace de noms | Contient | A le droit d'inclure |
| --- | --- | --- | --- |
| `core/` | `minesweeper` | `Pos`, `Grid`, `Minefield`, `Game`, `MineLayout` | `core/`, la bibliothèque standard **sans** E/S ni `<chrono>` |
| `app/` | `minesweeper` | les ports, `Command`, `Session`, `Stopwatch` | `core/`, `app/`, la bibliothèque standard sans E/S |
| `adapters/console/` | `minesweeper::console` | `ConsoleUi`, `notation`, `renderer` | tout |
| `adapters/sdl/` | `minesweeper::sdl` | `SdlUi`, les ressources RAII | tout |
| `launcher/` | `minesweeper` | `launch`, `parseArguments`, `SteadyClock` | tout |

Chaque module est une **bibliothèque CMake**, et le graphe de liaison *est*
la règle de dépendance :

```cmake
# One library per layer. The link graph is the dependency rule:
#   adapters -> app -> core, and never the other way round.
add_library(minesweeper_core STATIC
    src/core/game.cpp
    src/core/mine_layout.cpp
    src/core/minefield.cpp
)
add_library(minesweeper_app STATIC src/app/session.cpp)
target_link_libraries(minesweeper_app PUBLIC minesweeper_core)

add_library(minesweeper_launcher STATIC src/launcher/launch.cpp)
target_link_libraries(minesweeper_launcher PUBLIC minesweeper_app)
```

### Le test d'architecture

La liaison ne voit pas un `#include <iostream>` égaré dans `core/`. Un script
CMake lit donc **toutes les lignes `#include`** de chaque couche et refuse
celles qui violent la règle :

```cmake
function(forbid layer pattern reason)
    file(GLOB_RECURSE files "${SOURCE_DIR}/${layer}/*.hpp" "${SOURCE_DIR}/${layer}/*.cpp")
    foreach(file IN LISTS files)
        file(STRINGS "${file}" includes REGEX "^[ \t]*#[ \t]*include")
        foreach(line IN LISTS includes)
            if(line MATCHES "${pattern}")
                message(SEND_ERROR "${file}: `${line}` (${reason})")
            endif()
        endforeach()
    endforeach()
endfunction()

set(io "<(iostream|fstream|cstdio|SDL)")

forbid(core "\"(app|adapters|launcher)/" "the domain depends on nothing")
forbid(core "${io}" "the domain does no I/O")
forbid(core "<chrono>" "the domain does not know the time")
forbid(app "\"(adapters|launcher)/" "the application does not know its adapters")
forbid(app "${io}" "the application does no I/O")
```

Il est enregistré comme un test ordinaire, lancé par `make test` avec les
autres :

```cmake
add_test(NAME architecture.dependency_rule
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}/src
            -P ${PROJECT_SOURCE_DIR}/tests/check_layers.cmake)
```

### Les trois exécutables

| Exécutable | Point d'entrée | Ce qu'il assemble |
| --- | --- | --- |
| `minesweeper-console` | [console/main.cpp](../src/adapters/console/main.cpp) | `Session` + `ConsoleUi` |
| `minesweeper-sdl` | [sdl/main.cpp](../src/adapters/sdl/main.cpp) | `Session` + `SdlUi` |
| `minesweeper-tests` | GoogleTest | `Session`, `Game`… + doublures de test |

Les tests sont, au sens propre, un **troisième adaptateur** : ils pilotent le
cœur par les mêmes ports que les interfaces. Le système de build compile
l'exécutable SDL seulement si SDL2 est installé ; rien d'autre n'en dépend.

### Là où naît la couche application

En rangeant les éléments de l'[étape 2](02-elements-du-jeu.md) dans les
dossiers, deux d'entre eux ne trouvent pas leur place :

- le **chronomètre** : ce n'est pas une règle du démineur, donc pas dans
  `core/` ; et si on l'écrit dans `ConsoleUi` puis dans `SdlUi`, on l'écrit
  **deux fois** ;
- **recommencer une partie** avec la même configuration : même raisonnement.

Il faut un module **entre** le domaine et les adaptateurs, qui connaît le
domaine mais pas les interfaces. C'est `app/`, et l'objet qui y vit s'appelle
`Session`. On la nomme maintenant parce que le besoin vient d'apparaître ; on
l'écrira à l'[étape 17](17-session.md), quand le cœur existera.

De même, quelqu'un doit créer les objets concrets (`RandomMineLayout`,
`SteadyClock`, `Session`) et les brancher. Ce code connaît **toutes** les
classes : il ne peut être ni dans `core/`, ni dans `app/`, ni dans un
adaptateur particulier. C'est `launcher/`, le **point de composition**,
détaillé à l'[étape 19](19-lanceur.md).

## Les concepts

### Module

Un module, ici, c'est trois choses qui coïncident : un **dossier**, un
**espace de noms** (`minesweeper::console`) et une **règle d'inclusion**. Un
fichier de `core/` qui a besoin de quelque chose hors de `core/` signale une
erreur de conception, pas un `#include` manquant.

### Point de composition

L'unique endroit du programme qui connaît toutes les classes concrètes et les
assemble. Partout ailleurs, on ne manipule que des interfaces. Déplacer la
création des objets vers ce point est ce qui permet aux tests de créer les
leurs.

### Test d'architecture

Un test qui ne vérifie pas un comportement mais une **propriété du code** :
ici, la règle de dépendance. On l'appelle aussi *fitness function*. Il coûte
vingt lignes et empêche définitivement la régression la plus coûteuse d'un
projet en couches.

## Vérifier

Ajoutez `#include <iostream>` en tête de `src/core/game.hpp` et lancez
`make test`. Le test d'architecture (`dependency_rule`) échoue avec un message de
la forme :

```text
.../src/core/game.hpp: `#include <iostream>` (the domain does no I/O)
```

Retirez la ligne : tout repasse au vert. Faites de même avec
`#include "app/session.hpp"` dans `core/` (« the domain depends on nothing »).

Deuxième critère : `make` sans SDL2 installé produit quand même
`minesweeper-console` et `minesweeper-tests`.

## À retenir

- Une **couche** devient un **dossier**, un **espace de noms** et une **règle
  d'inclusion**.
- Une bibliothèque CMake par couche fait de l'ordre de liaison la règle de
  dépendance ; un **test d'architecture** attrape ce que la liaison ne voit
  pas.
- Le chronomètre et « recommencer » ne sont ni des règles ni propres à une
  interface : ils font naître la **couche application** et `Session`.
- Le **point de composition** est le seul code qui connaît toutes les
  classes concrètes.

## Exercices

1. On veut interdire `<random>` dans `app/` (le hasard appartient au port
   `MineLayout`). Écrivez la ligne `forbid(...)` correspondante. Pourquoi ne
   peut-on pas l'écrire pour `core/` ?
2. Un camarade met `Stopwatch` dans `core/`. Quel test échoue, avec quel
   message ? Et si `Stopwatch` ne lit jamais l'heure lui-même (il la reçoit en
   paramètre), le test échoue-t-il encore ? Est-ce pour autant le bon dossier ?
3. Dessinez le graphe de liaison si l'on ajoutait un adaptateur
   `adapters/web/`. Quelles bibliothèques CMake lie-t-il ? Lesquelles ne
   doivent surtout pas le connaître ?
