# Le démineur, une vitrine de conception objet

Ce dossier explique, concept par concept, comment ce démineur est construit
et **pourquoi** il est construit ainsi. Chaque chapitre part d'un problème
concret du jeu, montre la solution dans le code, puis en tire un principe
réutilisable ailleurs.

Le jeu est volontairement petit : on peut tout lire en une après-midi. Mais
les questions qu'il pose sont exactement celles d'un logiciel professionnel :
où mettre les règles, comment brancher plusieurs interfaces sur le même
cœur, comment tester sans clavier ni écran, comment gérer des ressources
système sans fuite.

## Parcours de lecture

Les chapitres se lisent dans l'ordre : chacun s'appuie sur les précédents.

| # | Chapitre | Concepts | Code |
| --- | --- | --- | --- |
| 1 | [Le démineur](01-le-demineur.md) | le jeu, l'exemple à construire, l'organisation du document | |
| 2 | [Vue d'ensemble](02-vue-ensemble.md) | architecture hexagonale, couches, règle de dépendance | tout `src/` |
| 3 | [Objets valeur](03-objets-valeur.md) | `struct` vs `class`, égalité, `constexpr`, types itérables | [position.hpp](../src/core/position.hpp) |
| 4 | [Encapsulation et invariants](04-encapsulation-invariants.md) | invariant, `private`, `const`, immuabilité, exceptions | [minefield.hpp](../src/core/minefield.hpp) |
| 5 | [L'agrégat `Game`](05-agregat.md) | racine d'agrégat, source unique de vérité, où mettre `reveal()` | [game.hpp](../src/core/game.hpp) |
| 6 | [Programmation générique](06-templates.md) | `template`, DRY, `static_assert`, `assert` vs exception | [grid.hpp](../src/core/grid.hpp) |
| 7 | [Polymorphisme et ports](07-polymorphisme-ports.md) | `virtual`, interface, inversion de dépendance, injection | [mine_layout.hpp](../src/core/mine_layout.hpp) |
| 8 | [Commandes et requêtes](08-commandes-requetes.md) | CQS (et pourquoi ce n'est pas CQRS), `std::variant`, `std::visit` | [commands.hpp](../src/app/commands.hpp) |
| 9 | [Projection et DTO](09-projection-dto.md) | ce que l'interface a le droit de voir, Visiteur | [cell_view.hpp](../src/core/cell_view.hpp) |
| 10 | [La couche application](10-couche-application.md) | service applicatif, temps injecté, héritage d'interfaces | [session.hpp](../src/app/session.hpp) |
| 11 | [Les adaptateurs](11-adaptateurs.md) | qui possède la boucle, pull vs push, Observer, point de composition | [adapters/](../src/adapters/) |
| 12 | [RAII](12-raii.md) | durée de vie, `unique_ptr` et deleters, ordre des membres | [resources.hpp](../src/adapters/sdl/resources.hpp) |
| 13 | [Tests](13-tests.md) | doublures de test (stub, spy, fake), test d'architecture | [tests/](../tests/) |
| 14 | [Avant / après](14-avant-apres.md) | les défauts de la version précédente et leur correction | |

Le [labo](../steps/README.md) propose ensuite d'écrire soi-même une version
minimaliste, étape par étape. Les [règles du jeu](regles-du-jeu.md) et le
[diagramme de classes](uml.md) complet sont en annexe.

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
```
