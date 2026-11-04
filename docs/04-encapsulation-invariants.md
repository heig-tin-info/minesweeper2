# Encapsulation et invariants : `Minefield`

> **Objectif.** Comprendre qu'encapsuler, ce n'est pas « mettre des
> getters », c'est **garantir un invariant**.
>
> **Code :** [src/core/minefield.hpp](../src/core/minefield.hpp),
> [src/core/minefield.cpp](../src/core/minefield.cpp)

## Le problème

Un champ de mines, c'est : où sont les mines, et pour chaque case, combien de
mines la touchent. Ces deux informations sont **liées** : si on ajoute une
mine sans mettre à jour les compteurs voisins, le jeu affiche des chiffres
faux.

Une première version pourrait être :

```cpp
struct Cell {
    bool bomb;
    int neighborBombCount;
    void setBomb(bool v) { bomb = v; }   // et les voisins ?
};
```

Tout est modifiable par tout le monde. La cohérence ne repose que sur la
discipline de chaque appelant. Un seul oubli, et le jeu ment.

## Qu'est-ce qu'un invariant ?

Un **invariant** est une propriété qui doit être vraie **à chaque instant
où un code extérieur peut observer l'objet**. Pour `Minefield` :

> Pour chaque case, `adjacentMines(p)` vaut exactement le nombre de mines
> parmi ses voisins.

L'encapsulation est l'outil qui rend cet invariant **impossible à casser** :
si personne ne peut modifier les données, personne ne peut les rendre
incohérentes.

## La solution : un objet immuable

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
    Grid<Square> squares_;
    int mineCount_;
};
```

Observez ce qui **n'existe pas** : aucun *setter*, aucune méthode non-`const`.
L'invariant est établi **une fois**, dans le constructeur, et plus rien ne
peut le casser ensuite. C'est l'**immuabilité**, la forme d'encapsulation la
plus forte qui soit.

C'est aussi fidèle au domaine : pendant une partie, les mines ne bougent pas.
Le joueur *découvre* le champ, il ne le *modifie* pas.

## Le constructeur établit l'invariant (ou échoue)

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

Le contrat est clair : soit l'objet est construit **et valide**, soit le
constructeur lance une exception et l'objet **n'existe pas**. Il n'y a jamais
de `Minefield` à moitié initialisé, ni de méthode `init()` qu'on aurait
oublié d'appeler.

> **`std::exchange(x, v)`** affecte `v` à `x` et renvoie l'**ancienne**
> valeur. Ici on pose la mine et, du même geste, on apprend si elle était déjà
> là.

L'algorithme est aussi plus efficace que la version naïve : au lieu de
compter, pour chaque case, les mines autour d'elle (8 × cases), chaque mine
incrémente ses voisines (8 × mines).

## `const` : une promesse vérifiée par le compilateur

Toutes les méthodes publiques sont marquées `const`. Ce n'est pas décoratif :

- le lecteur sait immédiatement qu'elles **ne modifient rien** ;
- le compilateur **refuse** toute modification à l'intérieur ;
- on peut les appeler sur un `const Minefield&`, ce que fait `Game`.

La *const-correctness* est la façon C++ d'exprimer « ceci est une question,
pas un ordre ». On y revient au [chapitre 8](08-commandes-requetes.md).

## `Square` est privé

La structure `Square` est déclarée dans la partie `private` de `Minefield`.
Personne, hors de la classe, ne peut même **nommer** ce type. On peut donc
changer sa représentation (un seul octet avec la mine codée en `9`, par
exemple) sans impacter le moindre appelant. C'est l'autre bénéfice de
l'encapsulation : **la liberté de changer l'implémentation**.

## Exceptions : laquelle choisir ?

| Exception | Signifie | Exemple |
| --- | --- | --- |
| `std::invalid_argument` | l'argument n'a pas de sens | mine en double |
| `std::out_of_range` | une position hors limites | mine en `{9, 9}` sur une grille 3×3 |
| `std::logic_error` | un contrat entre objets est violé | la stratégie de placement a menti ([chapitre 7](07-polymorphisme-ports.md)) |

Ces erreurs sont des **erreurs de programmation** : un appelant correct ne les
déclenche jamais. Elles ne servent pas au déroulement normal du jeu.

## À retenir

- Encapsuler, c'est **protéger un invariant**, pas ajouter des getters et des setters.
- La meilleure protection est souvent l'**immuabilité** : tout se décide
  dans le constructeur.
- Un constructeur **réussit avec un objet valide, ou lance une exception**.
  Jamais d'objet « à moitié construit ».
- Les méthodes qui ne modifient rien sont **`const`**.

## Pour réfléchir

1. Que faudrait-il changer pour permettre le mode « mines mouvantes », où une
   mine se déplace quand on clique dessus ? Quels invariants seraient en
   danger ?
2. Pourquoi `Minefield` vérifie-t-il les doublons alors que
   `RandomMineLayout` n'en produit jamais ?
