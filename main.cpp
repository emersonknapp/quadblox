#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "quadblox.h"

void PrintSDLError( const char* message ) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

SDL_Texture* loadTexture( const char* path, SDL_Renderer* renderer ) {
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load( path );
    if ( loadedSurface == NULL ) {
        PrintSDLError("IMG_Load");
    } else {
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
        if ( newTexture == NULL ) {
            PrintSDLError( "SDL_CreateTextureFromSurface" );
        }
        SDL_FreeSurface( loadedSurface );
    }
    return newTexture;
}

Assets* loadMedia( SDL_Renderer* renderer ) {
    bool success = true;
    Assets* assets = new Assets();
    

    assets->font = TTF_OpenFont( "assets/OpenSans.ttf", 16 );
    if ( assets->font == NULL ) {
        printf( "Failed to open font: %s\n", TTF_GetError() );
        success = false;
    } 

    if ( success ) {
        char path[256] = "assets/Block0.png";
        for ( size_t i = 0; i < AssetType::COUNT; i++ ) {
            const char* filename = AssetTextureFiles[i];
            if ( filename == NULL ) {
                printf("Undefined texture filename for type %zu\n", i);
                success = false;
                break;
            }

            if ( strncmp(filename, "TEXT_", 5) == 0 ) {
                SDL_Color textColor = { 0xFF, 0xFF, 0xFF, 0xFF };
                SDL_Surface* textSurface = TTF_RenderText_Blended(assets->font, filename+5, textColor);
                assets->textures[i] = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_FreeSurface(textSurface);
                continue; 
            }

            sprintf( path, "assets/%s", filename );
            assets->textures[i] = loadTexture( path, renderer );
            if ( assets->textures[i] == NULL ) {
                printf( "Failed to load texture %s\n", path );
                success = false;
                break;
            }
        }
    }

    if ( !success ) {
        delete assets;
        assets = NULL;
    }
    return assets;
}

void freeMedia( Assets* assets ) {
    for ( size_t i = 0; i < AssetType::COUNT; i++ ) {
        SDL_DestroyTexture( assets->textures[i] );
        assets->textures[i] = NULL;
    }
}

void initSDL( SDL_Renderer*& renderer, SDL_Window*& window ) {
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        return;
    } 

    if ( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
        printf( "Warning: Linear texture filtering not enabled!" );
    }
    window = SDL_CreateWindow("QuadBlocks!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if ( window == NULL ) {
        PrintSDLError("SDL_CreateWindow");
        return;
    } 

    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
    if ( renderer == NULL ) {
        PrintSDLError( "SDL_CreateRenderer" );
        return;
    } 


    int imgFlags = IMG_INIT_PNG;
    if ( !( IMG_Init( imgFlags ) & imgFlags ) ) {
        printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
        return;
    }

    if ( TTF_Init() == -1 ) {
        printf("SDL_ttf failed to initialize: %s\n", TTF_GetError());
        return;
    }

}

void shutdownSDL( SDL_Renderer* renderer, SDL_Window* window ) {
    if ( renderer ) {
        SDL_DestroyRenderer( renderer );
    }
    renderer = NULL;
    if ( window) {
        SDL_DestroyWindow( window );
    }
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}


void gameKeydownHandler( GameState* gameState, SDL_KeyboardEvent key ) {
    switch ( key.keysym.sym ) {
        case SDLK_LEFT:
            gameState->horizMove -= 1;
            break;
        case SDLK_RIGHT:
            gameState->horizMove += 1;
            break;
        case SDLK_UP:
            gameState->rotate += 1;
            break;
        case SDLK_DOWN:
            gameState->turbo = true;
            break;
        case SDLK_q:
        case SDLK_ESCAPE:
            gameState->wantsToQuit = true;
            break;
        case SDLK_p:
            gameState->paused = !gameState->paused;
            break;
        default:
            break;
    }
}

void gameKeyupHandler( GameState* gameState, SDL_KeyboardEvent key ) {
    switch ( key.keysym.sym ) {
        case SDLK_DOWN:
            gameState->turbo = false;
            break;
        default:
            break;
    }
}

void mainLoop( SDL_Renderer* renderer, Assets* assets ) {
    SDL_Event e;

    GameState gameState;
    for ( int i = 0; i < PLAYAREA_HEIGHT; i++ ) {
        for ( int j = 0; j < PLAYAREA_WIDTH; j++ ) {
            gameState.blockBake[i][j] = -1;
        }
    }

    static const uint64_t perfFreq = SDL_GetPerformanceFrequency();
    static const double targetTicsPerFrame = perfFreq / 60.0;
    static const double fpsSmoothing = 0.95;
    uint64 lastTime = SDL_GetPerformanceCounter();
    uint64 newTime;
    double frameTime;
    double currentFPS = 0;
    char fpsStr[4] = "000";

    SDL_Color textColor = { 0, 0, 0, 0xFF };
    SDL_Surface* textSurface = NULL;
    SDL_Texture* textTexture = NULL;
    SDL_Rect debugRect;

    while ( !gameState.wantsToQuit ) {
        newTime = SDL_GetPerformanceCounter();
        frameTime = newTime - lastTime;
        if ( frameTime < targetTicsPerFrame ) {
            continue;
        }
        frameTime /= double(perfFreq);
        lastTime = newTime;

        currentFPS = ( currentFPS * fpsSmoothing ) + ( ( 1.0 / frameTime ) * ( 1.0 - fpsSmoothing ) );
        snprintf(fpsStr, 4, "%d", int(currentFPS) );

        // Input handling
        while ( SDL_PollEvent( &e ) != 0 ) {
            if ( e.type == SDL_QUIT ) {
                break;
            } else if ( e.type == SDL_KEYDOWN ) {
                gameKeydownHandler( &gameState, e.key );
            } else if ( e.type == SDL_KEYUP ) {
                gameKeyupHandler( &gameState, e.key );
            }
        }

        // clear screen to debug color and draw any debug rendering, like FPS
        SDL_SetRenderDrawColor( renderer, 0x99, 0xA0, 0x99, 0xFF );
        SDL_RenderClear( renderer );

        {
            textSurface = TTF_RenderText_Blended(assets->font, fpsStr, textColor);
            textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            debugRect = Rect(0, 0, textSurface->w, textSurface->h);
            SDL_RenderCopy(renderer, textTexture, NULL, &debugRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }

        GameUpdateAndRender( renderer, assets, &gameState, frameTime );

        SDL_RenderSetViewport(renderer, NULL);
        SDL_RenderPresent( renderer );
    }
}

int main( int /*argc*/, char** /*argv*/ ) {
    SDL_Renderer* renderer = NULL;
    SDL_Window* window = NULL;
    Assets* assets = NULL;

    initSDL( renderer, window );
    if ( renderer == NULL || window == NULL ) {
        printf( "Failed to initialize\n" );
    } else {
        SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        assets = loadMedia( renderer );
        if ( assets == NULL ) {
            printf( "Failed to load media\n" );
        } else {
            mainLoop( renderer, assets );
        }
    }
    if ( assets ) {
        freeMedia( assets );
    }
    shutdownSDL( renderer, window );
    return 0;
}
