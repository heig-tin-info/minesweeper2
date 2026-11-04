# Et maintenant ?

> **Objectif.** Prouver que l'architecture **absorbe le changement** : quatre
> extensions réalistes, et pour chacune la question qui compte, « quels
> fichiers changent ? ». Une architecture ne se juge pas sur un schéma, mais
> sur ce qu'une nouvelle fonctionnalité coûte.

## Le besoin

Le jeu est fini, testé, jouable dans deux interfaces. Mais la vraie question
d'un logiciel n'est pas « marche-t-il aujourd'hui ? », c'est « que se
passe-t-il quand on le change ? ». Les étapes précédentes ont fait des
promesses : le cœur ne connaît pas l'interface, le hasard et le temps sont
injectés, les commandes sont des valeurs. Il est temps de les mettre à
l'épreuve avec des demandes qu'un vrai joueur ferait.

## Les options

| Option | Idée | Ce qui casse |
| --- | --- | --- |
| A. Juger sur le diagramme | compter les flèches, admirer les couches | un beau schéma peut cacher un cœur impossible à étendre |
| B. Juger sur le changement | faire une extension, regarder `git diff --stat` | rien ; c'est la seule mesure honnête |

## La décision

Option B. Pour chaque extension, on répond dans l'ordre à trois questions,
**avant** d'écrire une ligne :

1. **Quelle couche ?** Est-ce une règle du jeu (`core/`), une affaire de
   séance (`app/`), ou de présentation (`adapters/`) ?
2. **Quels fichiers changent, et lesquels ne changent pas ?** La liste de ce
   qui ne bouge pas est aussi importante que l'autre.
3. **Quel test prouve que c'est fait ?** Avec quelle doublure.

Le critère de réussite est le même partout : `git diff --stat` ne montre que
les fichiers annoncés à la question 2, et `make test` passe.

## Les concepts

### Ouvert à l'extension, fermé à la modification

Le principe **ouvert/fermé** dit qu'on doit pouvoir ajouter un comportement
sans modifier le code existant. Les ports le réalisent : un nouvel adaptateur
s'ajoute sans toucher `Session`. Mais le principe a des limites choisies : le
`std::variant` des commandes est volontairement **fermé** à l'ajout d'un type
(l'[étape 15](15-commandes.md) l'a expliqué, c'est l'*expression problem*).
Ajouter une commande touche donc chaque `std::visit`, et c'est **voulu** : le
compilateur refuse qu'on en oublie un.

### L'architecture se juge au changement

La mesure d'une architecture, c'est la liste des fichiers touchés par une
fonctionnalité, rapportée à ce que la fonctionnalité concerne vraiment. Une
règle de jeu qui oblige à modifier l'interface SDL, ou une couleur qui oblige
à modifier `Game`, signale une frontière mal placée. Chaque exercice ci-dessous
donne la liste attendue : si la vôtre est plus longue, cherchez pourquoi avant
de continuer.

## À retenir

- Une architecture se **mesure** : `git diff --stat` après une extension.
- Une règle de jeu ne touche que `core/` et ses tests, plus un `visit` par
  adaptateur si elle ajoute une commande.
- Un nouveau besoin du monde extérieur (un fichier, un réseau) est un **port
  de sortie** de plus, avec sa doublure.
- Une nouvelle interface est un **adaptateur** de plus : rien d'autre ne bouge.
- Ce que `Game` est copiable et `Session` ne l'est pas n'est pas un détail :
  c'est ce qui rend la sauvegarde possible et l'ambiguïté impossible.

## Exercices

### 1. La commande *chord*

Dans les démineurs modernes, cliquer sur un chiffre dévoilé dont le nombre de
drapeaux voisins est atteint ouvre d'un coup toutes ses voisines non
marquées. Si un drapeau était faux, on perd.

Questions à se poser : est-ce une règle ? Oui, elle décide ce qui est
dévoilé et quand on perd, donc elle vit dans `Game`. Comment le joueur la
demande-t-il ? C'est une nouvelle **commande**, `struct Chord { Pos pos; };`,
ajoutée au `variant` de [commands.hpp](../src/app/commands.hpp). Que doit
faire `Game::chord(Pos)` ? Rien si la case n'est pas un chiffre dévoilé ou si
le compte de drapeaux diffère ; sinon `reveal` sur chaque voisine `Hidden`,
en réutilisant `open` et `Neighbors`.

Fichiers qui changent : `core/game.hpp`, `core/game.cpp`,
`app/commands.hpp`, `app/session.cpp` (le `visit` ne compile plus tant que
`Chord` n'est pas traité : c'est le filet de l'[étape 15](15-commandes.md)),
`console/notation.cpp` (une syntaxe, par exemple `B3!`), `sdl/sdl_ui.cpp`
(le clic du milieu). Fichiers qui **ne changent pas** : `Minefield`, `Grid`,
`CellView`, `GameView`, `Session.hpp`, le lanceur.

Tests : dans `game_tests.cpp`, avec `FixedMineLayout`, trois cas : le compte
est atteint et les voisines s'ouvrent ; un drapeau est faux et la partie est
perdue ; le compte n'est pas atteint et rien ne bouge. Dans
`console_tests.cpp`, `parseInput("B3!", size)` donne `Chord`. Réussi quand
ces tests passent et que `git diff --stat` ne liste que les six fichiers.

### 2. Un tableau des meilleurs temps

On veut garder les cinq meilleurs temps par configuration, d'une exécution à
l'autre, et les afficher.

Questions à se poser : est-ce une règle ? Non, le démineur n'a pas de règle
sur les records ; c'est une affaire de **séance**, donc `app/`. D'où vient la
persistance ? Du monde extérieur (un fichier, demain un serveur) : c'est un
nouveau **port de sortie**, à côté de `Clock` dans
[ports.hpp](../src/app/ports.hpp), par exemple
`class HighScores { virtual void record(GameConfig, std::chrono::seconds) = 0; … }`.
Qui l'appelle ? `Session`, au moment où elle détecte la victoire, dans la
méthode qui gère déjà la transition de statut. Qui le réalise ? Un adaptateur
`FileHighScores` créé par le lanceur, comme `SteadyClock`. Comment l'afficher ?
Si l'interface doit montrer le record, `GameView` gagne une requête ; c'est
un port d'**entrée** qui change, donc les deux adaptateurs suivent.

Fichiers qui changent : `app/ports.hpp`, `app/session.hpp/.cpp`,
`launcher/launch.cpp`, un nouvel adaptateur, et les deux interfaces si l'on
affiche. Fichiers qui ne changent pas : tout `core/`.

Tests : une doublure `InMemoryHighScores` (un *fake*, comme `FakeClock`)
dans `test_doubles.hpp` ; dans `session_tests.cpp`, gagner avec
`FixedMineLayout` après `clock.advance(7s)` et vérifier que le fake a reçu
`7s`. Réussi quand ce test passe sans qu'aucun fichier n'ait été écrit sur le
disque pendant `make test`.

### 3. Sauvegarder et reprendre

On veut quitter en pleine partie et la retrouver au prochain lancement, ou
annuler le dernier coup.

Questions à se poser : pourquoi `Game` est-il copiable alors que `Session`
ne l'est pas ([étape 17](17-session.md)) ? Parce qu'une partie est un
**état** et qu'en garder une copie a un sens : `Game saved = game;` est un
instantané gratuit. Annuler, c'est garder une pile de copies dans `Session`
et revenir à la précédente : `app/` seulement, pas une règle de plus dans
`core/`. Sauvegarder sur disque est plus délicat : il faut sérialiser les
mines et les marques, que `Game` cache soigneusement. Deux réponses
possibles, à discuter : une valeur `Game::Snapshot` que `Game` sait produire
et reconstruire (le cœur garde le contrôle de ce qui sort), ou rejouer la
partie depuis la graine et la liste des commandes (rien à exposer, mais
`RandomMineLayout` doit rester déterministe, ce que le test
`RandomMineLayout.IsReproducibleWithTheSameSeed` garantit déjà).

Fichiers qui changent : `app/session.*` et `app/commands.hpp` (une commande
`Undo`), un port de sortie `GameStore` et son adaptateur fichier ;
`core/game.*` seulement si l'on choisit le `Snapshot`.

Tests : annuler après un coup rend la grille précédente, case par case ;
annuler sur une partie neuve ne fait rien ; sauvegarder puis recharger donne
le même `cell(p)` partout. Réussi quand `Session` reste non copiable et que
le test de rechargement passe avec un store en mémoire.

### 4. Une interface web

On veut jouer dans un navigateur : le serveur reçoit `POST /reveal` avec une
position et répond l'état en JSON.

Questions à se poser : où est la boucle ? Il n'y en a pas, une requête
arrive et on répond ; c'est un troisième style après la boucle bloquante et
la boucle d'événements de l'[étape 20](20-fenetre-sdl.md). Que traduit
l'adaptateur ? Le corps JSON en `Command`, puis `GameView` en JSON par un
`std::visit` sur `CellView`, exactement comme `drawCell`. Combien de
sessions ? Une par joueur connecté : l'hypothèse « une `Session` par
exécution » de l'[étape 17](17-session.md) tombe, et c'est le lanceur web qui
en crée une par client. `Session` elle-même n'a rien à savoir de cela.

Fichiers qui changent : un nouveau dossier `adapters/web/` et son `main`.
Fichiers qui ne changent pas : `core/`, `app/`, la console, SDL.

Tests : les fonctions de traduction JSON ↔ `Command` / `CellView` sont pures
et se testent sans serveur, comme `notation` à l'[étape 18](18-terminal.md).
Réussi quand `git diff --stat -- src/core src/app` est vide et que
`architecture.dependency_rule` passe toujours.
