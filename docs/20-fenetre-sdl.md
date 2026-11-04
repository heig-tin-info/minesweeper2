# La fenêtre SDL

> **Objectif.** Brancher une deuxième interface, graphique, sur le même jeu,
> **sans toucher une ligne** de `core/` ni de `app/`. C'est le test final de
> l'architecture : si le cœur doit bouger, c'est qu'il était mal découpé.
>
> **Code :** [sdl_ui.hpp](../src/adapters/sdl/sdl_ui.hpp),
> [sdl_ui.cpp](../src/adapters/sdl/sdl_ui.cpp),
> [sdl/main.cpp](../src/adapters/sdl/main.cpp)

## Le besoin

Le jeu se joue dans le terminal depuis l'[étape 18](18-terminal.md). On veut
maintenant la même partie à la souris : clic gauche pour dévoiler, clic droit
pour un drapeau, `N` pour recommencer, un chronomètre qui avance à l'écran.

La bibliothèque SDL apporte tout ce qui n'a rien à voir avec le démineur :
une fenêtre, des événements souris et clavier, des polices, des textures. Le
risque est de laisser ces détails s'infiltrer dans le cœur. Les trois
critères habituels :

- **Testable sans écran ?** Le cœur doit rester testable exactement comme
  avant. La fenêtre, elle, ne le sera pas : on verra plus bas ce qu'on teste
  à la place.
- **Écrit deux fois ?** Les règles existent déjà. Pas une seule ne doit être
  recopiée pour la version graphique.
- **Qui dépend de qui ?** L'adaptateur SDL dépend des ports. Rien ne dépend
  de lui.

## Les options

| Option | Idée | Ce qui casse |
| --- | --- | --- |
| A. Une classe fenêtre qui refait le jeu | `SdlBoard` avec ses mines, ses règles, son dessin | tout est écrit deux fois ; deux jeux qui divergent |
| B. Une classe de base `UserInterface` | `ConsoleUi` et `SdlUi` héritent d'un `run()` virtuel | elles n'ont rien en commun ; l'héritage ne partage rien ([étape 19](19-lanceur.md)) |
| C. Un deuxième adaptateur sur les mêmes ports | `SdlUi::run`, même signature que `ConsoleUi::run` | rien : c'est le but des ports de l'[étape 6](06-ports-et-adaptateurs.md) |

## La décision

Option C. `SdlUi` a exactement la même signature d'entrée que `ConsoleUi`,
parce qu'elle consomme les mêmes ports. Son `main` est identique à celui de la
console, à une ligne près :

```cpp
int main(int argc, char* argv[]) {
    using namespace minesweeper;
    return launch(argc, argv, [](Session& session) {
        sdl::SdlUi ui{session.size()};
        ui.run(session, session);
    });
}
```

La boucle n'est plus bloquante sur `std::getline` : c'est une **boucle
d'événements**, qui se réveille toutes les 200 ms même sans événement, pour
que le chronomètre avance :

```cpp
constexpr Uint32 kFrameMs = 200;  // wake up regularly so the HUD clock ticks

void SdlUi::run(const GameView& view, CommandSink& commands) {
    for (;;) {
        draw(view);
        SDL_Event event;
        if (!SDL_WaitEventTimeout(&event, kFrameMs)) continue;
        do {
            if (isQuit(event)) return;
            if (const auto command = translate(event))
                commands.handle(*command);
        } while (SDL_PollEvent(&event));
    }
}
```

Le travail de traduction est le même qu'en console, avec d'autres entrées :
un événement SDL au lieu d'une ligne de texte, des pixels au lieu d'une
notation d'échecs.

```cpp
std::optional<Command> SdlUi::translate(const SDL_Event& event) const {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_n || event.key.keysym.sym == SDLK_r)
                return NewGame{};
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (const auto p = cellAt(event.button.x, event.button.y)) {
                if (event.button.button == SDL_BUTTON_LEFT) return Reveal{*p};
                if (event.button.button == SDL_BUTTON_RIGHT) return ToggleFlag{*p};
            }
            break;
        default:
            break;
    }
    return std::nullopt;
}

// Pixels to grid coordinates: a concern of this adapter only.
std::optional<Pos> SdlUi::cellAt(int x, int y) const {
    if (y < theme_.hudHeight) return std::nullopt;
    const Pos p{(y - theme_.hudHeight) / theme_.tile, x / theme_.tile};
    if (!grid_.contains(p)) return std::nullopt;
    return p;
}
```

Un clic dans le bandeau du haut ou hors de la grille ne produit **aucune**
commande : une position invalide n'atteint jamais le domaine. Et le dessin
relit tout par le port de lecture, case par case :

```cpp
void SdlUi::draw(const GameView& view) {
    SDL_SetRenderDrawColor(renderer_.get(),
                           kBackground.r, kBackground.g, kBackground.b, 255);
    SDL_RenderClear(renderer_.get());
    drawHud(view);
    for (int row = 0; row < grid_.rows; ++row)
        for (int col = 0; col < grid_.cols; ++col)
            drawCell({row, col}, view.cell({row, col}));
    drawBanner(view.status());
    SDL_RenderPresent(renderer_.get());
}
```

`drawCell` fait un `std::visit` sur le `CellView` de l'[étape 14](14-ce-que-voit-l-interface.md) :
le compilateur garantit que les cinq situations (cachée, drapeau, dévoilée,
mine, drapeau faux) sont dessinées.

## Les concepts

### Un deuxième adaptateur sur les mêmes ports

Un port vaut le jour où un **second** adaptateur s'y branche sans que rien
d'autre ne change. `ConsoleUi` et `SdlUi` n'ont pas une ligne en commun, ni
classe de base, ni fonction partagée ; elles partagent uniquement les deux
interfaces qu'elles consomment. C'est ce que l'[étape 5](05-choisir-une-architecture.md)
promettait quand elle écartait l'approche « une classe `Board` qui fait tout ».

### Boucle bloquante, boucle d'événements

La console **attend** une ligne ; rien ne se passe entre deux saisies. SDL
**fournit** des événements, et c'est la bibliothèque qui décide quand. La
boucle de `SdlUi` doit donc attendre « un événement **ou** 200 ms », vider la
file d'événements en attente (`SDL_PollEvent`), puis redessiner. Chaque
adaptateur possède la boucle de sa technologie, comme décidé à
l'[étape 7](07-sens-des-flux.md).

### Pull à cadence fixe

`draw` ne reçoit aucune notification : il **tire** l'état complet à chaque
image, soit cinq fois par seconde ou après chaque commande. Une grille de
30 × 16 cases se relit en quelques microsecondes ; redessiner tout est plus
simple et plus sûr que de calculer ce qui a changé. L'[étape 7](07-sens-des-flux.md)
avait écarté l'Observateur pour cette raison : la fenêtre confirme le choix.

### Traduire, et valider en traduisant

`cellAt` convertit des pixels en `Pos` **et** filtre : le bandeau, les bords,
tout ce qui n'est pas une case. Le domaine ne connaît ni les pixels ni le
bandeau ; il ne connaît que `Pos`, et seulement des `Pos` valides. La
console fait la même chose avec `parsePos("Z9")` à l'[étape 18](18-terminal.md).

## Vérifier

Le critère de réussite de cette étape est **observable** dans l'historique
du projet : le commit qui ajoute la fenêtre SDL ne touche ni `src/core/` ni
`src/app/`.

```bash
git diff --stat <avant-sdl> <après-sdl> -- src/core src/app
# (aucune sortie)
```

Le test d'architecture de l'[étape 8](08-modules.md) le garantit pour
l'avenir : `#include <SDL2/SDL.h>` dans `core/` ou `app/` fait échouer
`make test`, grâce au motif `<(iostream|fstream|cstdio|SDL)`.

`SdlUi` n'a **pas** de test unitaire. Elle ouvre une vraie fenêtre dans son
constructeur, ce qui exige un écran : impossible sur un serveur d'intégration
continue. Et `translate` comme `cellAt` sont des méthodes privées qui
dépendent de l'objet construit. Ce qu'on teste à la place :

- **tout ce qui est sous les ports** : `Session`, `Game`, la console, avec les
  mêmes doublures ([étape 22](22-tests.md)) ;
- **la compilation conditionnelle** : CMake ne construit `minesweeper-sdl` que
  si SDL2 et SDL2_ttf sont trouvés, et `make test` passe sans eux ;
- **à la main** : `make run-sdl`, puis une partie gagnée et une partie perdue.

C'est un compromis assumé. L'exercice 1 montre comment réduire la part non
testée.

## À retenir

- Le **test final d'une architecture**, c'est d'ajouter un adaptateur sans
  toucher au cœur. Ici, `git diff` sur `core/` et `app/` est vide.
- Deux adaptateurs ne partagent **rien** sinon les ports qu'ils consomment.
- Chaque adaptateur possède la **boucle** de sa technologie : bloquante en
  console, événementielle avec SDL.
- Le **pull** suffit : redessiner tout, cinq fois par seconde, coûte moins
  qu'un mécanisme de notification.
- Un adaptateur **valide** ce qu'il traduit : aucune position hors grille
  n'atteint le domaine.

## Exercices

1. Sortez `cellAt` de la classe : écrivez une fonction libre
   `cellAt(const Theme&, Size, int x, int y)`, qui renvoie un `std::optional<Pos>`, dans un
   en-tête sans `#include <SDL2/SDL.h>`, et testez-la avec GoogleTest : un
   clic dans le bandeau, un clic sur la dernière case, un clic à droite de la
   grille. Que reste-t-il de non testable dans `SdlUi` ?
2. Esquissez un adaptateur HTTP : `POST /reveal` avec le corps `{"row": 2, "col": 3}`. Où
   est la boucle ? Que renvoie-t-il, et par quel port le lit-il ?
3. Pourquoi la hauteur du bandeau (`hudHeight`) est-elle soustraite dans
   `cellAt` et non dans `Pos` ou dans `Game` ? Que se passerait-il si le
   domaine connaissait le bandeau ?
