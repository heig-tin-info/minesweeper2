# Le champ de mines

> **Objectif.** Poser les mines et connaître, pour chaque case, le nombre de
> voisines minées, sans qu'il soit **possible** d'avoir un champ incohérent.
> Comprendre qu'encapsuler, ce n'est pas « mettre des getters », c'est
> **garantir un invariant**.
>
> **Code :** [src/core/minefield.hpp](../src/core/minefield.hpp),
> [src/core/minefield.cpp](../src/core/minefield.cpp)

Cette étape correspond à l'étape 2 du [labo](../steps/README.md).

## Le besoin

Un champ de mines, c'est deux informations : où sont les mines, et pour
chaque case, combien de mines la touchent. Ces deux informations sont
**liées**. Si quelqu'un ajoute une mine sans mettre à jour les compteurs
voisins, le jeu affiche des chiffres faux, et le joueur perd sur une case
annoncée sûre.

## Les options

**Un tableau public.** Une `struct` par case, tout modifiable :

```cpp
struct Cell {
    bool bomb;
    int neighborBombCount;
    void setBomb(bool v) { bomb = v; }   // et les voisins ?
};
std::vector<Cell> cells;                 // à la portée de tout le monde
```

La cohérence repose sur la discipline de **chaque** appelant. Un seul oubli,
et le jeu ment. Et le bug se manifeste loin de sa cause : un chiffre faux à
l'écran, trois étapes plus tard.

**Un objet qui calcule tout à la construction et n'offre que des lectures.**
Personne ne peut modifier les données ; personne ne peut donc les rendre
incohérentes. Troisième critère : **qui dépend de qui ?** Avec le tableau
public, tout le programme dépend de la représentation interne des cases.
Avec l'objet fermé, seul `Minefield` la connaît.

## La décision

Un objet **immuable**, construit depuis la liste des mines.

```cpp
class Minefield {
public:
    Minefield(Size size, std::span<const Pos> mines);

    Size size() const;
    int mineCount() const;
    bool hasMine(Pos p) const;
    int adjacentMines(Pos p) const;

private:
    struct Square {
        bool mine = false;
        std::uint8_t adjacent = 0;
    };
    // ... le stockage des cases : voir ci-dessous
    int mineCount_;
};
```

Observez ce qui **n'existe pas** : aucun *setter*, aucune méthode non-`const`.
C'est fidèle au domaine : pendant une partie, les mines ne bougent pas. Le
joueur *découvre* le champ, il ne le *modifie* pas.

### Le stockage, version de cette étape

À ce stade, on range les cases dans un `std::vector` plat, ligne par ligne,
avec le calcul d'index de `Size` :

```cpp
// Version de l'étape 10 : un vector et un calcul d'index.
class Minefield {
    // ...
private:
    Size size_;
    std::vector<Square> squares_;   // rows * cols cases, ligne par ligne
    int mineCount_;

    const Square& at(Pos p) const {
        assert(size_.contains(p));
        return squares_[static_cast<std::size_t>(size_.indexOf(p))];
    }
    Square& at(Pos p) { /* idem, non const */ }
};
```

C'est suffisant, et c'est ce qu'on écrit au labo. **La version finale du
dépôt** utilise à la place un `Grid<Square>` : il apparaîtra à
l'[étape 12](12-grille-generique.md), quand une deuxième grille nous
obligera à ne pas écrire ce code deux fois.

### Le constructeur établit l'invariant, ou échoue

```cpp
Minefield::Minefield(Size size, std::span<const Pos> mines)
    : squares_(size, Square{}), mineCount_(static_cast<int>(mines.size())) {
    for (Pos mine : mines) {
        if (!size.contains(mine))
            throw std::out_of_range("Minefield: mine outside the grid");
        if (std::exchange(squares_[mine].mine, true))
            throw std::invalid_argument("Minefield: duplicated mine");
        for (Pos n : Neighbors{size, mine}) ++squares_[n].adjacent;
    }
}
```

Soit l'objet est construit **et valide**, soit le constructeur lance une
exception et l'objet **n'existe pas**. Il n'y a jamais de `Minefield` à moitié
initialisé, ni de méthode `init()` qu'on aurait oublié d'appeler.

> **`std::exchange(x, v)`** affecte `v` à `x` et renvoie l'**ancienne**
> valeur. Ici on pose la mine et, du même geste, on apprend si elle était déjà
> là.

L'algorithme est aussi plus efficace que la version naïve : au lieu de
compter, pour chaque case, les mines autour d'elle (8 × cases), chaque mine
incrémente ses voisines (8 × mines). On réutilise `Neighbors` de
l'[étape 9](09-positions.md).

## Les concepts

### Invariant

Un **invariant** est une propriété qui doit être vraie **à chaque instant où
un code extérieur peut observer l'objet**. Pour `Minefield` :

> Pour chaque case, `adjacentMines(p)` vaut exactement le nombre de mines
> parmi ses voisines.

C'est la règle qu'on avait repérée à l'étape 2 en disant que le champ de mines
« a quelque chose à protéger », donc mérite une `class` et non une `struct`.

### Encapsulation et immuabilité

L'encapsulation est l'outil qui rend l'invariant **impossible à casser**.
L'**immuabilité** en est la forme la plus forte : tout se décide dans le
constructeur, puis plus rien ne bouge. Deuxième bénéfice : `Square` est déclaré
dans la partie `private`. Personne, hors de la classe, ne peut même **nommer**
ce type. On peut donc changer la représentation (un seul octet avec la mine
codée en `9`, ou un `Grid<Square>` à l'étape 12) sans toucher au moindre
appelant.

### `const` : une promesse vérifiée par le compilateur

Toutes les méthodes publiques sont `const`. Le lecteur sait qu'elles **ne
modifient rien**, le compilateur **refuse** toute modification à l'intérieur,
et on peut les appeler sur un `const Minefield&`, ce que fera `Game`. C'est la
façon C++ de dire « ceci est une question, pas un ordre » : la distinction
faite à l'étape 3 entre verbes qui lisent et verbes qui changent.

### Exception ou `assert`

| | Exception | `assert` |
| --- | --- | --- |
| Sert à | refuser une **donnée venue de l'extérieur** | attraper un **bug interne** |
| Active | toujours | en debug seulement |
| Ici | mine hors grille, mine en double | position hors grille dans `at()` |

La liste des mines vient d'ailleurs (d'un générateur, d'un test) : on la
vérifie **toujours**, par une exception. Une position passée à `at()` a déjà
été vérifiée par l'appelant : on l'`assert` pour se protéger pendant le
développement, sans payer le test en production.

| Exception | Signifie | Exemple |
| --- | --- | --- |
| `std::invalid_argument` | l'argument n'a pas de sens | mine en double |
| `std::out_of_range` | une position hors limites | mine en `{9, 9}` sur 3×3 |

Ces erreurs sont des **erreurs de programmation** : un appelant correct ne les
déclenche jamais. Elles ne servent pas au déroulement normal du jeu.

## Vérifier

[minefield_tests.cpp](../tests/minefield_tests.cpp) : trois tests, aucun
affichage, aucune attente.

```cpp
TEST(Minefield, CountsAdjacentMines) {
    // * . .
    // . . .
    // . . *
    const std::vector<Pos> mines{{0, 0}, {2, 2}};
    const Minefield field{{3, 3}, mines};

    EXPECT_EQ(field.mineCount(), 2);
    EXPECT_TRUE(field.hasMine({0, 0}));
    EXPECT_EQ(field.adjacentMines({1, 1}), 2);
    EXPECT_EQ(field.adjacentMines({0, 2}), 0);
}
```

`Minefield.RejectsMinesOutsideTheGrid` attend `std::out_of_range` pour une
mine en `{3, 0}` sur une grille 3×3 ; `Minefield.RejectsDuplicatedMines`
attend `std::invalid_argument` pour `{1, 1}` donnée deux fois. Ces deux tests
**documentent le contrat** du constructeur : ce qu'il refuse fait partie de
sa spécification.

## À retenir

- Encapsuler, c'est **protéger un invariant**, pas ajouter des getters et des
  setters.
- La meilleure protection est souvent l'**immuabilité** : tout se décide dans
  le constructeur.
- Un constructeur **réussit avec un objet valide, ou lance une exception**.
  Jamais d'objet « à moitié construit ».
- Les méthodes qui ne modifient rien sont **`const`**.
- Exception pour une donnée externe, `assert` pour un bug interne.

## Exercices

1. Que faudrait-il changer pour permettre le mode « mines mouvantes », où une
   mine se déplace quand on clique dessus ? Quels invariants seraient en
   danger, et où faudrait-il les rétablir ?
2. Pourquoi `Minefield` vérifie-t-il les doublons alors que le générateur de
   l'[étape 16](16-hasard.md) n'en produit jamais ?
3. Remplacez `std::uint8_t adjacent` par un `int`. Les tests passent-ils ?
   Quelle propriété de l'encapsulation ce changement illustre-t-il ?
