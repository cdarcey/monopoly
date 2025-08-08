

#include "m_init_game.h"


void m_game_loop(mGameData* game);


/*
    TODO -
        : auction system
        : trading system
        : ai decsion making 
        : need to add check for if prop has hotel you cannot sell houses - selling hotel will not downgrade to hotels
        : add emergency payment menu so properties can be selected and sale are not forced 
        : function to defrag prop, util, and rail arrays after trade
*/

int
main()
{
    mGameStartSettings mSettings = {
        .uStartingMoney       = 1500,
        .uStartingPlayerCount = 3,
        .uJailFine            = 50
    };

    mGameData* game = m_init_game(mSettings);
    if(!game)
    {
        return 1;
    }

    m_game_loop(game);

    return 0;
}

void
m_game_loop(mGameData* game)
{
    while(game->bRunning == true)
    {
        switch (game->mCurrentState) 
        {
            // Initialization
            case GAME_STARTUP:
            {
                // load game assets etc...
                m_set_player_piece(game, RACE_CAR, PLAYER_ONE);
                m_set_player_piece(game, TOP_HAT, PLAYER_TWO);
                m_set_player_piece(game, THIMBLE, PLAYER_THREE);

                game->mCurrentState = PHASE_PRE_ROLL;
                break;
            }
            case PHASE_PRE_ROLL:
            {
                printf("\nPlayer %d\n", (game->mGamePlayers[game->uCurrentPlayer]->ePlayerTurnPosition + 1));
                m_show_player_status(game->mGamePlayers[game->uCurrentPlayer]);
                m_pre_roll_phase(game);
                break;
            }
            case PHASE_POST_ROLL:
            {
                m_move_player(game->mGamePlayers[game->uCurrentPlayer], game->mGameDice);
                m_show_player_status(game->mGamePlayers[game->uCurrentPlayer]);
                m_phase_post_roll(game);
                break;
            }
            case PHASE_END_TURN:
            {
                m_next_player_turn(game);
                break;
            }
            // Special cases
            case AUCTION_IN_PROGRESS:
            {
                break;
            }
            case TRADE_NEGOTIATION:
            {
                break;
            }
            case GAME_OVER:
            {
                // any other clean up from game needs to go here
                m_free_game_data(game);
                break;
            }
        }
    }
}

