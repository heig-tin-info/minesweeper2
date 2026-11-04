# Les éléments du jeu

> **Objectif.** Passer du texte des règles à une liste de « choses » et
> d'« actions », et trier ces choses selon qu'elles appartiennent au jeu, au
> programme, ou au monde extérieur.

## Le besoin

Nous avons un lexique et des règles en prose. Un programme, lui, est fait de
**types** et d'**opérations**. Il faut un pont entre les deux, et ce pont ne
doit pas être improvisé : si chacun extrait « ses » objets du texte à sa
manière, on obtient une classe `Case` chez l'un, une classe `Grille` qui fait
tout chez l'autre, et une classe `Jeu` de deux mille lignes chez le troisième.

Il faut aussi, dès maintenant, séparer ce qui est **dans le jeu** de ce qui est
**autour**. Le clavier, l'écran et l'horloge sont indispensables au programme,
mais le démineur n'en parle jamais. Les mélanger aux règles est la première
cause de programmes qu'on ne peut ni tester ni réutiliser.

## La méthode

La méthode est ancienne et mécanique, c'est sa force : on souligne les
**noms** et les **verbes** dans le texte des règles.

- Un **nom** est un candidat **objet** (ou attribut d'un objet).
- Un **verbe** est un candidat **opération**, et il a un **sujet** : qui fait
  l'action ?

Puis on trie les noms en trois cercles concentriques :

1. **Les règles** : ce dont parle le démineur lui-même. Enlevez-le, ce n'est
   plus le démineur.
2. **Le programme** : ce que notre logiciel ajoute autour des règles, et qui
   serait là quelle que soit l'interface.
3. **Le monde** : ce qui existe en dehors du programme et avec quoi il
   dialogue.

Prenons le texte de [Règles du jeu](regles-du-jeu.md) et appliquons.

## Le livrable

### Les noms

Dans les deux premières sections des règles, les noms soulignés sont :
*grille*, *mine*, *case*, *case sûre*, *joueur*, *action*, *voisines*,
*nombre de mines*, *drapeau*, *propagation*, *premier clic*, *partie*. Les
sections suivantes ajoutent *symbole*, *coordonnées*, *ligne*, *colonne*,
*console*, *clic*, *touche*, *ligne de commande*, *graine*, *temps*.

Triés en trois cercles :

| Dans le jeu (règles) | Autour du jeu (programme) | Hors du programme (monde) |
| --- | --- | --- |
| grille, taille | chronomètre, temps écoulé | joueur |
| case, position (ligne, colonne) | configuration : taille, nombre de mines | clavier, souris |
| mine, case sûre | nouvelle partie | écran, terminal, fenêtre |
| drapeau | graine du hasard | horloge système |
| voisines, nombre de mines voisines | | générateur de hasard |
| case dévoilée, case cachée | | |
| premier clic | | |
| état de la partie : en cours, gagnée, perdue | | |

Quelques noms ont disparu : *symbole*, *coordonnées* (« B3 »), *clic*,
*touche*. Ce sont des **représentations** d'un concept du jeu pour une
interface donnée. `B3` et un clic en (130, 210) pixels désignent tous deux une
*position* : la position est dans le jeu, la façon de la taper ou de la
cliquer est dans le monde.

### Les verbes

| Verbe | Sujet | Complément | Cercle |
| --- | --- | --- | --- |
| dévoiler | le joueur | une case | règles |
| marquer, démarquer | le joueur | une case | règles |
| compter (les mines voisines) | le jeu | une case | règles |
| ouvrir de proche en proche | le jeu | une zone | règles |
| poser (les mines) | le jeu, au premier clic | la grille | règles |
| gagner, perdre | la partie | | règles |
| recommencer | le joueur | une partie | programme |
| chronométrer | le programme | la partie | programme |
| quitter | le joueur | le programme | programme |
| afficher | l'interface | l'état de la partie | monde |
| taper, cliquer | le joueur | une position | monde |
| donner l'heure | l'horloge | | monde |
| tirer au sort | le hasard | des positions | monde |

Deux remarques à garder sous le coude :

- Les verbes du joueur sur le jeu sont de **deux natures**. *Dévoiler*,
  *marquer* et *recommencer* **changent** la partie. *Afficher* ne change rien :
  il **lit** l'état. Cette différence reviendra à l'[étape 3](03-cas-d-utilisation.md),
  puis structurera le code à l'[étape 11](11-la-partie.md).
- *Donner l'heure* et *tirer au sort* ont pour sujet quelque chose **hors du
  programme**. Le programme ne les fait pas : il les **demande**. Nous y
  reviendrons à l'[étape 6](06-ports-et-adaptateurs.md).

### Identité ou valeur ?

Pour chaque nom du premier cercle, posez une dernière question : *si j'en ai
deux « pareils », sont-ils distincts ?*

| Nom | Deux « pareils » sont… | Nature |
| --- | --- | --- |
| position (2, 3) | la même position | **valeur** |
| taille 9 × 9 | la même taille | **valeur** |
| nombre de mines voisines : 3 | le même nombre | **valeur** |
| la partie en cours | deux parties différentes, même si les grilles sont identiques | **identité** |
| la grille de mines | deux champs différents | **identité** |

Une **valeur** se copie librement, se compare par son contenu, et n'a pas
d'histoire. Une chose qui a une **identité** se désigne (« la partie que je
suis en train de jouer »), évolue, et la copier serait une erreur ou un acte
délibéré (une sauvegarde). La différence paraît philosophique ; à
l'[étape 9](09-positions.md), elle deviendra la différence entre `struct` et
`class`.

## Les concepts

### L'analyse par les noms et les verbes

Souligner les **noms** et les **verbes** d'un énoncé pour en tirer les objets
et leurs opérations. Ce n'est pas une recette magique : elle produit des
*candidats*, dont certains disparaîtront (*symbole*) ou fusionneront
(*coordonnées* et *clic* dans *position*). Sa vertu est de forcer à partir du
**texte**, pas de l'idée qu'on se fait déjà du code.

### Les trois cercles

**Règles**, **programme**, **monde** : trois cercles concentriques, du plus
stable au plus changeant. Les règles du démineur n'ont pas bougé depuis 1989 ;
notre programme ajoute un chronomètre et une graine ; le monde, lui, change
avec chaque interface. On ne sait pas encore ce qu'est une « couche
logicielle », mais on vient d'en dessiner trois.

### Identité et valeur

Un objet **valeur** est entièrement défini par son contenu : deux positions
(2, 3) sont interchangeables. Un objet à **identité** est défini par ce qu'il
*est*, pas par ce qu'il *contient* : la partie en cours reste la même partie
après un coup, bien que son contenu ait changé.

## Vérifier

- **Chaque terme du lexique** de l'[étape 1](01-le-demineur.md) a trouvé une
  colonne, et une seule. Si un terme hésite entre deux colonnes, la
  définition du lexique est à préciser.
- **Chaque verbe a un sujet.** Un verbe sans sujet clair (« la case
  s'ouvre ») cache une décision qu'il faudra prendre : *qui* ouvre la case ?
- **Rien du troisième cercle n'apparaît dans le premier.** Si vous lisez
  « la grille s'affiche » ou « la partie lit le clavier », vous avez mélangé
  le jeu et le monde.

## À retenir

- Les **noms** des règles donnent les objets, les **verbes** donnent les
  opérations et chaque verbe a un **sujet**.
- Trois cercles : **règles**, **programme**, **monde**. Le clavier, l'écran,
  l'horloge et le hasard sont dans le monde, jamais dans les règles.
- Les verbes du joueur **changent** ou **lisent** la partie ; les verbes de
  l'horloge et du hasard sont des choses que le programme **demande**.
- Une **valeur** se compare par son contenu ; une **identité** se désigne et
  évolue.

## Exercices

1. Ajoutez la règle du *chord* (voir l'[étape 1](01-le-demineur.md),
   exercice 2) au texte des règles, puis soulignez ses noms et ses verbes.
   Quel est le sujet de « ouvre » ? Apparaît-il un nouveau nom, ou seulement
   un nouveau verbe ?
2. On veut un **tableau des meilleurs temps**, conservé entre deux lancements
   du programme. Listez les noms et les verbes que cela ajoute, et placez-les
   dans les trois cercles. Lequel des trois reçoit « le fichier sur le
   disque » ?
3. Les règles disent que les mines sont posées « ailleurs que sur ses
   voisines, si la grille le permet ». Qui est le sujet de « permet » ? Quel
   objet du premier cercle devra savoir répondre ?
