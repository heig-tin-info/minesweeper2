# Positions, taille et voisins

> **Objectif.** Écrire les premières briques du cœur : désigner une case,
> savoir si elle est dans la grille, parcourir ses voisines. Reconnaître un
> *objet valeur* et savoir quand `struct` suffit.
>
> **Code :** [src/core/position.hpp](../src/core/position.hpp)

À partir d'ici, chaque étape produit du code qui **compile et passe ses
tests** avant de passer à la suivante, et il n'y aura **aucun affichage**
avant l'[étape 18](18-terminal.md) : on construit de l'intérieur vers
l'extérieur, dans l'ordre du [labo](../steps/README.md). Cette étape
correspond à l'[étape 1 du labo](../steps/01-positions/).

## Le besoin

Tout, dans le démineur, parle de cases : dévoiler *celle-ci*, compter les
mines *autour de celle-là*, vérifier qu'un clic tombe *dans* la grille. Il
nous faut une façon de désigner une case, une façon de décrire la grille, et
une façon de parcourir les huit voisines d'une case sans sortir du tableau.

## Les options

| Option | Exemple | Ce qui coince |
| --- | --- | --- |
| Deux `int` | `reveal(int row, int col)` | on inverse `(row, col)` et `(x, y)` une fois sur deux ; rien ne dit que les deux entiers forment *un* concept |
| `std::pair<int, int>` | `p.first`, `p.second` | lequel est la ligne ? Le lecteur doit deviner |
| `struct Pos` | `reveal(Pos p)` | il faut écrire trois lignes de plus |

Avec deux `int`, l'arithmétique de grille se recopie aussi dans chaque classe :

```cpp
int index = row * cols + col;                      // répété partout
if (r >= 0 && r < rows && c >= 0 && c < cols) ...  // répété aussi
```

Deuxième critère : **a-t-on écrit deux fois la même chose ?** Oui. Et deux
copies d'un calcul finissent toujours par diverger.

## La décision

Nommer les concepts : une position, une taille, et un parcours des voisines.

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
    constexpr bool contains(Pos p) const {
        return 0 <= p.row && p.row < rows && 0 <= p.col && p.col < cols;
    }
    constexpr int indexOf(Pos p) const { return p.row * cols + p.col; }
    constexpr Pos posOf(int index) const { return {index / cols, index % cols}; }
    friend constexpr bool operator==(Size, Size) = default;
};
```

`reveal(Pos p)` dit exactement ce qu'il attend, et toute l'arithmétique de
grille vit **à un seul endroit**, dans `Size`.

Les voisines reviennent partout : compter les mines, ouvrir une zone, protéger
le premier clic. Plutôt qu'une fonction qui renvoie un `std::vector` (une
allocation à chaque appel), `Neighbors` est un petit objet qui **se parcourt** :

```cpp
class Neighbors {
public:
    constexpr Neighbors(Size size, Pos center) {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                const Pos n{center.row + dr, center.col + dc};
                if ((dr != 0 || dc != 0) && size.contains(n)) items_[count_++] = n;
            }
    }
    constexpr auto begin() const { return items_.begin(); }
    constexpr auto end() const { return items_.begin() + count_; }
    constexpr int size() const { return count_; }
private:
    std::array<Pos, 8> items_{};
    int count_ = 0;
};
```

```cpp
for (Pos n : Neighbors{size, mine}) ++squares_[n].adjacent;   // étape 10
```

Pourquoi `row`/`col` et pas `x`/`y` ? Parce que c'est le **langage du
domaine** (étape 1) : une grille a des lignes et des colonnes. `x`/`y` est le
langage d'un écran, où `y` descend. Chaque adaptateur traduira vers son
système : la console en notation d'échecs (`B3`), SDL en pixels. Le domaine
n'a qu'**un seul** système de coordonnées.

## Les concepts

### Objet valeur

Un objet valeur est défini **uniquement par sa valeur**, pas par une
identité. Deux `Pos{2, 3}` sont interchangeables ; on les copie librement, on
les passe par valeur, on les compare avec `==` sur leur contenu. À l'opposé,
une **entité** garde son identité quand son état change : la partie (étape
11) reste « la même partie » après un coup, et on ne compare pas deux parties
avec `==`. C'est le tri fait à l'étape 2.

| | Objet valeur | Entité |
| --- | --- | --- |
| Exemple | `Pos`, `Size`, `CellView` | `Game`, `Session` |
| Égalité | par contenu | par identité |
| Copie | libre | à réfléchir, voire interdite |
| Passage | par valeur | par référence |

### `struct` ou `class`

La seule différence technique est la visibilité par défaut. La convention
utile : **`struct`** quand **toutes les combinaisons de valeurs sont valides**
(`Pos{-1, 7}` est une position, simplement hors grille : rien à protéger) ;
**`class`** dès qu'il existe un **invariant**, une règle que les données
doivent toujours respecter. Des accesseurs `getRow()`/`setRow()` sur `Pos`
n'apporteraient rien : le *setter* laisserait tout passer. `Neighbors`, lui,
est une `class` : seules les `count_` premières cases de `items_` sont
valides, et c'est un invariant.

### Égalité par défaut et `constexpr`

```cpp
friend constexpr bool operator==(Pos, Pos) = default;
```

Depuis C++20, le compilateur génère la comparaison membre à membre : pas de
code, donc pas de champ oublié. `!=` est déduit. Les méthodes de `Size` sont
`constexpr` : le compilateur peut les évaluer, ce qui documente qu'elles n'ont
aucun effet de bord, et permet de les vérifier à la compilation :

```cpp
static_assert(Size{3, 4}.indexOf({1, 2}) == 6);
```

### Un type itérable

Une boucle `for (x : range)` n'exige que deux méthodes, `begin()` et
`end()`. En les fournissant, `Neighbors` s'utilise comme n'importe quel
conteneur, sans allouer.

> **Piège.** `begin()` et `end()` doivent venir **du même objet**. L'expression
>
> ```cpp
> std::vector<Pos>(Neighbors{s, p}.begin(), Neighbors{s, p}.end())
> ```
>
> construit deux objets temporaires différents : les itérateurs ne se
> correspondent pas. Le test `Neighbors.CornerIsClippedToTheGrid` a d'ailleurs
> été écrit ainsi par erreur, puis corrigé.

## Vérifier

Premiers tests, dans [position_tests.cpp](../tests/position_tests.cpp). Ils
tournent en quelques microsecondes, sans écran : premier critère rempli.

```cpp
TEST(Size, ContainsOnlyPositionsInsideTheGrid) {
    constexpr Size size{2, 3};
    EXPECT_TRUE(size.contains({1, 2}));
    EXPECT_FALSE(size.contains({2, 0}));
    EXPECT_FALSE(size.contains({-1, 0}));
}

TEST(Neighbors, CornerIsClippedToTheGrid) {
    const Neighbors around{{3, 3}, {0, 0}};
    EXPECT_EQ(std::vector<Pos>(around.begin(), around.end()),
              (std::vector<Pos>{{0, 1}, {1, 0}, {1, 1}}));
}
```

`Size.IndexAndPositionAreInverse` vérifie que `posOf` et `indexOf` sont
réciproques sur toute la grille, et contient le `static_assert` ci-dessus.
`Neighbors.InteriorCellHasEight` compte huit voisines au centre.

## À retenir

- Donnez un **nom et un type** à chaque concept, même petit.
- **`struct`** pour une donnée sans invariant, **`class`** dès qu'il faut
  protéger une règle.
- Un objet valeur se **copie**, se **compare par contenu**, se passe
  **par valeur**.
- `begin()`/`end()` suffisent pour rendre un type utilisable dans un `for`.
- L'arithmétique de grille n'est écrite qu'**une fois**, dans `Size`.

## Exercices

1. Pourquoi `Pos` n'a-t-il pas de constructeur qui refuse les coordonnées
   négatives ? Qu'est-ce que ça changerait pour `Neighbors`, qui en fabrique ?
2. `Neighbors` stocke au plus 8 positions dans un `std::array`. Qu'est-ce qui
   garantit qu'il n'y en aura jamais 9 ?
3. Écrivez `Size::posOf` et `Size::indexOf` pour une grille rangée **colonne
   par colonne**. Combien de fichiers faut-il toucher ? Comparez avec la
   version « deux `int` partout ».
