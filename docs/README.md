# Le démineur, une démarche de conception objet

Ce dossier raconte la construction d'un démineur, **du jeu au code, un
besoin à la fois**. On n'y présente pas une architecture finie qu'il faudrait
admirer : on part du jeu, on analyse, on décide, on construit, et chaque
concept arrive au moment où le code qu'on vient d'écrire en a besoin.

Le jeu est volontairement petit : on peut tout lire en quelques heures. Mais
les questions qu'il pose sont celles d'un logiciel professionnel : où mettre
les règles, comment brancher plusieurs interfaces sur le même cœur, comment
tester sans clavier ni écran, comment gérer des ressources système sans
fuite.

## Comment lire chaque étape

Toutes les étapes ont la même forme :

| Rubrique | La question à laquelle elle répond |
| --- | --- |
| **Le besoin** | Qu'est-ce qu'on veut obtenir maintenant ? Qu'est-ce qui coince ? |
| **Les options** | Quelles solutions naïves ou classiques ? Que coûtent-elles ? |
| **La décision** | Laquelle on retient, sur quel critère, et le code qui l'incarne. |
| **Les concepts** | Le nom de ce qu'on vient d'apprendre, en une définition. |
| **Vérifier** | Le test, ou le critère observable, qui prouve que ça marche. |
| **À retenir** | L'essentiel en quelques lignes. |
| **Exercices** | Une variation qui oblige à réappliquer le concept. |

Trois questions reviennent à chaque décision. Apprenez à les poser avant
d'écrire une ligne :

1. **Peut-on le tester sans clavier, sans écran, sans attendre ?**
2. **A-t-on écrit deux fois la même chose ?**
3. **Qui dépend de qui, et dans quel sens ?**

## Parcours

Les étapes se lisent dans l'ordre : chacune s'appuie sur les précédentes.

### I. Analyser (aucune ligne de code)

| # | Étape | Ce qu'on apprend |
| --- | --- | --- |
| 1 | [Le démineur](01-le-demineur.md) | le jeu, son lexique ; règle vs fonctionnalité |
| 2 | [Les éléments du jeu](02-elements-du-jeu.md) | noms → types, verbes → opérations ; trois cercles |
| 3 | [Les cas d'utilisation](03-cas-d-utilisation.md) | acteurs primaires et secondaires, scénarios |
| 4 | [Un scénario en boîte noire](04-scenario-boite-noire.md) | ordre, question, besoin : trois flèches |

### II. Décider l'architecture

| # | Étape | Ce qu'on apprend |
| --- | --- | --- |
| 5 | [Choisir une architecture](05-choisir-une-architecture.md) | couplage, dépendances, inversion de dépendance |
| 6 | [Ports et adaptateurs](06-ports-et-adaptateurs.md) | port, adaptateur, `Session` ; les tests sont un adaptateur |
| 7 | [Le sens des flux](07-sens-des-flux.md) | les ordres descendent, les données sont tirées |
| 8 | [Découper en modules](08-modules.md) | dossiers, règle des `#include`, test d'architecture |

### III. Construire le cœur

| # | Étape | Ce qu'on apprend | Code |
| --- | --- | --- | --- |
| 9 | [Positions, taille et voisins](09-positions.md) | objet valeur, `struct` vs `class`, `constexpr` | [position.hpp](../src/core/position.hpp) |
| 10 | [Le champ de mines](10-champ-de-mines.md) | invariant, encapsulation, immuabilité | [minefield.hpp](../src/core/minefield.hpp) |
| 11 | [La partie](11-la-partie.md) | agrégat, source unique de vérité, états, CQS | [game.hpp](../src/core/game.hpp) |
| 12 | [Une grille générique](12-grille-generique.md) | `template` comme réponse à la duplication | [grid.hpp](../src/core/grid.hpp) |
| 13 | [L'ouverture en cascade](13-ouverture-en-cascade.md) | récursif vs itératif, pile explicite | [game.cpp](../src/core/game.cpp) |
| 14 | [Ce que voit l'interface](14-ce-que-voit-l-interface.md) | projection, `std::variant`, états illégaux irreprésentables | [cell_view.hpp](../src/core/cell_view.hpp) |

### IV. L'application

| # | Étape | Ce qu'on apprend | Code |
| --- | --- | --- | --- |
| 15 | [Les commandes](15-commandes.md) | commande-valeur, interfaces pures, `std::visit` | [commands.hpp](../src/app/commands.hpp) |
| 16 | [Le hasard et le premier clic sûr](16-hasard.md) | polymorphisme, injection de dépendance, stub | [mine_layout.hpp](../src/core/mine_layout.hpp) |
| 17 | [Le temps, puis la `Session`](17-session.md) | service applicatif, le temps injecté, identité | [session.hpp](../src/app/session.hpp) |

### V. Les adaptateurs

| # | Étape | Ce qu'on apprend | Code |
| --- | --- | --- | --- |
| 18 | [Le terminal](18-terminal.md) | fonctions pures de traduction, tester une UI, spy | [console/](../src/adapters/console/) |
| 19 | [Le lanceur](19-lanceur.md) | racine de composition, `main()` minimal | [launcher/](../src/launcher/) |
| 20 | [La fenêtre SDL](20-fenetre-sdl.md) | mêmes ports, autre technologie | [sdl/](../src/adapters/sdl/) |
| 21 | [Libérer sans fuir : RAII](21-raii.md) | durée de vie, `unique_ptr` et deleters | [resources.hpp](../src/adapters/sdl/resources.hpp) |

### VI. Vérifier et prendre du recul

| # | Étape | Ce qu'on apprend |
| --- | --- | --- |
| 22 | [La stratégie de tests, relue](22-tests.md) | stub, fake, spy ; la testabilité comme critère |
| 23 | [Avant / après](23-avant-apres.md) | les défauts de l'ancienne version et leur correction |
| 24 | [Et maintenant ?](24-et-maintenant.md) | l'architecture se juge au changement |

Le [labo](../steps/README.md) propose d'écrire soi-même une version
minimaliste, étape par étape. Les [règles du jeu](regles-du-jeu.md) et le
[diagramme de classes](uml.md) complet sont en annexe. Le fil pédagogique
qui a servi à organiser ce document est dans [story.md](story.md).

## Comment lire le code

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

Commencez par [game.hpp](../src/core/game.hpp) : c'est le cœur. Tout le reste
existe pour le servir ou pour le montrer.

## Compiler et jouer

```bash
make test        # compile et lance les tests
make run         # joue dans le terminal
make run-sdl     # joue dans une fenêtre (si SDL2 est installé)
make run ARGS="16 30 99 --seed 42"
make doc         # ce document en PDF (build/doc/livre.pdf)
```
