#include <vector>
#include <stdint.h>

typedef uint64_t uint64;

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;

const int PLAYAREA_WIDTH = 10;
const int PLAYAREA_HEIGHT = 20;
const int NUM_BLOCKTYPES = 7;
const int NUM_BLOCKSTATES = 4;

const int TURBOFACTOR = 16;

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
    double timeSinceLastFall = 0;
    double timePerFall = 1.0;
    QuadBlock currentBlock = SpawnQuadBlock();
    int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] = { { -1 } };
    int horizMove = 0;
    int rotate = 0;

    bool wantsToQuit = false;
    bool paused = false;
    bool turbo = false;
    
    //Completed rows animation logic
    int numCompleteRows = 0;
    int completeRows[4] = { 0, 0, 0, 0 };
    bool flashOn = false;
    double flashSpeed = 0.1;
    double flashAccumulator = 0;
} GameState;


void printBake( int bake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] ) {
    for ( int i = 0; i < PLAYAREA_HEIGHT; i++ ) {
        for ( int j = 0; j < PLAYAREA_WIDTH; j++ ) {
            printf( "%02d ", bake[i][j] );    
        }
        printf("\n");
    }
    printf("\n\n");
}
void bakeBlock( int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH], QuadBlock qb ) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( qb.square( i, j ) ) {
                blockBake[ i + qb.y ][ j + qb.x ] = qb.blockType;
            }
        }
    }
}

bool blockHitsBake( const QuadBlock& qb, const int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH], const int verticalLookahead, const int horizLookahead ) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            int row = j + qb.y + verticalLookahead;
            int col = i + qb.x + horizLookahead;
            if ( row >= 0 && row < PLAYAREA_HEIGHT && col >= 0 && col < PLAYAREA_WIDTH && qb.square( j, i ) && ( blockBake[row][col] > -1 ) ) {
                return true;
            }
        }
    }
    return false;
}

void findCompleteRows( const int game[PLAYAREA_HEIGHT][PLAYAREA_WIDTH], int outRows[4], int& outNumRows ) {
    outNumRows = 0;
    for ( int row = 0; row < PLAYAREA_HEIGHT; row++ ) {
        bool full = true;
        for ( int col = 0; col < PLAYAREA_WIDTH; col++ ) {
            full = full && ( game[row][col] != -1 );
        }
        if ( full && outNumRows < 4) {
            outRows[outNumRows] = row;
            outNumRows++;
        }
    }
}

void updateGame( GameState* gameState, double dt ) {
    if ( gameState->paused ) {
        return;
    }

    //Update PlayArea state ( completed row flashing )
    gameState->timeSinceLastFall += dt;
    if ( gameState->numCompleteRows > 0 ) {
        gameState->flashAccumulator += dt;
        while ( gameState->flashAccumulator > gameState->flashSpeed ) {
            gameState->flashOn = !gameState->flashOn;
            gameState->flashAccumulator -= gameState->flashSpeed;
        }
    }


    //Update block state
    QuadBlock& qb = gameState->currentBlock;

    // Horizontal motion
    if ( !blockHitsBake( qb, gameState->blockBake, 0, gameState->horizMove ) ) {
        qb.x += gameState->horizMove;
    }
    gameState->horizMove = 0;

    //Rotation
    //TODO(ebk): rotation can put us through other blocks. 
    //Maybe accumulate all changes into one final state, and then reject or reconcile that state based on collision
    for ( int i = 0; i < gameState->rotate; i++ ) {
        qb.rotate();
    }
    gameState->rotate = 0;

    int realBottom = PLAYAREA_HEIGHT - 4 + qb.bottom() + 1;
    // Rotation can put our asses through the floor
    if ( qb.y >= realBottom - 1 ) {
        qb.y = realBottom - 1;
    }

    //Snap into playarea
    if ( qb.x <= -qb.left() ) qb.x = -qb.left();
    if ( qb.x >= PLAYAREA_WIDTH - 4 + qb.right() ) qb.x = PLAYAREA_WIDTH - 4 + qb.right();

    // Vertical Motion
    double timePerFall = gameState->timePerFall;
    if ( gameState->turbo ) {
        timePerFall /= TURBOFACTOR;
    }
    if ( gameState->timeSinceLastFall > timePerFall ) {
        //TODO: this is not fully correct. BUT, we don't want accumulated time from a full-length fall to cause multiple blocks fallage in a row
        gameState->timeSinceLastFall = 0;

        int newY = qb.y + 1;
        if ( newY >= realBottom || blockHitsBake( qb, gameState->blockBake, 1, 0 ) ) {
            bakeBlock( gameState->blockBake, gameState->currentBlock );
            
            int completeRows[4] = { 0, 0, 0, 0 };
            int completeNumRows = 0;
            findCompleteRows( gameState->blockBake, completeRows, completeNumRows );

            if ( completeNumRows > 0 ) {
                gameState->numCompleteRows = completeNumRows;
                for ( int i = 0; i < completeNumRows; i++ ) {
                    gameState->completeRows[i] = completeRows[i];
                }
            }
            gameState->currentBlock = SpawnQuadBlock();
            gameState->turbo = false;
        } else {
            qb.y = newY;
        }
    }
}



