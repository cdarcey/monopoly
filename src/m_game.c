

#include "m_init_game.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ==================== PHASE SYSTEM CORE ==================== //

void 
m_push_phase(mGameFlow* tFlow, fPhaseFunc tNewPhase, void* pNewData)
{
    if(tFlow->pfCurrentPhase) 
    {
        tFlow->apReturnStack[tFlow->iStackDepth] = tFlow->pfCurrentPhase;
        tFlow->apReturnDataStack[tFlow->iStackDepth] = tFlow->pCurrentPhaseData;
        tFlow->iStackDepth++;
    }

    tFlow->pfCurrentPhase = tNewPhase;
    tFlow->pCurrentPhaseData = pNewData;
    m_clear_input(tFlow);
}

void 
m_pop_phase(mGameFlow* tFlow)
{
    if(tFlow->pCurrentPhaseData) 
    {
        free(tFlow->pCurrentPhaseData);
        tFlow->pCurrentPhaseData = NULL;
    }

    if(tFlow->iStackDepth > 0) 
    {
        tFlow->iStackDepth--;
        tFlow->pfCurrentPhase = tFlow->apReturnStack[tFlow->iStackDepth];
        tFlow->pCurrentPhaseData = tFlow->apReturnDataStack[tFlow->iStackDepth];
    } 
    else 
    {
        tFlow->pfCurrentPhase = NULL;
    }
    
    m_clear_input(tFlow);
}

void 
m_run_current_phase(mGameFlow* tFlow, float fDeltaTime)
{
    if(tFlow->pfCurrentPhase) 
    {
        ePhaseResult result = tFlow->pfCurrentPhase(tFlow->pCurrentPhaseData, fDeltaTime, tFlow);

        if(result == PHASE_COMPLETE) 
        {
            m_pop_phase(tFlow);
        }
    }
}

bool 
m_check_input_int(int* pOut, void* tContextWindow)
{
    (GLFWwindow*)tContextWindow;
    // Check for number keys 0-9
    for(int i = GLFW_KEY_0; i <= GLFW_KEY_9; i++)
    {
        if(glfwGetKey(tContextWindow, i) == GLFW_PRESS)
        {
            *pOut = i - GLFW_KEY_0;
            return true;
        }
    }
    
    // Check for numpad 0-9
    for(int i = GLFW_KEY_KP_0; i <= GLFW_KEY_KP_9; i++)
    {
        if(glfwGetKey(tContextWindow, i) == GLFW_PRESS)
        {
            *pOut = i - GLFW_KEY_KP_0;
            return true;
        }
    }

    return false;
}

bool 
m_check_input_yes_no(bool* pOut, void* tContextWindow)
{
    (GLFWwindow*)tContextWindow;
    if(glfwGetKey(tContextWindow, GLFW_KEY_Y) == GLFW_PRESS)
    {
        *pOut = true;
        return true;
    }
    if(glfwGetKey(tContextWindow, GLFW_KEY_N) == GLFW_PRESS)
    {
        *pOut = false;
        return true;
    }
    return false;
}

bool
m_check_input_space(void* tContextWindow)
{
    (GLFWwindow*)tContextWindow;
    return glfwGetKey(tContextWindow, GLFW_KEY_SPACE) == GLFW_PRESS;
}

bool
m_check_input_enter(void* tContextWindow)
{
    (GLFWwindow*)tContextWindow;
    return glfwGetKey(tContextWindow, GLFW_KEY_ENTER) == GLFW_PRESS;
}

void 
m_clear_input(mGameFlow* tFlow)
{
    tFlow->bInputReceived = false;
    tFlow->iInputValue = 0;
    tFlow->szInputString[0] = '\0';
}

// ==================== PHASE IMPLEMENTATIONS ==================== //

ePhaseResult 
m_phase_pre_roll(void* pData, float fDeltaTime, mGameFlow* tGameFlow)
{
    mPreRollData* ptData = (mPreRollData*)pData;
    (void)fDeltaTime;

    if(ptData->pPlayer->bInJail)
    {
        // TODO: add once jail phase is built
    }

    // first call - print info
    if(!ptData->bWaitingForRoll) 
    {
        m_show_player_status(ptData->pPlayer);
        ptData->bWaitingForRoll = true;
    }


    // TODO: show menu
    // TODO: input handling 

    return PHASE_RUNNING;
}

ePhaseResult 
m_phase_post_roll(void* pData, float fDeltaTime, mGameFlow* tGameFlow)
{
    mPostRollData* ptData = (mPostRollData*)pData;
    mPlayer* current_player = ptData->pPlayer;
    (void)fDeltaTime;

    eBoardSquareType current_square = m_get_square_type(current_player->uPosition);

    switch(current_square) 
    {
        case GO_SQUARE_TYPE:
        {
            if(tGameFlow->pGameData->uGameRoundCount < 1)
            {
                DebugBreak();
                return PHASE_RUNNING;
            }
            
            current_player->uMoney += 200; // ubi
            printf("Landed on GO! Collected $200\n");
            
            // TODO: show menu
            // TODO: get input
            return PHASE_COMPLETE;
        }
        
        case PROPERTY_SQUARE_TYPE:
        {
            printf("Landed on a property!\n");
            mPropertyName current_property_name = m_property_landed_on(current_player->uPosition);
            mProperty*    current_property      = &tGameFlow->pGameData->mGameProperties[current_property_name];

            if(m_is_property_owned(current_property))
            {
                if(m_is_property_owner(current_player, current_property))
                {
                    printf("You own this property.\n");
                    // TODO: show menu
                    // TODO: get input
                    return PHASE_RUNNING;
                }
                else
                {
                    // Pay rent to owner
                    bool bSetOwned = m_color_set_owned(tGameFlow->pGameData, current_player, current_property->eColor);
                    m_pay_rent_property(tGameFlow->pGameData->mGamePlayers[current_property->eOwner], current_player, current_property, bSetOwned);
                    
                    // TODO: Check if can afford, spawn emergency payment if not
                    return PHASE_RUNNING;
                }
            }
            else
            {
                // Property unowned
                // TODO: show menu (buy/pass)
                // if player does not buy then enter auction
                // For now, just go to auction
                // m_enter_auction_prop(tGameFlow->pGameData, current_property);
                return PHASE_RUNNING;
            }
        }
        
        case RAILROAD_SQUARE_TYPE:
        {
            printf("Landed on a railroad!\n");
            mRailroadName current_railroad_name = m_railroad_landed_on(current_player->uPosition);
            mRailroad*    current_railroad      = &tGameFlow->pGameData->mGameRailroads[current_railroad_name];

            if(m_is_railroad_owned(current_railroad))
            {
                if(m_is_railroad_owner(current_player, current_railroad))
                {
                    printf("You own this railroad.\n");
                    // TODO: show menu
                    // TODO: get input
                    return PHASE_RUNNING;
                }
                else
                {
                    // Pay rent to owner
                    m_pay_rent_railroad(tGameFlow->pGameData->mGamePlayers[current_railroad->eOwner], current_player, current_railroad);
                    
                    // TODO: handle emergency/ forced payments 

                    return PHASE_RUNNING;
                }
            }
            else
            {
                // Railroad unowned
                // TODO: show menu 
                // if player does not buy then enter auction
                // m_enter_auction_rail(tGameFlow->pGameData, current_railroad);
                return PHASE_RUNNING;
            }
        }
        
        case UTILITY_SQUARE_TYPE:
        {
            printf("Landed on a utility!\n");
            mUtilityName current_utility_name = m_utility_landed_on(current_player->uPosition);
            mUtility*    current_utility      = &tGameFlow->pGameData->mGameUtilities[current_utility_name];

            if(m_is_utility_owned(current_utility))
            {
                if(m_is_utility_owner(current_player, current_utility))
                {
                    printf("You own this utility.\n");
                    // TODO: show menu
                    return PHASE_RUNNING;
                }
                else
                {
                    // Pay rent to owner
                    m_pay_rent_utility(tGameFlow->pGameData->mGamePlayers[current_utility->eOwner], current_player, current_utility, tGameFlow->pGameData->mGameDice);
                    
                    // TODO: Check if can afford, spawn emergency payment if not
                    return PHASE_RUNNING;
                }
            }
            else
            {
                // Utility unowned
                // TODO: show menu 
                // if player does not buy then enter auction
                // For now, just go to auction
                // m_enter_auction_util(tGameFlow->pGameData, current_utility);
                return PHASE_RUNNING;
            }
        }
        
        case INCOME_TAX_SQUARE_TYPE:
        {
            printf("Income Tax: $200\n");
            
            if(!m_can_player_afford(current_player, 200))
            {
                if(!m_attempt_emergency_payment(tGameFlow->pGameData, current_player, 200))
                {
                    // TODO: m_trigger_bankruptcy(tGameFlow->pGameData, tGameFlow->pGameData->uCurrentPlayer, false, NO_PLAYER);
                    return PHASE_COMPLETE;
                }
            }
            else
            {
                current_player->uMoney -= 200;
            }
            
            return PHASE_COMPLETE;
        }
        
        case LUXURY_TAX_SQUARE_TYPE:
        {
            printf("Luxury Tax: $100\n");
            
            if (!m_can_player_afford(current_player, 100))
            {
                if (!m_attempt_emergency_payment(tGameFlow->pGameData, current_player, 100))
                {
                    // TODO: m_trigger_bankruptcy(tGameFlow->pGameData, tGameFlow->pGameData->uCurrentPlayer, false, NO_PLAYER);
                    return PHASE_COMPLETE;
                }
            }
            else
            {
                current_player->uMoney -= 100;
            }
            
            return PHASE_COMPLETE;
        }
        
        case CHANCE_CARD_SQUARE_TYPE:
        {
            printf("Drew a Chance card!\n");
            m_execute_chance_card(tGameFlow->pGameData);
            return PHASE_COMPLETE;
        }
        
        case COMMUNITY_CHEST_SQUARE_TYPE:
        {
            printf("Drew a Community Chest card!\n");
            m_execute_community_chest_card(tGameFlow->pGameData);
            return PHASE_COMPLETE;
        }
        
        case FREE_PARKING_SQUARE_TYPE:
        {
            printf("Free Parking - Just resting!\n");
            return PHASE_COMPLETE;
        }
        
        case JAIL_SQUARE_TYPE:
        {
            if(!current_player->bInJail)
            {
                printf("Just visiting jail.\n");
            }
            return PHASE_COMPLETE;
        }
        
        case GO_TO_JAIL_SQUARE_TYPE:
        {
            printf("Go to Jail!\n");
            m_move_player_to(current_player, JAIL_SQUARE);
            current_player->bInJail = true;
            return PHASE_COMPLETE;
        }
        
        default:
        {
            printf("ERROR: Unknown square type!\n");
            return PHASE_COMPLETE;
        }
    }
}

// ==================== CORE GAME LOGIC ==================== //

void
m_next_player_turn(mGameData* mGame)
{
    if (mGame->mGamePlayers[mGame->uCurrentPlayer]->bBankrupt)
    {
        if(mGame->mGamePlayers[mGame->uCurrentPlayer]->uMoney != 0)
        {
            DebugBreak();
        }
        mGame->uActivePlayers--;
    }

    const uint8_t uMaxAttempts = mGame->uStartingPlayerCount; 
    uint8_t uAttempts = 0;

    while (uAttempts < uMaxAttempts)
    {
        // Move to next player (with wrap-around)
        mGame->uCurrentPlayer = (mGame->uCurrentPlayer + 1) % mGame->uStartingPlayerCount;

        if (mGame->uCurrentPlayer == 0) 
        {
            mGame->uGameRoundCount++;
        }

        if (mGame->uCurrentPlayer >= PLAYER_PIECE_TOTAL) // PLAYER_PIECE_TOTAL is total max players game supports
        {
            mGame->uCurrentPlayer = 0;
            return;
        }

        // Check for valid player
        if (!mGame->mGamePlayers[mGame->uCurrentPlayer]->bBankrupt) 
        {
            return; // Found valid player
        }

        uAttempts++;
    }

    // If all players are bankrupt
    mGame->bRunning = false;
}

void
m_game_over_check(mGameData* mGame)
{
    if(!mGame)
    {
        DebugBreak();
        return;
    }

    if(mGame->uActivePlayers >= 2)
    {
        mGame->bRunning = true;
    }
    else
    {
        mGame->bRunning = false;
    }
}

void
m_roll_dice(mDice* game_dice)
{
    game_dice->dice_one = (rand() % 6) + 1;
    game_dice->dice_two = (rand() % 6) + 1;
}

bool
m_attempt_jail_escape(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    if(mGame->mGameDice->dice_one == mGame->mGameDice->dice_two)
    {
        current_player->bInJail = false;
        current_player->uJailTurns = 0;
        m_move_player_to(current_player, JAIL_SQUARE);
        return true;
    }
    else
    {
        printf("Attempt to escape failed\n");
        return false;
    }
}

bool
m_use_jail_free_card(mPlayer* mPlayerInJail)
{
    if(mPlayerInJail->bGetOutOfJailFreeCard == true)
    {
        mPlayerInJail->bGetOutOfJailFreeCard = false;
        mPlayerInJail->bInJail = false;
        mPlayerInJail->uJailTurns = 0;
        mPlayerInJail->uPosition = JAIL_SQUARE;
        return true;
    }
    else 
    {
        printf("You do not have a get out of jail free card\n");
        return false;
    }
}

bool
m_can_player_afford(mPlayer* mCurrentPlayer, uint32_t uExpense)
{
    if(mCurrentPlayer->uMoney >= uExpense)
    {
        return true;
    }
    return false;
}

bool
m_attempt_emergency_payment(mGameData* mGame, mPlayer* currentPlayer, uint32_t uAmount) 
{
    if (!mGame || !currentPlayer || uAmount == 0) 
        return false;

    uint32_t uMoneyNeeded = currentPlayer->uMoney + uAmount;

    // Sell buildings (hotels -> houses first)
    // TODO: This should probably be a phase where player chooses
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)  // TODO: Replace 28 with PROPERTY_TOTAL
    {
        // TODO: Need property access - this assumes mGame->mGameProperties exists
        // if (mGame->mGameProperties[i].eOwner != currentPlayer->ePlayerTurnPosition) 
        //     continue;

        // while (mGame->mGameProperties[i].uNumberOfHotels > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        // {
        //     currentPlayer->uMoney += (mGame->mGameProperties[i].uHouseCost * 4) / 2;
        //     mGame->mGameProperties[i].uNumberOfHotels--;
        // }

        // while (mGame->mGameProperties[i].uNumberOfHouses > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        // {
        //     currentPlayer->uMoney += mGame->mGameProperties[i].uHouseCost / 2;
        //     mGame->mGameProperties[i].uNumberOfHouses--;
        // }
    }

    // Mortgage properties if still short
    // TODO: Similar implementation needed

    return (currentPlayer->uMoney >= uMoneyNeeded);
}

eBoardSquareType 
m_get_square_type(uint32_t uPlayerPosition)
{
    switch(uPlayerPosition) 
    {
        case GO_SQUARE:
            return GO_SQUARE_TYPE;

        case MEDITERRANEAN_AVENUE_SQUARE:
        case BALTIC_AVENUE_SQUARE:
        case ORIENTAL_AVENUE_SQUARE:
        case VERMONT_AVENUE_SQUARE:
        case CONNECTICUT_AVENUE_SQUARE:
        case ST_CHARLES_PLACE_SQUARE:
        case STATES_AVENUE_SQUARE:
        case VIRGINIA_AVENUE_SQUARE:
        case ST_JAMES_PLACE_SQUARE:
        case TENNESSEE_AVENUE_SQUARE:
        case NEW_YORK_AVENUE_SQUARE:
        case KENTUCKY_AVENUE_SQUARE:
        case INDIANA_AVENUE_SQUARE:
        case ILLINOIS_AVENUE_SQUARE:
        case ATLANTIC_AVENUE_SQUARE:
        case VENTNOR_AVENUE_SQUARE:
        case MARVIN_GARDENS_SQUARE:
        case PACIFIC_AVENUE_SQUARE:
        case NORTH_CAROLINA_AVENUE_SQUARE:
        case PENNSYLVANIA_AVENUE_SQUARE:
        case PARK_PLACE_SQUARE:
        case BOARDWALK_SQUARE:
            return PROPERTY_SQUARE_TYPE;

        case READING_RAILROAD_SQUARE:
        case PENNSYLVANIA_RAILROAD_SQUARE:
        case B_AND_O_RAILROAD_SQUARE:
        case SHORT_LINE_RAILROAD_SQUARE:
            return RAILROAD_SQUARE_TYPE;

        case ELECTRIC_COMPANY_SQUARE:
        case WATER_WORKS_SQUARE:
            return UTILITY_SQUARE_TYPE;

        case INCOME_TAX_SQUARE:
            return INCOME_TAX_SQUARE_TYPE;
        case LUXURY_TAX_SQUARE:
            return LUXURY_TAX_SQUARE_TYPE;

        case CHANCE_SQUARE_1:
        case CHANCE_SQUARE_2:
        case CHANCE_SQUARE_3:
            return CHANCE_CARD_SQUARE_TYPE;

        case COMMUNITY_CHEST_SQUARE_1:
        case COMMUNITY_CHEST_SQUARE_2:
        case COMMUNITY_CHEST_SQUARE_3:
            return COMMUNITY_CHEST_SQUARE_TYPE;

        case FREE_PARKING_SQUARE:
            return FREE_PARKING_SQUARE_TYPE;

        case JAIL_SQUARE:
            return JAIL_SQUARE_TYPE;

        case GO_TO_JAIL_SQUARE:
            return GO_TO_JAIL_SQUARE_TYPE;

        default:
            DebugBreak();
            return GO_SQUARE_TYPE; // Default fallback
    }
}

uint8_t 
m_get_empty_prop_owned_slot(mPlayer* mPlayer)
{
    uint8_t uPropOwnedSlot = 0;
    for(uPropOwnedSlot = 0; uPropOwnedSlot < PROP_OWNED_WITH_BUFFER; uPropOwnedSlot++)
    {
        if(mPlayer->ePropertyOwned[uPropOwnedSlot] == NO_PROPERTY)
        {
            break;
        }
    }
    return uPropOwnedSlot;
}

uint8_t 
m_get_empty_rail_owned_slot(mPlayer* mPlayer)
{
    uint8_t uRailOwnedSlot = 0;
    for(uRailOwnedSlot = 0; uRailOwnedSlot < RAIL_OWNED_WITH_BUFFER; uRailOwnedSlot++)
    {
        if(mPlayer->eRailroadOwned[uRailOwnedSlot] == NO_RAILROAD)
        {
            break;
        }
    }
    return uRailOwnedSlot;
}

uint8_t 
m_get_empty_util_owned_slot(mPlayer* mPlayer)
{
    uint8_t uUtilOwnedSlot = 0;
    for(uUtilOwnedSlot = 0; uUtilOwnedSlot < UTIL_OWNED_WITH_BUFFER; uUtilOwnedSlot++)
    {
        if(mPlayer->eUtilityOwned[uUtilOwnedSlot] == NO_UTILITY)
        {
            break;
        }
    }
    return uUtilOwnedSlot;
}

void
m_forced_release(mGameData* mGame) 
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    // Card is lost if not used by this point
    if (current_player->bGetOutOfJailFreeCard)
    {
        current_player->bGetOutOfJailFreeCard = false;
    }
    
    if (!m_can_player_afford(current_player, mGame->uJailFine))
    {
        if (!m_attempt_emergency_payment(mGame, current_player, mGame->uJailFine))
        {
            m_trigger_bankruptcy(mGame, mGame->uCurrentPlayer, false, NO_PLAYER);
            return;
        }
    }

    current_player->uMoney    -= mGame->uJailFine;
    current_player->uJailTurns = 0;
    current_player->bInJail    = false;
    m_move_player_to(current_player, JAIL_SQUARE);
}

bool
m_player_has_forced_actions(mGameData* mGame)
{
    // TODO: Implement logic
    (void)mGame;
    return false;
}

void
m_handle_emergency_actions(mGameData* mGame)
{
    // TODO: Implement logic
    (void)mGame;
}