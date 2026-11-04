# La stratégie de tests, relue

> **Objectif.** Relire tous les tests du projet d'un coup, et montrer que la
> testabilité n'a pas été un bonus obtenu à la fin : elle a été le **critère**
> de presque chaque décision des étapes précédentes.
>
> **Code :** [tests/](../tests/), [test_doubles.hpp](../tests/test_doubles.hpp),
> [check_layers.cmake](../tests/check_layers.cmake)

## Le besoin

À chaque étape, la première question était « peut-on le tester sans écran,
sans clavier, sans attendre ? ». Il faut maintenant vérifier que la réponse
est oui partout, et comprendre **comment** : quelles doublures, à quel
niveau, pour quelle promesse. Un test difficile à écrire est presque toujours
le symptôme d'un problème de conception. Dans l'ancienne version, tester une
défaite demandait de **fouiller** le plateau pour trouver où le hasard avait
mis les mines :

```cpp
// Ancien test : fouiller le plateau en passant par une porte dérobée
for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
        if (core.cell(x, y).bomb()) bombs.emplace_back(x, y);
```

Ce test accédait à `bomb()`, l'information que le jeu est censé cacher, et
dépendait du hasard.

## Les options

| Option | Idée | Ce qui casse |
| --- | --- | --- |
| A. Tester en jouant | lancer le programme, cliquer, regarder | lent, non reproductible, impossible en intégration continue |
| B. Tester tout par l'interface | piloter la console avec des flux, vérifier le texte | une règle cassée se voit à travers trois couches ; le message d'erreur parle de caractères, pas de règles |
| C. Une pyramide | beaucoup de tests de règles, quelques tests d'application et d'adaptateur, un test d'architecture | rien, à condition que chaque couche soit isolable : c'est ce que les ports permettent |

## La décision

Option C. Le projet compte 45 tests, répartis en quatre niveaux :

| Niveau | Ce qu'on vérifie | Avec | Fichiers |
| --- | --- | --- | --- |
| **Règles** | `Game`, `Minefield`, `Pos`, `Grid` | `FixedMineLayout` | `game_tests`, `minefield_tests`, `position_tests` |
| **Application** | `Session`, `Stopwatch`, `RandomMineLayout`, les arguments | `FixedMineLayout`, `FakeClock` | `session_tests`, `mine_layout_tests`, `launcher_tests` |
| **Adaptateur** | notation, rendu, boucle de la console | flux mémoire, `RecordingSink` | `console_tests` |
| **Architecture** | la règle de dépendance | un script CMake | `check_layers.cmake` |

Les trois doublures tiennent en cinquante lignes :

```cpp
// Stub (returns the mines it was given) and spy (records how it was called).
class FixedMineLayout final : public MineLayout {
public:
    explicit FixedMineLayout(std::vector<Pos> mines) : mines_(std::move(mines)) {}

    std::vector<Pos> place(Size, int, Pos firstClick) override {
        ++calls;
        lastFirstClick = firstClick;
        return mines_;
    }

    int calls = 0;
    std::optional<Pos> lastFirstClick;

private:
    std::vector<Pos> mines_;
};

// Fake: a working clock whose time only moves when the test says so.
class FakeClock final : public Clock {
public:
    TimePoint now() const override { return now_; }
    void advance(std::chrono::seconds d) { now_ += d; }

private:
    TimePoint now_{};
};

// Spy on the write port.
class RecordingSink final : public CommandSink {
public:
    void handle(const Command& command) override { received.push_back(command); }
    std::vector<Command> received;
};
```

Avec elles, le test de défaite **décide** où sont les mines et se lit comme
une phrase :

```cpp
FixedMineLayout layout{{{0, 1}}};
Game game{{3, 3}, 1, layout};
game.reveal({1, 1});
game.reveal({0, 1});
EXPECT_EQ(game.status(), Status::Lost);
```

## Les concepts

### Stub, fake, spy

Une **doublure** (*test double*) remplace une vraie dépendance pendant un
test. Le vocabulaire de Gerard Meszaros en distingue plusieurs sortes :

| Sorte | Rôle | Ici |
| --- | --- | --- |
| **Stub** | renvoie des réponses préparées | `FixedMineLayout` renvoie les mines voulues |
| **Spy** | enregistre comment on l'a appelé | `FixedMineLayout::calls`, `RecordingSink::received` |
| **Fake** | une implémentation qui marche vraiment, mais simplifiée | `FakeClock`, dont le temps n'avance que sur ordre |

Toutes passent par un **port**. Aucune n'aurait été possible si `Game`
tirait lui-même au sort ou si `Stopwatch` lisait l'horloge système.

### La testabilité comme critère de conception

Relisons cinq décisions des étapes précédentes avec le test qu'elles ont
rendu possible :

- **Étape [11](11-la-partie.md)** : `status()` est **calculé**, jamais stocké.
  Test : `OneMineGame.CommandsAreIgnoredOnceTheGameIsOver`, aucune
  incohérence possible à observer.
- **Étape [15](15-commandes.md)** : une commande est une **valeur**.
  Test : `ConsoleTest.TranslatesTypedLinesIntoCommands` compare un
  `std::vector<Command>`.
- **Étape [16](16-hasard.md)** : le hasard derrière le port `MineLayout`.
  Test : `OneMineGame.PlantsMinesOnTheFirstRevealOnly`, `layout.calls == 1`.
- **Étape [17](17-session.md)** : `Stopwatch` reçoit l'heure **en paramètre**.
  Test : `Stopwatch.MeasuresFromStartToStop` n'a besoin d'aucune doublure.
- **Étape [18](18-terminal.md)** : `ConsoleUi` reçoit `std::istream&` et
  `std::ostream&`. Test : `ConsoleTest.PlaysAWholeGame` gagne une partie
  avec `"C1\n"`.

Le *fake* en action :

```cpp
TEST_F(SessionTest, ClockStopsWhenTheGameEnds) {
    session.handle(Reveal{{2, 1}});
    clock.advance(3s);
    session.handle(Reveal{{0, 0}});
    ASSERT_EQ(session.status(), Status::Lost);
    clock.advance(100s);
    EXPECT_EQ(session.elapsed(), 3s);
}
```

Ce test dure moins d'une milliseconde et vérifie un comportement de cent
secondes.

### Tester des comportements, pas des implémentations

Les tests passent par les **méthodes publiques** et vérifient ce qu'un
utilisateur peut observer. Aucun test ne lit `hiddenSafe_` ni ne connaît
`Mark`. On peut donc réécrire l'intérieur de `Game` (remplacer la pile par
une file, changer la représentation des marques) sans toucher un seul test :
c'est ce qui a permis la refactorisation de l'[étape 12](12-grille-generique.md).
Un test qui casse à chaque refactorisation finit toujours supprimé.

Le nom de chaque test est une **phrase** qui décrit une règle :
`RevealingANumberOpensOnlyThatCell`, `FlagsStopTheFloodFill`. La liste des
tests se lit comme la spécification du jeu. Quelques-uns vérifient des
propriétés qu'on oublierait :

- `Game.FloodFillHandlesHugeEmptyGrids` : une grille de 1000 × 1000 ne fait
  pas déborder la pile ([étape 13](13-ouverture-en-cascade.md)) ;
- `Game.RejectsALayoutThatBreaksItsContract` : l'agrégat se protège d'un
  collaborateur défaillant ;
- `RandomMineLayout.IsReproducibleWithTheSameSeed` : la graine rend une
  partie rejouable ;
- `Arguments.LeavesGameRulesToTheDomain` : le parseur ne valide **pas** les
  règles, il laisse ce rôle à `Game` ([étape 19](19-lanceur.md)).

### Tester l'architecture elle-même

Une règle d'architecture qu'on ne vérifie pas finit violée, souvent par un
`#include` « juste pour déboguer ». La règle de dépendance de
l'[étape 8](08-modules.md) est un **test exécutable** :

```cmake
set(io "<(iostream|fstream|cstdio|SDL)")

forbid(core "\"(app|adapters|launcher)/" "the domain depends on nothing")
forbid(core "${io}" "the domain does no I/O")
forbid(core "<chrono>" "the domain does not know the time")
forbid(app "\"(adapters|launcher)/" "the application does not know its adapters")
forbid(app "${io}" "the application does no I/O")
```

Il tourne avec les autres (`architecture.dependency_rule` dans `ctest`).
Ajoutez `#include <iostream>` dans `core/position.hpp`, et `make test` échoue
en citant le fichier, la ligne et la raison. C'est une **fonction de
*fitness*** architecturale : un garde-fou automatique qui protège une
décision de conception à chaque compilation, sans dépendre de la vigilance de
quiconque.

## Vérifier

```bash
make test
```

La sortie attendue se termine par :

```text
100% tests passed, 0 tests failed out of 45

Total Test time (real) =   0.54 sec
```

Une demi-seconde pour l'ensemble, dont une grille de un million de cases.
C'est le prix de tests qui n'ouvrent ni fenêtre ni fichier et n'attendent
aucune horloge.

## À retenir

- Un test **difficile à écrire** signale un problème de **conception**, pas
  un problème de test.
- **Stub**, **spy**, **fake** : chaque doublure a son rôle, et toutes passent
  par un port.
- Une **pyramide** : beaucoup de tests de règles, peu de tests d'adaptateur,
  un test d'architecture.
- Testez les **comportements observables**, pas les détails internes : les
  tests survivent aux refactorisations.
- Une règle d'architecture importante mérite son **test automatique**.

## Exercices

1. Écrivez le test d'une nouvelle règle : « un drapeau posé sur une case la
   protège d'un `reveal` ». Quelles doublures faut-il ? Dans quel fichier ?
2. Quelle règle ajouteriez-vous à `check_layers.cmake` pour les adaptateurs ?
   (Indice : la console a-t-elle le droit d'inclure SDL ?) Écrivez l'appel à
   `forbid` correspondant.
3. `SdlUi` n'a aucun test ([étape 20](20-fenetre-sdl.md)). Classez ce qu'elle
   fait en trois tas : testable tel quel, testable après extraction d'une
   fonction pure, non testable sans écran. Que reste-t-il dans le troisième ?
