#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include <vector>
#include <queue>
#include <chrono>

#include <stdint.h>

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;

const int PLAYAREA_WIDTH = 10;
const int PLAYAREA_HEIGHT = 20;

const int NUM_BLOCKTYPES = 7;
const int NUM_BLOCKSTATES = 4;

const int TURBOFACTOR = 8;

// Draw each state of each block in a 4x4 grid, represent each grid as a 16-bit int. 1 is square, 0 is no square
const uint16_t BLOCKS[NUM_BLOCKTYPES][NUM_BLOCKSTATES] = {
    {0x08E0, 0x0644, 0x00E2, 0x044C}, //Reverse L
    {0x02E0, 0x0446, 0x00E8, 0x0C44}, //L
    {0x06C0, 0x0462, 0x006C, 0x08C4}, //S
    {0x00C6, 0x04C8, 0x0C60, 0x0264}, //Z
    {0x0660, 0x0660, 0x0660, 0x0660}, //Square
    {0x04C4, 0x04E0, 0x0464, 0x00E4}, //T
    {0x4444, 0x0F00, 0x2222, 0x00F0} //Line
};

// Convenience helper to check if there are any blocks in a column, for spawning fully on the play area.
// 4 columns in 4 states gives one 16-bit int per block type
const uint16_t BLOCKS_COL[NUM_BLOCKTYPES] = {
    0xE6EC, 0xE6EC, 0xE6EC, 0xECE6, 0x6666, 0xCE6E, 0x4F2F
};

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
    void rotate() {
        currentState = ( currentState + 1 ) % NUM_BLOCKSTATES;
        state = BLOCKS[blockType][currentState];
        cols = getCols();
    }

    int left() const {
        int emptyLeft = 0;
        for ( int i = 3; i >= 0; i-- ) {
            if ( ( ( cols >> i ) & 1 ) == 0 ) emptyLeft++;
            else break;
        }
        return emptyLeft;
    }

    int right() const {
        int emptyRight = 0;
        for ( int i = 0; i < 4; i++ ) {
            if ( ( ( cols >> i ) & 1 ) == 0 ) emptyRight++;
            else break;
        }
        return emptyRight;
    }

    int top() const {
        int emptyTop = 0;
        for ( int i = 12; i >= 0; i -= 4 ) {
            if ( ( ( state >> i ) & 0xF ) == 0 ) emptyTop++;
            else break;
        }
        return emptyTop;
    }

    int bottom() const {
        int emptyBottom = 0;
        for ( int i = 0; i < 16; i += 4 ) {
            if ( ( ( state >> i ) & 0xF ) == 0 ) emptyBottom++;
            else break;
        }
        return emptyBottom;
    }

    int row( int i ) const {
       return 0xF & ( state >> ( 4 * ( 4 - i - 1 ) ) ); 
    }

    int square( int r, int c ) const {
        int irow = row( r );
        return 1 & ( irow >> ( 4 - c - 1 ) ); 
    }

    uint8_t getCols() const {
        return ( BLOCKS_COL[blockType] >> ( ( 4 - currentState - 1 ) * 4 ) ) & 0xF;
    }

    int x;
    int y;
    int blockType;
    int currentState;
    uint16_t state;
    uint8_t cols;
};

QuadBlock SpawnQuadBlock() {
    QuadBlock qb;
    qb.currentState = rand() % NUM_BLOCKSTATES;
    qb.blockType = rand() % NUM_BLOCKTYPES;
    qb.state = BLOCKS[qb.blockType][qb.currentState];
    qb.cols = qb.getCols();

    //Fit to top, in case of empty-top
    qb.y = -qb.top();
    qb.x = ( rand() % ( PLAYAREA_WIDTH + qb.left() + qb.right() ) ) - qb.left();
    return qb;
}
typedef struct GameState {
    std::chrono::duration<double> timeSinceLastFall = std::chrono::duration<double>( 0.0 );
    std::chrono::duration<double> timePerFall = std::chrono::duration<double>( 1.0 );
    QuadBlock currentBlock = SpawnQuadBlock();
    std::vector<QuadBlock> blocks;
    bool blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] = { { 0 } };
    int horizMove = 0;
    int rotate = 0;
    bool wantsToQuit = false;
    bool paused = false;
    bool turbo = false;
} GameState;

typedef struct Assets {
    static const int numBlockTextures = 7;
    SDL_Texture* blockTextures[ numBlockTextures ];
    SDL_Texture* backgroundTexture = NULL;
} Assets;

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

typedef struct Platform {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
} Platform;

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
    SDL_DestroyRenderer( platform->renderer );
    platform->renderer = NULL;
    SDL_DestroyWindow( platform->window );
    platform->window = NULL;
    IMG_Quit();
    SDL_Quit();

    delete platform;
}

void finalizeBlock( GameState* gameState, QuadBlock qb ) {
    gameState->blocks.push_back( qb );
    memset( gameState->blockBake, 0, sizeof gameState->blockBake );

    for ( const QuadBlock& b : gameState->blocks ) {
        for ( int i = 0; i < 4; i++ ) {
            for ( int j = 0; j < 4; j++ ) {
                int square = b.square( i, j );
                gameState->blockBake[j][i] |= square; 
            }
        }
    }
}

bool blockHitsBake( const QuadBlock& qb, const bool blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] ) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( qb.square( i, j ) & blockBake[i+qb.x][j+qb.y] ) {
                return true;
            }
        }
    }
    return false;
}

void updateGame( GameState* gameState, std::chrono::duration<double> dt ) {
    if ( gameState->paused ) {
        return;
    }

    gameState->timeSinceLastFall += dt;

    QuadBlock& qb = gameState->currentBlock;

    // Horizontal motion
    qb.x += gameState->horizMove;
    gameState->horizMove = 0;

    for ( int i = 0; i < gameState->rotate; i++ ) {
        qb.rotate();
    }
    gameState->rotate = 0;

    int realBottom = PLAYAREA_HEIGHT - 4 + qb.bottom() + 1;
    // Rotation can put our asses through the floor
    if ( qb.y >= realBottom - 1 ) {
        qb.y = realBottom - 1;
    }

    if ( qb.x <= -qb.left() ) qb.x = -qb.left();
    if ( qb.x >= PLAYAREA_WIDTH - 4 + qb.right() ) qb.x = PLAYAREA_WIDTH - 4 + qb.right();

    // Vertical Motion
    auto timePerFall = gameState->timePerFall;
    if ( gameState->turbo ) {
        timePerFall /= TURBOFACTOR;
    }
    if ( gameState->timeSinceLastFall > timePerFall ) {
        //TODO: this is not fully correct. BUT, we don't want accumulated time from a full-length fall to cause multiple blocks fallage in a row
        gameState->timeSinceLastFall -= gameState->timeSinceLastFall;

        int newY = qb.y + 1;
        if ( newY >= realBottom ) {
            gameState->blocks.push_back( gameState->currentBlock );
            gameState->currentBlock = SpawnQuadBlock();
            gameState->turbo = false;
        } else {
            qb.y = newY;
        }
    }
}

void drawBlock( SDL_Renderer* renderer, const Assets* assets, const QuadBlock& qb, int w, int h ) {
    SDL_Rect blockRect;
    int blocksDrawn = 0;
    for ( int i = 0; i < 4; i++ ) {
        int row = ( qb.state >> ( 4 * ( 4 - i - 1 ) ) ) & 0xF;
        for ( int j = 0; j < 4; j++ ) {
            int square = row >> ( 4 - j - 1 ) & 1;
            if ( square ) {
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
    Platform* platform = initSDL();
    Assets* assets = NULL;
    if ( platform == NULL ) {
        printf( "Failed to initialize\n" );
    } else {
        assets = loadMedia( platform->renderer );
        if ( assets == NULL ) {
            printf( "Failed to load media\n" );
        } else {
            mainLoop( platform->renderer, assets );
        }
    }

    freeMedia( assets );
    shutdownSDL( platform );
    return 0;
}
