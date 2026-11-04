# Libérer sans fuir : RAII

> **Objectif.** Lier la durée de vie de chaque ressource SDL (fenêtre, rendu,
> texture, police, bibliothèque) à celle d'un objet, pour qu'elle soit
> **toujours** libérée, dans le bon ordre, même quand une exception survient
> au milieu de l'initialisation.
>
> **Code :** [resources.hpp](../src/adapters/sdl/resources.hpp),
> [sdl_ui.hpp](../src/adapters/sdl/sdl_ui.hpp)

## Le besoin

SDL est une bibliothèque C. Chaque ressource s'obtient par une fonction et se
libère par une autre, qu'il ne faut pas oublier d'appeler :

```c
SDL_Window* w = SDL_CreateWindow(...);    // ... SDL_DestroyWindow(w);
TTF_Font* f = TTF_OpenFont(...);          // ... TTF_CloseFont(f);
```

`SdlUi` en tient une dizaine : la bibliothèque elle-même (`SDL_Init`,
`TTF_Init`), une fenêtre, un rendu, deux polices, et une texture par chiffre.
Elles dépendent les unes des autres : la texture a besoin du rendu, le rendu
de la fenêtre, tout a besoin de la bibliothèque. Il faut donc les libérer
**dans l'ordre inverse** de leur création. Et si l'ouverture de la police
échoue (fichier absent), la fenêtre déjà créée doit être fermée quand même.

## Les options

| Option | Idée | Ce qui casse |
| --- | --- | --- |
| A. Libérer à la main dans le destructeur | `~SdlUi() { teardown(); }` avec un drapeau `ready_` | une exception entre deux acquisitions **fuit** ; un objet peut exister sans être utilisable ; `teardown` doit tout savoir |
| B. `std::unique_ptr` avec un *deleter* | chaque pointeur SDL est tenu par un `unique_ptr` qui sait le libérer | rien pour les pointeurs ; ne couvre pas `SDL_Init`/`SDL_Quit`, qui ne renvoient pas de pointeur |
| C. Une petite classe garde | `Library` appelle `SDL_Init` dans son constructeur, `SDL_Quit` dans son destructeur | rien ; c'est B écrit à la main pour le cas sans pointeur |

L'option A est celle de l'ancienne version du projet :

```cpp
SDLUI(...) { ready_ = initSDL() && loadFonts(); if (!ready_) teardown(); }
~SDLUI() { teardown(); }
bool ready() const { return ready_; }
```

C'est l'anti-patron de l'**initialisation en deux temps** : chaque appelant
doit penser à tester `ready()`. Le critère « qui dépend de qui ? » se pose
ici entre ressources : laquelle doit survivre aux autres ?

## La décision

B pour tout ce qui est un pointeur, C pour la bibliothèque. Un seul
*deleter* générique suffit, paramétré par la fonction de libération :

```cpp
template <auto Destroy>
struct Deleter {
    template <class T>
    void operator()(T* resource) const { Destroy(resource); }
};

using WindowPtr = std::unique_ptr<SDL_Window, Deleter<SDL_DestroyWindow>>;
using RendererPtr = std::unique_ptr<SDL_Renderer, Deleter<SDL_DestroyRenderer>>;
using TexturePtr = std::unique_ptr<SDL_Texture, Deleter<SDL_DestroyTexture>>;
using SurfacePtr = std::unique_ptr<SDL_Surface, Deleter<SDL_FreeSurface>>;
using FontPtr = std::unique_ptr<TTF_Font, Deleter<TTF_CloseFont>>;
```

Une fonction `check` transforme le `nullptr` que renvoie SDL en cas d'échec
en exception, avec le message d'erreur de SDL :

```cpp
template <class T>
T* check(T* resource, const char* what) {
    if (!resource) throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
    return resource;
}
```

Le constructeur de `SdlUi` enchaîne alors les acquisitions, sans aucun code
de nettoyage :

```cpp
SdlUi::SdlUi(Size grid, Theme theme) : grid_(grid), theme_(std::move(theme)) {
    window_.reset(check(SDL_CreateWindow(/* ... */), "SDL_CreateWindow"));
    renderer_.reset(check(
        SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_PRESENTVSYNC),
        "SDL_CreateRenderer"));
    cellFont_.reset(check(
        TTF_OpenFont(theme_.font.c_str(), theme_.tile * 2 / 3), "TTF_OpenFont"));
    hudFont_.reset(check(
        TTF_OpenFont(theme_.font.c_str(), theme_.hudHeight / 2), "TTF_OpenFont"));
    for (int n = 1; n <= 8; ++n)
        glyphs_.digits[n] = text(cellFont_.get(), std::to_string(n), kDigit[n]);
    // ...
}
```

Pour `SDL_Init`/`SDL_Quit`, la classe garde :

```cpp
class Library {
public:
    Library() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
        if (TTF_Init() != 0) {
            SDL_Quit();  // the destructor will not run: undo by hand
            throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());
        }
    }
    ~Library() {
        TTF_Quit();
        SDL_Quit();
    }
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
};
```

Et l'ordre des membres de `SdlUi` encode l'ordre des dépendances :

```cpp
    Library library_;       // 1. SDL_Init / TTF_Init
    Size grid_;
    Theme theme_;
    WindowPtr window_;      // 2. a besoin de SDL initialisé
    RendererPtr renderer_;  // 3. a besoin de la fenêtre
    FontPtr cellFont_;      // 4. a besoin de TTF initialisé
    FontPtr hudFont_;
    Glyphs glyphs_;         // 5. a besoin du renderer
```

## Les concepts

### RAII : la ressource appartient à un objet

*Resource Acquisition Is Initialization* : acquérir dans le constructeur,
libérer dans le destructeur. Le destructeur d'un objet local ou d'un membre
est appelé **automatiquement** quand il sort de sa portée, quelle que soit la
sortie : `return`, fin de bloc, exception. Si chaque ressource est tenue par
un objet, la libération ne peut plus être oubliée. Et quand un constructeur
lance une exception, **les membres déjà construits sont détruits**, dans
l'ordre inverse : si `TTF_OpenFont` échoue, `window_` et `renderer_` se
libèrent seuls. Un `SdlUi` qui existe est toujours utilisable : c'est
l'invariant de l'[étape 10](10-champ-de-mines.md), appliqué aux ressources.

### L'ordre de destruction est l'inverse de l'ordre de déclaration

Les membres sont construits dans l'ordre où ils sont **déclarés** dans la
classe, et détruits dans l'ordre inverse. L'ordre dans la liste
d'initialisation du constructeur n'y change rien (`-Wall` avertit si les deux
diffèrent). `library_` est donc déclaré en premier pour être détruit **en
dernier**, après toutes les textures et polices créées grâce à lui. Inverser
deux lignes, c'est appeler `SDL_DestroyTexture` après `SDL_Quit()`.

### Un *deleter* par fonction de libération

`template <auto Destroy>` prend un paramètre de modèle **non-type** : ici un
pointeur de fonction. `Deleter<SDL_DestroyWindow>` est un type vide qui sait
appeler `SDL_DestroyWindow` ; un `WindowPtr` a la taille d'un simple pointeur.
`unique_ptr` n'appelle jamais le deleter sur `nullptr`, ce qui supprime les
tests `if (ptr)` de l'ancienne version. Une fois écrit, le même `Deleter`
sert pour cinq types de ressources : zéro duplication.

### La règle du zéro

`SdlUi` n'a **ni destructeur, ni constructeur de copie, ni affectation**.
Chaque membre sait se libérer, le destructeur généré par le compilateur les
appelle dans le bon ordre. Les interdictions se propagent aussi : `unique_ptr`
n'est pas copiable et `Library` non plus, donc `SdlUi` ne l'est pas, sans une
ligne écrite. Une fenêtre ne se duplique pas.

| Règle | Quand |
| --- | --- |
| **Règle du zéro** | presque toujours : composer avec des membres RAII |
| **Règle des cinq** | dans la petite classe qui gère **une** ressource brute (`Library`) |

## Vérifier

Le comportement à vérifier est « rien ne fuit, même en cas d'échec ». Deux
moyens :

**Provoquer l'échec.** Lancer la version SDL avec une police introuvable :
le constructeur de `SdlUi` lance `std::runtime_error`, `launch`
([étape 19](19-lanceur.md)) l'attrape et affiche `fatal: TTF_OpenFont: …`,
code de retour 1. Entre-temps, `window_` et `renderer_` ont été détruits par
la garantie du constructeur, et `library_` a appelé `SDL_Quit`.

**Mesurer.** Compiler avec `-fsanitize=address` (qui inclut *LeakSanitizer*)
ou lancer sous `valgrind` (option `--leak-check=full`), jouer une partie, quitter : aucun
bloc alloué par SDL ne doit rester. Ce contrôle n'est pas automatisé dans
`make test`, parce qu'il exige une fenêtre.

Le raisonnement sur `Library` se vérifie à la lecture : si `TTF_Init` échoue,
le constructeur lance **avant** d'avoir terminé, donc `~Library()` ne sera
jamais appelé. D'où le `SDL_Quit()` explicite juste avant le `throw`.

## À retenir

- **RAII** : une ressource appartient à un objet ; le destructeur la libère,
  toujours, même en cas d'exception.
- `std::unique_ptr` + **deleter** adapte n'importe quelle API C en une ligne.
- Un constructeur **réussit avec un objet utilisable, ou lance une
  exception**. Pas de `ready()`, pas d'`init()`.
- Les membres sont détruits **dans l'ordre inverse de leur déclaration** :
  déclarez d'abord ce qui doit vivre le plus longtemps.
- Visez la **règle du zéro** ; réservez la règle des cinq à la classe qui
  tient une ressource brute.

## Exercices

1. Que se passerait-il si `library_` était déclaré **après** `glyphs_` ?
   Décrivez la séquence d'appels à la destruction d'un `SdlUi`.
2. Pourquoi `SdlUi::text()` crée-t-il un `SurfacePtr` local, alors qu'on
   pourrait appeler `SDL_FreeSurface` « à la main » juste après avoir créé la
   texture ? Quel chemin d'exécution l'approche manuelle oublie-t-elle ?
3. Où le projet applique-t-il RAII sans SDL ? Cherchez dans `Grid`, dans
   `Game` (`std::optional<Minefield>`) et dans les flux de la console.
