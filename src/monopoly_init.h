#ifndef MONOPOLY_INIT_H
#define MONOPOLY_INIT_H

#include "monopoly.h"

// ==================== GAME INITIALIZATION ==================== //

// initialize game 
mGameData* m_init_game(mGameSettings tSettings);

// cleanup game memory
void m_free_game(mGameData* pGame);

#endif // MONOPOLY_INIT_H
