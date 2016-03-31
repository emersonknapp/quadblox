#include "SDL.h"

void PrintSDLError(const char* const message) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

int main( int /*argc*/, char** /*argv*/ ) {
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if ( window == NULL ) {
        PrintSDLError("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    SDL_Surface* screenSurface = SDL_GetWindowSurface( window );
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x00, 0xFF, 0xFF ) );
    SDL_UpdateWindowSurface( window );
    SDL_Delay( 2000 );

    SDL_DestroyWindow( window );

    // SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // if ( renderer == nullptr ) {
    //     PrintSDLError("SDL_CreateRenderer");
    //     SDL_DestroyWindow(win);
    //     SDL_Quit();
    //     return 1;
    // }

    SDL_Quit();
    return 0;
}
