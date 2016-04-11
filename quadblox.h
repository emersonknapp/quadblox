#include <stdint.h>
#include "SDL.h"

typedef uint64_t uint64;
typedef uint16_t uint16;
typedef uint8_t uint8;

const int PLAYAREA_WIDTH = 10;
const int PLAYAREA_HEIGHT = 20;

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;

const int NUM_BLOCKTYPES = 7;
const int NUM_BLOCKSTATES = 4;

const int TURBOFACTOR = 16;

inline SDL_Rect Rect( int x, int y, int w, int h ) {
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
}

struct QuadBlock {
    int x;
    int y;
    int blockType;
    int currentState;
    uint16 state;
    uint8 cols;
};

typedef struct Assets {
    static const int numBlockTextures = 7;
    SDL_Texture* blockTextures[ numBlockTextures ];
    SDL_Texture* backgroundTexture = NULL;
} Assets;

typedef int GameBlocks[PLAYAREA_HEIGHT][PLAYAREA_WIDTH];

typedef struct GameState {
    double timeSinceLastFall = 0;
    double timePerFall = 1.0;
    QuadBlock* currentBlock = NULL;
    int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] = { { -1 } };
    int horizMove = 0;
    int rotate = 0;

    bool wantsToQuit = false;
    bool paused = false;
    bool turbo = false;
    
    //Completed rows animation logic
    double animating = 0;
    int numCompleteRows = 0;
    int completeRows[4] = { 0, 0, 0, 0 };
    bool flashOn = false;
    double flashAccumulator = 0;

    const double flashSpeed = 1.0 / 20.0;
    const double flashTime = 0.5;
} GameState;

void GameUpdateAndRender( SDL_Renderer* renderer, Assets* assets, GameState* gameState, double dtSeconds );

