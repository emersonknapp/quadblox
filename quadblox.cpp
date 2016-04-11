#include <cstdlib>
#include <cstdio>
#include "stdlib.h"
#include "SDL.h"
#include "quadblox.h"

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

uint8 GetCols(int blockType, int stateIdx) {
    return ( BLOCKS_COL[blockType] >> ( ( 4 - stateIdx - 1 ) * 4 ) ) & 0xF;
}
void RotateBlock(QuadBlock& qb, int numTimes) {
    qb.currentState = ( qb.currentState + numTimes ) % NUM_BLOCKSTATES;
    qb.state = BLOCKS[qb.blockType][qb.currentState];
    qb.cols = GetCols(qb.blockType, qb.currentState);
}

int Left(const QuadBlock& qb) {
    int emptyLeft = 0;
    for ( int i = 3; i >= 0; i-- ) {
        if ( ( ( qb.cols >> i ) & 1 ) == 0 ) emptyLeft++;
        else break;
    }
    return emptyLeft;
}

int Right(const QuadBlock& qb) {
    int emptyRight = 0;
    for ( int i = 0; i < 4; i++ ) {
        if ( ( ( qb.cols >> i ) & 1 ) == 0 ) emptyRight++;
        else break;
    }
    return emptyRight;
}

int Top(const QuadBlock& qb) {
    int emptyTop = 0;
    for ( int i = 12; i >= 0; i -= 4 ) {
        if ( ( ( qb.state >> i ) & 0xF ) == 0 ) emptyTop++;
        else break;
    }
    return emptyTop;
}

int Bottom(const QuadBlock& qb) {
    int emptyBottom = 0;
    for ( int i = 0; i < 16; i += 4 ) {
        if ( ( ( qb.state >> i ) & 0xF ) == 0 ) emptyBottom++;
        else break;
    }
    return emptyBottom;
}

int Row(const QuadBlock& qb, int i) {
    return 0xF & ( qb.state >> ( 4 * ( 4 - i - 1 ) ) );  
}

int Cell(const QuadBlock& qb, int r, int c) {
    int irow = Row( qb, r );
    return 1 & ( irow >> ( 4 - c - 1 ) ); 
}

uint8 Cols(const QuadBlock& qb) {
    return ( BLOCKS_COL[qb.blockType] >> ( ( 4 - qb.currentState - 1 ) * 4 ) ) & 0xF;
}

QuadBlock* SpawnQuadBlock() {
    QuadBlock* pqb = new QuadBlock();
    QuadBlock& qb = *pqb;
    qb.currentState = rand() % NUM_BLOCKSTATES;
    qb.blockType = rand() % NUM_BLOCKTYPES;
    qb.state = BLOCKS[qb.blockType][qb.currentState];
    qb.cols = Cols(qb);

    //Fit to top, in case of empty-top
    qb.y = -Top(qb);
    qb.x = ( rand() % ( PLAYAREA_WIDTH + Left(qb) + Right(qb) ) ) - Left(qb);
    return pqb;
}


void printBake( int bake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH] ) {
    for ( int i = 0; i < PLAYAREA_HEIGHT; i++ ) {
        for ( int j = 0; j < PLAYAREA_WIDTH; j++ ) {
            printf( "%02d ", bake[i][j] );    
        }
        printf("\n");
    }
    printf("\n\n");
}
void bakeBlock( int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH], const QuadBlock* qb ) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( Cell(*qb, i, j ) ) {
                blockBake[ i + qb->y ][ j + qb->x ] = qb->blockType;
            }
        }
    }
}

bool blockHitsBake( const QuadBlock& qb, const int blockBake[PLAYAREA_HEIGHT][PLAYAREA_WIDTH], const int verticalLookahead, const int horizLookahead ) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            int row = j + qb.y + verticalLookahead;
            int col = i + qb.x + horizLookahead;
            if ( row >= 0 && row < PLAYAREA_HEIGHT && col >= 0 && col < PLAYAREA_WIDTH && Cell(qb, j, i ) && ( blockBake[row][col] > -1 ) ) {
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

void ClearRow(GameBlocks blocks, int row) {
    for ( int i = 0; i < PLAYAREA_WIDTH; i++ ) {
        blocks[row][i] = -1;
    }
}

void CopyRow(GameBlocks blocks, int from, int to) {
    for ( int i = 0; i < PLAYAREA_WIDTH; i++ ) {
        blocks[to][i] = blocks[from][i];
    }
}

int compare_ints(const void* a, const void* b) {
    const int* ia = (const int *)a;
    const int* ib = (const int *)b;
    return *ia - *ib;
}

void ClearCompletedRows(GameState* gameState) {
    qsort(gameState->completeRows, (size_t)gameState->numCompleteRows, sizeof(int), compare_ints);

    for ( int rowClearedIdx = 0; rowClearedIdx < gameState->numCompleteRows; rowClearedIdx++ ) {
        int completedRow = gameState->completeRows[rowClearedIdx];
        for ( int i = completedRow - 1; i >= 0; i--) {
            CopyRow(gameState->blockBake, i, i+1);
            ClearRow(gameState->blockBake, i);
        }
    }
}

void drawBlock( SDL_Renderer* renderer, const Assets* assets, const QuadBlock& qb, int w, int h ) {
    SDL_Rect blockRect;
    int blocksDrawn = 0;
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( Cell(qb, i, j ) ) {
                blockRect = Rect( ( qb.x + j ) * w, ( qb.y + i ) * h, w, h );
                SDL_RenderCopy( renderer, assets->textures[AssetTypeForBlockType(qb.blockType)], NULL, &blockRect );
                blocksDrawn++;
            }
        }
    }
}

void drawGame( SDL_Renderer* renderer, const Assets* assets, const GameState* gameState ) {
    // Game area
    int gameAreaHeight = int(SCREEN_HEIGHT * 0.8);
    gameAreaHeight = gameAreaHeight - ( gameAreaHeight % PLAYAREA_HEIGHT );
    int gameAreaWidth = gameAreaHeight / 2; 
    SDL_Rect gameAreaViewport = Rect( int(0.1 * SCREEN_HEIGHT), int(0.1 * SCREEN_WIDTH), gameAreaWidth, gameAreaHeight );
    SDL_RenderSetViewport( renderer, &gameAreaViewport );
    SDL_SetRenderDrawColor( renderer, 0xCC, 0xCC, 0xCC, 0xFF );
    SDL_Rect gameAreaBackground = Rect( 0, 0, gameAreaWidth, gameAreaHeight );
    SDL_RenderCopy( renderer, assets->textures[AssetType::BACKGROUND], NULL, &gameAreaBackground );

    // Blocks
    int blockWidth = gameAreaWidth / PLAYAREA_WIDTH;
    int blockHeight = gameAreaHeight / PLAYAREA_HEIGHT;
    if ( gameState->currentBlock != NULL ) {
        drawBlock( renderer, assets, *gameState->currentBlock, blockWidth, blockHeight );
    }

    SDL_Rect blockRect;
    for ( int row = 0; row < PLAYAREA_HEIGHT; row++ ) {
        for ( int col = 0; col < PLAYAREA_WIDTH; col++ ) {
            int blockType = gameState->blockBake[row][col];
            if ( blockType > -1 ) {
                bool flash = false;
                if ( gameState->flashOn && gameState->numCompleteRows > 0 ) {
                    for ( int completeRowIdx = 0; completeRowIdx < gameState->numCompleteRows; completeRowIdx++ ) {
                        flash |= gameState->completeRows[completeRowIdx] == row;
                    }
                }
                
                blockRect = Rect(col*blockWidth, row*blockHeight, blockWidth, blockHeight );
                SDL_RenderCopy( renderer, assets->textures[AssetTypeForBlockType(blockType)], NULL, &blockRect );
                if ( flash ) {
                    SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0x88 );
                    SDL_RenderFillRect( renderer, &blockRect );
                }
            }
        }
    }

}


void updateGame( GameState* gameState, double dt ) {
    if ( gameState->paused ) {
        return;
    }

    //Update PlayArea state ( completed row flashing )
    gameState->timeSinceLastFall += dt;
    if ( gameState->animating > 0 ) {
        gameState->flashAccumulator += dt;
        gameState->animating -= dt;
        while ( gameState->flashAccumulator > gameState->flashSpeed ) {
            gameState->flashOn = !gameState->flashOn;
            gameState->flashAccumulator -= gameState->flashSpeed;
        }
        if ( gameState-> animating <= 0 ) {
            gameState->animating = 0;
            gameState->flashOn = false;
            gameState->flashAccumulator = 0;
        } else {
            return;
        }
    }

    if ( gameState->numCompleteRows > 0 ) {
        ClearCompletedRows(gameState);
        gameState->numCompleteRows = 0;
    }

    if ( gameState->currentBlock == NULL ) {
        gameState->currentBlock = SpawnQuadBlock();
    }

    //Update block state
    QuadBlock& qb = *gameState->currentBlock;

    // Horizontal motion
    if ( !blockHitsBake( qb, gameState->blockBake, 0, gameState->horizMove ) ) {
        qb.x += gameState->horizMove;
    }
    gameState->horizMove = 0;

    //Rotation
    //TODO(ebk): rotation can put us through other blocks. 
    //Maybe accumulate all changes into one final state, and then reject or reconcile that state based on collision
    RotateBlock(qb, gameState->rotate);
    gameState->rotate = 0;

    int realBottom = PLAYAREA_HEIGHT - 4 + Bottom(qb) + 1;
    // Rotation can put our asses through the floor
    if ( qb.y >= realBottom - 1 ) {
        qb.y = realBottom - 1;
    }

    //Snap into playarea
    if ( qb.x <= -Left(qb) ) qb.x = -Left(qb);
    if ( qb.x >= PLAYAREA_WIDTH - 4 + Right(qb)) qb.x = PLAYAREA_WIDTH - 4 + Right(qb);

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
            delete gameState->currentBlock;
            gameState->currentBlock = NULL;

            findCompleteRows( gameState->blockBake, gameState->completeRows, gameState->numCompleteRows );
            if ( gameState->numCompleteRows > 0 ) {
                gameState->animating = gameState->flashTime;
            }

            gameState->turbo = false;
        } else {
            qb.y = newY;
        }
    }
}



void GameUpdateAndRender( SDL_Renderer* renderer, Assets* assets, GameState* gameState, double dtSeconds) {
    static double accumulator = 0;
    static const double kUpdateTimeSeconds = 0.01;
    accumulator += dtSeconds;

    while ( accumulator >= kUpdateTimeSeconds ) {
        updateGame( gameState, kUpdateTimeSeconds );
        accumulator -= kUpdateTimeSeconds;
    }

    drawGame( renderer, assets, gameState );
}


