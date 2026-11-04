# Les adaptateurs : console et SDL

> **Objectif.** Comprendre qui **possède la boucle** du programme, comment
> les événements circulent, et quand le patron Observer est utile (ou ne
> l'est pas).
>
> **Code :** [src/adapters/](../src/adapters/), [src/launcher/](../src/launcher/)

## Le point décisif : qui possède la boucle ?

Une première conception, très répandue, met la boucle dans le jeu :

```cpp
void Game::loop() {
    while (!over()) {
        auto action = input_->read();   // le jeu « tire » l'entrée
        apply(action);
        display_->show(*this);
    }
}
```

Cela fonctionne pour une console, mais pas ailleurs :

- avec **SDL**, la bibliothèque fournit des événements dans **sa** boucle
  (`SDL_PollEvent`) ;
- dans un **navigateur**, il n'y a pas de boucle du tout : une requête arrive,
  on répond ;
- dans un **test**, on veut appeler `reveal()` et vérifier le résultat, sans
  rien simuler.

D'où la règle : **le cœur est passif**. Il ne lit rien, il ne boucle pas. C'est
**l'adaptateur** qui possède la boucle adaptée à sa technologie, et qui
appelle le cœur quand il le décide.

## Deux boucles, deux styles

**Console** : une boucle **bloquante** sur l'entrée
([console_ui.cpp](../src/adapters/console/console_ui.cpp)).

```cpp
void ConsoleUi::run(const GameView& view, CommandSink& commands) {
    render(out_, view, style_);
    std::string line;
    for (;;) {
        out_ << "> " << std::flush;
        if (!std::getline(in_, line)) return;

        const auto input = parseInput(line, view.size());
        if (!input) { out_ << "Unknown command. Examples: B3, B3?, n, q\n"; continue; }
        if (std::holds_alternative<Quit>(*input)) return;

        commands.handle(std::get<Command>(*input));
        render(out_, view, style_);
    }
}
```

**SDL** : une boucle **d'événements** avec délai
([sdl_ui.cpp](../src/adapters/sdl/sdl_ui.cpp)).

```cpp
void SdlUi::run(const GameView& view, CommandSink& commands) {
    for (;;) {
        draw(view);
        SDL_Event event;
        if (!SDL_WaitEventTimeout(&event, kFrameMs)) continue;   // réveil régulier pour l'horloge
        do {
            if (isQuit(event)) return;
            if (const auto command = translate(event)) commands.handle(*command);
        } while (SDL_PollEvent(&event));
    }
}
```

Elles n'ont **rien en commun**, et c'est très bien ainsi : chacune épouse sa
technologie. Ce qu'elles partagent, ce sont les **ports** qu'elles appellent.

## Le travail d'un adaptateur : traduire

Un adaptateur fait trois choses, et seulement celles-là :

1. **Traduire l'entrée** du monde extérieur en commandes.
   - Console : `"B3?"` → `ToggleFlag{Pos{...}}`, en notation d'échecs où `A1`
     est **en bas** à gauche ([notation.cpp](../src/adapters/console/notation.cpp)).
   - SDL : clic droit en pixels `(130, 210)` → `ToggleFlag{Pos{5, 4}}`
     (`cellAt()`).
2. **Transmettre** la commande au port `CommandSink`.
3. **Présenter** l'état lu par le port `GameView`, dans son propre médium.

La notation `A1`, les pixels, les couleurs ANSI, les polices : ce sont des
**détails d'adaptateur**. Le domaine ne connaît que `Pos`.

L'adaptateur **valide aussi ce qu'il traduit** : `parsePos("Z9")` sur une
grille 8 × 8 renvoie « rien » et la console affiche une aide. Une position
hors grille n'atteint jamais le domaine. Si elle l'atteignait malgré tout,
`Game` lèverait `std::out_of_range`, car c'est une erreur de programmation et
non une saisie à corriger.

## Pull : l'adaptateur relit l'état

Après chaque commande, l'adaptateur **relit** l'état et redessine tout. C'est
le modèle *pull* : on tire les données quand on en a besoin.

Pour ce jeu, c'est le bon choix :

- la console redessine la grille après chaque saisie, de toute façon ;
- SDL fonctionne en **mode immédiat** : chaque image est entièrement redessinée
  (et il faut de toute façon rafraîchir le chronomètre) ;
- une grille de 30 × 16 cases se relit en quelques microsecondes.

## Et le patron Observer ?

L'**Observer** (ou *publish/subscribe*) inverse le sens : le modèle
**notifie** des abonnés quand il change.

```cpp
class GameObserver {
public:
    virtual ~GameObserver() = default;
    virtual void cellsChanged(std::span<const Pos> cells) = 0;
    virtual void statusChanged(Status status) = 0;
};
```

Il devient utile quand :

- l'interface est en **mode retenu** (*retained mode*), comme les widgets Qt ou
  le DOM d'une page web : chaque case est un objet qu'il faut mettre à jour
  **individuellement**, et savoir *lesquelles* ont changé évite de tout
  reconstruire ;
- **plusieurs vues** doivent réagir à un changement venu d'ailleurs (une
  mini-carte, un client réseau, un son) ;
- des changements arrivent **sans action de l'interface** (une partie en
  réseau, un *replay*).

Ce projet n'en a **aucun** de ces besoins, donc il n'implémente pas d'Observer.
Ajouter un patron sans besoin, c'est payer sa complexité sans son bénéfice.

Si un jour on l'ajoute, voici les règles à respecter :

1. **Les ordres descendent, les notifications remontent, les données sont
   tirées.** La notification dit *quoi* a changé, l'abonné relit *comment*
   via `GameView`.
2. **Notifier une fois, à la fin de la commande**, jamais pendant le
   *flood fill* (sinon 500 notifications par clic, et des abonnés qui voient
   un état à moitié calculé).
3. **Pas de réentrance** : un abonné ne lance pas de commande depuis sa
   notification.
4. **Gérer la durée de vie** : l'abonnement renvoie un jeton RAII qui se
   désabonne dans son destructeur. Sinon, un abonné détruit reste inscrit et le
   programme plante à la notification suivante (voir le [chapitre 12](12-raii.md)).

## Le point de composition

Où les objets sont-ils créés et reliés ? À **un seul endroit**, le plus près
possible de `main` ([launch.cpp](../src/launcher/launch.cpp)) :

```cpp
int launch(int argc, const char* const argv[], const std::function<void(Session&)>& play) {
    ...
    const LaunchOptions options = parseArguments({argv + 1, argv + argc});
    RandomMineLayout layout{options.seed.value_or(std::random_device{}())};
    SteadyClock clock;
    Session session{options.game, layout, clock};
    play(session);
    ...
}
```

C'est le **point de composition** (*composition root*) : le seul endroit qui
connaît **toutes** les classes concrètes. Chaque exécutable n'a plus qu'à dire
quelle interface utiliser :

```cpp
// console/main.cpp
return launch(argc, argv, [](Session& session) {
    console::ConsoleUi ui{std::cin, std::cout, {.colour = console::stdoutSupportsColour()}};
    ui.run(session, session);
});

// sdl/main.cpp
return launch(argc, argv, [](Session& session) {
    sdl::SdlUi ui{session.size()};
    ui.run(session, session);
});
```

L'ancienne version avait un seul `main.cpp` truffé de `#ifdef SDL`, avec deux
boucles différentes dans le même fichier. Ici, **deux exécutables, deux
`main` de 8 lignes, zéro préprocesseur**. Le système de build compile la
version SDL seulement si SDL2 est installé.

> **Pourquoi pas une interface `UserInterface` avec `run()` virtuel ?** Parce
> que rien ne choisit l'interface à l'exécution : chaque `main` sait
> laquelle il veut. Une interface sans choix dynamique n'apporterait rien
> (revoir « Quand ne pas créer d'interface » au [chapitre 7](07-polymorphisme-ports.md)).
> Si un jour un seul exécutable proposait `--ui=console|sdl`, elle aurait
> alors sa raison d'être.

## Des adaptateurs testables

`ConsoleUi` ne lit pas `std::cin` directement : elle reçoit un `std::istream&`
et un `std::ostream&`. Un test joue donc une partie entière avec des chaînes :

```cpp
std::istringstream in{"C1\n"};
std::ostringstream out;
ConsoleUi{in, out, Style{}}.run(session, session);
EXPECT_EQ(session.status(), Status::Won);
```

C'est encore de l'injection de dépendance, appliquée aux flux.

## À retenir

- **Le cœur est passif** : c'est l'adaptateur qui possède la boucle de sa
  technologie.
- Un adaptateur **traduit** (entrée → commande, état → affichage), et rien
  d'autre.
- **Pull** suffit quand on redessine tout. **Observer** se justifie avec une
  interface à mode retenu, plusieurs vues, ou des changements extérieurs.
- **Un seul point de composition** connaît les classes concrètes. Pas de
  `#ifdef` pour choisir une interface.

## Pour réfléchir

1. Esquissez un adaptateur HTTP : `POST /reveal {"row": 2, "col": 3}`. Où
   est la boucle ? Que renvoie-t-il ?
2. Pourquoi la conversion « ligne 1 = en bas » est-elle dans la console et
   pas dans `Pos` ?
