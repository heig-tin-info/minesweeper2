# Le lanceur

> **Objectif.** Décider qui crée les objets concrets et les branche entre eux,
> et le faire à un seul endroit.
>
> **Code :** [launch.hpp](../src/launcher/launch.hpp),
> [launch.cpp](../src/launcher/launch.cpp),
> [console/main.cpp](../src/adapters/console/main.cpp),
> [sdl/main.cpp](../src/adapters/sdl/main.cpp)

## Le besoin

Chaque pièce ne connaît que des interfaces : `Game` reçoit un `MineLayout&`,
`Session` un `MineLayout&` et une `Clock&`, `ConsoleUi` un `GameView&` et un
`CommandSink&`. Très bien, mais **quelqu'un** doit créer `RandomMineLayout`,
`SteadyClock`, `Session`, l'interface, et les brancher. Qui ? Et il faut
aussi lire la ligne de commande (`16 30 99 --seed 42`) et refuser une partie
impossible.

Il y a deux exécutables (console et fenêtre, [étape 20](20-fenetre-sdl.md)),
qui veulent la même chose à une ligne près.

## Les options

| Option | Idée | Ce qui coince |
| --- | --- | --- |
| A. Chaque `main()` fait tout | la console et la fenêtre ont chacune leur lecture d'arguments, leur création d'objets, leur gestion d'erreurs | **écrit deux fois** ; l'ancienne version avait un seul `main.cpp` truffé de `#ifdef SDL`, avec deux boucles dans le même fichier |
| B. Une fonction commune `launch` | elle fait tout ce qui est commun et reçoit en paramètre ce qui diffère : la fonction qui joue | il faut passer une fonction en paramètre |

## La décision

L'option B. `launch` reçoit `play`, la seule chose qui distingue les deux
exécutables ([launch.cpp](../src/launcher/launch.cpp)) :

```cpp
int launch(int argc, const char* const argv[],
           const std::function<void(Session&)>& play) {
    const std::string program = argc > 0 ? argv[0] : "minesweeper";
    try {
        const LaunchOptions options = parseArguments({argv + 1, argv + argc});
        if (options.help) {
            std::cout << usage(program);
            return 0;
        }

        RandomMineLayout layout{options.seed.value_or(std::random_device{}())};
        SteadyClock clock;
        Session session{options.game, layout, clock};
        play(session);
        return 0;
    } catch (const std::invalid_argument& e) {
        std::cerr << "error: " << e.what() << '\n' << usage(program);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
```

La vraie horloge vit ici, avec les autres effets de bord, dans un espace de
noms anonyme :

```cpp
class SteadyClock final : public Clock {
public:
    TimePoint now() const override { return std::chrono::steady_clock::now(); }
};
```

Chaque exécutable n'a plus qu'à dire quelle interface il utilise :

```cpp
// console/main.cpp
int main(int argc, char* argv[]) {
    using namespace minesweeper;
    return launch(argc, argv, [](Session& session) {
        const console::Style style{.colour = console::stdoutSupportsColour()};
        console::ConsoleUi ui{std::cin, std::cout, style};
        ui.run(session, session);
    });
}

// sdl/main.cpp
int main(int argc, char* argv[]) {
    using namespace minesweeper;
    return launch(argc, argv, [](Session& session) {
        sdl::SdlUi ui{session.size()};
        ui.run(session, session);
    });
}
```

**Deux exécutables, deux `main` de quelques lignes, zéro préprocesseur.** Le
système de build compile la version SDL seulement si SDL2 est installé.

La ligne de commande est lue par `parseArguments`, qui ne fait que lire des
nombres :

```cpp
struct LaunchOptions {
    GameConfig game{.size = {9, 9}, .mines = 10};
    std::optional<std::uint64_t> seed;
    bool help = false;
};

LaunchOptions parseArguments(std::span<const char* const> args);
```

## Les concepts

### La racine de composition

`launch.cpp` est le **seul** fichier qui connaît **toutes** les classes
concrètes : `RandomMineLayout`, `SteadyClock`, `Session`, et, par `play`,
l'interface. Partout ailleurs, on ne voit que des interfaces. C'est la
**racine de composition** (*composition root*) : l'endroit, le plus près
possible de `main`, où les dépendances sont créées et **injectées**. Les
objets vivent sur la pile de `launch`, dans l'ordre de leurs dépendances, et
sont détruits dans l'ordre inverse à la sortie.

*Qui dépend de qui ?* `launcher/` dépend de `app/` et de `core/` ; rien ne
dépend de `launcher/`. C'est la seule couche qui a le droit de tout connaître,
précisément parce que personne ne la connaît.

### Un `main()` minimal

`main` ne décide rien : il nomme l'interface et délègue. Tout ce qui est
testable est ailleurs, dans des fonctions qui prennent des paramètres et
renvoient des valeurs (`parseArguments`, `usage`). On ne teste pas `main` ;
on fait en sorte qu'il n'y ait rien à y tester.

### Les exceptions remontent jusqu'au bord

`parseArguments` lève `std::invalid_argument` pour une ligne de commande
malformée. Le constructeur de `Game` lève la même exception pour une partie
impossible (trop de mines). Personne entre les deux ne les attrape : elles
**remontent** jusqu'à `launch`, le bord du programme, qui est le seul à savoir
quoi en faire (afficher l'usage, code de retour 2) :

```text
$ minesweeper-console 2 2 9
error: Game: mines must be between 1 and rows * cols - 1
Usage: minesweeper-console [rows cols mines] [--seed N]
```

Toute autre exception est une erreur interne (`fatal`, code 1). Attraper une
exception au milieu, « pour être sûr », c'est généralement perdre
l'information sans savoir quoi faire à la place.

### Le parseur parse, le domaine valide

`parseArguments` accepte `2 2 50` sans broncher. Ce n'est pas un oubli : la
règle « le nombre de mines doit être compris entre 1 et `rows * cols - 1` »
est une règle du **domaine**, écrite une seule fois dans le constructeur de
`Game`. Si le parseur la recopiait, il faudrait la maintenir à deux endroits,
et le jour où `Game` l'assouplit, le parseur continuerait de refuser. *Écrit
deux fois ?* Non : une règle, un seul endroit.

### Pourquoi pas une interface `UserInterface` ?

On pourrait déclarer `class UserInterface { virtual void run(...) = 0; }`
et passer un objet plutôt qu'une lambda. Mais rien ne choisit l'interface à
l'exécution : chaque `main` sait laquelle il veut. Une interface sans choix
dynamique n'apporterait rien (revoir « Quand ne pas créer d'interface » à
l'[étape 16](16-hasard.md)). Si un jour un seul exécutable proposait
`--ui=console|sdl`, elle aurait alors sa raison d'être.

## Vérifier

Dans [launcher_tests.cpp](../tests/launcher_tests.cpp) :

- `DefaultsToABeginnerGame` : sans argument, 9 × 9 et 10 mines, pas de
  graine.
- `ReadsSizeMinesAndSeed` : `16 30 99 --seed 7`.
- `RejectsMalformedInput` : deux nombres au lieu de trois, `x` à la place
  d'un nombre, `--seed` sans valeur : `std::invalid_argument` à chaque fois.
- `LeavesGameRulesToTheDomain` : `2 2 50` **passe** le parseur. Le
  commentaire du test dit tout : *the parser parses; the Game validates*.

Et dans [session_tests.cpp](../tests/session_tests.cpp),
`RejectsAnImpossibleConfiguration` vérifie que c'est bien la construction de
la `Session` (donc du `Game`) qui refuse 4 mines sur 2 × 2.

## À retenir

- **Un seul point de composition** connaît les classes concrètes ; le reste
  du programme ne voit que des interfaces.
- `main()` nomme l'interface et délègue. Pas de `#ifdef` pour choisir une
  interface.
- Les exceptions **remontent jusqu'au bord**, le seul endroit qui sait quoi
  en faire.
- **Le parseur parse, le domaine valide** : une règle, un seul endroit.

## Exercices

1. Ajoutez une option `--no-colour` pour la console. Quels fichiers
   changent ? `launch.cpp` doit-il connaître la notion de couleur ?
2. Fusionnez les deux exécutables en un seul avec `--ui=console|sdl`. Que
   devient `play` ? L'interface `UserInterface` devient-elle utile ?
3. Déplacez la validation du nombre de mines dans `parseArguments`. Quel test
   échoue ? Quel bug apparaît le jour où `Game` accepte une grille sans mine ?
