# RAII : gérer les ressources sans fuite

> **Objectif.** Lier la durée de vie d'une ressource (fenêtre, police,
> texture) à celle d'un objet, pour qu'elle soit **toujours** libérée.
>
> **Code :** [src/adapters/sdl/resources.hpp](../src/adapters/sdl/resources.hpp),
> [src/adapters/sdl/sdl_ui.hpp](../src/adapters/sdl/sdl_ui.hpp)

## Le problème

SDL est une bibliothèque C. Chaque ressource s'obtient par une fonction et se
libère par une autre :

```c
SDL_Window* w = SDL_CreateWindow(...);    // ... SDL_DestroyWindow(w);
TTF_Font* f = TTF_OpenFont(...);          // ... TTF_CloseFont(f);
```

L'ancienne version gérait cela à la main :

```cpp
SDLUI(...) { ready_ = initSDL() && loadFonts(); if (!ready_) teardown(); }
~SDLUI() { teardown(); }
bool ready() const { return ready_; }
```

Ce code a trois défauts :

- **Un objet peut exister sans être utilisable.** Chaque appelant doit penser à
  tester `ready()`. C'est l'anti-patron de l'**initialisation en deux temps**.
- **`teardown()` doit tout savoir** : quelles ressources ont été acquises, dans
  quel ordre les libérer, les remettre à `nullptr`... À chaque nouvelle
  ressource, il faut le modifier.
- **Une exception** lancée entre deux acquisitions provoque une **fuite**.

## Le principe RAII

> **R**esource **A**cquisition **I**s **I**nitialization : acquérir une
> ressource dans un constructeur, la libérer dans le destructeur.

En C++, le destructeur d'un objet local est appelé **automatiquement** quand
on sort de sa portée, quelle que soit la manière : `return`, fin de bloc,
exception. Si chaque ressource est tenue par un objet, la libération ne peut
plus être oubliée.

## `std::unique_ptr` avec un *deleter*

Pas besoin d'écrire une classe par type de ressource : `std::unique_ptr`
accepte un **deleter**, l'objet chargé de la libération.

```cpp
template <auto Destroy>
struct Deleter {
    template <class T>
    void operator()(T* resource) const { Destroy(resource); }
};

using WindowPtr   = std::unique_ptr<SDL_Window,   Deleter<SDL_DestroyWindow>>;
using RendererPtr = std::unique_ptr<SDL_Renderer, Deleter<SDL_DestroyRenderer>>;
using TexturePtr  = std::unique_ptr<SDL_Texture,  Deleter<SDL_DestroyTexture>>;
using SurfacePtr  = std::unique_ptr<SDL_Surface,  Deleter<SDL_FreeSurface>>;
using FontPtr     = std::unique_ptr<TTF_Font,     Deleter<TTF_CloseFont>>;
```

`template <auto Destroy>` est un paramètre de modèle **non-type** : ici, un
pointeur de fonction. `Deleter<SDL_DestroyWindow>` est un type vide qui sait
appeler `SDL_DestroyWindow`. Il ne coûte rien en mémoire : un `WindowPtr`
a la taille d'un simple pointeur.

`unique_ptr` n'appelle jamais le deleter sur `nullptr`, ce qui élimine les
tests `if (ptr)` suivis de `destroy(ptr); ptr = nullptr;` de l'ancienne version.

## Échouer dans le constructeur

```cpp
template <class T>
T* check(T* resource, const char* what) {
    if (!resource) throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
    return resource;
}

SdlUi::SdlUi(Size grid, Theme theme) : grid_(grid), theme_(std::move(theme)) {
    window_.reset(check(SDL_CreateWindow(...), "SDL_CreateWindow"));
    renderer_.reset(check(SDL_CreateRenderer(window_.get(), ...), "SDL_CreateRenderer"));
    cellFont_.reset(check(TTF_OpenFont(...), "TTF_OpenFont"));
    ...
}
```

Si l'ouverture de la police échoue, le constructeur lance une exception. Que
devient la fenêtre déjà créée ? C'est la garantie fondamentale du C++ :

> Quand un constructeur lance une exception, les **membres déjà construits
> sont détruits**, dans l'ordre inverse.

`window_` et `renderer_` se libèrent donc tout seuls. Il n'y a ni
`teardown()`, ni `ready()`, ni fuite. Un `SdlUi` qui existe est **toujours**
utilisable. C'est l'invariant du [chapitre 4](04-encapsulation-invariants.md),
appliqué aux ressources.

## L'ordre des membres compte

```cpp
class SdlUi {
    ...
    Library library_;       // 1. SDL_Init / TTF_Init
    Size grid_;
    Theme theme_;
    WindowPtr window_;      // 2. a besoin de SDL initialisé
    RendererPtr renderer_;  // 3. a besoin de la fenêtre
    FontPtr cellFont_;      // 4. a besoin de TTF initialisé
    FontPtr hudFont_;
    Glyphs glyphs_;         // 5. a besoin du renderer
};
```

Les membres sont **construits dans l'ordre de déclaration**, et **détruits dans
l'ordre inverse**. Cet ordre n'est pas esthétique, il est **sémantique** :
`library_` est déclaré en premier pour être détruit **en dernier**, après
toutes les textures, polices et fenêtres créées grâce à lui. Inverser deux
lignes ici, c'est appeler `SDL_DestroyTexture` après `SDL_Quit()`.

> L'ordre dans la liste d'initialisation du constructeur **n'y change rien** :
> seul compte l'ordre de déclaration dans la classe. `-Wall` avertit si les
> deux ordres diffèrent.

## Une classe RAII écrite à la main

`SDL_Init`/`SDL_Quit` ne renvoient pas de pointeur : `unique_ptr` ne convient
pas. On écrit donc une petite classe :

```cpp
class Library {
public:
    Library() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(...);
        if (TTF_Init() != 0) {
            SDL_Quit();   // le destructeur ne sera pas appelé : on défait à la main
            throw std::runtime_error(...);
        }
    }
    ~Library() { TTF_Quit(); SDL_Quit(); }

    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
};
```

Deux détails importants :

- Si le constructeur **lui-même** échoue, son **propre** destructeur n'est pas
  appelé (l'objet n'a jamais existé). D'où le `SDL_Quit()` explicite avant le
  second `throw`.
- La **copie est supprimée** : deux copies appelleraient `SDL_Quit()` deux
  fois. Une ressource unique ne se copie pas.

## La règle du zéro

Remarquez que `SdlUi` **n'a pas de destructeur**. Il n'en a pas besoin : chacun
de ses membres sait se libérer seul. Le destructeur généré par le compilateur
appelle ceux des membres, dans le bon ordre.

C'est la **règle du zéro** (*rule of zero*) :

> Si chaque membre gère sa propre ressource, la classe n'a besoin d'écrire
> **aucune** des cinq fonctions spéciales (destructeur, constructeurs de copie
> et de déplacement, affectations).

Les propriétés de copie se propagent aussi toutes seules : `unique_ptr` n'est
pas copiable, et `Library` n'est ni copiable ni déplaçable (déclarer sa copie
`= delete` supprime aussi le déplacement implicite). `SdlUi` hérite donc de ces
interdictions sans écrire une ligne, et c'est exactement ce qu'on veut : une
fenêtre ne se duplique pas.

| Règle | Quand |
| --- | --- |
| **Règle du zéro** | presque toujours : composez avec des membres RAII |
| **Règle des cinq** | seulement dans la petite classe qui gère *une* ressource brute (comme `Library`) |

## RAII ailleurs dans le projet

Le principe dépasse SDL :

- `std::vector` dans `Grid` libère sa mémoire tout seul ;
- `std::optional<Minefield>` dans `Game` détruit le champ de mines quand la
  partie est remplacée ;
- un abonnement à un Observer devrait être un objet RAII qui se désabonne
  (voir le [chapitre 11](11-adaptateurs.md)).

## À retenir

- **RAII** : une ressource appartient à un objet, et le destructeur la libère.
  Toujours, même en cas d'exception.
- `std::unique_ptr` + **deleter** adapte n'importe quelle API C.
- Un constructeur **réussit avec un objet utilisable, ou lance une
  exception**. Pas de `ready()`, pas d'`init()`.
- Les membres sont détruits **dans l'ordre inverse de leur déclaration**.
  Déclarez d'abord ce qui doit vivre le plus longtemps.
- Visez la **règle du zéro**.

## Pour réfléchir

1. Que se passerait-il si `library_` était déclaré **après** `glyphs_` ?
2. Pourquoi `SdlUi::text()` crée-t-il un `SurfacePtr` local, alors qu'on
   pourrait libérer la surface « à la main » juste après avoir créé la
   texture ?
