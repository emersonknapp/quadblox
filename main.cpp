#include "SDL.h"
#include "SDL_image.h"

void PrintSDLError(const char* const message) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

int main( int /*argc*/, char** /*argv*/ ) {
    SDL_Window * window;
    SDL_Surface * screenSurface;
    //Image to display on screen
    SDL_Surface * helloWorld;

    // Initialization
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        return 1;
    }

    window = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if ( window == NULL ) {
        PrintSDLError("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    screenSurface = SDL_GetWindowSurface( window );

    helloWorld = IMG_Load( "assets/01.jpg" );
    if ( helloWorld == NULL ) {
        PrintSDLError("Couldn't load image");
        SDL_Quit();
        return 1;
    }

    // Draw
    SDL_BlitSurface(helloWorld, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface( window );
    SDL_Delay( 2000 );


    // Shutdown
    SDL_FreeSurface(helloWorld);
    helloWorld = NULL;
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}
