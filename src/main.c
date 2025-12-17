#include "m_init_game.h"
#include <stdio.h>
#include <stdlib.h>

// handles console input for testing
// in a graphics implementation, replace with event polling (GLFW)
void handle_console_input(mGameFlow* pFlow)
{
    // blocking input for console testing
    // real implementation would be event-driven and non-blocking
    if (!pFlow->bInputReceived)
    {
        int input;
        if (scanf("%d", &input) == 1)
        {
            m_set_input_int(pFlow, input);
        }
        // clear input buffer
        while (getchar() != '\n');
    }
}

int main(void)
{
    // initialize game settings
    mGameStartSettings settings = {
        .uStartingMoney = 1500,
        .uStartingPlayerCount = 3,
        .uJailFine = 50
    };

    // create game data
    mGameData* pGame = m_init_game(settings);
    if (!pGame)
    {
        printf("Failed to initialize game\n");
        return 1;
    }

    // set up player pieces
    m_set_player_piece(pGame, RACE_CAR, PLAYER_ONE);
    m_set_player_piece(pGame, TOP_HAT, PLAYER_TWO);
    m_set_player_piece(pGame, THIMBLE, PLAYER_THREE);

    // initialize game flow system
    mGameFlow gameFlow;
    m_init_game_flow(&gameFlow, pGame, NULL); // null for console, window pointer for graphics

    printf("\n=== Monopoly Game Started ===\n");
    printf("Players: %d\n", pGame->uStartingPlayerCount);
    printf("Starting money: $%d\n\n", settings.uStartingMoney);

    // main game loop
    float fDeltaTime = 0.016f; // ~60 fps target

    while (pGame->bRunning)
    {
        // handle input (blocking for tests, event-driven for graphics)
        handle_console_input(&gameFlow);

        // run current phase
        m_run_current_phase(&gameFlow, fDeltaTime);

        // check for game over
        m_game_over_check(pGame);

        // graphics implementation would:
        // - clear screen
        // - render game board
        // - render player positions
        // - render current menu/ui
        // - present/swap buffers
    }

    // game over
    printf("\n=== Game Over ===\n");

    // find winner
    uint32_t highestMoney = 0;
    int iWinner = -1;
    for(uint8_t i = 0; i < pGame->uStartingPlayerCount; i++)
    {
        if(!pGame->mGamePlayers[i]->bBankrupt)
        {
            if(pGame->mGamePlayers[i]->uMoney > highestMoney)
            {
                highestMoney = pGame->mGamePlayers[i]->uMoney;
                iWinner = i;
            }
        }
    }

    if(iWinner >= 0)
    {
        printf("Winner: Player %d with $%d\n", iWinner + 1, highestMoney);
    }

    // cleanup
    m_free_game_data(pGame);

    return 0;
}

