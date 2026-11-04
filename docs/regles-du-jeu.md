# Règles du jeu

Le démineur a été inventé par Robert Donner en 1989 et popularisé par
Microsoft Windows. Une grille cache un certain nombre de mines. Le but est
de dévoiler **toutes les cases sûres** sans jamais dévoiler de mine.

## Actions

Le joueur dispose de deux actions :

1. **Dévoiler** une case. Si c'est une mine, la partie est perdue. Sinon la
   case affiche le nombre de mines parmi ses 8 voisines (diagonales comprises).
   Si ce nombre est zéro, toutes les voisines sont dévoilées automatiquement,
   de proche en proche.
2. **Marquer** (ou démarquer) une case d'un drapeau, pour se rappeler qu'on la
   croit minée. Une case marquée ne peut pas être dévoilée par accident, et
   elle arrête la propagation automatique.

**Le premier clic est toujours sûr** : les mines ne sont posées qu'à ce
moment, ailleurs que sous la case choisie et, si la grille le permet, ailleurs
que sur ses voisines. Le premier clic ouvre donc une zone.

La partie est **gagnée** quand toutes les cases sûres sont dévoilées, que les
mines soient marquées ou non. Elle est **perdue** dès qu'une mine est
dévoilée.

## États d'une case dans la console

| Symbole | Signification |
| --- | --- |
| `.` | case non dévoilée |
| `?` | drapeau posé par le joueur |
| ` ` | case dévoilée sans mine voisine |
| `1`…`8` | case dévoilée, nombre de mines voisines |
| `X` | la mine qui a explosé (partie perdue) |
| `*` | une autre mine, montrée en fin de partie perdue |
| `!` | un drapeau posé sur une case sûre, montré en fin de partie perdue |

Quand la partie est gagnée, les mines restantes s'affichent avec un drapeau.

## Coordonnées

La console utilise la notation d'un échiquier : les colonnes sont des
lettres à partir de la gauche, les lignes des nombres à partir du **bas**.
`A1` est la case en bas à gauche.

```text
  Mines: 3   Time: 00:12
  ┌───┬───┬───┬───┬───┬───┬───┐
7 │   │ 1 │ . │ . │ . │ . │ . │
  ├───┼───┼───┼───┼───┼───┼───┤
6 │   │ 1 │ . │ . │ . │ . │ . │
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

Ci-dessus, le joueur a dévoilé `A6` : la colonne A est vide, la zone s'est
ouverte jusqu'aux cases qui touchent une mine.

## Commandes de la console

| Saisie | Effet |
| --- | --- |
| `B3` | dévoile la case B3 |
| `B3?` | pose ou retire un drapeau sur B3 |
| `n`, `new`, `r`, `restart` | nouvelle partie |
| `q`, `quit`, `exit` | quitter |

Les majuscules et minuscules sont indifférentes.

## Commandes de la version graphique

| Action | Effet |
| --- | --- |
| clic gauche | dévoile la case |
| clic droit | pose ou retire un drapeau |
| `N` ou `R` | nouvelle partie |
| `Échap` | quitter |

## Ligne de commande

```bash
minesweeper-console [lignes colonnes mines] [--seed N]
minesweeper-console 16 30 99 --seed 42
```

Sans argument, la partie est une grille 9 × 9 avec 10 mines. La graine
`--seed` rend la position des mines reproductible.
