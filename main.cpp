#include "SDL.h"
#include "SDL_image.h"

#include <vector>
#include <chrono>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int PLAYAREA_WIDTH = 10;
const int PLAYAREA_HEIGHT = 10;

void PrintSDLError( const char* message ) {
    printf("%s Error: %s\n", message, SDL_GetError());
}

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

struct QuadBlock {
    int x;
    int y;
};

typedef struct GameState {
    float fallsPerSecond = 1;
    std::vector<QuadBlock> blocks;
} GameState;

typedef struct Platform {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
} Platform;



bool loadMedia(SDL_Renderer* /*renderer*/) {
    bool success = true;
    /*
    gTexture = loadTexture( "assets/03.png", renderer );
    if ( gTexture == NULL ) {
        printf( "Failed to load texture image!\n" );
        success = false;
    }
    */
    return success;
}

Platform* initSDL() {
    bool success = true;
    SDL_Renderer* renderer = NULL;
    SDL_Window* window = NULL;

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        PrintSDLError("SDL_Init");
        success = false;
    } else {
        if ( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        window = SDL_CreateWindow("QuadBlocks!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if ( window == NULL ) {
            PrintSDLError("SDL_CreateWindow");
            success = false;
        } else {
            renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
            if ( renderer == NULL ) {
                PrintSDLError( "SDL_CreateRenderer" );
                success = false;
            } else {
                int imgFlags = IMG_INIT_PNG;
                if ( !( IMG_Init( imgFlags ) & imgFlags ) ) {
                    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
                    success = false;
                }
            }
        }
    }

    if ( success ) {
        Platform* platform = new Platform();
        platform->renderer = renderer;
        platform->window = window;
        return platform;
    } else {
        return NULL;
    }
}

void shutdownSDL( Platform* platform ) {
    //SDL_DestroyTexture( gTexture );
    //gTexture = NULL;

    SDL_DestroyRenderer( platform->renderer );
    platform->renderer = NULL;
    SDL_DestroyWindow( platform->window );
    platform->window = NULL;
    IMG_Quit();
    SDL_Quit();

    delete platform;
}

void updateGame( GameState* gameState, std::chrono::duration<double> dt ) {

}

void drawGame( SDL_Renderer* renderer, const GameState* gameState ) {
    // background
    SDL_SetRenderDrawColor( renderer, 0xFF, 0x00, 0x00, 0xFF );
    SDL_RenderClear( renderer );

    // Game area
    int gameAreaWidth = 3 * SCREEN_WIDTH / 4;
    int gameAreaHeight = SCREEN_HEIGHT;
    SDL_Rect gameAreaViewport = Rect( 0, 0, gameAreaWidth, gameAreaHeight );
    SDL_RenderSetViewport( renderer, &gameAreaViewport );
    SDL_SetRenderDrawColor( renderer, 0xCC, 0xCC, 0xCC, 0xFF );
    SDL_Rect gameAreaBackground = Rect( 0, 0, gameAreaWidth, gameAreaHeight );
    SDL_RenderFillRect( renderer, &gameAreaBackground );

    // Blocks
    int blockWidth = gameAreaWidth / PLAYAREA_WIDTH;
    int blockHeight = gameAreaHeight / PLAYAREA_HEIGHT;
    SDL_SetRenderDrawColor( renderer, 0x00, 0xAA, 0xAA, 0xFF );
    SDL_Rect blockRect;
    for ( const QuadBlock& qb : gameState->blocks ) {
        blockRect = Rect( qb.x * blockWidth, qb.y * blockHeight, blockWidth, blockHeight );
        SDL_RenderFillRect( renderer, &blockRect );
    }

    SDL_RenderPresent( renderer );
}

void mainLoop( SDL_Renderer* renderer ) {
    bool quit = false;
    SDL_Event e;

    GameState gameState;
    QuadBlock block0 = { 0, 0 };
    gameState.blocks.push_back( block0 );

    std::chrono::duration<double> tSeconds( 0.0 );
    std::chrono::duration<double> dtSeconds( 0.01 );
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> newTime;
    std::chrono::duration<double> accumulator( 0.0 );
    std::chrono::duration<double> frameTime;

    while ( !quit ) {
        newTime = std::chrono::high_resolution_clock::now();
        frameTime = newTime - lastTime;
        lastTime = newTime;

        accumulator += frameTime;

        // Input handling
        while ( SDL_PollEvent( &e ) != 0 ) {
            if ( e.type == SDL_QUIT ) {
                quit = true;
            } else if ( e.type == SDL_KEYDOWN ) {
                switch ( e.key.keysym.sym ) {
                    case SDLK_UP:
                        printf( "UP\n" );
                        break;
                    case SDLK_DOWN:
                        printf( "DOWN\n" );
                        break;
                    case SDLK_LEFT:
                        printf( "LEFT\n" );
                        break;
                    case SDLK_RIGHT:
                        printf( "RIGHT\n" );
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

        while ( accumulator >= dtSeconds ) {
            updateGame( &gameState, dtSeconds );
            accumulator -= dtSeconds;
            tSeconds += dtSeconds;
        }

        drawGame( renderer, &gameState );
    }



}

int main( int /*argc*/, char** /*argv*/ ) {
    Platform* platform = initSDL();
    if ( platform == NULL ) {
        printf( "Failed to initialize\n" );
    } else {
        if ( !loadMedia( platform->renderer ) ) {
            printf( "Failed to load media\n" );
        } else {
            mainLoop(platform->renderer);
        }
    }

    shutdownSDL( platform );
    return 0;
}
