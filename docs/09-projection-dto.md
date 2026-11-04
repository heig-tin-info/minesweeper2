# Projection et DTO : ce que l'interface a le droit de voir

> **Objectif.** Concevoir les données qui **sortent** du domaine : ni trop
> (fuite d'information), ni sous une forme qui oblige à les décoder.
>
> **Code :** [src/core/cell_view.hpp](../src/core/cell_view.hpp)

## Le problème

L'interface doit dessiner chaque case. Que faut-il lui donner ?

**Option A : la cellule interne.** On renvoie un `const Cell&` avec `bomb()`,
`revealed()`, `flagged()`. L'interface sait alors **où sont toutes les mines**,
même cachées. Un bug d'affichage, et le joueur les voit. Dans une version
web, un joueur malin les lirait dans les données envoyées au navigateur.

**Option B : un caractère.** On renvoie `"."`, `"?"`, `"3"`, `"*"`. C'est la
solution de la version précédente de ce projet. L'interface console affiche le
caractère tel quel. Mais l'interface graphique doit **décoder** ce caractère
pour choisir une couleur ou une image :

```cpp
// Ancien code SDL : on « parse » la convention d'affichage de la console
switch (token.front()) {
    case '.': return CellKind::Hidden;
    case '?': return CellKind::Flagged;
    case '*': return CellKind::Bomb;
    ...
}
```

Les données font un aller-retour absurde :
**sens → caractère console → sens**. Si quelqu'un change `'?'` en `'F'` pour
la console, l'interface SDL casse. Et c'était le domaine (`Cell::getSymbol()`)
qui choisissait le caractère : le cœur dépendait de la présentation.

## La solution : une projection sémantique

```cpp
namespace cell {
struct Hidden {};
struct Flagged {};
struct Revealed { int adjacentMines; };
struct Mine { bool exploded; };   // seulement en fin de partie perdue
struct WrongFlag {};              // seulement en fin de partie perdue
}

using CellView = std::variant<cell::Hidden, cell::Flagged, cell::Revealed,
                              cell::Mine, cell::WrongFlag>;
```

`CellView` dit **ce que le joueur sait** de la case, dans le vocabulaire du
jeu. Il ne dit rien de sa **forme** à l'écran : ni caractère, ni couleur, ni
image. Chaque adaptateur décide lui-même de l'apparence.

Une **projection**, c'est une vue calculée sur un modèle, taillée pour un
usage. Le modèle interne (`Mark` + `Minefield`) n'est jamais exposé. On n'en
publie qu'une image filtrée.

## DTO : un objet de transfert

`CellView` est un **DTO** (*Data Transfer Object*) : un objet sans
comportement, fait pour **traverser une frontière** entre couches. Ses
qualités :

- **immuable** et copiable par valeur ;
- **autonome** : il ne pointe pas vers le modèle, on peut le garder sans
  risque de référence pendante ;
- **minimal** : il ne contient que ce que le client a le droit de savoir.

Pendant la partie, `Game::cell()` ne renvoie **jamais** `cell::Mine` pour une
mine cachée. L'information ne sort tout simplement pas du domaine.

## Le patron Visiteur, version moderne

Chaque adaptateur doit traiter les cinq cas. Le patron de conception
**Visiteur** (*Gang of Four*) répond à ce besoin pour une **hiérarchie de
classes** : chaque classe a une méthode `accept(Visitor&)`, et le visiteur une
méthode `visit()` par type. C'est verbeux, et réservé aux hiérarchies avec
`virtual`.

Avec un `std::variant`, `std::visit` **est** le visiteur, sans aucune classe à
écrire. Voici l'adaptateur console
([renderer.cpp](../src/adapters/console/renderer.cpp)) :

```cpp
Glyph glyphOf(const CellView& view) {
    return std::visit(overloaded{
        [](cell::Hidden)     { return Glyph{".", "\033[90m"}; },
        [](cell::Flagged)    { return Glyph{"?", "\033[33m"}; },
        [](cell::Revealed r) { return Glyph{kDigits[r.adjacentMines], kDigitColours[r.adjacentMines]}; },
        [](cell::Mine m)     { return m.exploded ? Glyph{"X", ...} : Glyph{"*", ...}; },
        [](cell::WrongFlag)  { return Glyph{"!", "\033[35;1m"}; },
    }, view);
}
```

Et l'adaptateur SDL ([sdl_ui.cpp](../src/adapters/sdl/sdl_ui.cpp)) :

```cpp
std::visit(overloaded{
    [&](cell::Hidden)     { fill(tile, kGrass[shade]); },
    [&](cell::Flagged)    { fill(tile, kGrass[shade]); blit(glyphs_.flag.get(), cx, cy); },
    [&](cell::Revealed r) { fill(tile, kSand[shade]); /* chiffre */ },
    [&](cell::Mine m)     { fill(tile, m.exploded ? kExploded : kSand[shade]); drawMine(cx, cy); },
    [&](cell::WrongFlag)  { fill(tile, kSand[shade]); blit(glyphs_.wrongFlag.get(), cx, cy); },
}, cell);
```

Même donnée, deux rendus totalement différents, **aucun décodage**. Et si l'on
ajoute un état (par exemple `cell::Questioned` pour le marquage « ? » de
Windows), chaque adaptateur qui l'oublie **refuse de compiler**. Avec des
caractères, l'oubli serait passé inaperçu jusqu'à ce qu'un joueur voie une case
mal dessinée.

## Le HUD : des valeurs, pas du texte

Même raisonnement pour le temps et le compteur de mines. L'ancienne version
fournissait des chaînes déjà formatées :

```cpp
struct HudSnapshot { std::string time_mmss; std::string count; };
```

Le cœur incluait `<iomanip>` pour fabriquer `"03:07"`, et une interface qui
aurait voulu afficher le temps en chiffres lumineux aurait dû re-parser la
chaîne. Désormais, le port expose des **valeurs** :

```cpp
virtual int minesLeft() const = 0;
virtual std::chrono::seconds elapsed() const = 0;
```

Le formatage (`std::format("{:02}:{:02}", ...)`) est fait par chaque
adaptateur, qui est seul à savoir comment il affiche.

## À retenir

- Ce qui sort du domaine est une **projection** : une vue filtrée, jamais
  l'état interne.
- Un DTO porte du **sens**, pas de la **présentation**. Le domaine ne choisit
  ni caractère, ni couleur, ni format.
- **`std::variant` + `std::visit`** remplacent le patron Visiteur classique, et
  le compilateur vérifie que tous les cas sont traités.
- Si un client doit « parser » ce que vous lui donnez, le type est mal choisi.

## Pour réfléchir

1. Pourquoi `cell::Mine` porte-t-il un booléen `exploded` plutôt que d'avoir
   deux types `Mine` et `ExplodedMine` ? Les deux se défendent : lequel
   préférez-vous ?
2. Une version réseau enverrait des `CellView` au navigateur en JSON.
   Pourquoi est-ce sûr, alors qu'envoyer les `Cell` internes ne l'était pas ?
