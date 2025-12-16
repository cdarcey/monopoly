
#include "m_init_game.h"


// ==================== CORE GAME FLOW ==================== //
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

        if (mGame->uCurrentPlayer >= PLAYER_PIECE_TOTAL) 
        {
            mGame->uCurrentPlayer = 0; // Reset to prevent crashes
            return;
        }

        // Check for valid player
        if (!mGame->mGamePlayers[mGame->uCurrentPlayer]->bBankrupt) 
        {
            mGame->mCurrentState = PHASE_PRE_ROLL;
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
        __debugbreak();
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


// ==================== JAIL MECHANICS ==================== //
void
m_jail_subphase(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    // Forced release after 3 turns safety check in case first check is missed somehow
    if(current_player->uJailTurns >= 3) {
        m_forced_release(mGame);
        return;
    }

    printf("\n=== Jail (Turn %d/3) ===\n", current_player->uJailTurns + 1);
    
    // TODO: Non-blocking jail choice system goes here
}

bool
m_attempt_jail_escape(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    if(mGame->mGameDice->dice_one == mGame->mGameDice->dice_two)
    {
        current_player->bInJail = false;
        m_move_player_to(current_player, JAIL_SQUARE);
        mGame->mCurrentState = PHASE_END_TURN;
        return true;
    }
    else
    {
        mGame->mCurrentState = PHASE_END_TURN;
        printf("attempt to escape failed\n");
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
        mPlayerInJail->uPosition = JAIL_SQUARE;
        return true;
    }
    else 
    {
        printf("you do not have a get out of jail free card\n");
        return false;
    }
}


// ==================== PRE-ROLL ACTIONS ==================== //
void
m_pre_roll_phase(mGameData* mGame)
{
    if (!mGame || mGame->mCurrentState != PHASE_PRE_ROLL) 
    {
        __debugbreak();
        return;
    }

    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    if (current_player->bInJail)
    {
        current_player->uJailTurns++;
        if (current_player->uJailTurns >= 3)
        {
            m_forced_release(mGame);
        } 
        else 
        {
            m_jail_subphase(mGame);
        }
        m_next_player_turn(mGame);
        return; // turn ends even if successfully getting out of jail
    }

    // TODO: Non-blocking pre-roll menu system goes here
}


// ==================== POST-ROLL ACTIONS ==================== //
void
m_phase_post_roll(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    if(!mGame)
    {
        DebugBreak();
        return;
    }

    mBoardSquareType current_square = m_get_square_type(mGame->mGamePlayers[mGame->uCurrentPlayer]->uPosition);

    switch(current_square) 
    {
        case GO_SQUARE_TYPE:
        {
            if(mGame->uGameRoundCount < 1)
            {
                DebugBreak();
                return;
            }
            else
            {
                current_player->uMoney += UNIVERSAL_BASIC_INCOME;
                mGame->mCurrentState = PHASE_END_TURN;
                return;
            }
            break;
        }
        case PROPERTY_SQUARE_TYPE:
        {
            mPropertyName current_property_name = m_property_landed_on(current_player->uPosition);
            mProperty* current_property = &mGame->mGameProperties[current_property_name];
            if(m_is_property_owned(current_property))
            {
                if(m_is_property_owner(current_player, current_property))
                {
                    return;
                }
                else if(!m_is_property_owner(current_player, current_property))
                {
                    bool bSetOwned = m_color_set_owned(mGame, current_player, current_property->eColor);
                    m_pay_rent_property(mGame->mGamePlayers[current_property->eOwner], current_player, current_property, bSetOwned);
                }
            }
            // TODO: Non-blocking property purchase/auction system
            break;
        }
        case RAILROAD_SQUARE_TYPE:
        {
            mRailroadName current_railroad_name = m_property_landed_on(current_player->uPosition);
            mRailroad* current_railroad = &mGame->mGameRailroads[current_railroad_name];
            if(m_is_railroad_owned(current_railroad))
            {
                if(m_is_railroad_owner(current_player, current_railroad))
                {
                    return;
                }
                else if(!m_is_railroad_owner)
                {
                    m_pay_rent_railroad(mGame->mGamePlayers[current_railroad->eOwner], current_player, current_railroad);
                }
            }
            // TODO: Non-blocking railroad purchase/auction system
            break;
        }
        case UTILITY_SQUARE_TYPE:
        {
            mUtilityName current_utility_name = m_utility_landed_on(current_player->uPosition);
            mUtility* current_utility = &mGame->mGameUtilities[current_utility_name];
            if(m_is_utility_owned(current_utility))
            {
                if(m_is_utility_owner(current_player, current_utility))
                {
                    return;
                }
                else if(!m_is_utility_owner)
                {
                    m_pay_rent_utility(mGame->mGamePlayers[current_utility->eOwner], current_player, current_utility, mGame->mGameDice);
                }
            }
            // TODO: Non-blocking utility purchase/auction system
            break;
        }
        case INCOME_TAX_SQUARE_TYPE:
        {
            if (!m_can_player_afford(current_player, INCOME_TAX))
            {
                if (!m_attempt_emergency_payment(mGame, current_player, INCOME_TAX))
                {
                    m_trigger_bankruptcy(mGame, mGame->uCurrentPlayer, false, NO_PLAYER);
                    return; 
                }
            }
            else
            {
                current_player->uMoney -= INCOME_TAX;
            }
            // TODO: Non-blocking post-roll menu
            break;
        }
        case LUXURY_TAX_SQUARE_TYPE:
        {
            if (!m_can_player_afford(current_player, LUXURY_TAX))
            {
                if (!m_attempt_emergency_payment(mGame, current_player, LUXURY_TAX)) 
                {
                    m_trigger_bankruptcy(mGame, mGame->uCurrentPlayer, false, NO_PLAYER);
                    return; 
                }
            }
            else
            {
                current_player->uMoney -= LUXURY_TAX;
            }
            // TODO: Non-blocking post-roll menu
            break;
        }
        case CHANCE_CARD_SQUARE_TYPE:
        {
            m_execute_chance_card(mGame);
            // TODO: Non-blocking post-roll menu
            break;
        }
        case COMMUNITY_CHEST_SQUARE_TYPE:
        {
            m_execute_community_chest_card(mGame);
            // TODO: Non-blocking post-roll menu
            break;
        }
        case FREE_PARKING_SQUARE_TYPE:
        {
            // No action needed 
            // TODO: Non-blocking post-roll menu
            break;
        }
        case JAIL_SQUARE_TYPE:
        {
            // Either just visiting or handle jail time
            if(!current_player->bInJail)
            {
                // TODO: Non-blocking post-roll menu
            }
            else
            {
                return;
            }
           break;
        }
        case GO_TO_JAIL_SQUARE_TYPE:
        {
            m_move_player_to(current_player, JAIL_SQUARE); // end turn
            current_player->bInJail = true;
            m_next_player_turn(mGame);
            mGame->mCurrentState = PHASE_PRE_ROLL;
            break;
        }
        default:
        {
            // Error handling
            break;
        }
    }
}


// ==================== FINANCIAL MANAGEMENT ==================== //
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
    if (!mGame || !currentPlayer || uAmount == 0) return false;

    uint32_t uMoneyNeeded = currentPlayer->uMoney + uAmount;

    // sell buildings (hotels -> houses first)
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++) 
    {
        if (mGame->mGameProperties[i].eOwner != currentPlayer->ePlayerTurnPosition) continue;

        // TODO: need to give player the option on which buildings to sell
        while (mGame->mGameProperties[i].uNumberOfHotels > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        {
            currentPlayer->uMoney += (mGame->mGameProperties[i].uHouseCost * 4) / 2; // value of hotel when sold
            mGame->mGameProperties[i].uNumberOfHotels--;
        }

        while (mGame->mGameProperties[i].uNumberOfHouses > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        {
            currentPlayer->uMoney += mGame->mGameProperties[i].uHouseCost / 2; // house sell for half cost to build
            mGame->mGameProperties[i].uNumberOfHouses--;
        }
    }

    // mortgage properties if still short
    for (uint8_t i = 0; i < PROPERTY_TOTAL && currentPlayer->uMoney < uMoneyNeeded; i++) 
    {
        if (mGame->mGameProperties[i].eOwner != currentPlayer->ePlayerTurnPosition || mGame->mGameProperties[i].bMortgaged) continue;

        currentPlayer->uMoney += mGame->mGameProperties[i].uPrice / 2;
        mGame->mGameProperties[i].bMortgaged = true;
    }

    return (currentPlayer->uMoney >= uMoneyNeeded);
}


// ==================== FORCED ACTIONS ==================== //
void
m_forced_release(mGameData* mGame) 
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    // card is lost if not used by this point
    if (current_player->bGetOutOfJailFreeCard)
    {
        current_player->bGetOutOfJailFreeCard = false;
    }

    if (m_can_player_afford(current_player, mGame->uJailFine) == false)
    {
        if (m_attempt_emergency_payment(mGame, current_player, mGame->uJailFine) == false)
        {
            m_trigger_bankruptcy(mGame, mGame->uCurrentPlayer, false, NO_PLAYER);
            return;
        }
    }

    current_player->uMoney -= mGame->uJailFine;
    // TODO: do we want fines to go to free parking for player collection

    current_player->uJailTurns = 0;
    current_player->bInJail = false;
    m_move_player_to(current_player, JAIL_SQUARE); 

}

bool
m_player_has_forced_actions(mGameData* mGame)
{
    // TODO: handle logic here
    mGame->uCurrentPlayer += 0; // dummy codes for warnings 
    return true;
}

void
m_handle_emergency_actions(mGameData* mGame)
{
    // TODO: handle logic here
    mGame->uCurrentPlayer += 0; //dummy code for warnings
}


// ==================== HELPERS ==================== //
mBoardSquareType 
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
