# L'ouverture en cascade

> **Objectif.** Dévoiler une case sans voisine minée ouvre ses voisines, de
> proche en proche. Écrire l'algorithme, puis le rendre robuste : passer de
> « ça marche sur l'exemple » à « ça marche toujours ».
>
> **Code :** `Game::open` dans [src/core/game.cpp](../src/core/game.cpp)

Cette étape correspond à l'étape 4 du [labo](../steps/README.md).

## Le besoin

Règle du jeu : si la case dévoilée affiche `0`, toutes ses voisines sont
dévoilées automatiquement, et ainsi de suite tant qu'on rencontre des zéros.
Les cases marquées d'un drapeau arrêtent la propagation. Sur une grille
presque vide, un seul clic peut ouvrir des milliers de cases.

Ce qui existe déjà : `Game::reveal` appelle `open(p)` quand la case n'est pas
une mine ([étape 11](11-la-partie.md)). Il reste à écrire `open`.

## Les options

### Récursif

La formulation naturelle : ouvrir une case, et si elle vaut zéro, ouvrir
chaque voisine encore cachée.

```cpp
// Première idée, récursive.
void Game::open(Pos p) {
    if (marks_[p] != Mark::Hidden) return;
    marks_[p] = Mark::Revealed;
    --hiddenSafe_;
    if (field_->adjacentMines(p) != 0) return;
    for (Pos n : Neighbors{size(), p}) open(n);
}
```

Six lignes, lisibles, correctes sur une grille 9×9. Mais chaque case vide
ajoute un **appel sur la pile** avant que le précédent soit terminé. Sur une
grille 1000×1000 presque vide, c'est jusqu'à un million d'appels imbriqués.
La pile d'appels fait typiquement quelques mégaoctets : elle déborde, et le
programme s'arrête brutalement, sans exception à attraper.

### Itératif, avec une pile explicite

Même algorithme, mais les cases « à traiter » sont rangées dans un
`std::vector` qu'on gère soi-même. La mémoire vient du tas, qui est des
milliers de fois plus grand que la pile d'appels.

| | Récursif | Pile explicite |
| --- | --- | --- |
| Lignes | 6 | 11 |
| Mémoire | pile d'appels, quelques Mo | tas, autant que nécessaire |
| Grille 1000×1000 | plantage | fonctionne |
| Ordre de parcours | profondeur d'abord | profondeur d'abord aussi |

## La décision

```cpp
// Reveals `start` and, when it has no adjacent mine, its whole empty region.
// An explicit stack instead of recursion: a huge empty grid cannot overflow
// the call stack.
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

Lisez-le ligne à ligne :

1. `pending` contient les cases à examiner ; on commence par celle du clic.
2. On en prend une. Si elle n'est plus cachée (déjà ouverte par un autre
   chemin, ou marquée d'un drapeau), on l'ignore : c'est ce `continue` qui
   fait que **les drapeaux arrêtent la cascade** et qu'aucune case n'est
   comptée deux fois dans `hiddenSafe_`.
3. On l'ouvre et on décompte.
4. Si elle a des voisines minées, elle affiche un chiffre et la cascade
   s'arrête là.
5. Sinon, ses voisines cachées rejoignent la file d'attente.

Le test `marks_[n] == Mark::Hidden` avant le `push_back` n'est pas
indispensable (la case serait ignorée à l'étape 2), mais il évite d'empiler
des cases qu'on sait inutiles.

## Les concepts

### Parcours de graphe

Les cases sont des sommets, le voisinage des arêtes, et l'ouverture en cascade
un **parcours** de la composante connexe des cases à zéro, plus leur bordure
de chiffres. C'est l'algorithme du *flood fill* (le « pot de peinture » des
logiciels de dessin). On le retrouvera chaque fois qu'il faut « propager »
quelque chose de proche en proche.

### Pile d'appels et pile explicite

Une fonction récursive utilise la **pile d'appels** comme mémoire implicite :
chaque appel y dépose ses variables locales et son adresse de retour. Cette
pile est petite et de taille fixe. Une **pile explicite** (`std::vector` avec
`push_back`/`pop_back`) fait le même travail, mais dans le **tas**, avec une
taille qui s'adapte. Remplacer l'une par l'autre est une transformation
mécanique : ce que la récursion mettait dans les paramètres, on le met dans
la pile.

### Profondeur ou largeur

Avec `pending.back()`, on traite la dernière case ajoutée : parcours **en
profondeur** (DFS), le même ordre que la récursion. Avec une file
(`std::deque` et `pop_front`), ce serait **en largeur** (BFS), par anneaux
concentriques autour du clic. Pour ouvrir une zone, le résultat final est
identique : seul l'ordre des ouvertures change. Il compterait pour une
animation, pas pour la règle.

### « Sur l'exemple » et « toujours »

La version récursive passe tous les tests sur une grille 3×3. Elle est
**fausse** quand même, parce que son domaine de validité n'est pas celui du
jeu : rien n'interdit une grille de 100×100 (et le lanceur accepte
`minesweeper-console 1000 1000 1`). Un algorithme doit être jugé sur **toutes
les entrées admises**, pas sur celles qu'on a sous la main. C'est le rôle du
test suivant : écrire l'entrée extrême *avant* de se dire que ça marche.

## Vérifier

Dans [game_tests.cpp](../tests/game_tests.cpp), trois tests couvrent la
cascade.

```cpp
TEST(Game, FloodFillHandlesHugeEmptyGrids) {
    FixedMineLayout layout{{{0, 0}}};
    Game game{{1000, 1000}, 1, layout};
    game.reveal({999, 999});  // a recursive fill would overflow the stack here
    EXPECT_EQ(game.status(), Status::Won);
}
```

| Test | Ce qu'il prouve |
| --- | --- |
| `FloodFillHandlesHugeEmptyGrids` | un million de cases ouvertes en un clic, sans déborder |
| `RevealingAnEmptyCellOpensItsRegion` | la cascade s'arrête sur les chiffres et gagne la partie |
| `FlagsStopTheFloodFill` | une case marquée n'est pas ouverte, la partie continue |

Remplacez `open` par la version récursive : les deux derniers passent, le
premier plante. C'est exactement la différence entre « sur l'exemple » et
« toujours ».

## À retenir

- L'ouverture en cascade est un **parcours de graphe** (*flood fill*).
- Une récursion profonde peut **déborder la pile d'appels** ; une **pile
  explicite** sur le tas fait le même travail sans limite pratique.
- `back()`/`pop_back()` donne un parcours en profondeur ; une file donnerait
  un parcours en largeur. Même résultat ici.
- Un algorithme se juge sur **toutes les entrées admises** : écrivez le test
  extrême.

## Exercices

1. Réécrivez `open` avec un `std::deque` et `pop_front()`. Les trois tests
   passent-ils ? Dessinez l'ordre d'ouverture sur une grille 5×5 vide, pour
   les deux versions.
2. Supprimez le `if (marks_[n] == Mark::Hidden)` devant le `push_back`.
   Les tests passent-ils encore ? Qu'est-ce qui change dans la taille maximale
   de `pending` ?
3. Comptez le nombre de fois où chaque case passe dans `pending` sur une
   grille vide. Donnez une borne en fonction du nombre de cases. Que
   deviendrait cette borne sans le `continue` de l'étape 2 ?
