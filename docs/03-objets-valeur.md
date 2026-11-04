# Objets valeur : `Pos`, `Size`, `Neighbors`

> **Objectif.** Savoir reconnaître un *objet valeur*, et savoir quand utiliser
> `struct` plutôt que `class`.
>
> **Code :** [src/core/position.hpp](../src/core/position.hpp)

## Le problème

Une case se désigne par une ligne et une colonne. On pourrait passer deux
`int` partout :

```cpp
void reveal(int row, int col);
int index = row * cols + col;           // répété dans chaque classe
if (r >= 0 && r < rows && c >= 0 && c < cols) ...  // répété aussi
```

Trois défauts : on confond facilement `(row, col)` et `(x, y)`, la même
arithmétique est recopiée partout, et rien n'indique au lecteur que ces deux
entiers forment *un seul concept*.

## La solution : nommer les concepts

```cpp
struct Pos {
    int row;
    int col;

    friend constexpr bool operator==(Pos, Pos) = default;
};

struct Size {
    int rows;
    int cols;

    constexpr int area() const { return rows * cols; }
    constexpr bool contains(Pos p) const { ... }
    constexpr int indexOf(Pos p) const { return p.row * cols + p.col; }
    constexpr Pos posOf(int index) const { return {index / cols, index % cols}; }
};
```

Maintenant `reveal(Pos p)` dit exactement ce qu'il attend, et toute
l'arithmétique de grille vit **à un seul endroit**.

## Qu'est-ce qu'un objet valeur ?

Un objet valeur (*value object*) est défini **uniquement par sa valeur**,
pas par une identité :

- deux `Pos{2, 3}` sont **interchangeables** : on se moque de savoir « lequel »
  on a ;
- on le **copie** librement, on le passe **par valeur** (il est petit) ;
- il est comparé avec `==` sur son contenu.

Par opposition, une **entité** a une identité qui persiste quand son état
change : une partie (`Game`) reste « la même partie » quand on y révèle une
case. On ne compare pas deux parties avec `==`.

| | Objet valeur | Entité |
| --- | --- | --- |
| Exemple | `Pos`, `Size`, `CellView` | `Game`, `Session` |
| Égalité | par contenu | par identité |
| Copie | libre, naturelle | à réfléchir (voire interdite) |
| Passage | par valeur | par référence |

## `struct` ou `class` ?

En C++, la seule différence technique est la visibilité par défaut
(`public` pour `struct`, `private` pour `class`). La convention utile est :

- **`struct`** quand **toutes les combinaisons de valeurs sont valides**. Une
  `Pos{-1, 7}` est une position, simplement hors de la grille. Il n'y a rien à
  protéger, donc rien à cacher.
- **`class`** quand il existe un **invariant** à garantir : une règle que les
  données doivent toujours respecter. On cache alors les données derrière des
  méthodes. C'est le sujet du [chapitre suivant](04-encapsulation-invariants.md).

Écrire des accesseurs `getRow()` / `setRow()` pour `Pos` n'apporterait rien :
le *setter* laisserait tout passer, il n'y a donc aucune encapsulation, juste du
bruit.

## L'égalité par défaut

```cpp
friend constexpr bool operator==(Pos, Pos) = default;
```

Depuis C++20, le compilateur génère la comparaison membre à membre. Pas de
code à écrire, donc pas de bug possible (comme oublier un champ ajouté plus
tard). `!=` est déduit automatiquement.

## `constexpr` : du code qui tourne à la compilation

Toutes les méthodes de `Size` sont `constexpr`. Elles peuvent donc être
évaluées par le compilateur :

```cpp
static_assert(Size{3, 4}.indexOf({1, 2}) == 6);   // vérifié à la compilation
```

Pour des fonctions aussi simples, c'est gratuit, et ça documente qu'elles
n'ont aucun effet de bord.

## `Neighbors` : un type itérable sans allocation

Les voisins d'une case reviennent partout : compter les mines, propager
l'ouverture d'une zone, protéger le premier clic. Plutôt qu'une fonction qui
renvoie un `std::vector` (une allocation à chaque appel), `Neighbors` est un
petit objet valeur qui **se parcourt** :

```cpp
class Neighbors {
public:
    constexpr Neighbors(Size size, Pos center) { /* remplit items_ */ }
    constexpr auto begin() const { return items_.begin(); }
    constexpr auto end() const { return items_.begin() + count_; }
private:
    std::array<Pos, 8> items_{};
    int count_ = 0;
};
```

Une boucle `for (x : range)` n'exige que deux méthodes, `begin()` et `end()`.
En les fournissant, `Neighbors` s'utilise comme n'importe quel conteneur :

```cpp
for (Pos n : Neighbors{size, mine}) ++squares_[n].adjacent;
```

Ici on utilise `class` : `items_` et `count_` doivent rester cohérents
(seules les `count_` premières cases sont valides). C'est un invariant, donc
on le protège.

> **Piège.** `begin()` et `end()` doivent venir **du même objet**. L'expression
>
> ```cpp
> std::vector<Pos>(Neighbors{s, p}.begin(), Neighbors{s, p}.end())
> ```
>
> construit deux objets temporaires différents : les itérateurs ne se correspondent pas.
> Le test `Neighbors.CornerIsClippedToTheGrid` a d'ailleurs été écrit ainsi par
> erreur, puis corrigé.

## Pourquoi `row`/`col` et pas `x`/`y` ?

Parce que c'est le **langage du domaine** : une grille a des lignes et des
colonnes. `x`/`y` est le langage d'un écran, où `y` descend. Chaque
adaptateur traduit vers son propre système : la console en notation
d'échecs (`B3`), SDL en pixels. Le domaine n'a qu'**un seul** système de
coordonnées.

## À retenir

- Donnez un **nom et un type** à chaque concept, même petit.
- **`struct`** pour une donnée sans invariant, **`class`** dès qu'il faut
  protéger une règle.
- Un objet valeur se **copie**, se **compare par contenu**, se passe **par valeur**.
- `begin()`/`end()` suffisent pour rendre un type utilisable dans une boucle `for`.

## Pour réfléchir

1. Pourquoi `Pos` n'a-t-il pas de constructeur qui refuse les coordonnées
   négatives ?
2. `Neighbors` stocke au plus 8 positions dans un `std::array`. Qu'est-ce qui
   garantit qu'il n'y en aura jamais 9 ?
