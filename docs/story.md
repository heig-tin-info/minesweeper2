# Fil pédagogique : du jeu au code, un besoin à la fois

Document de travail, désormais mis en œuvre : le [README](README.md) donne le
parcours. Il propose l'ordre dans lequel les étudiants rencontrent
les problèmes, et donc l'ordre dans lequel les concepts arrivent. Un concept
n'est jamais introduit « parce qu'il est au programme » : il arrive quand le
code qu'on vient d'écrire fait mal, et qu'il faut une réponse.

## Le principe : chaque étape a la même forme

| Rubrique | Question à laquelle elle répond |
| --- | --- |
| **Besoin** | Qu'est-ce qu'on veut obtenir maintenant ? Qu'est-ce qui coince ? |
| **Options** | Quelles solutions naïves ou classiques ? Que coûtent-elles ? |
| **Décision** | Laquelle on retient, et sur quel critère (testabilité, duplication, dépendance…). |
| **Code** | L'extrait minimal qui incarne la décision. |
| **Concept** | Le nom du concept acquis, en une définition. |
| **Vérifier** | Le test (ou le critère observable) qui prouve que ça marche. |
| **Exercice** | Une variation qui oblige à réappliquer le concept. |

Les trois critères de décision reviennent à chaque étape, et les étudiants
doivent finir par les anticiper eux-mêmes :

1. **Peut-on le tester sans clavier, sans écran, sans attendre ?**
2. **A-t-on écrit deux fois la même chose ?**
3. **Qui dépend de qui, et dans quel sens ?**

---

## Partie I — Analyser (aucune ligne de code)

### 1. Présenter le jeu

Besoin
:   Que tout le monde parle du même jeu. Jouer deux parties (terminal, puis
    fenêtre) sur la version finie, pour voir la cible.

Livrable
:   Les règles en une page, et un **lexique** : grille, case, mine, drapeau,
    voisines, case dévoilée, premier clic, partie gagnée/perdue, chronomètre.

Concept
:   **Langage du domaine** : les mots du lexique seront les noms des types.
    On remarque déjà que le chronomètre n'est pas une *règle* du démineur :
    c'est une fonctionnalité du programme. Cette distinction portera toute
    l'architecture.

Existant
:   [01-le-demineur.md](01-le-demineur.md), [regles-du-jeu.md](regles-du-jeu.md).

### 2. Identifier les éléments du jeu

Besoin
:   Passer du texte des règles à une liste de « choses » et d'« actions ».

Méthode
:   Souligner les **noms** (candidats objets) et les **verbes** (candidats
    opérations) dans les règles. Puis trier en trois colonnes :

| Dans le jeu (règles) | Autour du jeu (programme) | Hors du programme (monde) |
| --- | --- | --- |
| grille, case, position, taille, mine, drapeau, voisines, état de la partie | chronomètre, configuration (taille, nombre de mines), nouvelle partie | joueur, clavier, souris, écran, horloge système, hasard |

Concept
:   **Analyse par les noms et les verbes** ; **trois cercles** (règles /
    programme / monde) qui deviendront les trois couches. On n'a pas encore
    dit le mot « architecture ».

Exercice
:   Pour chaque nom, dire s'il a une identité (on peut en avoir deux
    « pareils » mais distincts) ou si c'est une pure valeur. `Pos{2,3}` est
    une valeur ; « la partie en cours » a une identité. Ce sera la différence
    `struct` / `class` de l'étape 9.

### 3. Cas d'utilisation, acteurs, actions

Besoin
:   Savoir précisément ce que le programme doit permettre, et à qui.

Livrable
:   Un diagramme de cas d'utilisation et, pour chaque cas, le scénario nominal
    et les scénarios alternatifs.

| Acteur | Type | Cas d'utilisation |
| --- | --- | --- |
| Joueur | primaire | dévoiler une case, poser/retirer un drapeau, recommencer, quitter, consulter l'état (grille, mines restantes, temps) |
| Horloge | secondaire | fournir l'heure courante |
| Hasard | secondaire | fournir un placement de mines |

Scénarios à écrire : *dévoiler une case sûre* (nominal), *dévoiler une case à
zéro voisine* (ouverture en cascade), *dévoiler une mine* (défaite), *dévoiler
la dernière case sûre* (victoire), *premier clic* (les mines ne sont pas encore
posées), *dévoiler une case marquée* (refusé).

Concept
:   **Acteur primaire / secondaire** ; **scénario nominal / alternatif**.
    Deux observations à faire remarquer, qui serviront plus tard :

    - les verbes du joueur sont de deux natures : ceux qui **changent** la
      partie (dévoiler, marquer, recommencer) et ceux qui la **lisent**
      (consulter). C'est la graine du CQS (étape 13) et des deux ports
      d'entrée (étape 15) ;
    - les acteurs **secondaires** (horloge, hasard) sont des choses dont le
      programme a *besoin*. Ils deviendront les ports de **sortie** (étape 6).

Existant
:   l'emplacement réservé dans [02-vue-ensemble.md](06-ports-et-adaptateurs.md).

### 4. Raconter un scénario de bout en bout

Besoin
:   Voir le système comme une boîte noire qui reçoit des ordres et répond à
    des questions, avant de l'ouvrir.

Livrable
:   Un diagramme de séquence **à deux colonnes** : Joueur ↔ Système. « Je
    clique en B3 » → « voici la nouvelle grille ». Rien d'autre.

Concept
:   **Ordre** (entrant, change l'état) vs **question** (entrante, lit l'état)
    vs **besoin** (sortant : l'heure, le hasard). Trois flèches, trois sens.
    Ce vocabulaire sera réutilisé tel quel pour les ports.

---

## Partie II — Décider l'architecture

### 5. Comparer les approches possibles

Besoin
:   Choisir une organisation du code qui supporte **deux interfaces** (puis
    peut-être une troisième), et qu'on puisse **tester sans jouer**.

Options
:   À présenter chacune en quelques lignes de code et un schéma, avec ce
    qu'elle casse :

| Approche | Idée | Ce qui casse |
| --- | --- | --- |
| A. Tout dans `main()` | une boucle, des tableaux, des `if` | rien n'est testable ; la 2ᵉ interface duplique tout |
| B. Une classe `Board` qui fait tout | elle stocke, applique les règles **et** affiche | c'est la version précédente de ce projet ([14-avant-apres.md](23-avant-apres.md)) : `std::cout` dans les règles, impossible d'ajouter SDL sans tout réécrire |
| C. Modèle / Vue (MVC) | le modèle ne connaît pas la vue | mieux ; mais le temps et le hasard restent dans le modèle, et l'entrée clavier est souvent collée au contrôleur |
| D. Couches (UI → logique → données) | dépendances dans un seul sens | bien pour le sens UI→logique ; mais la logique dépend toujours *vers le bas* de l'horloge et du hasard |
| E. Ports et adaptateurs (hexagonale) | le cœur définit **lui-même** les interfaces dont il a besoin ; tout le reste s'y branche | répond aux trois critères ; coût : quelques interfaces de plus |

Décision
:   E. Le critère qui tranche est la **testabilité** du cœur, et la
    **symétrie** entre ce qui entre (ordres, questions) et ce qui sort
    (besoins). Les étudiants doivent pouvoir dire pourquoi D ne suffit pas :
    parce que l'horloge et le hasard, en D, sont « en dessous » du cœur, donc
    le cœur en dépend.

Concept
:   **Couplage**, **cohésion**, **sens des dépendances**, **inversion de
    dépendance** (le cœur possède l'interface, l'extérieur l'implémente).

### 6. Ports et adaptateurs : la grande image

Besoin
:   Nommer les frontières trouvées à l'étape 4 et les répartir.

Méthode
:   Reprendre les trois flèches de l'étape 4 et les acteurs de l'étape 3 :

| Flèche (étape 4) | Acteur (étape 3) | Port | Sorte | Adaptateurs |
| --- | --- | --- | --- | --- |
| ordre entrant | joueur | `CommandSink` | entrée | console, SDL, tests |
| question entrante | joueur | `GameView` | entrée | console, SDL, tests |
| besoin sortant | horloge | `Clock` | sortie | `SteadyClock`, `FakeClock` |
| besoin sortant | hasard | `MineLayout` | sortie | `RandomMineLayout`, `FixedMineLayout` |

Concept
:   **Port** = interface = contrat ; **adaptateur** = implémentation pour une
    technologie ; **les tests sont un adaptateur comme les autres**. Un port
    d'entrée est *réalisé* par l'application et *utilisé* par l'adaptateur ;
    un port de sortie est *utilisé* par l'application et *réalisé* par
    l'adaptateur. Faire écrire cette phrase par les étudiants, elle est
    toujours confondue.

Existant
:   la figure et le tableau des ports de [02-vue-ensemble.md](06-ports-et-adaptateurs.md).

### 7. Le sens des flux

Besoin
:   Décider **qui appelle qui**, pour qu'il n'y ait jamais de boucle de
    dépendance.

Décision
:   Trois règles, à vérifier sur le diagramme de séquence de l'étape 4
    maintenant ouvert en quatre colonnes (Joueur, adaptateur, application,
    domaine) :

    1. **Les ordres descendent** : l'adaptateur traduit un geste en commande
       et l'envoie vers le centre.
    2. **Les données sont tirées** (*pull*) : le cœur ne prévient personne ;
       c'est l'adaptateur qui relit l'état quand il veut redessiner.
    3. **Chaque couche ne parle qu'à sa voisine** : l'adaptateur ne connaît
       pas `Game`.

Options
:   Discuter l'alternative *push* (le cœur notifie l'interface : patron
    Observateur). Pourquoi on ne la prend pas ici : le cœur devrait connaître
    un abonné, et l'interface SDL redessine de toute façon à cadence fixe.

Concept
:   **Pull vs push**, **qui possède la boucle** (l'adaptateur), **Observateur**
    (nommé, écarté, avec la raison).

Existant
:   le diagramme de séquence de [02-vue-ensemble.md](06-ports-et-adaptateurs.md),
    « Qui possède la boucle » dans [11-adaptateurs.md](18-terminal.md).

### 8. Découper en modules

Besoin
:   Traduire les cercles en dossiers, et empêcher les dépendances interdites
    **par la machine**, pas par la bonne volonté.

Décision
:   `core/` (règles), `app/` (session, ports), `adapters/console/`,
    `adapters/sdl/`, `launcher/` (assemblage), `tests/`. Règle des `#include` :
    un fichier de `core/` n'inclut que `core/` et la bibliothèque standard ;
    `app/` inclut `core/` ; les adaptateurs incluent `app/`. Un test
    ([check_layers.cmake](../tests/check_layers.cmake)) grep les `#include` et
    fait échouer la compilation si la règle est violée.

C'est ici qu'apparaît le **besoin d'une couche application** : le chronomètre
et « recommencer » ne sont ni des règles (donc pas dans `core/`), ni propres à
une interface (sinon on les écrit deux fois). Il faut un endroit entre les
deux : `Session`. On la nomme maintenant, on l'écrira à l'étape 17.

Concept
:   **Module** = dossier + espace de noms + règle d'inclusion ; **point de
    composition** (le seul endroit qui connaît tout : `launcher/`) ; **test
    d'architecture**.

---

## Partie III — Construire le cœur (de l'intérieur vers l'extérieur)

Chaque étape produit du code qui **compile et passe ses tests** avant la
suivante. Aucun affichage avant la partie V. C'est l'ordre du labo `steps/`.

### 9. Positions, taille, voisins

Besoin
:   Désigner une case, savoir si elle est dans la grille, parcourir ses
    voisines sans sortir du tableau.

Options
:   Deux `int` passés partout (on les inverse une fois sur deux) ; une
    `std::pair` (`.first` ne dit pas si c'est la ligne) ; une `struct Pos`.

Concept
:   **Objet valeur** (égalité par contenu, copie libre, pas d'identité) ;
    **`struct` vs `class`** (tout est public parce que toute combinaison est
    valide) ; `constexpr` ; un type itérable (`Neighbors`) à la place d'une
    fonction qui alloue un `std::vector`.

Vérifier
:   Premiers tests GoogleTest : coin, bord, centre → 3, 5, 8 voisines.

Existant
:   [03-objets-valeur.md](09-positions.md), labo étape 1.

### 10. Le champ de mines

Besoin
:   Poser des mines et connaître, pour chaque case, le nombre de voisines
    minées, **sans jamais** avoir un champ incohérent (un compteur faux, une
    mine hors grille).

Options
:   Un `std::vector<bool>` public que tout le monde modifie ; ou une classe
    qui calcule tout dans son constructeur et n'offre que des lectures.

Concept
:   **Invariant** ; **encapsulation** (`private`, constructeur qui valide,
    aucune méthode qui modifie) ; **immuabilité** ; **exception** pour refuser
    une construction impossible (vs `assert` pour une erreur de programmation).

Note de progression
:   À ce stade, `Minefield` stocke ses cases dans un simple `std::vector` avec
    un calcul d'index. On ne parle **pas encore** de `Grid<T>`.

Existant
:   [04-encapsulation-invariants.md](10-champ-de-mines.md), labo étape 2.

### 11. La partie : règles, états, source unique de vérité

Besoin
:   Dévoiler, marquer, détecter la victoire et la défaite, refuser les coups
    interdits.

Options
:   Mettre `reveal()` dans `Minefield` (il connaît les mines) ; ou dans un
    nouvel objet `Game` qui **possède** le champ et les marques. Discuter :
    `Minefield` est immuable et ne sait pas ce que le joueur a fait ; les
    marques sont un état *de la partie*, pas *du terrain*.

Concept
:   **Agrégat et racine** (`Game` est le seul point d'entrée) ; **source
    unique de vérité** (pas de compteur de drapeaux à côté de la grille des
    marques… ou alors dérivé et vérifié) ; **machine à états** (`Status` :
    Ready, Playing, Won, Lost) ; **CQS** : les méthodes `const` lisent, les
    autres modifient, jamais les deux. L'étape 3 l'avait annoncé.

Vérifier
:   Tests de règles avec un champ fixe passé en dur : gagner, perdre,
    sur-marquer, dévoiler une case marquée.

Existant
:   [05-agregat.md](11-la-partie.md), labo étape 3.

### 12. Deux grilles, un seul code : `Grid<T>`

Besoin
:   `Game` a besoin d'une grille de **marques** (caché / drapeau / dévoilé) de
    la même taille que la grille de **cases** de `Minefield`. On vient de
    copier-coller le calcul d'index et la vérification des bornes.

Options
:   Garder la copie (deux bugs pour le prix d'un) ; une classe de base commune
    (héritage pour partager du code : mauvaise raison) ; un **patron de
    classe** `Grid<T>`.

Concept
:   **Template** comme réponse à la duplication, pas comme fin en soi ;
    `static_assert` pour documenter une contrainte ; `assert` sur l'index
    (précondition) vs exception sur la taille (donnée externe).

Vérifier
:   `Minefield` réécrit avec `Grid<Square>`, mêmes tests, aucun changé.
    C'est la première **refactorisation sous tests** : montrer que les tests
    de l'étape 10 ne bougent pas.

Existant
:   [06-templates.md](12-grille-generique.md) — à déplacer après l'agrégat.

### 13. L'ouverture en cascade

Besoin
:   Dévoiler une case à zéro voisine ouvre ses voisines, de proche en proche.

Options
:   Récursif (élégant, mais une grille 100×100 vide fait déborder la pile) ;
    itératif avec une pile explicite.

Concept
:   **Parcours de graphe**, **pile d'appels vs pile explicite**, la différence
    entre « ça marche sur l'exemple » et « ça marche toujours ».

Existant
:   dans [05-agregat.md](11-la-partie.md), labo étape 4. Peut rester une
    section de l'étape 11 si l'on veut moins d'étapes.

### 14. Ce que l'interface a le droit de voir

Besoin
:   L'affichage doit dessiner une case sans pouvoir **tricher** : il ne doit
    pas pouvoir demander « y a-t-il une mine ici ? » tant que la partie n'est
    pas finie.

Options
:   Exposer `Minefield` en lecture (l'interface voit tout) ; un `enum` + un
    `int` (on peut écrire un état absurde) ; un **type par situation** réuni
    dans un `std::variant`.

Concept
:   **Projection / DTO** (ce qu'on montre n'est pas ce qu'on stocke) ;
    **type somme** (`std::variant`) ; **rendre les états illégaux
    irreprésentables** ; `std::visit` (première rencontre, dans un test).

Existant
:   [09-projection-dto.md](14-ce-que-voit-l-interface.md), labo étape 5.

---

## Partie IV — L'application : ce qui n'est pas une règle

### 15. Les ordres du joueur deviennent des valeurs

Besoin
:   Les deux interfaces doivent envoyer « dévoile B3 » au cœur sans que le
    cœur sache si ça vient d'un clavier ou d'une souris.

Options
:   Trois méthodes virtuelles (`reveal`, `toggleFlag`, `newGame`) ; un `enum`
    + une position optionnelle ; **un type par commande** et un `variant`.

Décision
:   Le `variant`, parce qu'une commande-valeur peut être **produite** par une
    fonction pure (parseur, traduction d'événement), **comparée** dans un test
    et **enregistrée** par un espion. Expliquer honnêtement le prix
    (*expression problem*).

Concept
:   **Commande comme valeur** ; les deux ports d'entrée `CommandSink` /
    `GameView` séparés (**ISP** : deux petites interfaces plutôt qu'une
    grosse) ; interface = classe abstraite pure, `virtual`, `override`,
    destructeur virtuel.

Existant
:   [08-commandes-requetes.md](15-commandes.md) y compris
    « Pourquoi pas trois méthodes ? ».

### 16. Le hasard et le premier clic sûr

Besoin
:   Placer les mines au hasard, **mais pas sous le premier clic**, et pouvoir
    tester une partie avec des mines connues.

Options
:   `std::mt19937_64` dans `Game` avec une graine (les tests dépendent de
    l'implémentation du générateur) ; passer la liste des mines au
    constructeur (le premier clic sûr devient impossible : on ne connaît pas
    encore le clic) ; une **interface** `MineLayout` appelée au premier clic.

Concept
:   **Polymorphisme par interface**, **injection de dépendance**, première
    **doublure de test** (`FixedMineLayout`, un *stub*). Le cœur **définit**
    le port, l'adaptateur le réalise : c'est l'inversion de dépendance de
    l'étape 5, vue dans le code.

Note
:   Le labo place cette étape **après** le terminal (étape 7 du labo) pour
    que le jeu soit jouable plus tôt. Les deux ordres se défendent ; la doc
    peut suivre le labo.

Existant
:   [07-polymorphisme-ports.md](16-hasard.md).

### 17. Le temps, puis la `Session`

Besoin
:   Chronométrer la partie du premier clic à la fin, recommencer une partie
    avec la même configuration, et **tester** le chronomètre sans attendre.

Options
:   `std::chrono::steady_clock::now()` dans `Game` (le cœur dépend de
    l'horloge, les tests attendent) ; dans chaque interface (écrit deux fois) ;
    un port `Clock` et un `Stopwatch` **qui reçoit l'heure en paramètre**.

Décision
:   `Session` réalise `GameView` et `CommandSink`, utilise `Clock` et
    `MineLayout`, possède un `Game` et un `Stopwatch`. `NewGame` =
    reconstruire le `Game`, pas le réinitialiser.

Concept
:   **Service applicatif** (orchestration sans règle) ; **héritage multiple
    d'interfaces** ; **les effets de bord aux bords** (l'heure est lue une
    fois, au bord, et passée en valeur) ; **identité vs valeur** : la session
    n'est pas copiable (`= delete`), la partie l'est ; **reconstruire plutôt
    que `reset()`**.

Existant
:   [10-couche-application.md](17-session.md).

---

## Partie V — Les adaptateurs

### 18. Le terminal

Besoin
:   Jouer. Lire « B3 », afficher la grille, boucler jusqu'à « q ».

Décision
:   Trois morceaux séparés et testables : `notation` (texte ↔ `Pos`, texte →
    `Command`, fonctions pures), `renderer` (`GameView` → texte), `ConsoleUi`
    (la boucle, qui reçoit `std::istream&`/`std::ostream&`).

Concept
:   **Fonctions pures de traduction** ; **séparer lire / afficher / boucler** ;
    tester une UI avec `std::istringstream` / `std::ostringstream` ; l'espion
    `RecordingSink` (deuxième doublure).

Existant
:   [11-adaptateurs.md](18-terminal.md) (partie console), labo étape 6.

### 19. Le point de composition

Besoin
:   Quelqu'un doit créer `RandomMineLayout`, `SteadyClock`, `Session`,
    `ConsoleUi` et les brancher. Qui ?

Décision
:   `launcher/launch.cpp` : le **seul** fichier qui connaît toutes les classes
    concrètes. `main()` tient en trois lignes. La ligne de commande y est
    analysée ; une configuration impossible est refusée par le constructeur
    de `Game` (une règle, un seul endroit), et le lanceur affiche l'usage.

Concept
:   **Racine de composition** ; `main()` minimal ; propagation des exceptions
    jusqu'au bord.

### 20. La fenêtre SDL

Besoin
:   La même partie, à la souris. **Sans toucher `core/` ni `app/`** : c'est le
    test final de l'architecture.

Décision
:   `SdlUi::translate(event) → optional<Command>`, `cellAt(x, y)`, redessin à
    cadence fixe en tirant l'état par `GameView`.

Concept
:   Un deuxième adaptateur sur **les mêmes ports** ; boucle d'événements ;
    confirmation du *pull* de l'étape 7.

Existant
:   [11-adaptateurs.md](18-terminal.md) (partie SDL), labo étape 8.

### 21. Libérer sans fuir : RAII

Besoin
:   Fenêtre, rendu, textures, polices, bibliothèque : tout doit être libéré,
    dans le bon ordre, **même si une exception survient au milieu**.

Options
:   `SDL_Destroy*` à la main dans le destructeur (oublié dès la première
    exception) ; `std::unique_ptr` avec un *deleter* ; une classe garde pour
    `SDL_Init`/`SDL_Quit`.

Concept
:   **RAII**, **durée de vie**, **ordre de destruction = inverse de l'ordre
    de déclaration**, *deleter* personnalisé, règle de zéro.

Existant
:   [12-raii.md](21-raii.md).

---

## Partie VI — Vérifier et prendre du recul

### 22. La stratégie de tests, relue

Besoin
:   Montrer que la testabilité n'a pas été un bonus mais le **critère** qui a
    guidé chaque décision.

Livrable
:   La pyramide du projet : tests de règles (`Game` + `FixedMineLayout`),
    d'application (`Session` + `FakeClock`), d'adaptateur (console + flux
    mémoire + `RecordingSink`), d'architecture (`check_layers`). Nommer les
    doublures : *stub*, *fake*, *spy*.

Existant
:   [13-tests.md](22-tests.md).

### 23. Avant / après

Besoin
:   Revenir à l'approche B de l'étape 5 avec les yeux de maintenant : chaque
    défaut de l'ancienne version, le principe violé, l'étape qui l'a corrigé.

Existant
:   [14-avant-apres.md](23-avant-apres.md).

### 24. Et maintenant ?

Besoin
:   Prouver que l'architecture **absorbe le changement**. Exercices :
    la commande *chord*, un tableau des meilleurs temps (nouveau port de
    sortie : où ?), sauvegarder/reprendre (pourquoi `Game` est copiable),
    une interface web (quel adaptateur, quels ports).

---

## Carte des concepts : quand, et parce que quoi

| Concept | Étape | Déclencheur (« le besoin ») |
| --- | --- | --- |
| Langage du domaine, règle vs fonctionnalité | 1 | parler du même jeu |
| Noms → types, verbes → opérations | 2 | passer des règles aux objets |
| Acteurs primaires / secondaires, scénarios | 3 | savoir ce qu'on doit permettre |
| Ordre / question / besoin | 4 | voir le système en boîte noire |
| Couplage, dépendance, inversion de dépendance | 5 | deux interfaces + tests |
| Port, adaptateur | 6 | nommer les frontières |
| Pull vs push, Observateur | 7 | décider qui appelle qui |
| Module, test d'architecture, couche application | 8 | empêcher les dépendances interdites |
| Objet valeur, `struct`/`class`, `constexpr` | 9 | désigner une case |
| Invariant, encapsulation, immuabilité, exception | 10 | un champ jamais incohérent |
| Agrégat, SSOT, machine à états, CQS | 11 | où mettre `reveal()` |
| Template, `assert` vs exception, refactoring sous tests | 12 | une deuxième grille |
| Parcours de graphe, pile explicite | 13 | la cascade déborde la pile |
| Projection, `variant`, états illégaux irreprésentables | 14 | l'interface ne doit pas tricher |
| Commande-valeur, ISP, interface pure | 15 | deux interfaces, un cœur |
| Polymorphisme, injection, stub | 16 | tester avec des mines connues |
| Service applicatif, effets de bord aux bords, identité | 17 | le chrono n'est pas une règle |
| Fonctions pures de traduction, spy | 18 | tester une UI |
| Racine de composition | 19 | qui crée quoi |
| Mêmes ports, autre technologie | 20 | SDL sans toucher le cœur |
| RAII, durée de vie, deleter | 21 | ne rien fuir |
| Stub / fake / spy, pyramide | 22 | relire la testabilité |

---

## Ce que ça change dans `docs/`

Proposition, à discuter :

| Aujourd'hui | Demain |
| --- | --- |
| 01 Le démineur | Partie I, étape 1 (inchangé) |
| 02 Vue d'ensemble | éclaté en étapes 2 à 8 (Partie I et II). Le cas d'utilisation manquant devient l'étape 3 |
| 03 Objets valeur | étape 9 |
| 04 Encapsulation | étape 10, **sans** `Grid<T>` (vector + index) |
| 05 Agrégat | étapes 11 et 13 |
| 06 Templates | étape 12, **après** l'agrégat, présenté comme refactorisation |
| 07 Polymorphisme et ports | étape 16, recentré sur `MineLayout` ; `Clock` passe à l'étape 17 |
| 08 Commandes et requêtes | étape 15 ; le CQS lui-même remonte à l'étape 11 |
| 09 Projection | étape 14 |
| 10 Couche application | étape 17 |
| 11 Adaptateurs | étapes 18, 19 (nouvelle : le lanceur), 20 |
| 12 RAII | étape 21 |
| 13 Tests | étape 22 |
| 14 Avant / après | étape 23 (+ un aperçu dès l'étape 5) |
| — | étape 24, nouvelle : exercices d'extension |

Chaque chapitre adopte le gabarit **Besoin / Options / Décision / Code /
Concept / Vérifier / Exercice**. Le contenu existant est largement
réutilisable : il manque surtout les rubriques *Options* (les solutions
naïves qu'on écarte) et *Vérifier*.

## Questions ouvertes

1. **Le hasard avant ou après le terminal ?** Le labo le met après (jeu
   jouable plus tôt). La doc peut suivre le labo (étape 16 → entre 18 et 19).
2. **Faut-il faire écrire la version naïve (approche B) ?** Une séance où
   les étudiants écrivent `Board` tout-en-un puis doivent ajouter SDL rend
   l'étape 5 inoubliable, mais coûte une semaine.
3. **Un seul document ou deux ?** Un fil unique de 24 étapes ; ou un fil
   court (Parties I-II, ~15 pages) suivi de fiches par étape. Le PDF se prête
   au fil unique ; GitHub aux fiches.
4. **Les étapes 13 et 19 méritent-elles un chapitre ?** Elles peuvent être
   des sections de 11 et 18 pour garder une vingtaine d'étapes.
