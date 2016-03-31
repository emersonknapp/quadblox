#include "SDL.h"
#include "SDL_image.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

void PrintSDLError( const char* message ) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Texture* gTexture = NULL;

SDL_Texture* loadTexture( const char* path ) {
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load( path );
    if ( loadedSurface == NULL ) {
        PrintSDLError("IMG_Load");
    } else {
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
        if ( newTexture == NULL ) {
            PrintSDLError( "SDL_CreateTextureFromSurface" );
        }
        SDL_FreeSurface( loadedSurface );
    }
    return newTexture;
}

bool loadMedia() {
    bool success = true;
    gTexture = loadTexture( "assets/03.png" );
    if ( gTexture == NULL ) {
        printf( "Failed to load texture image!\n" );
        success = false;
    }
    return success;
}

void close() {
    SDL_DestroyTexture( gTexture );
    gTexture = NULL;
    SDL_DestroyRenderer( gRenderer );
    gRenderer = NULL;
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    IMG_Quit();
    SDL_Quit();
}

bool init() {
    bool success = true;
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        success = false;
    } else {
        if ( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        gWindow = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if ( gWindow == NULL ) {
            PrintSDLError("SDL_CreateWindow");
            success = false;
        } else {
            gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
            if ( gRenderer == NULL ) {
                PrintSDLError( "SDL_CreateRenderer" );
                success = false;
            } else {
                SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF );
                int imgFlags = IMG_INIT_PNG;
                if ( !( IMG_Init( imgFlags ) & imgFlags ) ) {
                    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
                    success = false;
                }
            }
        }
    }
    return success;
}

int main( int /*argc*/, char** /*argv*/ ) {
    // Initialization
    if ( !init() ) {
        printf( "Failed to initialize\n" );
    } else {
        if ( !loadMedia() ) {
            printf( "Failed to load media\n" );
        } else {
            // Main Loop
            bool quit = false;
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
                SDL_RenderClear( gRenderer );
                SDL_RenderCopy( gRenderer, gTexture, NULL, NULL );
                SDL_RenderPresent( gRenderer );
            }
        }
    }

    // Shutdown
    close();
    return 0;
}
