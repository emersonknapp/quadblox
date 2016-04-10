#include <stdint.h>

typedef uint64_t uint64;
typedef uint16_t uint16;
typedef uint8_t uint8;

const int PLAYAREA_WIDTH = 10;
const int PLAYAREA_HEIGHT = 20;

struct QuadBlock {
    int x;
    int y;
    int blockType;
    int currentState;
    uint16 state;
    uint8 cols;
};

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

    const double flashSpeed = 0.05;
    const double flashTime = 0.5;
} GameState;

