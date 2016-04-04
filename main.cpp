#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "quadblox.cpp"

void PrintSDLError( const char* message ) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

typedef struct Assets {
    static const int numBlockTextures = 7;
    SDL_Texture* blockTextures[ numBlockTextures ];
    SDL_Texture* backgroundTexture = NULL;
} Assets;

SDL_Rect Rect( int x, int y, int w, int h ) {
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
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
    char path[18] = "assets/Block0.png";
    for ( int i = 0; i < NUM_BLOCKTYPES; i++ ) {
        sprintf( path, "assets/Block%01d.png", i + 1 );
        assets->blockTextures[i] = loadTexture( path, renderer );
        if ( assets->blockTextures[i] == NULL ) {
            printf( "Failed to load texture %s\n", path );
            success = false;
            break;
        }
    }
    assets->backgroundTexture = loadTexture( "assets/BG.png", renderer );
    if ( assets->backgroundTexture == NULL ) {
        printf( "Failed to load texture assets/BG.png\n");
        success = false;
    }

    if ( !success ) {
        delete assets;
        assets = NULL;
    }
    return assets;
}

void freeMedia( Assets* assets ) {
    for ( int i = 0; i < assets->numBlockTextures; i++ ) {
        SDL_DestroyTexture( assets->blockTextures[i] );
        assets->blockTextures[i] = NULL;
    }
    SDL_DestroyTexture( assets->backgroundTexture );
    assets->backgroundTexture = NULL;
}

void initSDL( SDL_Renderer*& renderer, SDL_Window*& window ) {
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
    } else {
        if ( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        window = SDL_CreateWindow("QuadBlocks!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if ( window == NULL ) {
            PrintSDLError("SDL_CreateWindow");
        } else {
            renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
            if ( renderer == NULL ) {
                PrintSDLError( "SDL_CreateRenderer" );
            } else {
                int imgFlags = IMG_INIT_PNG;
                if ( !( IMG_Init( imgFlags ) & imgFlags ) ) {
                    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
                }
            }
        }
    }    
}

void shutdownSDL( SDL_Renderer* renderer, SDL_Window* window ) {
    SDL_DestroyRenderer( renderer );
    renderer = NULL;
    SDL_DestroyWindow( window );
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}

void drawBlock( SDL_Renderer* renderer, const Assets* assets, const QuadBlock& qb, int w, int h ) {
    SDL_Rect blockRect;
    int blocksDrawn = 0;
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( qb.square( i, j ) ) {
                blockRect = Rect( ( qb.x + j ) * w, ( qb.y + i ) * h, w, h );
                SDL_RenderCopy( renderer, assets->blockTextures[qb.blockType], NULL, &blockRect );
                //SDL_RenderFillRect( renderer, &blockRect );
                blocksDrawn++;
            }
        }
    }
}

void drawGame( SDL_Renderer* renderer, const Assets* assets, const GameState* gameState ) {
    // background
    SDL_SetRenderDrawColor( renderer, 0x99, 0xA0, 0x99, 0xFF );
    SDL_RenderClear( renderer );

    // Game area
    int gameAreaHeight = SCREEN_HEIGHT * 0.8;
    gameAreaHeight = gameAreaHeight - ( gameAreaHeight % PLAYAREA_HEIGHT );
    int gameAreaWidth = gameAreaHeight / 2; 
    SDL_Rect gameAreaViewport = Rect( 0.1 * SCREEN_HEIGHT, 0.1 * SCREEN_WIDTH, gameAreaWidth, gameAreaHeight );
    SDL_RenderSetViewport( renderer, &gameAreaViewport );
    SDL_SetRenderDrawColor( renderer, 0xCC, 0xCC, 0xCC, 0xFF );
    SDL_Rect gameAreaBackground = Rect( 0, 0, gameAreaWidth, gameAreaHeight );
    SDL_RenderCopy( renderer, assets->backgroundTexture, NULL, &gameAreaBackground );

    // Blocks
    int blockWidth = gameAreaWidth / PLAYAREA_WIDTH;
    int blockHeight = gameAreaHeight / PLAYAREA_HEIGHT;
    drawBlock( renderer, assets, gameState->currentBlock, blockWidth, blockHeight );
    for ( const QuadBlock& qb : gameState->blocks ) {
        drawBlock( renderer, assets, qb, blockWidth, blockHeight );
    }

    SDL_RenderPresent( renderer );
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

    std::chrono::duration<double>                               tSeconds( 0.0 );
    std::chrono::duration<double>                               dtSeconds( 0.01 );
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> newTime;
    std::chrono::duration<double>                               accumulator( 0.0 );
    std::chrono::duration<double>                               frameTime;

    std::chrono::duration<double> runningFrameAverage( 0.0 );
    static const double fpsSmoothing = 0.9;
    std::chrono::duration<double> currentFPS;

    while ( !gameState.wantsToQuit ) {
        newTime = std::chrono::high_resolution_clock::now();
        frameTime = newTime - lastTime;
        lastTime = newTime;

        accumulator += frameTime;

        currentFPS = ( currentFPS * fpsSmoothing ) + ( frameTime * ( 1.0 - fpsSmoothing ) );

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

        while ( accumulator >= dtSeconds ) {
            updateGame( &gameState, dtSeconds );
            accumulator -= dtSeconds;
            tSeconds += dtSeconds;
        }
        drawGame( renderer, assets, &gameState );
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
        assets = loadMedia( renderer );
        if ( assets == NULL ) {
            printf( "Failed to load media\n" );
        } else {
            mainLoop( renderer, assets );
        }
    }

    freeMedia( assets );
    shutdownSDL( renderer, window );
    return 0;
}
