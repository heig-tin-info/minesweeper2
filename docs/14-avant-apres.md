# Avant / après

> **Objectif.** Relire les défauts de la version précédente de ce projet et
> comprendre, pour chacun, **quel principe** il violait et **comment** il a
> été corrigé. C'est le meilleur résumé des chapitres précédents.

La version précédente avait la bonne intention : séparer le cœur des
interfaces par des ports. Mais plusieurs détails annulaient une partie des
bénéfices promis. Le README annonçait du « CQRS » et des « DTO » ; le code
contenait surtout du CQS et des chaînes de caractères.

## 1. Un DTO qui transportait de la présentation

| Avant | Après |
| --- | --- |
| `using CellToken = std::string;` contenant `"."`, `"?"`, `"3"`, `"*"` | `CellView`, un `std::variant` de types sémantiques |
| Le domaine choisissait le caractère (`Cell::getSymbol()`) | Le domaine dit ce que le joueur **sait**. Chaque adaptateur choisit l'apparence |
| SDL décodait les caractères de la console (`classifyToken`) | `std::visit`, vérifié par le compilateur |

**Principe :** un port ne doit pas être couplé à un adaptateur particulier.
→ [Chapitre 9](09-projection-dto.md)

## 2. Un HUD formaté dans le cœur

| Avant | Après |
| --- | --- |
| `struct HudSnapshot` avec deux `std::string` déjà formatées | `std::chrono::seconds elapsed()`, `int minesLeft()` |
| `<iomanip>` et `<sstream>` dans le cœur | Le formatage est fait par chaque adaptateur |

**Principe :** le cœur fournit des **valeurs**, les adaptateurs les
**présentent**. → [Chapitre 9](09-projection-dto.md)

## 3. « CQRS » qui était du CQS

Un même objet (`GameCore`) répondait aux commandes et aux requêtes, à partir
du même état. C'est de la séparation commande/requête (CQS), une excellente
pratique, mais ce n'est pas l'architecture CQRS.

**Principe :** nommer les choses exactement. → [Chapitre 8](08-commandes-requetes.md)

## 4. Un agrégat poreux

| Avant | Après |
| --- | --- |
| `Cell` publique avec `setBomb()`, `setRevealed()` | `Minefield` immuable, `Mark` privé dans `Game` |
| `Board::operator()` renvoyait un `Cell&` modifiable | Aucun accès en écriture hors des commandes |
| `Board::revealAll()` public, cassait `won()` | La fin de partie est une **projection**, rien n'est modifié |
| `GameCore::cell()` public exposait `bomb()` | Les mines cachées ne sortent jamais du domaine |

**Principe :** l'encapsulation protège des **invariants**. Une seule méthode
publique qui permet de les casser suffit à les rendre fragiles.
→ [Chapitres 4](04-encapsulation-invariants.md) et [5](05-agregat.md)

## 5. Deux sources de vérité

| Avant | Après |
| --- | --- |
| `Board::_lost` **et** `GameCore::state_` | `Game::status()` **calculé**, jamais stocké |
| Victoire testée deux fois | Une seule règle |
| Révélation de fin de partie faite deux fois | Aucune : c'est une projection |
| La fin de partie était contrôlée par `GameCore`, pas par `Board` | `Game` refuse lui-même les commandes |

**Principe :** SSOT, une information et un propriétaire. → [Chapitre 5](05-agregat.md)

## 6. Un bug de durée de vie

```cpp
class GameCore {
    BoardPresenter presenter_;   // contient « const GameCore& game » → *this
};
GameCore b = a;   // compile ! b.board() lit... a.
```

Un membre référence interdit l'affectation, mais **pas** la construction par
copie. La copie gardait une référence vers l'original : si `a` était détruit,
la référence devenait pendante.

**Correction :** plus aucune référence vers soi-même. `Session` supprime
explicitement sa copie, parce qu'une session a une identité.
→ [Chapitres 5](05-agregat.md) et [10](10-couche-application.md)

## 7. Des intentions mal typées

| Avant | Après |
| --- | --- |
| `enum class Action` + `std::optional<Pos>` | un type par commande dans un `std::variant` |
| `Reveal` sans position représentable | impossible à écrire |
| `Quit`, `Redraw` dans le port du cœur | `Quit` appartient à la console seule |

**Principe :** rendre les états illégaux impossibles à représenter.
→ [Chapitre 8](08-commandes-requetes.md)

## 8. Deux systèmes de coordonnées

`Pos{x, y}` côté ports, `Coord{r, c}` côté domaine, avec des inversions
manuelles (`cellAt(col, row)` qui construisait `Coord{row, col}`), et des
`int` mêlés à des `size_t` au point qu'une structure stockait les deux
versions.

**Correction :** une seule `Pos{row, col}`, un seul type d'indice (`int`).
Chaque adaptateur traduit **son** système (notation d'échecs, pixels).
→ [Chapitre 3](03-objets-valeur.md)

## 9. Le hasard et le temps câblés en dur

| Avant | Après |
| --- | --- |
| `std::random_device` en argument par défaut de `Board` | port `MineLayout`, graine injectée (`--seed`) |
| `Stopwatch` lisait `steady_clock` | `Stopwatch` pur + port `Clock` |
| Tests obligés de fouiller les mines | `FixedMineLayout`, `FakeClock` |

**Principe :** inversion et injection de dépendances.
→ [Chapitre 7](07-polymorphisme-ports.md)

## 10. Un `main` à `#ifdef`

Un seul `main.cpp`, deux boucles différentes selon `-DSDL`, et deux
interfaces sans API commune (`wait_intent()` d'un côté, `pollIntent()` de
l'autre).

**Correction :** un point de composition partagé (`launch`), deux `main` de
8 lignes, chaque adaptateur possède **sa** boucle derrière `run(view, sink)`.
→ [Chapitre 11](11-adaptateurs.md)

## 11. Initialisation en deux temps

`SDLUI` pouvait exister sans fenêtre (`ready() == false`), et `teardown()`
libérait tout à la main.

**Correction :** RAII avec `unique_ptr` et deleters, un constructeur qui
réussit ou lance une exception, et la règle du zéro.
→ [Chapitre 12](12-raii.md)

## 12. Une architecture non vérifiée

Rien n'empêchait d'inclure SDL dans le cœur. Un en-tête unique
(`minesweeper_headers`) servait à tout le monde.

**Correction :** une bibliothèque CMake par couche, et un test
d'architecture qui échoue dès qu'un `#include` viole la règle de dépendance.
→ [Chapitre 13](13-tests.md)

## Ce qui a été gardé

La version précédente avait aussi de bonnes idées, conservées ici :

- le placement des mines **au premier clic**, avec une ouverture de 3 × 3 ;
- un *flood fill* **itératif** plutôt que récursif ;
- une grille stockée dans un **vecteur plat** ;
- la séparation entre **port d'entrée** et **port de lecture** ;
- des **tests** sur le domaine.

Une bonne refactorisation garde ce qui est juste et ne corrige que ce qui ne
l'est pas.
