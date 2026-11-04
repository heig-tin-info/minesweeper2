# Diagramme de classes

Ces diagrammes couvrent tout le code de production de `src/`. Ils sont écrits
à la main d'après les en-têtes ; la configuration [.clang-uml](../.clang-uml)
permet d'en générer un équivalent depuis le code.

Le premier montre toutes les classes et toutes leurs relations, sans les
membres. Les suivants détaillent chaque couche, membres compris.

## Vue complète

```mermaid
flowchart LR
    ConsoleUi --> Style
    ConsoleUi -.-> renderer["«functions»<br/>renderer"]
    ConsoleUi -.-> notation["«functions»<br/>notation"]
    notation -.-> Input["«variant»<br/>Input"]
    SdlUi --> Theme
    SdlUi --> Library
    SdlUi -.-> Deleter["Deleter&lt;Destroy&gt;"]
    launch["«functions»<br/>launch"] -.-> LaunchOptions
    launch -. crée .-> SteadyClock
    Session -- réalise --> GameView["«interface»<br/>GameView"]
    Session -- réalise --> CommandSink["«interface»<br/>CommandSink"]
    Session --> Stopwatch
    Session --> GameConfig
    Session --> Clock["«interface»<br/>Clock"]
    CommandSink -.-> Command["«variant»<br/>Command"]
    Stopwatch -.-> Clock
    Game --> Minefield
    Game --> Grid["Grid&lt;T&gt;"]
    Game --> MineLayout["«interface»<br/>MineLayout"]
    Game -.-> Status["«enumeration»<br/>Status"]
    Game -.-> CellView["«variant»<br/>CellView"]
    RandomMineLayout -- réalise --> MineLayout
    Minefield --> Grid
    Grid --> Size
    Neighbors -.-> Size
    Size -.-> Pos
    ConsoleUi -- lit --> GameView
    ConsoleUi -- envoie --> CommandSink
    SdlUi -- lit --> GameView
    SdlUi -- envoie --> CommandSink
    Input --> Command
    launch -. crée .-> Session
    launch -. crée .-> RandomMineLayout
    LaunchOptions --> GameConfig
    SteadyClock -- réalise --> Clock
    Session --> Game
    Session --> MineLayout
    GameView -.-> CellView
    Command -.-> Pos
```

Flèche pleine : composition, référence ou pointeur ; « réalise » : la classe
implémente l'interface ; flèche pointillée : simple dépendance.

Les flèches ne traversent les couches que dans un sens : `console` et `sdl`
connaissent `app`, `app` connaît `core`, et `core` ne connaît personne. Les
deux adaptateurs ne voient la session qu'à travers ses ports `GameView` et
`CommandSink` ; seul `launcher` connaît la classe concrète `Session`.

## Le domaine (`core`)

```mermaid
classDiagram
    direction LR
    class Pos {
        +int row
        +int col
    }
    class Size {
        +int rows
        +int cols
        +area() int
        +contains(Pos) bool
        +indexOf(Pos) int
        +posOf(int) Pos
    }
    class Neighbors {
        -array~Pos, 8~ items_
        -int count_
        +Neighbors(Size, Pos)
        +begin()
        +end()
        +size() int
    }
    class Grid~T~ {
        -Size size_
        -vector~T~ cells_
        +Grid(Size, T fill)
        +size() Size
        +operator[](Pos) T&
    }
    class Minefield {
        -Grid~Square~ squares_
        -int mineCount_
        +Minefield(Size, span~Pos~ mines)
        +size() Size
        +mineCount() int
        +hasMine(Pos) bool
        +adjacentMines(Pos) int
    }
    class MineLayout {
        <<interface>>
        +place(Size, int count, Pos firstClick)* vector~Pos~
    }
    class RandomMineLayout {
        -mt19937_64 rng_
        +RandomMineLayout(uint64_t seed)
        +place(Size, int, Pos) vector~Pos~
    }
    class Status {
        <<enumeration>>
        Ready
        Playing
        Won
        Lost
    }
    class Game {
        -MineLayout* layout_
        -int mines_
        -Grid~Mark~ marks_
        -optional~Minefield~ field_
        -optional~Pos~ exploded_
        -int flags_
        -int hiddenSafe_
        +Game(Size, int mines, MineLayout&)
        +reveal(Pos)
        +toggleFlag(Pos)
        +size() Size
        +mines() int
        +minesLeft() int
        +status() Status
        +cell(Pos) CellView
        -plant(Pos) Minefield&
        -open(Pos)
        -require(Pos)
    }
    class CellView {
        <<variant>>
    }
    class Hidden
    class Flagged
    class Revealed {
        +int adjacentMines
    }
    class Mine {
        +bool exploded
    }
    class WrongFlag

    Neighbors ..> Size
    Neighbors ..> Pos
    Size ..> Pos
    Grid~T~ *-- Size
    Minefield *-- Grid~T~ : squares_
    Game *-- Grid~T~ : marks_
    Game *-- "0..1" Minefield : field_
    Game --> MineLayout : layout_ (non possédé)
    Game ..> Status
    Game ..> CellView : produit
    RandomMineLayout ..|> MineLayout
    CellView *-- Hidden
    CellView *-- Flagged
    CellView *-- Revealed
    CellView *-- Mine
    CellView *-- WrongFlag
```

## L'application et le lanceur (`app`, `launcher`)

```mermaid
classDiagram
    direction LR
    class Command {
        <<variant>>
    }
    class Reveal {
        +Pos pos
    }
    class ToggleFlag {
        +Pos pos
    }
    class NewGame
    class GameView {
        <<interface>>
        +size()* Size
        +cell(Pos)* CellView
        +status()* Status
        +minesLeft()* int
        +elapsed()* seconds
    }
    class CommandSink {
        <<interface>>
        +handle(Command)*
    }
    class Clock {
        <<interface>>
        +now()* TimePoint
    }
    class Stopwatch {
        -optional~TimePoint~ start_
        -optional~TimePoint~ stop_
        +start(TimePoint)
        +stop(TimePoint)
        +elapsed(TimePoint now) seconds
    }
    class GameConfig {
        +Size size
        +int mines
    }
    class Session {
        -GameConfig config_
        -MineLayout& layout_
        -const Clock& clock_
        -Game game_
        -Stopwatch stopwatch_
        +Session(GameConfig, MineLayout&, const Clock&)
        +handle(Command)
        -updateStopwatch(Status before, Status after)
    }
    class LaunchOptions {
        +GameConfig game
        +optional~uint64_t~ seed
        +bool help
    }
    class SteadyClock {
        +now() TimePoint
    }
    class launch {
        <<functions>>
        +parseArguments(span~char*~) LaunchOptions
        +usage(string program) string
        +launch(argc, argv, play) int
    }

    Command *-- Reveal
    Command *-- ToggleFlag
    Command *-- NewGame
    CommandSink ..> Command
    Session ..|> GameView
    Session ..|> CommandSink
    Session *-- GameConfig
    Session *-- Game
    Session *-- Stopwatch
    Session --> MineLayout
    Session --> Clock
    Stopwatch ..> Clock : TimePoint
    LaunchOptions *-- GameConfig
    SteadyClock ..|> Clock
    launch ..> LaunchOptions
    launch ..> Session : crée
    launch ..> RandomMineLayout : crée
    launch ..> SteadyClock : crée
```

## Les adaptateurs (`console`, `sdl`)

```mermaid
classDiagram
    direction LR
    class GameView {
        <<interface>>
    }
    class CommandSink {
        <<interface>>
    }
    class ConsoleUi {
        -istream& in_
        -ostream& out_
        -Style style_
        +ConsoleUi(istream&, ostream&, Style)
        +run(const GameView&, CommandSink&)
    }
    class Style {
        +bool colour
    }
    class renderer {
        <<functions>>
        +render(ostream&, const GameView&, Style)
    }
    class notation {
        <<functions>>
        +columnLabel(int) string
        +rowLabel(int, Size) string
        +parsePos(string_view, Size) optional~Pos~
        +parseInput(string_view, Size) optional~Input~
    }
    class Input {
        <<variant>>
    }
    class Quit
    class SdlUi {
        -Library library_
        -Size grid_
        -Theme theme_
        -WindowPtr window_
        -RendererPtr renderer_
        -FontPtr cellFont_
        -FontPtr hudFont_
        -Glyphs glyphs_
        +SdlUi(Size, Theme)
        +run(const GameView&, CommandSink&)
        -translate(SDL_Event) optional~Command~
        -cellAt(int x, int y) optional~Pos~
        -draw(const GameView&)
    }
    class Theme {
        +int tile
        +int hudHeight
        +string font
    }
    class Library {
        +Library()
        +~Library()
    }
    class Deleter~Destroy~ {
        +operator()(T*)
    }

    ConsoleUi *-- Style
    ConsoleUi --> GameView : lit
    ConsoleUi --> CommandSink : envoie
    ConsoleUi ..> renderer
    ConsoleUi ..> notation
    renderer ..> GameView
    notation ..> Input
    Input *-- Command
    Input *-- Quit
    SdlUi *-- Library
    SdlUi *-- Theme
    SdlUi ..> Deleter~Destroy~ : WindowPtr, RendererPtr, FontPtr
    SdlUi --> GameView : lit
    SdlUi --> CommandSink : envoie
```

## Légende

Stéréotypes
:   `<<interface>>` : classe abstraite pure (un port) ; `<<variant>>` :
    `std::variant` dont les alternatives sont reliées par une composition ;
    `<<functions>>` : fonctions libres d'un même en-tête.

Relations
:   losange plein : composition (membre par valeur) ; flèche pleine :
    référence ou pointeur non possédé ; triangle pointillé : réalisation d'une
    interface ; flèche pointillée : simple dépendance (paramètre, type de
    retour).

Hors des diagrammes
:   les doublures de test de [tests/test_doubles.hpp](../tests/test_doubles.hpp)
    (`FixedMineLayout`, `FakeClock`, `RecordingSink`) réalisent les mêmes
    ports que `RandomMineLayout`, `SteadyClock` et `Session`.
