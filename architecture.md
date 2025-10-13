# Architecture du démineur

Une approche objet modulaire est la clé pour structurer ce projet de démineur. Voici les étapes de réflexion liée à l'architecture. Nous allons proposer une architecture CQRS minimaliste pull-based... Voyons cela en détail.

## 1. Identifier les composants principaux

On peut aisément identifier que le démineur peut être divisé en plusieurs composants principaux :

- **Modèle de données (Game Logic)** : Gère l'état du jeu, y compris la grille, les mines, les cellules révélées, et la logique de victoire/défaite.
- **Interface utilisateur (UI)** : Gère l'affichage du jeu et les interactions utilisateur.
- **Contrôleur (Game Controller)** : Coordonne les interactions entre le modèle de données et l'interface utilisateur.

## 2. Identifier le modèle de données

Le principe de flux des données peut être imaginé ainsi :

1. Entrée (du monde vers le jeu) : Il s'agit d'une interface d'intentions que le jeu implémente. Par exemple, "révéler la cellule A3" ou "placer un drapeau sur B4".
2. Sortie (du jeu vers le monde) : Il s'agit d'une vue plate (DTO) que le jeu expose. Par exemple, "la cellule A3 est révélée et contient une mine" ou "le jeu est gagné".
3. Présentation : Il s'agit de la transformation de la vue plate en une interface utilisateur. Par exemple, afficher la grille dans la console ou dans une interface graphique.

En ingénierie logicielle, il y a par exemple un principe appelé [CQRS (Command Query Responsibility Segregation)](https://martinfowler.com/bliki/CQRS.html) qui correspond à cette idée de séparer les commandes (intentions) des requêtes (vues). L'objectif est d'aligner le logiciel avec le domaine métier (ici, le démineur) et de simplifier la compréhension et la maintenance du code.

Un autre principe comme le [DDD (Domain-Driven Design)](https://en.wikipedia.org/wiki/Domain-driven_design) peut aussi être utile pour structurer le modèle de données en fonction des concepts métier. Son objectif est de créer un langage commun entre les développeurs et les experts métier, facilitant ainsi la communication et la compréhension du domaine.

Notre problème est qu'ici nous avons une idée assez précise des composants nécessaires, mais pas forcément des détails de leur implémentation - et surtout de la manière dont ils interagissent entre eux. Il y a plusieurs façons de structurer ces interactions. La présence d'une interface utilisateur (UI) impose des contraintes :

- L'utilisateur ne peut pas jouer s'il ne connaît pas l'état du jeu.
- Le jeu ne peut pas changer d'état sans une action de l'utilisateur.
- Tant que l'utilisateur ne réagit pas, l'état du jeu ne change pas.

La communication est donc bidirectionnelle entre l'UI et le modèle de données. Un élément supplémentaire est donc nécessaire : le contrôleur. Il agit comme un médiateur entre l'UI et le modèle de données. Il reçoit les intentions de l'UI, les transmet au modèle de données, puis récupère la vue plate du modèle de données pour la transmettre à l'UI. Il peut fonctionner sur plusieurs principes :

- **Pull** : l'UI demande au contrôleur l'état du jeu à chaque action de l'utilisateur. Le contrôleur interroge le modèle de données et renvoie la vue plate à l'UI.
- **Push** : le modèle de données notifie le contrôleur à chaque changement d'état. Le contrôleur récupère la vue plate et la transmet à l'UI.
- **Événements** : Le modèle de données émet des événements à chaque changement d'état. Le contrôleur écoute ces événements, récupère la vue plate et la transmet à l'UI.
- **Pull-based** ou **Boucle de jeu** : Le contrôleur gère une boucle de jeu qui interroge périodiquement le modèle de données et met à jour l'UI.

Il s'agit de déterminer ce qui convient le mieux à notre cas d'utilisation. Pour un jeu simple comme le démineur, une approche pull-based est souvent suffisante, car elle ne nécessite pas de gestion complexe des événements, qui nécessite des concepts avancés que nous n'avons pas (encore) vus en cours.

## 3. Définir les interfaces

### Intentions (entrées vers le jeu)

C'est assez simple, car ici nous avons seulement deux composantes :

1. l'action de l'utilisateur (révéler une cellule, placer/enlever un drapeau);
2. sur quelle cellule (position) l'action doit être effectuée.

```cpp
struct Pos { int x; int y; };

enum class Action { Reveal, ToggleFlag, NewGame, Quit, Redraw };

struct UserIntent {
  Action action;
  std::optional<Pos> where;
};
```

Le jeu quant à lui implémente un port d'entrée pour recevoir ces intentions :

```cpp
struct IIntentSink {
    virtual void handle(const UserIntent& i) = 0;

    virtual ~IIntentSink() = default;
};
```

Il s'agit d'une interface reconnaissable au `I` devant le nom de la classe, au fait qu'elle ne contient que des méthodes virtuelles pures, et qu'elle possède un destructeur virtuel.

### Lecture de l'état du jeu (sorties du jeu)

L'état du jeu du démineur est essentiellement une grille de cellules, où chaque cellule peut être dans différents états (cachée, révélée, avec un drapeau, etc.). Mais c'est aussi l'état global du jeu (en cours, gagné, perdu).

Traitons d'abord la grille. L'UI doit pouvoir lire l'état de la grille pour l'afficher, mais elle ne doit pas pouvoir la modifier. Nous devons donc définir une interface de lecture seule pour la grille :

```cpp
// Token textuel que l'UI sait afficher (un caractère)
using CellToken = std::string;

struct IBoardReadable {
    // Dimensions de la grille
    virtual int width()  const = 0;
    virtual int height() const = 0;

    // Accès en lecture seule à une cellule
    virtual CellToken at(int x, int y) const = 0;

    virtual ~IBoardReadable() = default;
};
```

Dans l'implémentation classique du démineur, trois informations supplémentaires sont communiquées à l'UI :

- le nombre de mines restantes ;
- le temps écoulé ;
- l'état global du jeu (en cours, gagné, perdu).

Dans un raisonnement générique (il ne faut pas s'arrêter au cas spécifique du démineur), ces informations peuvent être considérées comme des propriétés du jeu qui peuvent être plus génériques (une liste de propriétés clé-valeur). Mais nous allons rester spécifique au démineur pour le moment.

Quant à l'état du jeu, conservons l'idée d'une énumération simple :

```cpp
enum class GameState { InProgress, Won, Lost };
```

Le jeu expose donc une interface de lecture seule pour l'état du jeu :

```cpp
// Propres à l'affichage dans le HUD (Head-Up Display)
struct HudSnapshot {
  std::string time_mmss; // "00:23"
  std::string count;     // "0"
};

// Interface de sortie du jeu
struct IOutSnapshot {
    // Vue plate de l'état du jeu (définie plus haut)
    virtual const IBoardReadable& board() const = 0;

    // États propres du jeu
    virtual HudSnapshot hud() const = 0;

    // État global du jeu
    virtual GameState state() const = 0;

    virtual ~IOutSnapshot() = default;
};
```

### Présentateur (adaptateur sortant)

Le présentateur est responsable de la transformation de la vue plate en une interface utilisateur. Il implémente une interface pour recevoir la vue plate du jeu et la transformer en une interface utilisateur.

Le jeu expose des données brutes via `IBoardReadable`, où chaque cellule est représentée par un *token* (une chaîne de caractères courte). Le présentateur doit interpréter ces *tokens* et les afficher de manière appropriée.

C'est générique, on peut représenter le jeu du morpion avec `X` et `O`, les échecs avec `♞` et `♜` ou le sudoku avec des chiffres, etc. Dans le cas du démineur les *tokens* peuvent être assez simple :

- `.` : cellule cachée
- `F` : cellule avec un drapeau
- `*` : cellule révélée avec une mine
- `0` à `8` : cellule révélée avec le nombre de mines adjacentes
- ` ` (espace) : cellule révélée sans mine adjacente

Dans le cas d'une interface graphique ou même console, on voudrait afficher des icônes ou des couleurs différentes pour chaque état : une bombe `💣`, un drapeau `🚩`, des chiffres colorés, etc.

```cpp
struct BoardPresenter : public IBoardReadable {

    BoardPresenter(const GameCore& g) : game(g) {}

    int  width() const override  { return game.width(); }
    int  height() const override { return game.height(); }

    CellToken at(int x,int y) const override {
        const auto c = game.cell(x,y); // type métier

        // mapping pur vers un token textuel
        if (!c.revealed) return " ";
        if (c.flagged)   return "?";
        if (c.adjacent==0) return " ";
        static std::string digits = "012345678";
        return std::string(1, digits[c.adjacent]);
    }
  private:
    const GameCore& game;
};
```

### UI

L'interface utilisateur (UI) est responsable de l'affichage du jeu et de la gestion des interactions utilisateur. Elle utilise le présentateur pour obtenir une vue plate du jeu et l'afficher.

```cpp
class ConsoleUI {
public:
   // Affiche la grille et le HUD
   void render(const IBoardReadable& board, const HudSnapshot& hud) {
       // Affichage de la grille, du HUD
   }

   // Affiche un message
   void show_banners(std::string bs) {
       std::cout << bs << "\n";
   }

   // Attend une intention de l'utilisateur
   UserIntent wait_intent();
};
```

## 4. Contrôleur

Le contrôleur coordonne les interactions entre l'UI et le modèle de données. Il reçoit les intentions de l'UI, les transmet au modèle de données, puis récupère la vue plate du modèle de données pour la transmettre à l'UI.

Chez nous cela peut simplement être le `main()` :

```cpp
int main() {
    GameCore game(9, 9, 10); // 9x9 avec 10 mines

    BoardPresenter presenter(game); // adapter, fournit IBoardReadable
    ConsoleUI ui;

    ui.render(presenter, game.hud());

    for (game.state() == GameState::InProgress; ) {
        auto intent = ui.wait_intent();

        if (intent.action == Action::Quit) break;
        game.handle(intent);

        ui.render(presenter, game.hud());
    }

    ui.show_banners(game.state() == GameState::Won ? "You win!" : "Game over!");
};
```

## 5. Le cœur du jeu

Le cœur du jeu (GameCore) implémente les interfaces `IIntentSink` et `IOutSnapshot`. Il gère l'état du jeu, y compris la grille, les mines, les cellules révélées, et la logique de victoire/défaite.

```cpp
struct GameCore : public IIntentSink, public IOutSnapshot {

};
```