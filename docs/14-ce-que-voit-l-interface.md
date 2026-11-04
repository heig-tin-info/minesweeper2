# Ce que voit l'interface

> **Objectif.** Concevoir les données qui **sortent** du cœur : ce que
> l'interface a le droit de savoir d'une case, ni plus (fuite d'information),
> ni sous une forme qui l'oblige à décoder.
>
> **Code :** [src/core/cell_view.hpp](../src/core/cell_view.hpp),
> `Game::cell` dans [src/core/game.cpp](../src/core/game.cpp)

Cette étape correspond à l'étape 5 du [labo](../steps/README.md).

## Le besoin

Les règles sont écrites, mais invisibles. L'interface (qu'on écrira à
l'[étape 18](18-terminal.md)) devra dessiner chaque case. Que lui donner ?
Une contrainte forte : elle ne doit pas pouvoir **tricher**. Tant que la
partie n'est pas finie, rien de ce qui sort de `Game` ne doit permettre de
savoir où sont les mines cachées. Et quand la partie est perdue, le jeu doit
pouvoir montrer les mines et les drapeaux faux **sans modifier** l'état.

## Les options

**A. Exposer l'intérieur.** Renvoyer un accès à `Minefield` ou à la case
interne, avec `hasMine()`, `revealed()`, `flagged()`. L'interface sait alors
**où sont toutes les mines**. Un bug d'affichage, et le joueur les voit. Dans
une version réseau, un joueur malin les lirait dans les données envoyées au
navigateur.

**B. Un `enum` plus un entier.**

```cpp
enum class Kind { Hidden, Flagged, Revealed, Mine, WrongFlag };
struct CellInfo { Kind kind; int adjacent; bool exploded; };
```

Trois champs dont deux ne veulent rien dire la plupart du temps. On peut
écrire `{Kind::Hidden, 3, true}` : une case cachée, avec trois voisines et
qui a explosé ? Le code qui reçoit doit tout re-vérifier.

**C. Un caractère.** Renvoyer `"."`, `"?"`, `"3"`, `"*"`. C'était la version
précédente de ce projet. La console l'affiche tel quel, mais l'interface
graphique doit **décoder** le caractère pour choisir une image :

```cpp
// Ancien code SDL : on « parse » la convention d'affichage de la console
switch (token.front()) {
    case '.': return CellKind::Hidden;
    case '?': return CellKind::Flagged;
    case '*': return CellKind::Bomb;
}
```

Les données font un aller-retour absurde : **sens → caractère → sens**.
Troisième critère : **qui dépend de qui ?** Ici le cœur choisissait le
caractère : il dépendait de la présentation.

**D. Un type par situation**, réunis dans un `std::variant`. C'est l'option
retenue.

## La décision

```cpp
namespace cell {
struct Hidden {};
struct Flagged {};
struct Revealed { int adjacentMines; };   // a safe square the player opened
struct Mine { bool exploded; };           // shown once the game is lost
struct WrongFlag {};                      // a flag on a safe square, once lost
}

using CellView = std::variant<cell::Hidden, cell::Flagged, cell::Revealed,
                              cell::Mine, cell::WrongFlag>;
```

`CellView` dit **ce que le joueur sait** de la case, dans le vocabulaire du
jeu. Il ne dit rien de sa **forme** à l'écran : ni caractère, ni couleur, ni
image. Chaque adaptateur décidera.

### La projection, calculée par `Game`

```cpp
CellView Game::cell(Pos p) const {
    require(p);
    const Mark mark = marks_[p];
    if (mark == Mark::Revealed) {
        if (field_->hasMine(p)) return cell::Mine{.exploded = true};
        return cell::Revealed{field_->adjacentMines(p)};
    }

    const Status s = status();
    if (!isOver(s)) {
        if (mark == Mark::Flagged) return cell::Flagged{};
        return cell::Hidden{};
    }

    // Game over: the projection now tells the whole truth. Nothing is
    // mutated to show it.
    const bool mine = field_->hasMine(p);
    if (mark == Mark::Flagged) {
        if (mine) return cell::Flagged{};
        return cell::WrongFlag{};
    }
    if (!mine) return cell::Hidden{};
    if (s == Status::Won) return cell::Flagged{};
    return cell::Mine{.exploded = false};
}
```

Pendant la partie, une mine cachée ressort comme `cell::Hidden{}` :
l'information **ne sort pas** du cœur. Une fois la partie terminée, la même
méthode en dit davantage, **sans rien modifier**. La version précédente du
projet faisait l'inverse : un `revealAll()` passait toutes les cases à
« révélée », détruisant l'information « qu'avait réellement ouvert le
joueur ? ». Afficher n'est pas modifier.

## Les concepts

### Projection et DTO

Une **projection** est une vue calculée sur un modèle, taillée pour un usage.
Le modèle interne (`Mark` + `Minefield`) n'est jamais exposé ; on n'en publie
qu'une image filtrée. `CellView` est un **DTO** (*Data Transfer Object*) : un
objet sans comportement, fait pour **traverser une frontière** entre couches.
Il est immuable, copiable par valeur, **autonome** (il ne pointe pas vers le
modèle : on peut le garder sans risque de référence pendante) et **minimal**
(il ne contient que ce que le client a le droit de savoir).

### Type somme

Un `std::variant<A, B, C>` contient **une** valeur, qui est **soit** un `A`,
**soit** un `B`, **soit** un `C`, et il sait toujours laquelle. C'est un
**type somme**, par opposition à une `struct` qui contient un `A` **et** un
`B` **et** un `C` (type produit). L'option B était un type produit : elle
permettait toutes les combinaisons, y compris les absurdes.

### États illégaux irreprésentables

Chaque alternative porte **exactement** les données dont elle a besoin : un
`Revealed` a un nombre de voisines, un `Mine` sait s'il a explosé, un `Hidden`
ne porte rien. Un « caché avec trois voisines » **ne compile pas**. Toute une
famille de vérifications à l'exécution disparaît, remplacée par le système de
types. C'est un principe général, qu'on retrouvera pour les commandes à
l'[étape 15](15-commandes.md).

### `std::visit` : traiter chaque cas

Pour agir selon l'alternative contenue, on utilise `std::visit` avec une
fonction par cas. Voici la première rencontre, dans l'adaptateur console de
l'étape 18 ([renderer.cpp](../src/adapters/console/renderer.cpp)) :

```cpp
Glyph glyphOf(const CellView& view) {
    return std::visit(overloaded{
        [](cell::Hidden) { return Glyph{".", "\033[90m"}; },
        [](cell::Flagged) { return Glyph{"?", "\033[33m"}; },
        [](cell::Revealed r) {
            return Glyph{kDigits[r.adjacentMines], kDigitColours[r.adjacentMines]};
        },
        [](cell::Mine m) {
            return m.exploded ? Glyph{"X", "\033[41;97;1m"} : Glyph{"*", "\033[31;1m"};
        },
        [](cell::WrongFlag) { return Glyph{"!", "\033[35;1m"}; },
    }, view);
}
```

`overloaded` est un petit utilitaire qui réunit plusieurs lambdas en un seul
objet appelable ([overloaded.hpp](../src/core/overloaded.hpp)). L'avantage
décisif sur un `switch` : si l'on ajoute une alternative (par exemple
`cell::Questioned` pour le marquage « ? » de Windows), chaque adaptateur qui
l'oublie **refuse de compiler**. Avec des caractères, l'oubli serait passé
inaperçu jusqu'à ce qu'un joueur voie une case mal dessinée. C'est le patron
**Visiteur** du *Gang of Four*, sans aucune classe à écrire.

## Vérifier

Les tests de [game_tests.cpp](../tests/game_tests.cpp) comparent directement
des `CellView` : un `variant` d'objets valeur se compare avec `==`, alternative
et contenu compris.

```cpp
TEST(Game, HittingAMineLosesAndShowsTheTruth) {
    FixedMineLayout layout{{{0, 1}}};
    Game game{{3, 3}, 1, layout};
    game.reveal({1, 1});
    game.toggleFlag({2, 2});   // wrongly
    game.reveal({0, 1});       // steps on the mine

    EXPECT_EQ(game.status(), Status::Lost);
    EXPECT_EQ(game.cell({0, 1}), CellView{cell::Mine{.exploded = true}});
    EXPECT_EQ(game.cell({2, 2}), CellView{cell::WrongFlag{}});
    EXPECT_EQ(game.cell({0, 0}), CellView{cell::Hidden{}});
}
```

| Test | Ce qu'il prouve sur la projection |
| --- | --- |
| `StartsReadyWithEverythingHidden` | une mine cachée ressort `Hidden` |
| `RevealingANumberOpensOnlyThatCell` | `Revealed{1}` ici, `Hidden` à côté |
| `HittingAMineLosesAndShowsTheTruth` | la mine qui a sauté, le drapeau faux, une case sûre non ouverte restée `Hidden` |
| `LosingShowsTheOtherMinesUnexploded` | `Mine{.exploded = false}` pour les autres |
| `WinningFlagsTheRemainingMines` | en cas de victoire, les mines ressortent `Flagged` |

Aucun de ces tests ne sait comment une case sera dessinée : on vérifie le
**sens**, pas la forme.

## À retenir

- Ce qui sort du cœur est une **projection** : une vue filtrée, jamais l'état
  interne.
- Un DTO porte du **sens**, pas de la **présentation**. Le cœur ne choisit ni
  caractère, ni couleur, ni format.
- **`std::variant`** : une valeur parmi plusieurs types, chacun avec
  exactement ses données. Les états absurdes ne compilent pas.
- **`std::visit`** traite chaque cas, et le compilateur vérifie qu'aucun n'est
  oublié.
- Si un client doit « parser » ce que vous lui donnez, le type est mal choisi.

## Exercices

1. Pourquoi `cell::Mine` porte-t-il un booléen `exploded` plutôt que d'avoir
   deux types `Mine` et `ExplodedMine` ? Les deux se défendent : lequel
   préférez-vous, et pourquoi ?
2. Une version réseau enverrait des `CellView` au navigateur en JSON.
   Pourquoi est-ce sûr, alors qu'envoyer les cases internes ne l'était pas ?
3. Ajoutez `struct Questioned {};` au `variant` sans rien changer d'autre, et
   compilez. Quels fichiers refusent de compiler ? Est-ce une bonne nouvelle ?
