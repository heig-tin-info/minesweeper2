# Le temps, puis la `Session`

> **Objectif.** Loger ce qui n'est pas une règle du démineur mais dont le
> programme a besoin : le chronomètre et « recommencer ». Écrire l'objet qui
> réalise enfin les deux ports d'entrée.
>
> **Code :** [src/app/session.hpp](../src/app/session.hpp),
> [src/app/session.cpp](../src/app/session.cpp),
> [src/app/stopwatch.hpp](../src/app/stopwatch.hpp)

## Le besoin

Trois choses manquent encore, et aucune n'est une règle du jeu :

- **chronométrer** la partie, du premier clic à la fin ;
- **recommencer** une partie avec la même configuration ;
- **réaliser** les ports `GameView` et `CommandSink` écrits à
  l'[étape 15](15-commandes.md) : ce sont des interfaces, personne ne les
  implémente encore.

Et une contrainte, toujours la même : **tester le chronomètre sans attendre**.
Un test qui dort trois secondes pour vérifier « 00:03 » est un test qu'on
finit par supprimer.

## Les options

| Option | Idée | Ce qui coince |
| --- | --- | --- |
| A. `now()` dans `Game` | `Game` lit `std::chrono::steady_clock::now()` au premier clic et à la fin | le cœur dépend de l'horloge système ; les tests attendent vraiment |
| B. Dans chaque interface | la console et la fenêtre ont chacune leur chronomètre | **écrit deux fois**, et les deux peuvent diverger |
| C. Un port `Clock` et un `Stopwatch` pur | l'heure est **passée en paramètre** ; un objet entre le cœur et les interfaces orchestre | une couche de plus à nommer : l'**application** |

C'est le besoin annoncé à l'[étape 8](08-modules.md) : le chronomètre n'est
ni dans `core/` (pas une règle), ni dans chaque adaptateur (deux fois). Il
faut un endroit entre les deux.

## La décision

L'option C, en trois morceaux.

**Le port `Clock`**, dans [ports.hpp](../src/app/ports.hpp), sur le modèle de
`MineLayout` à l'[étape 16](16-hasard.md) :

```cpp
class Clock {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    virtual ~Clock() = default;
    virtual TimePoint now() const = 0;
};
```

**`Stopwatch` est pur.** Il ne lit **jamais** d'horloge ; le temps lui est
toujours passé en paramètre :

```cpp
class Stopwatch {
public:
    using TimePoint = Clock::TimePoint;
    void start(TimePoint t) { start_ = t; stop_.reset(); }
    void stop(TimePoint t) { if (start_) stop_ = t; }
    std::chrono::seconds elapsed(TimePoint now) const {
        if (!start_) return {};
        const auto end = stop_.value_or(now);
        return std::chrono::duration_cast<std::chrono::seconds>(end - *start_);
    }
private:
    std::optional<TimePoint> start_;
    std::optional<TimePoint> stop_;
};
```

**`Session` relie le tout.** Elle réalise les deux ports d'entrée, utilise les
deux ports de sortie, possède la partie et le chronomètre :

```cpp
struct GameConfig {
    Size size;
    int mines;
};

class Session final : public GameView, public CommandSink {
public:
    Session(GameConfig config, MineLayout& layout, const Clock& clock);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void handle(const Command& command) override;

    Size size() const override { return game_.size(); }
    CellView cell(Pos p) const override { return game_.cell(p); }
    Status status() const override { return game_.status(); }
    int minesLeft() const override { return game_.minesLeft(); }
    std::chrono::seconds elapsed() const override {
        return stopwatch_.elapsed(clock_.now());
    }

private:
    void updateStopwatch(Status before, Status after);

    GameConfig config_;
    MineLayout& layout_;
    const Clock& clock_;
    Game game_;
    Stopwatch stopwatch_;
};
```

Toutes les questions sont **déléguées** à `Game` ; seule `elapsed()` est à
elle. Côté commandes, `handle()` relève le statut, délègue, puis met le
chronomètre à jour :

```cpp
void Session::handle(const Command& command) {
    const Status before = game_.status();
    std::visit(overloaded{
                   [&](const Reveal& c) { game_.reveal(c.pos); },
                   [&](const ToggleFlag& c) { game_.toggleFlag(c.pos); },
                   [&](const NewGame&) {
                       game_ = Game{config_.size, config_.mines, layout_};
                       stopwatch_ = {};
                   },
               },
               command);
    updateStopwatch(before, game_.status());
}

// Le chronomètre tourne du premier clic à la fin de la partie.
void Session::updateStopwatch(Status before, Status after) {
    const auto now = clock_.now();
    if (before == Status::Ready && after != Status::Ready) stopwatch_.start(now);
    if (!isOver(before) && isOver(after)) stopwatch_.stop(now);
}
```

Le chronomètre réagit aux **transitions** de statut, observées avant et après
chaque commande. `Game` n'a pas besoin de le prévenir : il ignore jusqu'à
l'existence d'un chronomètre.

Un mot sur le nom. `Session` représente **ce qu'un joueur fait devant le
programme** : une suite de parties, chronométrées, avec une configuration
fixe. Le mot n'a pas le sens qu'il prend sur un serveur web (une connexion par
utilisateur) : il y a exactement **une** `Session` par exécution, créée par le
lanceur et détruite à la sortie. C'est le `Game` qu'elle contient qui change à
chaque nouvelle partie.

## Les concepts

### Le service applicatif

`Session` est une **couche d'orchestration** : elle relie le domaine aux
ports et ajoute uniquement ce qui n'est pas une règle (quelle partie est en
cours, depuis quand). Un service applicatif qui commence à contenir des `if`
sur les règles du jeu est un signal d'alarme : la règle est sans doute au
mauvais endroit.

### Hériter de plusieurs interfaces

`Session` hérite de **deux** classes abstraites. C'est la forme saine de
l'héritage multiple en C++ : on hérite de **contrats** (des interfaces sans
données), pas d'implémentations. Chaque client ne voit que la facette dont il
a besoin ; l'interface reçoit les deux séparément :

```cpp
ui.run(session, session);   // run(const GameView&, CommandSink&)
```

Passer deux fois le même objet peut surprendre, mais c'est voulu : la
signature de `run` documente que l'interface **lit** par un canal et
**écrit** par l'autre. Un test peut même séparer les deux : lire une vraie
session tout en enregistrant les commandes dans un espion.

### Les effets de bord aux bords

Lire l'heure est un **effet de bord** : deux appels ne donnent pas le même
résultat. Le projet le repousse le plus loin possible du cœur : `Stopwatch`
reçoit l'heure en valeur, `Session` la lit **une fois** par commande via le
port, et la vraie horloge (`SteadyClock`) n'existe que dans le lanceur. On
appelle parfois ce patron *functional core, imperative shell*. Dans les
tests, `FakeClock` avance quand le test le décide :

```cpp
class FakeClock final : public Clock {
public:
    TimePoint now() const override { return now_; }
    void advance(std::chrono::seconds d) { now_ += d; }
private:
    TimePoint now_{};
};
```

C'est une doublure d'un autre genre que `FixedMineLayout` : un **fake**, une
implémentation qui fonctionne vraiment, en plus simple.

### Identité ou valeur : la copie est un choix

```cpp
Session(const Session&) = delete;
Session& operator=(const Session&) = delete;
```

Copier une `Session` produirait **deux** parties indépendantes qui se croient
la même. Si une interface recevait une copie par erreur (un oubli de `&` dans
une signature), ses clics modifieraient la copie et l'écran n'afficherait
jamais rien. En supprimant la copie, cette erreur devient une **erreur de
compilation**. `Game`, au contraire, reste copiable : c'est un état de partie,
et le copier a un sens (une sauvegarde, une annulation). La `Session` a une
**identité** ; la partie est une **valeur**.

### Reconstruire plutôt que réinitialiser

Pour `NewGame`, on ne remet pas l'ancienne partie à zéro champ par champ,
avec le risque d'en oublier un. On **remplace** l'objet entier par un neuf,
construit par le même constructeur, qui vérifie les mêmes invariants. Une
méthode `reset()` est presque toujours une duplication du constructeur, et
donc une source de bugs.

## Vérifier

Tous dans [session_tests.cpp](../tests/session_tests.cpp), avec
`FixedMineLayout` et `FakeClock` :

- `Stopwatch.MeasuresFromStartToStop` teste le chronomètre pur avec des
  valeurs fixes, sans aucune doublure.
- `ClockStartsOnTheFirstReveal` : trente secondes avancées avant le premier
  coup ne comptent pas, un drapeau ne démarre pas le chrono, un `Reveal` oui.
- `ClockStopsWhenTheGameEnds` : après la défaite, cent secondes de plus
  laissent `elapsed()` à 3 s.
- `NewGameStartsAFreshGameAndClock` : statut `Ready`, cases cachées, chrono
  à zéro.
- `RejectsAnImpossibleConfiguration` : une `Session` 2 × 2 avec 4 mines lève
  `std::invalid_argument`, levé par le constructeur de `Game`.

Aucun de ces tests n'attend : *testable sans attendre*, le critère est
rempli.

## À retenir

- Un **service applicatif** orchestre : il relie le domaine aux ports, sans
  règle métier.
- **Héritage multiple d'interfaces** : un objet, plusieurs facettes, chaque
  client ne voit que la sienne.
- Les effets de bord (l'heure) restent **au bord**. Le cœur reçoit des
  valeurs.
- Supprimez la copie (`= delete`) des objets qui ont une **identité**.
- Pour réinitialiser, **reconstruisez** plutôt que d'écrire un `reset()`.

## Exercices

1. On veut un tableau des meilleurs temps. Dans quelle couche va-t-il ? De
   quel nouveau port aurait-il besoin ?
2. Pourquoi `clock_.now()` est-il lu **une seule fois** dans
   `updateStopwatch`, plutôt qu'une fois par `if` ?
3. Rendez `Session` copiable et écrivez un test qui montre le bug que cela
   permet.
