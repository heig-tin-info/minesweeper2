# Étape 1 : positions, taille et voisins

## Objectif

Écrire les trois briques géométriques sur lesquelles tout le jeu reposera :

- `Pos` : une case, désignée par sa ligne et sa colonne ;
- `Size` : les dimensions d'une grille, et l'arithmétique qui va avec ;
- `neighbors()` : les cases qui entourent une position.

Rien de spécifique au démineur ici : pas de mine, pas de règle. C'est
voulu. On commence par la brique la plus petite qui soit **testable seule**.

## Ce qu'il faut comprendre avant de commencer

**Donner un nom à un concept.** On pourrait passer deux `int` partout :
`reveal(int row, int col)`. Mais on finit toujours par inverser ligne et
colonne, et par recopier `row * cols + col` dans chaque fichier. Une `Pos`
dit au lecteur, et au compilateur, que ces deux nombres forment **une seule
chose**.

**Objet valeur.** Une `Pos` n'est définie que par sa valeur : deux
`Pos{2, 3}` sont interchangeables. On la copie librement et on la compare
avec `==`.

**`struct` ou `class` ?** On utilise `struct`, avec des membres publics, quand
**toutes les valeurs sont acceptables**. `Pos{-1, 7}` est une position valide :
elle est simplement hors de la grille. Il n'y a donc rien à protéger, et des
*getters*/*setters* n'apporteraient que du bruit. On réservera `class` à
l'étape 2, quand il y aura une règle à garantir.

## Ce qui est fourni

| Fichier | Rôle |
| --- | --- |
| [test_position.cpp](test_position.cpp) | les tests, qui doivent tous passer |
| [Makefile](Makefile) | `make test` compile et lance les tests |

## Ce qu'il faut écrire

Deux fichiers, `position.hpp` et `position.cpp`, qui offrent exactement
cette interface :

```cpp
struct Pos {
    int row;   // 0 = ligne du haut
    int col;   // 0 = colonne de gauche
    bool operator==(const Pos&) const = default;
};

struct Size {
    int rows;
    int cols;

    int area() const;              // nombre de cases
    bool contains(Pos p) const;    // p est-elle dans la grille ?
    int indexOf(Pos p) const;      // Pos  -> indice dans un tableau à plat
    Pos posOf(int index) const;    // indice -> Pos
};

// Les cases qui entourent p (8 au plus), sans celles hors de la grille.
std::vector<Pos> neighbors(Size size, Pos p);
```

`indexOf` et `posOf` rangent la grille **ligne par ligne** dans un tableau
à une dimension :

```text
       col 0  col 1  col 2
row 0    0      1      2
row 1    3      4      5        indexOf({1, 2}) == 5
                                posOf(5) == Pos{1, 2}
```

## Consignes

- `position.hpp` ne contient que des déclarations ; le code va dans
  `position.cpp`.
- Aucun `#include <iostream>` : ces types ne savent pas s'afficher.
- Les méthodes qui ne modifient rien sont `const`.
- Ne recopiez pas le calcul « est dans la grille » : `neighbors` doit utiliser
  `contains`.

## Valider

```bash
make test
```

Tous les tests doivent passer, sans aucun avertissement du compilateur.

## Pièges fréquents

- **Inverser lignes et colonnes.** Pensez « ligne, colonne » comme dans une
  matrice, pas « x, y » comme sur un écran.
- **Oublier les bords.** Une case de coin a 3 voisins, une case de bord en a 5.
- **Compter la case elle-même** parmi ses voisins.
- **`posOf` avec la mauvaise division.** C'est le nombre de *colonnes* qui sert
  de diviseur, pas le nombre de lignes. Le test sur une grille non carrée
  (3 × 4) vous le signalera.

## Questions

1. Pourquoi `Pos` n'a-t-il pas de constructeur qui refuse les coordonnées
   négatives ?
2. `operator==` est généré par `= default`. Que compare-t-il exactement ?
   Que se passerait-il si on ajoutait un troisième champ à `Pos` ?
3. Où, dans la suite du jeu, aura-t-on besoin de `neighbors` ? Citez au moins
   deux règles.

## Et ensuite

À l'étape 2, on construira le **champ de mines** : la première classe avec
une règle à protéger, « chaque chiffre vaut le nombre de mines voisines ».
Vous y utiliserez `neighbors`, `contains` et `indexOf`.

Pour aller plus loin : le chapitre [Objets valeur](../../docs/03-objets-valeur.md)
de la documentation.
