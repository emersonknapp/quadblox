#include "SDL.h"
#include "SDL_image.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

enum Assets {
    ASSET_BACKGROUND,
    ASSET_COUNT
};

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Surface* gAssetSurfaces[ ASSET_COUNT ];





void PrintSDLError(const char* const message) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

SDL_Surface* LoadSurface( const char* path ) {
    SDL_Surface* optimizedSurface = NULL;
    SDL_Surface* loadedSurface = IMG_Load( path );
    if ( loadedSurface == NULL ) {
        PrintSDLError( "Couldn't load image" );
    } else {
        optimizedSurface = SDL_ConvertSurface( loadedSurface, gScreenSurface->format, 0 );
        if ( optimizedSurface == NULL ) {
            PrintSDLError( "Unable to optimize surface" );
        }
        SDL_FreeSurface( loadedSurface );
    }
    return optimizedSurface;
}

int main( int /*argc*/, char** /*argv*/ ) {
    // Initialization
    bool quit = false;

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        quit = true;
    } else {
        gWindow = SDL_CreateWindow("Hello World!", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if ( gWindow == NULL ) {
            PrintSDLError("SDL_CreateWindow");
            quit = true;
        } else {
            gScreenSurface = SDL_GetWindowSurface( gWindow );

            gAssetSurfaces[ ASSET_BACKGROUND ] = LoadSurface( "assets/02.png" );
        }
    }

    // Main Loop

    SDL_Event e;

    while ( !quit ) {
        // Input handling
        while ( SDL_PollEvent( &e ) != 0 ) {
            if ( e.type == SDL_QUIT ) {
                quit = true;
            } else if ( e.type == SDL_KEYDOWN ) {
                switch ( e.key.keysym.sym ) {
                    case SDLK_UP:
                        printf("UP\n");
                        break;
                    case SDLK_DOWN:
                        printf("DOWN\n");
                        break;
                    case SDLK_LEFT:
                        printf("LEFT\n");
                        break;
                    case SDLK_RIGHT:
                        printf("RIGHT\n");
                        break;
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    default:
                        break;
                }
            }
        }

        // Rendering
        SDL_Rect stretchRect;
        stretchRect.x = 0;
        stretchRect.y = 0;
        stretchRect.w = SCREEN_WIDTH;
        stretchRect.h = SCREEN_HEIGHT;
        SDL_BlitScaled( gAssetSurfaces[ ASSET_BACKGROUND ], NULL, gScreenSurface, &stretchRect );
        //SDL_BlitSurface(gAssetSurfaces, NULL, screenSurface, NULL);
        SDL_UpdateWindowSurface( gWindow );
    }



    // Shutdown
    SDL_DestroyWindow( gWindow );
    SDL_Quit();
    return 0;
}
