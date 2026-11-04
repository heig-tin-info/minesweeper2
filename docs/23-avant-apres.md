# Avant / après

> **Objectif.** Revenir à la version précédente de ce projet, l'approche
> « une classe qui fait tout » écartée à l'[étape 5](05-choisir-une-architecture.md),
> avec les yeux de maintenant : pour chaque défaut, nommer le principe violé
> et l'étape qui l'a corrigé. C'est le meilleur résumé du parcours.

## Le besoin

La version précédente avait la bonne intention : séparer le cœur des
interfaces par des ports. Mais une série de détails annulait une partie des
bénéfices promis. Le README annonçait du « CQRS » et des « DTO » ; le code
contenait surtout du CQS et des chaînes de caractères. On veut comprendre
**pourquoi** ces détails comptaient, et vérifier que chacun est aujourd'hui
couvert par une décision explicite et, le plus souvent, par un test.

## Les options

| Option | Idée | Ce qui casse |
| --- | --- | --- |
| A. Tout réécrire de zéro | repartir d'une page blanche | on perd les bonnes idées de l'ancienne version, et on refait d'autres erreurs |
| B. Corriger défaut par défaut | nommer chaque défaut, le principe, la correction, le test | demande une analyse ; c'est ce chapitre |

## La décision

Option B. On garde ce qui était juste, on corrige ce qui ne l'était pas, et
chaque correction est rattachée à une étape du parcours. Ce qui a été
**conservé** de l'ancienne version :

- le placement des mines **au premier clic**, avec une ouverture de 3 × 3 ;
- un *flood fill* **itératif** plutôt que récursif ([étape 13](13-ouverture-en-cascade.md)) ;
- une grille stockée dans un **vecteur plat** ([étape 12](12-grille-generique.md)) ;
- la séparation entre **port d'écriture** et **port de lecture** ;
- des **tests** sur le domaine.

Une bonne refactorisation garde ce qui est juste et ne corrige que ce qui ne
l'est pas. Les douze défauts suivants sont regroupés par principe.

## Les concepts

### Les ports transportaient de la présentation

**Un DTO fait de chaînes.**

| Avant | Après |
| --- | --- |
| `using CellToken = std::string;` contenant `"."`, `"?"`, `"3"`, `"*"` | `CellView`, un `std::variant` de types sémantiques |
| Le domaine choisissait le caractère (`Cell::getSymbol()`) | Le domaine dit ce que le joueur **sait** ; chaque adaptateur choisit l'apparence |
| SDL décodait les caractères de la console (`classifyToken`) | `std::visit`, vérifié par le compilateur |

Principe : un port ne doit pas être couplé à un adaptateur particulier.
→ [étape 14](14-ce-que-voit-l-interface.md).

**Un HUD formaté dans le cœur.**

| Avant | Après |
| --- | --- |
| `struct HudSnapshot` avec deux `std::string` déjà formatées | `std::chrono::seconds elapsed()`, `int minesLeft()` |
| `<iomanip>` et `<sstream>` dans le cœur | Le formatage est fait par chaque adaptateur |

Principe : le cœur fournit des **valeurs**, les adaptateurs les
**présentent**. → [étape 14](14-ce-que-voit-l-interface.md).

**Des intentions mal typées.**

| Avant | Après |
| --- | --- |
| `enum class Action` + `std::optional<Pos>` | un type par commande dans un `std::variant` |
| `Reveal` sans position représentable | impossible à écrire |
| `Quit`, `Redraw` dans le port du cœur | `Quit` appartient à la console seule |

Principe : rendre les états illégaux impossibles à représenter.
→ [étape 15](15-commandes.md).

**« CQRS » qui était du CQS.** Un même objet (`GameCore`) répondait aux
commandes et aux requêtes, à partir du même état. C'est de la séparation
commande/requête (CQS), une excellente pratique, mais ce n'est pas
l'architecture CQRS. Principe : nommer les choses exactement.
→ [étape 11](11-la-partie.md).

### Les invariants n'étaient pas protégés

**Un agrégat poreux.**

| Avant | Après |
| --- | --- |
| `Cell` publique avec `setBomb()`, `setRevealed()` | `Minefield` immuable, `Mark` privé dans `Game` |
| `Board::operator()` renvoyait un `Cell&` modifiable | Aucun accès en écriture hors des commandes |
| `Board::revealAll()` public, cassait `won()` | La fin de partie est une **projection**, rien n'est modifié |
| `GameCore::cell()` public exposait `bomb()` | Les mines cachées ne sortent jamais du domaine |

Principe : l'encapsulation protège des **invariants** ; une seule méthode
publique qui permet de les casser suffit à les rendre fragiles.
→ [étapes 10](10-champ-de-mines.md) et [11](11-la-partie.md).

**Deux sources de vérité.**

| Avant | Après |
| --- | --- |
| `Board::_lost` **et** `GameCore::state_` | `Game::status()` **calculé**, jamais stocké |
| Victoire testée deux fois | Une seule règle |
| Révélation de fin de partie faite deux fois | Aucune : c'est une projection |
| La fin de partie était contrôlée par `GameCore`, pas par `Board` | `Game` refuse lui-même les commandes |

Principe : SSOT, une information et un propriétaire. → [étape 11](11-la-partie.md).

**Deux systèmes de coordonnées.** `Pos{x, y}` côté ports, `Coord{r, c}` côté
domaine, avec des inversions manuelles (`cellAt(col, row)` qui construisait
`Coord{row, col}`), et des `int` mêlés à des `size_t` au point qu'une
structure stockait les deux versions. Correction : une seule `Pos{row, col}`,
un seul type d'indice (`int`) ; chaque adaptateur traduit **son** système
(notation d'échecs, pixels). → [étape 9](09-positions.md).

**Un bug de durée de vie.**

```cpp
class GameCore {
    BoardPresenter presenter_;   // contient « const GameCore& game » → *this
};
GameCore b = a;   // compile ! b.board() lit... a.
```

Un membre référence interdit l'affectation, mais **pas** la construction par
copie. La copie gardait une référence vers l'original : si `a` était détruit,
la référence devenait pendante. Correction : plus aucune référence vers
soi-même, et `Session` supprime explicitement sa copie parce qu'une session a
une identité. → [étapes 11](11-la-partie.md) et [17](17-session.md).

### Le monde extérieur était câblé en dur

| Avant | Après |
| --- | --- |
| `std::random_device` en argument par défaut de `Board` | port `MineLayout`, graine injectée (`--seed`) |
| `Stopwatch` lisait `steady_clock` | `Stopwatch` pur + port `Clock` |
| Tests obligés de fouiller les mines | `FixedMineLayout`, `FakeClock` |

Principe : inversion et injection de dépendances.
→ [étapes 16](16-hasard.md) et [17](17-session.md).

### L'assemblage et les ressources étaient fragiles

**Un `main` à `#ifdef`.** Un seul `main.cpp`, deux boucles différentes selon
`-DSDL`, et deux interfaces sans API commune (`wait_intent()` d'un côté,
`pollIntent()` de l'autre). Correction : un point de composition partagé
(`launch`), deux `main` de 8 lignes, chaque adaptateur possède **sa** boucle
derrière `run(view, sink)`. → [étapes 18](18-terminal.md),
[19](19-lanceur.md) et [20](20-fenetre-sdl.md).

**Initialisation en deux temps.** `SDLUI` pouvait exister sans fenêtre
(`ready() == false`), et `teardown()` libérait tout à la main. Correction :
RAII avec `unique_ptr` et deleters, un constructeur qui réussit ou lance une
exception, et la règle du zéro. → [étape 21](21-raii.md).

**Une architecture non vérifiée.** Rien n'empêchait d'inclure SDL dans le
cœur ; un en-tête unique (`minesweeper_headers`) servait à tout le monde.
Correction : une bibliothèque CMake par couche, et un test d'architecture qui
échoue dès qu'un `#include` viole la règle de dépendance.
→ [étapes 8](08-modules.md) et [22](22-tests.md).

## Vérifier

Chaque défaut corrigé est aujourd'hui tenu par un test ou par le
compilateur. Le tableau sert de liste de contrôle :

| Défaut | Ce qui l'empêche de revenir |
| --- | --- |
| DTO de chaînes, HUD formaté | `check_layers` interdit `<iostream>`, `<sstream>` n'a plus de raison d'être dans `core/` |
| Intentions mal typées | `Reveal{}` sans position ne compile pas |
| Agrégat poreux | `Game.HittingAMineLosesAndShowsTheTruth` passe sans aucun accesseur de mine |
| Deux sources de vérité | `Game.CommandsAreIgnoredOnceTheGameIsOver` : le statut est dérivé |
| Hasard et temps câblés | `OneMineGame.PlantsMinesOnTheFirstRevealOnly`, `SessionTest.ClockStopsWhenTheGameEnds` |
| Copie dangereuse | `Session(const Session&) = delete` : erreur de compilation |
| `#ifdef` dans `main` | deux cibles CMake, aucun `#ifdef` dans `src/` |
| Deux temps | `SdlUi` n'a ni `ready()` ni `teardown()` |
| Architecture non vérifiée | `architecture.dependency_rule` dans `ctest` |

## À retenir

- Un défaut de conception se nomme par le **principe** qu'il viole ; c'est ce
  nom qui dit comment le corriger.
- La plupart des défauts de l'ancienne version se résument à deux fautes :
  **de la présentation dans le cœur**, et **des invariants accessibles en
  écriture**.
- Refactoriser, c'est **garder ce qui est juste** : le premier clic sûr, le
  *flood fill* itératif et les tests du domaine ont survécu.
- Chaque correction doit laisser un **garde-fou** derrière elle : un test, un
  `= delete`, une règle d'architecture.

## Exercices

1. Reprenez l'ancien `CellToken = std::string`. Écrivez le test qui aurait
   dû échouer et qui ne pouvait pas être écrit : « SDL affiche une mine là où
   la console affiche `*` ». Pourquoi est-il inutile avec `CellView` ?
2. L'ancienne version stockait `_lost` et `state_`. Trouvez une séquence de
   commandes qui les faisait diverger, puis montrez pourquoi `Game::status()`
   calculé rend cette séquence impossible.
3. Parmi les douze défauts, lesquels auraient été détectés par le test
   d'architecture de l'[étape 8](08-modules.md), et lesquels ne le seraient
   jamais ? Qu'est-ce que cela dit des limites d'un tel test ?
