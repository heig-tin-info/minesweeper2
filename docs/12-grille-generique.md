# Une grille générique

> **Objectif.** Supprimer la duplication repérée à l'étape précédente par une
> **refactorisation sous tests** : écrire une fois un code qui sert à
> plusieurs types, avec un `template`.
>
> **Code :** [src/core/grid.hpp](../src/core/grid.hpp)

## Le besoin

À l'[étape 10](10-champ-de-mines.md), `Minefield` range ses `Square` dans un
`std::vector` plat avec un calcul d'index. À l'[étape 11](11-la-partie.md),
`Game` a besoin d'une grille de `Mark` de la même taille. Dans les deux cas,
il faut :

- un `std::vector` de `rows × cols` éléments ;
- convertir une `Pos` en indice avec `Size::indexOf` ;
- vérifier (en debug) que la position est dans la grille.

On vient d'écrire ce code **deux fois**. Deuxième critère de décision, en
plein : deux copies d'un même calcul finissent toujours par diverger, au
premier correctif appliqué à une seule des deux.

## Les options

| Option | Idée | Ce qui coince |
| --- | --- | --- |
| Garder la copie | « ce n'est que six lignes » | deux bugs pour le prix d'un ; la troisième grille arrivera |
| Une classe de base `GridBase` | hériter pour partager l'indexation | l'héritage sert à exprimer un « est un », pas à recycler du code ; il faudrait un `void*` ou un type d'élément fixé |
| Un `std::vector<std::vector<T>>` | pas d'index à calculer | une allocation par ligne, données éparpillées, rien n'empêche deux lignes de longueurs différentes |
| Un **patron de classe** `Grid<T>` | le compilateur génère une classe par type | il faut apprendre la syntaxe |

## La décision

```cpp
template <class T>
class Grid {
    static_assert(!std::is_same_v<T, bool>, "use an enum or a struct, not bool");

public:
    Grid(Size size, const T& fill)
        : size_(size), cells_(checkedArea(size), fill) {}

    Size size() const { return size_; }
    T& operator[](Pos p) { return cells_[index(p)]; }
    const T& operator[](Pos p) const { return cells_[index(p)]; }

private:
    static std::size_t checkedArea(Size size) {
        if (size.rows < 0 || size.cols < 0)
            throw std::invalid_argument("Grid: negative size");
        return static_cast<std::size_t>(size.area());
    }
    std::size_t index(Pos p) const {
        assert(size_.contains(p));
        return static_cast<std::size_t>(size_.indexOf(p));
    }

    Size size_;
    std::vector<T> cells_;
};
```

`Grid<Square>` et `Grid<Mark>` sont deux classes distinctes, **générées par
le compilateur** à partir du même modèle. Le code n'est écrit qu'une fois.

### La refactorisation

`Minefield` est réécrit : le `std::vector<Square>` et la méthode `at()` de
l'étape 10 disparaissent au profit d'un seul membre.

```cpp
private:
    struct Square {
        bool mine = false;
        std::uint8_t adjacent = 0;
    };
    Grid<Square> squares_;
    int mineCount_;
```

```cpp
bool hasMine(Pos p) const { return squares_[p].mine; }
int adjacentMines(Pos p) const { return squares_[p].adjacent; }
```

Et `Game` déclare simplement `Grid<Mark> marks_;`. L'interface publique de
`Minefield` **n'a pas changé** : `Square` était privé, personne ne savait
comment les cases étaient rangées. C'est la liberté que l'encapsulation nous
avait achetée à l'étape 10.

Un vecteur **plat** de `rows × cols` éléments est contigu (le processeur le
lit efficacement), et la forme rectangulaire est garantie par construction.

## Les concepts

### Template et DRY

Un **patron de classe** (`template <class T>`) décrit une famille de classes ;
chaque instanciation (`Grid<Mark>`) est une classe à part entière, compilée
comme si on l'avait écrite à la main. C'est la réponse C++ au principe **DRY**
(*Don't Repeat Yourself*) : chaque connaissance, ici « une grille se range
ligne par ligne dans un vecteur », a une seule représentation dans le code.
Le template est la **réponse à une duplication constatée**, pas un point de
départ : on n'écrit pas `Grid<T>` « au cas où » avant d'avoir deux grilles.

### Deux surcharges de `operator[]`

```cpp
T& operator[](Pos p);              // sur une grille modifiable
const T& operator[](Pos p) const;  // sur une grille en lecture seule
```

Le compilateur choisit selon la constance de l'objet. Dans
`Minefield::hasMine() const`, `squares_` est `const`, donc c'est la seconde
version qui est appelée : impossible de modifier une case par erreur dans
une méthode de lecture. La promesse `const` de l'étape 10 se propage.

### Une contrainte à la compilation : `static_assert`

`std::vector<bool>` est une exception célèbre de la bibliothèque standard :
il range les booléens **un bit par élément** et ne peut pas renvoyer un vrai
`bool&`, mais un objet *proxy*. `grid[p]` ne se comporterait pas comme pour
les autres types. Plutôt que de laisser cette surprise arriver, `Grid`
**refuse de compiler** avec `bool`, avec un message clair. Le type trait
`std::is_same_v<A, B>` vaut `true` si `A` et `B` sont le même type.

### `assert` ou exception, à nouveau

`Grid` utilise les deux, au même endroit qu'à l'étape 10 :

| | `assert` (dans `index`) | exception (dans `checkedArea`) |
| --- | --- | --- |
| Actif | en debug uniquement | toujours |
| Coût en release | nul | un test par construction |
| Sert à | détecter un **bug interne** | refuser une **entrée invalide** |

`operator[]` sera appelé des millions de fois dans l'ouverture en cascade
([étape 13](13-ouverture-en-cascade.md)), toujours avec des positions déjà
vérifiées par `Game::require()`. On vérifie **une fois, à la frontière
publique**, et l'`assert` attrape nos propres erreurs pendant le
développement.

## Vérifier

C'est le point de la refactorisation : les tests de l'étape 10
([minefield_tests.cpp](../tests/minefield_tests.cpp)) et de l'étape 11
([game_tests.cpp](../tests/game_tests.cpp)) **passent sans qu'une ligne en
soit modifiée**. Ils ne parlaient que de l'interface publique
(`hasMine`, `adjacentMines`, `reveal`, `cell`), jamais du stockage. C'est ce
qui distingue une refactorisation d'une réécriture : le comportement observable
est inchangé, et les tests le prouvent.

`Grid` reçoit en plus deux tests dans
[position_tests.cpp](../tests/position_tests.cpp) :

```cpp
TEST(Grid, StoresOneValuePerPosition) {
    Grid<int> grid{{2, 2}, 0};
    grid[{1, 0}] = 7;
    EXPECT_EQ((grid[{1, 0}]), 7);
    EXPECT_EQ((grid[{0, 1}]), 0);
}

TEST(Grid, RejectsNegativeSize) {
    EXPECT_THROW((Grid<int>{{-1, 2}, 0}), std::invalid_argument);
}
```

## À retenir

- Un `template` évite d'écrire deux fois la même logique pour des types
  différents (**DRY**). On l'introduit quand la duplication existe, pas avant.
- Une **refactorisation** change la structure sans changer le comportement :
  les tests existants en sont la preuve.
- Deux versions d'un accesseur, `const` et non-`const`, pour respecter la
  constance de l'objet.
- **`static_assert`** transforme une mauvaise utilisation en erreur de
  compilation.
- **`assert`** pour les bugs internes, **exception** pour les entrées
  invalides. On vérifie une fois, à la frontière.

## Exercices

1. Que se passe-t-il en écrivant `Grid<bool> g{{3, 3}, false};` ? Essayez,
   et lisez le message d'erreur.
2. `Grid` pourrait offrir `begin()`/`end()` pour parcourir toutes les cases.
   Qui en aurait besoin aujourd'hui ? (Rappel : on n'écrit pas de code
   « au cas où ».)
3. Supprimez l'`assert` de `index()` et dévoilez `{9, 9}` sur une grille 3×3
   dans un test, **sans** passer par `Game::require()`. Que se passe-t-il en
   debug, puis en release ? Qu'en concluez-vous sur « vérifier à la
   frontière » ?
