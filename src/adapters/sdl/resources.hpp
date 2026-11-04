#pragma once

// RAII for the C resources of SDL. Each handle frees itself, in the reverse
// order of construction, whatever path (return or exception) leaves the scope.
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace minesweeper::sdl {

// A deleter calling the given C function; unique_ptr never calls it on null.
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

// Turns SDL's "null means failure" convention into an exception.
template <class T>
T* check(T* resource, const char* what) {
    if (!resource) throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
    return resource;
}

// Keeps the SDL and SDL_ttf libraries initialised for its whole lifetime.
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

}  // namespace minesweeper::sdl
