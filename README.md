# Démineur en C++ : version simple

Un démineur jouable dans le terminal, en un peu plus de 300 lignes, organisé
en **ports et adaptateurs** : les règles du jeu ne savent rien du terminal ni
du hasard, et on peut leur brancher d'autres interfaces sans les modifier.

```text
    0 1 2 3 4 5 6 7 8
  0 F # 1 . . 1 # # #
  1 # # 1 . 1 2 # # #
  2 1 1 1 . 1 # # # #
  3 . . . . 1 2 # # #
  4 . . . . . 1 # # #
  5 1 1 . . . 1 # # #
  6 # 2 . . . 1 # # #
  7 # 3 . . . 1 1 2 1
  8 # 2 . . . . . . .
> r 6 0
```

`#` case cachée, `F` drapeau, `.` case vide, `1`–`8` mines voisines, `*` mine.

## Jouer

Prérequis : `make` et un compilateur C++20 (`g++` 11+ ou `clang++` 14+).

```bash
make test                     # compile et lance les tests
make run                      # partie 9x9 avec 10 mines
make run ARGS=20              # 20 mines sur la grille 9x9
make run ARGS="99 16 30 42"   # mines, lignes, colonnes, graine
```

| Commande | Effet |
| --- | --- |
| `r 2 3` | dévoile la case ligne 2, colonne 3 |
| `f 2 3` | pose ou retire un drapeau |
| `q` | quitte |

La première case dévoilée n'est jamais une mine. Les règles complètes sont
dans [docs/regles-du-jeu.md](docs/regles-du-jeu.md).

## Architecture

```text
              ┌──────────── ports.hpp ────────────┐
 Console ───▶ │ UserInterface    MineLayout       │ ◀─── RandomLayout
 (terminal)   └───────┬──────────────────┬────────┘      (rand + graine)
                      │                  │
                  play()  ──────────▶  Game  (les règles, sans E/S)
```

| Fichier | Rôle |
| --- | --- |
| [src/ports.hpp](src/ports.hpp) | `Pos`, `Command` et les deux interfaces (ports) |
| [src/game.hpp](src/game.hpp) | le domaine : la grille, dévoiler, drapeau, victoire/défaite |
| [src/play.hpp](src/play.hpp) | la boucle de jeu, qui ne connaît que les ports |
| [src/random_layout.hpp](src/random_layout.hpp) | adaptateur : place les mines avec `rand()` et une graine |
| [src/console.hpp](src/console.hpp) | adaptateur : affiche la grille brute, lit les commandes |
| [src/main.cpp](src/main.cpp) | point de composition : choisit les adaptateurs et lance la partie |
| [tests/tests.cpp](tests/tests.cpp) | tests avec doublures (`FixedLayout`, `ScriptedUi`) |

Les dépendances pointent vers le centre : `Game` n'inclut que `ports.hpp`, et
seul `main.cpp` connaît toutes les classes concrètes.

## Pistes d'extension

Chacune se fait en ajoutant du code, sans toucher aux règles du jeu :

- un affichage plus joli ou en couleurs : un autre `show()` ;
- une notation `B3` à la place de `r 2 1` : un autre `parse()` ;
- un compteur de mines restantes, un chronomètre : une requête sur `Game` ;
- une nouvelle commande (nouvelle partie, aide) : un `Command::Type` de plus ;
- une fenêtre graphique (SDL) : un nouvel adaptateur de `UserInterface` ;
- un autre générateur (`<random>`, grilles prédéfinies) : un adaptateur de
  `MineLayout`.
