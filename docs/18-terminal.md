# Le terminal

> **Objectif.** Jouer enfin : lire `B3`, afficher la grille, boucler jusqu'à
> `q`. Et découvrir qu'une interface se teste aussi.
>
> **Code :** [src/adapters/console/](../src/adapters/console/)

## Le besoin

Le cœur et la session sont prêts et testés, mais personne ne peut encore
jouer. Il faut un premier **adaptateur** : lire des lignes au clavier, les
traduire en commandes, envoyer ces commandes au port `CommandSink`, relire
l'état par le port `GameView` et le dessiner en texte.

C'est le moment où le labo dit « le jeu est jouable »
([étape 6 du labo](../steps/README.md)). Avant d'écrire, une question : faut-il
une classe, ou plusieurs ?

## Les options

| Option | Idée | Ce qui coince |
| --- | --- | --- |
| A. Une classe `ConsoleUi` qui fait tout | une méthode `run()` qui lit, analyse, affiche et boucle | pour tester le parseur de `B3`, il faut lancer la boucle entière ; pour tester l'affichage, il faut fournir des saisies |
| B. Trois morceaux séparés | `notation` (texte ↔ positions et commandes), `renderer` (état → texte), `ConsoleUi` (la boucle) | trois fichiers au lieu d'un |

*Testable sans écran ?* Avec A, à peine. Avec B, chaque morceau se teste
seul : le parseur avec des chaînes, le rendu avec un flux en mémoire, la
boucle avec les deux.

## La décision

L'option B. Trois morceaux, trois responsabilités.

**`notation`** : des **fonctions pures**, du texte vers des valeurs du
domaine, et inversement
([notation.hpp](../src/adapters/console/notation.hpp)) :

```cpp
std::string columnLabel(int col);                  // 0 -> "A", 26 -> "AA"
std::string rowLabel(int row, Size size);          // la ligne du haut a le plus grand n°
std::optional<Pos> parsePos(std::string_view text, Size size);
std::optional<Input> parseInput(std::string_view line, Size size);
```

`parseInput` reconnaît les mots de sortie et de redémarrage, puis une position
suivie ou non de `?` :

```cpp
std::optional<Input> parseInput(std::string_view line, Size size) {
    line = trim(line);
    const std::string word = lower(line);
    if (word == "q" || word == "quit" || word == "exit") return Quit{};
    if (word == "n" || word == "new" || word == "r" || word == "restart")
        return Command{NewGame{}};

    const bool flag = line.ends_with('?');
    if (flag) line.remove_suffix(1);
    const auto pos = parsePos(line, size);
    if (!pos) return std::nullopt;
    if (flag) return Command{ToggleFlag{*pos}};
    return Command{Reveal{*pos}};
}
```

**`renderer`** : une fonction `render(std::ostream&, const GameView&, Style)`
qui dessine la grille. Le seul endroit qui décide à quoi ressemble une case
dans un terminal est un visiteur sur `CellView` :

```cpp
Glyph glyphOf(const CellView& view) {
    return std::visit(overloaded{
                          [](cell::Hidden) { return Glyph{".", "\033[90m"}; },
                          [](cell::Flagged) { return Glyph{"?", "\033[33m"}; },
                          [](cell::Revealed r) {
                              const int n = r.adjacentMines;
                              return Glyph{kDigits[n], kDigitColours[n]};
                          },
                          [](cell::Mine m) {
                              return m.exploded ? Glyph{"X", "\033[41;97;1m"}
                                                : Glyph{"*", "\033[31;1m"};
                          },
                          [](cell::WrongFlag) { return Glyph{"!", "\033[35;1m"}; },
                      },
                      view);
}
```

**`ConsoleUi`** : la boucle. Elle ne lit pas `std::cin` directement : elle
reçoit un `std::istream&` et un `std::ostream&`
([console_ui.cpp](../src/adapters/console/console_ui.cpp)) :

```cpp
void ConsoleUi::run(const GameView& view, CommandSink& commands) {
    render(out_, view, style_);
    std::string line;
    for (;;) {
        out_ << "> " << std::flush;
        if (!std::getline(in_, line)) return;

        const auto input = parseInput(line, view.size());
        if (!input) {
            out_ << "Unknown command. Examples: B3, B3?, n, q\n";
            continue;
        }
        if (std::holds_alternative<Quit>(*input)) return;

        commands.handle(std::get<Command>(*input));
        render(out_, view, style_);
    }
}
```

*Qui dépend de qui ?* Les trois fichiers incluent `app/ports.hpp` et
`app/commands.hpp`. Aucun n'inclut `session.hpp` ni `game.hpp` : la console
ne sait pas que `Session` existe.

## Les concepts

### Traduire, et rien d'autre

Un adaptateur fait trois choses, et seulement celles-là :

1. **Traduire l'entrée** du monde extérieur en commandes : `"B3?"` →
   `ToggleFlag{Pos{...}}`, en notation d'échecs où `A1` est **en bas** à
   gauche.
2. **Transmettre** la commande au port `CommandSink`.
3. **Présenter** l'état lu par le port `GameView`, dans son propre médium.

La notation `A1`, les couleurs ANSI, le texte « Boom! You lose. » : ce sont des
**détails d'adaptateur**. Le domaine ne connaît que `Pos`, `CellView` et
`Status`.

L'adaptateur **valide aussi ce qu'il traduit** : `parsePos("Z9")` sur une
grille 8 × 8 renvoie « rien » et la console affiche une aide. Une position
hors grille n'atteint jamais le domaine. Si elle l'atteignait malgré tout,
`Game` lèverait `std::out_of_range`, car ce serait une erreur de programmation
et non une saisie à corriger.

### Des fonctions pures pour traduire

`parsePos` et `parseInput` ne touchent à rien : mêmes entrées, même sortie,
pas d'état. C'est ce qui les rend triviales à tester, et c'est possible parce
qu'une commande est une **valeur** ([étape 15](15-commandes.md)) : la fonction
la fabrique et la renvoie, quelqu'un d'autre l'exécutera.

### Le cœur est passif : l'adaptateur possède la boucle

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

Cela fonctionne pour une console, mais pas ailleurs : avec SDL, la
bibliothèque fournit des événements dans **sa** boucle ; dans un test, on veut
appeler `reveal()` et vérifier le résultat, sans rien simuler. D'où la règle :
**le cœur ne lit rien et ne boucle pas**. C'est l'adaptateur qui possède la
boucle adaptée à sa technologie. Après chaque commande, il **relit** l'état et
redessine tout : c'est le modèle *pull* décidé à l'[étape 7](07-sens-des-flux.md).

### Tester une interface

Comme `ConsoleUi` reçoit ses flux par le constructeur, un test joue une partie
entière avec des chaînes. C'est encore de l'**injection de dépendance**,
appliquée aux flux :

```cpp
std::istringstream in{"C1\n"};
std::ostringstream out;
ConsoleUi{in, out, Style{}}.run(session, session);
EXPECT_EQ(session.status(), Status::Won);
```

Et pour vérifier la traduction seule, on remplace la session par un
**espion** ([test_doubles.hpp](../tests/test_doubles.hpp)) qui note tout ce
qu'on lui envoie :

```cpp
class RecordingSink final : public CommandSink {
public:
    void handle(const Command& command) override { received.push_back(command); }
    std::vector<Command> received;
};
```

Troisième doublure du projet, après le stub `FixedMineLayout` et le fake
`FakeClock`.

## Vérifier

Tous dans [console_tests.cpp](../tests/console_tests.cpp) :

- `ColumnsAreLettersRowsCountFromTheBottom` : `columnLabel(26)` vaut `"AA"`
  et `parsePos("A1", {8, 30})` vaut `Pos{7, 0}`.
- `RejectsMalformedOrOutsidePositions` : `"I1"`, `"A9"`, `"A0"`, `"3A"`,
  `"A"`, `"A1x"` sont tous refusés.
- `ParsesEveryKindOfInput` : `" c3 "`, `"C3?"`, `"N"`, `"quit"`, `"hello"`.
- `RendersTheGridFromTheReadPort` compare le texte complet d'une grille 3 × 3
  avec une case dévoilée.
- `TranslatesTypedLinesIntoCommands` envoie `B2`, `bogus`, `A3?`, `n`, `q`,
  `C1` à un `RecordingSink` et attend exactement
  `{Reveal{{1, 1}}, ToggleFlag{{0, 0}}, NewGame{}}` : la ligne fausse est
  sautée, tout s'arrête à `q`.
- `PlaysAWholeGame` : une mine en `{0, 0}`, le joueur tape `C1`, la partie
  est gagnée et la sortie contient « You win! ».

## À retenir

- Un adaptateur **traduit** (entrée → commande, état → affichage), et rien
  d'autre.
- **Séparer lire, afficher, boucler** : trois morceaux, chacun testable seul.
- **Le cœur est passif** : c'est l'adaptateur qui possède la boucle.
- Une interface qui reçoit ses flux par le constructeur se teste avec des
  chaînes.
- Un **espion** (`RecordingSink`) vérifie ce qu'un adaptateur envoie, sans
  vraie partie.

## Exercices

1. Pourquoi la conversion « ligne 1 = en bas » est-elle dans la console et
   pas dans `Pos` ?
2. Ajoutez le mot `help` qui affiche la liste des commandes. Quels fichiers
   touchez-vous ? Lequel ne touchez-vous surtout pas ?
3. Esquissez un adaptateur HTTP : `POST /reveal {"row": 2, "col": 3}`. Où est
   la boucle ? Que renvoie-t-il ?
