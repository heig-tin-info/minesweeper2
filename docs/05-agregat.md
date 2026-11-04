# L'agrégat `Game`

> **Objectif.** Savoir **où placer une règle**. Comprendre pourquoi
> `cell.reveal()` est une fausse bonne idée, et ce qu'est une source unique de
> vérité.
>
> **Code :** [src/core/game.hpp](../src/core/game.hpp),
> [src/core/game.cpp](../src/core/game.cpp)

## Le problème : la cellule doit-elle savoir se révéler ?

Le réflexe « objet » est de donner à chaque chose ses propres méthodes. On
imagine volontiers :

```cpp
grid.cell(2, 3).reveal();
```

Mais que se passe-t-il vraiment quand on révèle une case ?

1. Si c'est le **premier clic**, il faut d'abord **poser les mines**, ailleurs.
2. Si la case est vide, il faut **ouvrir toute la zone** autour (les voisins,
   puis leurs voisins...).
3. Il faut **décompter** les cases sûres restantes.
4. Il faut **passer en « gagné » ou « perdu »**.
5. Il faut **refuser** l'action si la partie est déjà terminée.

Aucune de ces règles ne concerne *une* case : elles concernent **la partie
entière**. Une cellule qui porte `reveal()` n'a que deux options :

- **contourner** ces règles, et l'état devient incohérent ;
- ou **rappeler la grille**, et elle n'est plus qu'un intermédiaire inutile.

Dans le second cas, il lui faut une référence vers la grille et ses propres
coordonnées. En C++, c'est un piège :

- les cellules vivent dans un `std::vector` ; copier ou déplacer la grille
  laisse des milliers de **pointeurs vers l'ancienne grille** ;
- la position est **stockée deux fois** : l'indice dans le vecteur, et le
  champ de la cellule. Les deux peuvent diverger ;
- `Cell` et `Grid` **dépendent l'une de l'autre** (cycle).

## La notion d'agrégat

Un **agrégat** est un groupe d'objets qui doivent rester cohérents
**ensemble**. On y accède par une seule porte, la **racine d'agrégat**, qui
garantit toutes les règles du groupe. Rien d'extérieur ne touche
directement aux objets internes.

Ici, la racine est `Game`. Ce qu'elle contient :

```cpp
class Game {
public:
    Game(Size size, int mines, MineLayout& layout);

    void reveal(Pos p);        // commandes : les seules façons de changer une partie
    void toggleFlag(Pos p);

    Size size() const;         // requêtes : lecture seule
    int minesLeft() const;
    Status status() const;
    CellView cell(Pos p) const;

private:
    enum class Mark : std::uint8_t { Hidden, Flagged, Revealed };

    MineLayout* layout_;
    int mines_;
    Grid<Mark> marks_;                // ce que le joueur a fait
    std::optional<Minefield> field_;  // ce que le terrain cache
    std::optional<Pos> exploded_;
    int flags_ = 0;
    int hiddenSafe_;
};
```

Deux grilles distinctes, deux natures distinctes :

| | `Minefield` | `marks_` |
| --- | --- | --- |
| Représente | la vérité du terrain | la connaissance du joueur |
| Change ? | jamais (immuable) | à chaque commande |
| Existe | à partir du 1er clic | dès le début |

C'est ce découpage qui permet de **poser un drapeau avant le premier clic** :
les marques existent avant que le terrain ne soit miné.

## Les règles sont dans la racine

```cpp
void Game::reveal(Pos p) {
    require(p);
    if (isOver(status()) || marks_[p] != Mark::Hidden) return;

    const Minefield& field = field_ ? *field_ : plant(p);
    if (field.hasMine(p)) {
        marks_[p] = Mark::Revealed;
        exploded_ = p;
        return;
    }
    open(p);
}
```

En une dizaine de lignes, on lit toute la règle : vérifier la position,
ignorer si la partie est finie ou la case déjà jouée, poser les mines au
premier clic, perdre sur une mine, sinon ouvrir.

## Une seule source de vérité

La version précédente de ce projet stockait l'état de la partie **à deux
endroits** : un booléen `_lost` dans le plateau, et un `state_` dans le
contrôleur. Deux informations qui disent la même chose finissent toujours par
se contredire.

Ici, le statut **n'est pas stocké** : il est **calculé** à partir des faits.

```cpp
Status Game::status() const {
    if (exploded_) return Status::Lost;
    if (!field_) return Status::Ready;
    return hiddenSafe_ == 0 ? Status::Won : Status::Playing;
}
```

C'est le principe **SSOT** (*Single Source Of Truth*) : chaque information a
**un seul** propriétaire. Tout le reste en est déduit. Le statut ne peut pas
être faux, puisqu'il n'est écrit nulle part.

> `hiddenSafe_` est lui aussi déductible (on pourrait compter les cases), mais
> le recompter à chaque appel coûterait cher. C'est un **cache** : il est
> maintenu à jour par `open()`, et comme il est privé, personne d'autre ne peut
> le désynchroniser.

## La fin de partie est une projection, pas une mutation

Quand on perd, le jeu montre toutes les mines. La version précédente le
faisait en **modifiant** le plateau (`revealAll()` passait toutes les cases à
« révélée »). Résultat : l'information « qu'est-ce que le joueur avait
réellement ouvert ? » était détruite, et la méthode publique `revealAll()`
pouvait casser les invariants si on l'appelait au mauvais moment.

Ici, rien n'est modifié. C'est `cell()` qui, **une fois la partie
terminée**, montre davantage :

```cpp
CellView Game::cell(Pos p) const {
    ...
    if (!isOver(s)) { /* le joueur ne voit que ses marques */ }

    // Partie terminée : la projection dit toute la vérité.
    const bool mine = field_->hasMine(p);
    if (mark == Mark::Flagged) {
        if (mine) return cell::Flagged{};
        return cell::WrongFlag{};
    }
    ...
}
```

On voit la même idée qu'avec le statut : **afficher n'est pas modifier**.
Le [chapitre 9](09-projection-dto.md) détaille ce qu'est une projection.

## Algorithme : l'ouverture d'une zone

```cpp
void Game::open(Pos start) {
    std::vector<Pos> pending{start};
    while (!pending.empty()) {
        const Pos p = pending.back();
        pending.pop_back();
        if (marks_[p] != Mark::Hidden) continue;

        marks_[p] = Mark::Revealed;
        --hiddenSafe_;
        if (field_->adjacentMines(p) != 0) continue;
        for (Pos n : Neighbors{size(), p})
            if (marks_[n] == Mark::Hidden) pending.push_back(n);
    }
}
```

C'est un parcours de graphe (*flood fill*). La version **récursive** est plus
courte à écrire, mais chaque case vide ajoute un appel sur la pile. Sur une
grille de 1000 × 1000 presque vide, c'est un million d'appels imbriqués : la
pile déborde et le programme plante. Avec une **pile explicite**
(`std::vector`), la mémoire vient du tas, qui est bien plus grand. Le test
`Game.FloodFillHandlesHugeEmptyGrids` le vérifie.

## Et l'API `grid.cell(2, 3).reveal()` ?

Elle reste possible, sans aucun des défauts cités plus haut, grâce à une
**poignée** (*handle*). C'est un petit objet temporaire qui contient une
référence à la partie et une position, et qui **délègue** :

```cpp
class CellRef {
    Game* game_;
    Pos pos_;
public:
    void reveal() { game_->reveal(pos_); }
    CellView view() const { return game_->cell(pos_); }
};
```

Rien n'est stocké dans la grille. La poignée est créée à la demande et jetée
aussitôt, comme un itérateur. La règle reste dans `Game`. Ce projet ne
l'implémente pas, car aucun appelant n'en a besoin : les adaptateurs envoient
des commandes (`Reveal{p}`). Mais c'est la bonne façon de faire si l'on veut
cette syntaxe.

## À retenir

- Une règle qui concerne **plusieurs objets** appartient à celui qui les
  **possède tous** : la racine d'agrégat.
- La cellule est une **donnée**, pas un acteur. Pas de `reveal()`, pas de
  coordonnées, pas de pointeur vers la grille.
- **Une information, un propriétaire.** Ce qui peut être calculé ne doit pas
  être stocké, ou alors comme un cache privé.
- **Afficher n'est pas modifier** : la fin de partie se montre par projection.

## Pour réfléchir

1. Ajoutez mentalement l'action « *chord* » (clic sur un chiffre dont tous les
   drapeaux sont posés, qui ouvre les voisins restants). Dans quelle classe
   va-t-elle ? Quels membres utilise-t-elle ?
2. Pourquoi `layout_` est-il un pointeur et non une référence ?
   (Indice : l'affectation `game_ = Game{...}` dans `Session`.)
