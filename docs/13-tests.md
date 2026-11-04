# Tester une architecture objet

> **Objectif.** Voir comment une bonne conception rend les tests **simples**,
> utiliser des **doublures de test**, et tester l'architecture elle-même.
>
> **Code :** [tests/](../tests/)

## Les tests révèlent la conception

Un test difficile à écrire est presque toujours le symptôme d'un problème de
conception. Dans l'ancienne version, pour tester une défaite, il fallait
d'abord **chercher** où le hasard avait mis les mines :

```cpp
// Ancien test : fouiller le plateau en passant par une porte dérobée
for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
        if (core.cell(x, y).bomb()) bombs.emplace_back(x, y);
```

Ce test devait accéder à `bomb()`, l'information même que le jeu est censé
cacher, et il dépendait du hasard.

Avec le hasard injecté, le test **décide** où sont les mines :

```cpp
FixedMineLayout layout{{{0, 1}}};
Game game{{3, 3}, 1, layout};
game.reveal({1, 1});
game.reveal({0, 1});
EXPECT_EQ(game.status(), Status::Lost);
```

Il se lit comme une phrase, et il passe toujours de la même façon.

## Les doublures de test

Une **doublure** (*test double*) remplace une vraie dépendance pendant un
test. Le vocabulaire (Gerard Meszaros) distingue plusieurs sortes, et ce projet
en utilise trois ([test_doubles.hpp](../tests/test_doubles.hpp)) :

| Sorte | Rôle | Ici |
| --- | --- | --- |
| **Stub** | renvoie des réponses préparées | `FixedMineLayout` renvoie les mines voulues |
| **Spy** | enregistre comment on l'a appelé, pour vérifier ensuite | `FixedMineLayout::calls`, `RecordingSink::received` |
| **Fake** | une implémentation qui fonctionne vraiment, mais simplifiée | `FakeClock`, dont le temps n'avance que sur ordre |

Quelques exemples de ce qu'elles permettent :

```cpp
// Spy : les mines sont posées au premier clic, et seulement au premier.
game.reveal({2, 2});
game.reveal({0, 1});
EXPECT_EQ(layout.calls, 1);
EXPECT_EQ(layout.lastFirstClick, (Pos{2, 2}));

// Fake : le chronomètre s'arrête à la fin de la partie.
session.handle(Reveal{{2, 1}});
clock.advance(3s);
session.handle(Reveal{{0, 0}});   // boom
clock.advance(100s);
EXPECT_EQ(session.elapsed(), 3s);

// Spy : la console traduit les lignes tapées en commandes.
std::istringstream in{"B2\nbogus\nA3?\nn\nq\nC1\n"};
ConsoleUi{in, out, Style{}}.run(session, sink);
EXPECT_EQ(sink.received, (std::vector<Command>{Reveal{{1, 1}}, ToggleFlag{{0, 0}}, NewGame{}}));
```

Aucune de ces doublures n'aurait été possible sans les **ports** du
[chapitre 7](07-polymorphisme-ports.md). Interfaces et tests vont ensemble.

## Tester des comportements, pas des implémentations

Les tests passent par les **méthodes publiques** et vérifient ce qu'un
utilisateur peut observer :

```cpp
TEST_F(OneMineGame, FlagsStopTheFloodFill) { ... }
TEST_F(SessionTest, ClockStartsOnTheFirstReveal) { ... }
```

Aucun test ne lit `hiddenSafe_` ni ne connaît `Mark`. On peut donc réécrire
l'intérieur de `Game` (remplacer la pile par une file, changer la
représentation des marques) sans toucher un seul test. Un test qui casse à
chaque refactorisation finit toujours supprimé.

Le nom de chaque test est une **phrase** qui décrit une règle :
`RevealingANumberOpensOnlyThatCell`. La liste des tests se lit comme la
spécification du jeu.

## Les cas limites

Quelques tests vérifient des propriétés qu'on oublierait facilement :

- `Game.FloodFillHandlesHugeEmptyGrids` : une grille de 1000 × 1000 ne fait pas
  déborder la pile ([chapitre 5](05-agregat.md)) ;
- `Game.RejectsALayoutThatBreaksItsContract` : l'agrégat se protège d'un
  collaborateur défaillant ;
- `RandomMineLayout.IsReproducibleWithTheSameSeed` : la graine rend une partie
  rejouable ;
- `Arguments.LeavesGameRulesToTheDomain` : le parseur ne valide **pas** les
  règles, il laisse ce rôle à `Game`.

## Tester l'architecture elle-même

Une règle d'architecture qu'on ne vérifie pas finit toujours par être violée,
souvent par un `#include` « juste pour déboguer ». Ce projet transforme la
règle de dépendance en **test exécutable**
([check_layers.cmake](../tests/check_layers.cmake)) :

```cmake
forbid(core "\"(app|adapters|launcher)/" "the domain depends on nothing")
forbid(core "${io}"                       "the domain does no I/O")
forbid(core "<chrono>"                    "the domain does not know the time")
forbid(app  "\"(adapters|launcher)/"      "the application does not know its adapters")
forbid(app  "${io}"                       "the application does no I/O")
```

Il tourne avec les autres (`architecture.dependency_rule` dans `ctest`).
Ajoutez `#include <iostream>` dans `core/position.hpp`, et :

```text
CMake Error at tests/check_layers.cmake:14 (message):
  .../src/core/position.hpp: `#include <iostream>` (the domain does no I/O)
```

C'est ce qu'on appelle une **fonction de *fitness*** architecturale : un
garde-fou automatique qui protège une décision de conception, à chaque
compilation, sans dépendre de la vigilance de quiconque.

Le découpage en bibliothèques CMake (`minesweeper_core`, `minesweeper_app`,
`minesweeper_console`...) rend aussi la règle **visible** : `minesweeper_core`
ne déclare aucune dépendance vers une autre bibliothèque du projet. Mais le
répertoire `src/` est partagé, si bien qu'un en-tête d'une autre couche
resterait techniquement incluable. C'est précisément ce trou que le test
d'architecture bouche.

## Lancer les tests

```bash
make test
# ou
ctest --test-dir build --output-on-failure
```

## À retenir

- Un test **difficile à écrire** signale un problème de **conception**.
- **Stub**, **spy**, **fake** : chaque doublure a son rôle. Toutes passent par
  des ports.
- Testez les **comportements observables**, pas les détails internes.
- Une règle d'architecture importante mérite son **test automatique**.

## Pour réfléchir

1. Écrivez le test d'une nouvelle règle : « un drapeau posé sur une case la
   protège d'un `reveal` ». Quelles doublures faut-il ?
2. Quelle règle ajouteriez-vous à `check_layers.cmake` pour les adaptateurs ?
   (Indice : la console a-t-elle le droit d'inclure SDL ?)
