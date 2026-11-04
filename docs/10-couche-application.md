# La couche application : `Session`

> **Objectif.** Comprendre le rôle d'un **service applicatif** : orchestrer
> le domaine avec le monde extérieur, sans contenir de règle métier.
>
> **Code :** [src/app/session.hpp](../src/app/session.hpp),
> [src/app/stopwatch.hpp](../src/app/stopwatch.hpp)

## Le problème

Certaines choses ne sont pas des règles du démineur, mais le programme en a
besoin quand même :

- **chronométrer** la partie (le démineur n'a pas de règle sur le temps ; c'est
  une fonctionnalité du programme) ;
- **recommencer** une partie avec la même configuration ;
- exposer les **ports** aux interfaces.

Si on les met dans `Game`, le domaine dépend de l'horloge système. Si on les
met dans chaque interface, on les écrit deux fois. Il faut une couche entre
les deux.

## `Session` : un cas d'utilisation

`Session` représente **ce qu'un joueur fait devant le programme** : une suite
de parties, chronométrées, avec une configuration fixe.

```cpp
class Session final : public GameView, public CommandSink {
public:
    Session(GameConfig config, MineLayout& layout, const Clock& clock);

    void handle(const Command& command) override;

    Size size() const override { return game_.size(); }
    CellView cell(Pos p) const override { return game_.cell(p); }
    Status status() const override { return game_.status(); }
    int minesLeft() const override { return game_.minesLeft(); }
    std::chrono::seconds elapsed() const override { return stopwatch_.elapsed(clock_.now()); }

private:
    GameConfig config_;
    MineLayout& layout_;
    const Clock& clock_;
    Game game_;
    Stopwatch stopwatch_;
};
```

Elle **délègue** les questions de jeu à `Game`, et ajoute **uniquement**
l'orchestration : quelle partie est en cours, depuis quand.

> Un service applicatif qui commence à contenir des `if` sur les règles du
> jeu est un signal d'alarme : la règle est sans doute au mauvais endroit.

## Hériter de plusieurs interfaces

`Session` hérite de **deux** classes abstraites. C'est la forme saine de
l'héritage multiple en C++ : on hérite de **contrats** (des interfaces sans
données), pas d'implémentations.

Chaque client ne voit que la facette dont il a besoin. C'est le **principe de
ségrégation des interfaces** (le « I » de SOLID) :

> Un client ne doit pas dépendre de méthodes qu'il n'utilise pas.

Une fonction de rendu reçoit un `const GameView&` : elle peut lire, elle ne
peut **rien** ordonner. L'interface reçoit les deux facettes séparément :

```cpp
ui.run(session, session);   // run(const GameView&, CommandSink&)
```

Passer deux fois le même objet peut surprendre, mais c'est voulu : la
signature de `run` documente que l'interface **lit** par un canal et
**écrit** par l'autre. Et un test peut séparer les deux, en lisant une vraie
session tout en enregistrant les commandes dans un espion
([console_tests.cpp](../tests/console_tests.cpp)).

## Le temps, sans dépendre de l'heure

Le chronomètre est découpé en deux morceaux, et c'est instructif :

**`Stopwatch` est pur.** Il ne lit **jamais** d'horloge. Le temps lui est
toujours **passé en paramètre** :

```cpp
class Stopwatch {
public:
    void start(TimePoint t);
    void stop(TimePoint t);
    std::chrono::seconds elapsed(TimePoint now) const;
};
```

On le teste avec des valeurs fixes, sans aucune doublure :

```cpp
watch.start(t0);
watch.stop(t0 + 7s);
EXPECT_EQ(watch.elapsed(t0 + 60s), 7s);
```

**`Session` lit le port `Clock`**, une seule fois par commande, et décide
quand démarrer et arrêter :

```cpp
// Le chronomètre tourne du premier clic à la fin de la partie.
void Session::updateStopwatch(Status before, Status after) {
    const auto now = clock_.now();
    if (before == Status::Ready && after != Status::Ready) stopwatch_.start(now);
    if (!isOver(before) && isOver(after)) stopwatch_.stop(now);
}
```

Le chronomètre réagit aux **transitions** de statut, qu'il observe avant et
après chaque commande. Il n'a pas besoin que `Game` le prévienne : `Game`
ignore jusqu'à l'existence d'un chronomètre.

> **Patron général** : repousser les effets de bord (lire l'heure, le disque,
> le réseau) vers les bords, et garder le cœur fait de **fonctions pures** qui
> prennent ces valeurs en paramètre. On l'appelle parfois *functional core,
> imperative shell*.

## Une session a une identité : copie interdite

```cpp
Session(const Session&) = delete;
Session& operator=(const Session&) = delete;
```

Copier une `Session` produirait **deux** parties indépendantes qui se croient
la même. Si une interface recevait une copie par erreur (un oubli de `&` dans
une signature), ses clics modifieraient la copie, et l'écran n'afficherait
jamais rien. En supprimant la copie, cette erreur devient une **erreur de
compilation**.

`Game`, au contraire, reste copiable : c'est un état de partie, et le copier
a un sens (une sauvegarde, une annulation...). La copie est un **choix de
conception**, pas un réglage par défaut à laisser au hasard.

## Recommencer une partie

```cpp
[&](const NewGame&) {
    game_ = Game{config_.size, config_.mines, layout_};
    stopwatch_ = {};
},
```

On ne « réinitialise » pas l'ancienne partie champ par champ (avec le risque
d'en oublier un). On **remplace** l'objet entier par un neuf, construit par le
même constructeur, qui vérifie les mêmes invariants. Une méthode `reset()` est
presque toujours une duplication du constructeur, et donc une source de bugs.

## Les erreurs de configuration

`Session` ne valide pas la configuration : elle construit un `Game`, et c'est
le constructeur de `Game` qui refuse une partie impossible. Le lanceur attrape
l'exception et affiche l'usage :

```text
$ minesweeper-console 2 2 9
error: Game: mines must be between 1 and rows * cols - 1
```

Une règle, un seul endroit. Le parseur de la ligne de commande se contente de
lire des nombres. Il ne sait pas ce qu'est une partie valide.

## À retenir

- Un **service applicatif** orchestre : il relie le domaine aux ports, sans
  règle métier.
- **Héritage multiple d'interfaces** : un objet, plusieurs facettes, chaque
  client ne voit que la sienne (**ISP**).
- Les effets de bord (l'heure) restent **au bord**. Le cœur reçoit des valeurs.
- Supprimez la copie (`= delete`) des objets qui ont une **identité**.
- Pour réinitialiser, **reconstruisez** plutôt que d'écrire un `reset()`.

## Pour réfléchir

1. On veut un tableau des meilleurs temps. Dans quelle couche va-t-il ? De
   quel nouveau port aurait-il besoin ?
2. Pourquoi `clock_.now()` est-il lu **une seule fois** dans
   `updateStopwatch`, plutôt qu'une fois par `if` ?
