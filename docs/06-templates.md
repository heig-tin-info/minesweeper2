# Programmation générique : `Grid<T>`

> **Objectif.** Utiliser un `template` pour écrire **une fois** un code qui
> sert à plusieurs types, et voir comment exprimer des contraintes à la
> compilation.
>
> **Code :** [src/core/grid.hpp](../src/core/grid.hpp)

## Le problème

`Minefield` a besoin d'une grille de `Square`. `Game` a besoin d'une grille
de `Mark`. Dans les deux cas, il faut :

- un `std::vector` de taille `rows × cols` ;
- convertir une `Pos` en indice ;
- vérifier (en debug) que la position est dans la grille.

Sans abstraction, ce code serait écrit deux fois. Et deux copies d'un même
calcul finissent toujours par diverger, au premier correctif appliqué à une
seule des deux.

## La solution : un modèle de classe

```cpp
template <class T>
class Grid {
public:
    Grid(Size size, const T& fill);

    Size size() const;
    T& operator[](Pos p);
    const T& operator[](Pos p) const;

private:
    Size size_;
    std::vector<T> cells_;
};
```

`Grid<Square>` et `Grid<Mark>` sont deux classes distinctes, **générées par le
compilateur** à partir du même modèle. Le code n'est écrit qu'une fois.

C'est le principe **DRY** (*Don't Repeat Yourself*) : chaque connaissance (ici,
« une grille se range ligne par ligne dans un vecteur ») a une seule
représentation dans le code.

## Deux surcharges de `operator[]`

```cpp
T& operator[](Pos p);              // sur une grille modifiable
const T& operator[](Pos p) const;  // sur une grille en lecture seule
```

Le compilateur choisit selon la *constance* de l'objet. Dans
`Minefield::hasMine() const`, `squares_` est `const`, donc c'est la seconde
version qui est appelée : impossible de modifier une case par erreur dans une
méthode de lecture.

## Une contrainte à la compilation : `static_assert`

```cpp
static_assert(!std::is_same_v<T, bool>, "use an enum or a struct, not bool");
```

`std::vector<bool>` est une exception célèbre de la bibliothèque standard :
pour économiser la mémoire, il range les booléens **un bit par élément**. Il
ne peut donc pas renvoyer un vrai `bool&`, mais un objet *proxy* qui imite une
référence. `grid[p]` ne se comporterait pas comme pour les autres types.

Plutôt que de laisser cette surprise arriver, `Grid` **refuse de compiler**
avec `bool`. Une erreur à la compilation, avec un message clair, vaut toujours
mieux qu'un comportement étrange à l'exécution.

> Le type trait `std::is_same_v<A, B>` vaut `true` si `A` et `B` sont le même
> type. Les *type traits* de `<type_traits>` permettent d'interroger les types
> pendant la compilation.

## `assert` ou exception ?

`Grid` utilise les deux, et ce n'est pas un hasard :

```cpp
Grid(Size size, const T& fill) : size_(size), cells_(checkedArea(size), fill) {}
//                                                   ^ exception si taille < 0

std::size_t index(Pos p) const {
    assert(size_.contains(p));   // vérifié en debug seulement
    return static_cast<std::size_t>(size_.indexOf(p));
}
```

| | `assert` | exception |
| --- | --- | --- |
| Actif | en debug uniquement | toujours |
| Coût en release | nul | un test à chaque appel |
| Sert à | détecter un **bug interne** | refuser une **entrée invalide** |

`operator[]` est appelé des millions de fois dans le *flood fill*, toujours
avec des positions déjà vérifiées par `Game`. Le vérifier à nouveau en release
serait du gaspillage. On vérifie donc **une fois, à la frontière publique**
(`Game::require()`), et l'`assert` attrape nos propres erreurs pendant le
développement.

## Pourquoi pas un `std::vector<std::vector<T>>` ?

Un vecteur de vecteurs fait une allocation par ligne, éparpille les données en
mémoire, et rien n'empêche deux lignes d'avoir des longueurs différentes. Un
vecteur **plat** de `rows × cols` éléments est contigu (le processeur le lit
efficacement), et la forme rectangulaire est garantie par construction.

## À retenir

- Un `template` évite d'écrire deux fois la même logique pour des types
  différents (**DRY**).
- On fournit deux versions d'un accesseur, `const` et non-`const`, pour
  respecter la constance de l'objet.
- **`static_assert`** transforme une mauvaise utilisation en erreur de
  compilation.
- **`assert`** traque les bugs internes, les **exceptions** refusent les entrées
  invalides. On vérifie une fois, à la frontière.

## Pour réfléchir

1. Que se passerait-il en écrivant `Grid<bool> g{{3, 3}, false};` ?
   Essayez !
2. `Grid` pourrait offrir `begin()`/`end()` pour parcourir toutes les cases.
   Qui en aurait besoin aujourd'hui ? (Rappel : on n'écrit pas de code « au
   cas où ».)
