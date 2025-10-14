# Minesweeper moderne en C++

Ce dépôt propose une implémentation modulaire du jeu du Démineur, conçue pour
illustrer concretement les notions de ports & adapters, DTO et CQRS. C’est un
bac à sable idéal pour décortiquer comment organiser un domaine métier, le
séparer des interfaces et brancher plusieurs frontaux (console et SDL2) sur la
même colonne vertébrale.

## Règles du jeu

- Une grille rectangulaire contient un nombre fixe de bombes cachées.
- Révélez les cases sûres : un chiffre représente le nombre de bombes autour de
  la case, une case vide indique zéro bombe adjacente.
- Posez un drapeau (`?` en mode console) sur les cases suspectées.
- La première case révélée est toujours sûre, ainsi que son voisinage immédiat,
  pour éviter un game-over instantané.
- Victoire : toutes les cases non piégées sont révélées. Défaite : une bombe est
  exposée.

## Vision d’ensemble

Le projet s’articule autour de trois couches :

1. **Domaine** – règles métier pures (`Board`, `GameCore`, `Stopwatch`).
2. **Ports / DTO** – contrats d’entrée et de sortie (`ports.hpp`,
   `BoardPresenter`, `cli-grid.hpp`).
3. **Adaptateurs** – interfaces utilisateur et point d’entrée (`ui-console.hpp`,
   `ui-sdl.hpp`, `main.cpp`).

Le cœur n’a aucune dépendance sur la présentation : il ne manipule que des
interfaces abstaites et des structures de données immuables pour publier son
état, ce qui rend les frontaux remplaçables.

## Architecture détaillée

### Couche domaine

- **`Board`** (`board.hpp`) est l’agrégat central. Il stocke la grille, place les
  bombes (avec graine optionnelle), exécute la révélation par flood fill, gère
  les drapeaux et l’état de victoire/défaite. Tout est encapsulé : aucune UI ne
  touche aux cellules directement.
- **`GameCore`** (`minesweeper.hpp`) orchestre le modèle. Il implémente le port
  d’entrée `IIntentSink` pour recevoir les commandes et le port de sortie
  `IOutSnapshot` pour exposer l’état actuel. C’est lui qui:
  - traduit une intention en mutations sur `Board`,
  - déclenche les transitions de `GameState`,
  - gère le chronomètre (`Stopwatch`) et publie le HUD.
- **`Stopwatch`** (`stopwatch.hpp`) fournit un service minimal de mesure du temps
  écoulé, indépendant du reste.

Cette couche est dénuée d’`iostream`, de SDL ou de dépendances système : elle
peut être testée et réutilisée dans n’importe quel contexte.

### Ports et DTO

Les ports sont regroupés dans `ports.hpp` et définissent les contracts entre la
logique métier et le monde extérieur :

- `UserIntent` + `IIntentSink` ≈ **commands** côté CQRS.
- `IBoardReadable`, `HudSnapshot`, `IOutSnapshot` ≈ **queries**. Ils décrivent
  une projection prête à être consommée sans exposer les structures internes.
- `CellToken` (alias `std::string`) joue le rôle de DTO minimal pour une case.

`BoardPresenter` (`presenter.hpp`) convertit le modèle en DTO : il parcourt
`GameCore` et synthétise des `CellToken` textuels (`"."`, `"?"`, `"3"`, `"*"`,
etc.). La console et l’UI SDL n’ont donc jamais besoin d’accéder aux champs
internes du `Board`.

`CliGrid` (`cli-grid.hpp`) est un adaptateur secondaire qui mise sur ces DTO
pour tracer une grille Unicode colorisable en console. Il n’a aucune logique
métier : il s’appuie sur les données déjà préparées.

### CQRS concret

Le projet applique CQRS de manière pragmatique :

- **Commands** : `ConsoleUI` ou `SDLUI` convertissent une action utilisateur
  (clic, entrée clavier) en `UserIntent`. Cet objet transite vers `GameCore`
  via `IIntentSink::handle`. Aucune donnée de lecture n’est modifiée à ce stade.
- **Queries** : après chaque commande (ou à chaque frame pour SDL),
  l’interface récupère une photographie du jeu (`IOutSnapshot`), constituée de :
  - `const IBoardReadable& board()` -> projection **pull** de la grille,
  - `HudSnapshot hud()` -> DTO contenant minuteur et compteurs,
  - `GameState state()` -> statut coarse-grained.

La projection est immuable côté UI : toute transformation est pure (ex. mapping
des tokens vers des glyphes). Cela évite qu’une UI contamine l’état partagé ou
effectue des modifications cachées.

### Flux de données (pas-à-pas)

1. `main.cpp` parse les arguments CLI et instancie un `GameCore`.
2. Selon la build, il crée `ConsoleUI` ou `SDLUI`.
3. L’UI construit un `UserIntent` (ex. `Action::Reveal`, `Pos{2,5}`).
4. `GameCore::handle` traite l’intent :
   - vérifie les bornes,
   - interagit avec `Board`,
   - met à jour `GameState`,
   - capture le temps final le cas échéant.
5. L’UI appelle `game.board()` + `game.hud()` + `game.state()` pour rendre un
   écran. Aucun accès direct à `Board`.
6. Le cycle continue jusqu’à ce que l’utilisateur quitte ou remporte la partie.

Grâce à cette boucle `pull`, l’UI console s’exécute sur stdin/stdout tandis que
l’UI SDL tourne dans une boucle d’événements ; leurs cadences sont différentes,
mais elles partagent les mêmes ports.

## Frontaux disponibles

- **Console** (`ui-console.hpp`, `cli-grid.hpp`) :
  - affiche une grille Unicode bordée (via `CliGrid`),
  - active automatiquement les couleurs ANSI quand le terminal les supporte,
  - transforme les tokens du presenter en pictogrammes ASCII.
- **SDL2** (`ui-sdl.hpp`) :
  - fenêtrage résponsive, textures vectorielles, fonts via SDL_ttf,
  - gestion de la souris (clic gauche = révélation, clic droit = drapeau),
  - animations et rafraîchissement à ~60 FPS.

La sélection se fait à la compilation via `-DSDL`. `main.cpp` contient les
`#ifdef SDL` qui branchent la bonne implémentation.

## Installation sur Ubuntu

### Prérequis système

```bash
sudo apt update
sudo apt install build-essential pkg-config
```

Interface SDL optionnelle :

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev
```

### Récupération du dépôt

```bash
git clone https://exemple.org/minesweeper2.git
cd minesweeper2
```

## Compilation & exécution

### Makefile (par défaut)

```bash
make                 # version console
./build/minesweeper-console

make sdl             # version SDL si les paquets sont installés
./build/minesweeper-sdl

make clean           # nettoyage des artefacts
```

### CMake

```bash
cmake -S . -B build
cmake --build build
```

Ajouter `-DMINESWEEPER_BUILD_SDL=OFF` pour forcer un build sans SDL.

## Personnalisation

- Taille/grille personnalisée :
  ```bash
  ./build/minesweeper-console 16 16 40
  ```
- Forcer un rendu console coloré :
  ```cpp
  ConsoleUI ui(ConsoleUI::Config{
      .grid_options = ConsoleUI::colourfulGridOptions()
  });
  ```
- Adapter l’UI SDL :
  ```cpp
  SDLUI ui(cols, rows, SDLUI::Config{
      .tile_px = 40,
      .font_path = "assets/collegiate.ttf"
  });
  ```

## Ressources complémentaires

- `architecture.md` détaille les choix CQRS/DDD avec exemples additionnels.
- `minesweeper.md` sert d’entrée pédagogique pour découvrir le kata.
- `assets/` contient la police utilisée par l’UI SDL.

Bon jeu ! N’hésitez pas à forker, brancher une UI supplémentaire ou enrichir le
core (modes de difficulté, meilleurs scores, etc.). :)
