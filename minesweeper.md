# Fonctionnement du jeu

Le démineur est un jeu inventé par Robert Donner en 1989. Le but du jeu est de dévoiler toutes les cases du plateau de jeu qui ne contiennent pas de bombes. Si une bombe est dévoilée, le joueur a perdu. Si toutes les cases sans bombe sont dévoilées, le joueur a gagné. Il a été popularisé par Microsoft Windows et est devenu un jeu classique.

Vous pouvez y jouer en line par exemple sur le site de Google : [Minesweeper](https://share.google/g0mV5UAuynuX5uQxE).

Imaginons un plateau de jeu de 7x7 cases avec 3 bombes. Le plateau est composé de cellules qui peuvent avoir différents états :

1. `.` : terrain non dévoilé,
2. `*` : terrain miné qui explosera s'il est dévoilé,
3. `?` : terrain marqué comme miné par le joueur afin de ne pas le dévoiler,
4. ` `, `1` à `8` : terrain dévoilé indiquant le nombre de bombes adjacentes (diagonales incluses). On préfèrera ` ` plutôt que `0` par souci esthétique.

Voici un plateau de jeu entièrement dévoilé :

```text
  ┌───┬───┬───┬───┬───┬───┬───┐
7 │   │ 1 │ 1 │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
6 │   │ 1 │ * │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
5 │   │ 2 │ 2 │ 2 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
4 │   │ 1 │ * │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
3 │   │ 1 │ 1 │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
2 │   │   │   │ 1 │ 1 │ 1 │   │
  ├───┼───┼───┼───┼───┼───┼───┤
1 │   │   │   │ 1 │ * │ 1 │   │
  └───┴───┴───┴───┴───┴───┴───┘
    A   B   C   D   E   F   G
```

Le joueur doit pouvoir interagir avec le plateau. Il possède deux actions :

1. Dévoiler une case
2. Marquer une case comme minée

Le joueur gagne si il a dévoilé tout le plateau sans dévoiler de bombe.
Il perd si il dévoile une bombe. Dans ce cas le plateau est révélé.

Au démarrage du programme, une grille aléatoire est générée avec un certain nombre de bombes. Le joueur ne voit la grille avec aucune cellule exposée :

```text
  ┌───┬───┬───┬───┬───┬───┬───┐
7 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
6 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
5 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
4 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
3 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
2 │ . │ . │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
1 │ . │ . │ . │ . │ . │ . │ . │
  └───┴───┴───┴───┴───┴───┴───┘
    A   B   C   D   E   F   G
```

Activer la cellule `A6` pourrait donner ceci, les cellules se dévoilent automatiquement si elles sont vides, jusqu'aux cellules qui possèdent des bombes adjacentes :

```text
 Colonne A
    v
  ┌───┬───┬───┬───┬───┬───┬───┐
7 │   │ 1 │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
6 │   │ 1 │ . │ . │ . │ . │ . │ < Ligne 6
  ├───┼───┼───┼───┼───┼───┼───┤
5 │   │ 2 │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
4 │   │ 1 │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
3 │   │ 1 │ 1 │ 1 │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
2 │   │   │   │ 1 │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
1 │   │   │   │ 1 │ . │ . │ . │
  └───┴───┴───┴───┴───┴───┴───┘
    A   B   C   D   E   F   G
```

L'utilisateur gagne le jeu lorsque seules les bombes sont exposées :

```text
  ┌───┬───┬───┬───┬───┬───┬───┐
7 │   │ 1 │ 1 │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
6 │   │ 1 │ ? │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
5 │   │ 2 │ 2 │ 2 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
4 │   │ 1 │ ? │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
3 │   │ 1 │ 1 │ 1 │   │   │   │
  ├───┼───┼───┼───┼───┼───┼───┤
2 │   │   │   │ 1 │ 1 │ 1 │   │
  ├───┼───┼───┼───┼───┼───┼───┤
1 │   │   │   │ 1 │ ? │ 1 │   │
  └───┴───┴───┴───┴───┴───┴───┘
    A   B   C   D   E   F   G
```

## UI (User Interface)

L'interface utilisateur est minimaliste et textuelle. Le plateau de jeu est affiché dans la console avec des caractères ASCII. Le joueur interagit avec le jeu en saisissant des commandes dans la console.

On modélisera les coordonnées de la grille comme dans un échiquier. Les colonnes sont nommées de `A` à `Z` et les lignes de `1` à `N`. La cellule en bas à gauche est donc `A1`, la cellule en haut à droite est `HN` pour une grille de taille HxN.

La saisie de l'utilisateur est de la forme `[A-Z][1-9][0-9]?` où le `?` optionnel indique que la cellule doit être marquée comme minée. On affichera un prompt `>` pour indiquer que l'utilisateur peut saisir une commande. Celle-ci seralue depuis l'entrée standard. On peut imaginer d'autres commandes :

- `q` pour quitter le jeu
- `r` pour recommencer une nouvelle partie
- `a4` pour dévoiler la cellule A4
- `b7?` pour marquer/démarquer la cellule B7

L'architecture logicielle de cette interface utilisateur doit être générique, c'est à dire qu'elle peut être remplacée par une interface graphique dans un second temps, ou même par un rendu `porcelain` (par exemple pour une IA).