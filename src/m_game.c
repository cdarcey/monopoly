

#include <stdio.h>
#include <Windows.h>

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

    mJailAction choice = m_get_player_jail_choice();

    switch(choice) 
    {
        case USE_JAIL_CARD:
        {
            m_use_jail_free_card(current_player);
            break;
        }
        case PAY_JAIL_FINE:
        {
            if(m_can_player_afford(current_player, mGame->uJailFine))
            {
                current_player->uMoney -= mGame->uJailFine;
                current_player->bInJail = false;
                return;
            }
            else
            {
                printf("you can not afford\n");
            }
            mGame->mCurrentState = PHASE_END_TURN;
            break;
        }
        case ROLL_FOR_DOUBLES:
        {
            m_attempt_jail_escape(mGame);
            break;

            default:
            printf("Invalid choice!\n");
            break;
        }
    }
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

mJailAction
m_get_player_jail_choice()
{
    printf("\n=== Jail Options ===\n");
    printf("1: Use Get Out of Jail Free card\n");
    printf("2: Pay $50 fine\n");
    printf("3: Roll for doubles\n");
    printf("> ");

    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n'); // Clear input buffer

    switch(choice)
    {
        case 1: return USE_JAIL_CARD;
        case 2: return PAY_JAIL_FINE;
        case 3: return ROLL_FOR_DOUBLES;
        default: return ROLL_FOR_DOUBLES; // Default to rolling
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

    // proceed to optional pre-roll actions
    m_show_pre_roll_menu(mGame); 
}

void
m_show_pre_roll_menu(mGameData* mGame) 
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    while (mGame->mCurrentState == PHASE_PRE_ROLL) 
    {
        mActions action = m_get_player_pre_roll_choice();

        switch (action) 
        {
            case VIEW_STATUS:
            {
                m_show_player_status(current_player);
                break;
            }
            case VIEW_PROPERTIES:
            {
                m_show_props_owned(current_player);
                break;
            }
            case PROPERTY_MANAGMENT:
            {
                m_show_building_managment_menu(mGame);
                break;
            }
            case PROPOSE_TRADE:
            {
                // handle trade proposals
                break;
            }
            case ROLL_DICE:
            {
                m_roll_dice(mGame->mGameDice);
                mGame->mCurrentState = PHASE_POST_ROLL;
                break;
            }
            default:
            {
                // Handle unexpected actions
                break;
            }
        }
    }
}


mActions
m_get_player_pre_roll_choice()
{
    printf("\n=== Pre-Roll Menu ===\n");
    printf("1: View status\n");
    printf("2: View properties\n");
    printf("3: Property managment\n");
    printf("4: Roll dice\n");
    printf("5: Propose trade\n");
    printf("> ");

    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n'); // Clear input buffer

    switch(choice)
    {
        case 1:  return VIEW_STATUS;
        case 2:  return VIEW_PROPERTIES;
        case 3:  return PROPERTY_MANAGMENT;
        case 4:  return ROLL_DICE;
        case 5:  return PROPOSE_TRADE;
        default: return ROLL_DICE; // Default to roll dice for invalid input
    }
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
            else if(!m_is_property_owned(current_property))
            {
                m_show_post_roll_menu(mGame);
            }
            else if(current_property->eOwner == NO_PLAYER)
            {
                // forced auction for property
            }
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
            else if(!m_is_railroad_owned(current_railroad))
            {
                m_show_post_roll_menu(mGame);
            }
            break;
        }
        case UTILITY_SQUARE_TYPE:
        {
            // Utility-specific logic
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
            m_show_post_roll_menu(mGame); 
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
            m_show_post_roll_menu(mGame);
            break;
        }
        case CHANCE_CARD_SQUARE_TYPE:
        {
            m_execute_chance_card(mGame);
            m_show_post_roll_menu(mGame);
            break;
        }
        case COMMUNITY_CHEST_SQUARE_TYPE:
        {
            m_execute_community_chest_card(mGame);
            m_show_post_roll_menu(mGame);
            break;
        }
        case FREE_PARKING_SQUARE_TYPE:
        {
            // No action needed 
            m_show_post_roll_menu(mGame);
            break;
        }
        case JAIL_SQUARE_TYPE:
        {
            // Either just visiting or handle jail time
            if(!current_player->bInJail)
            {
                m_show_post_roll_menu(mGame);
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

mActions
m_get_player_post_roll_choice()
{
    printf("\n=== Post-Roll Menu ===\n");
    printf("1: View status\n");
    printf("2: View properties\n");
    printf("3: Property managment\n");
    printf("4: Propose trade\n");
    printf("5: Buy square\n");
    printf("6: End turn\n");
    printf("> ");

    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n'); // Clear input buffer

    switch(choice)
    {
        case 1:  return VIEW_STATUS;
        case 2:  return VIEW_PROPERTIES;
        case 3:  return PROPERTY_MANAGMENT;
        case 4:  return PROPOSE_TRADE;
        case 5:  return BUY_SQUARE;
        case 6:  return END_TURN;
        default: return END_TURN; // Default to end turn for invalid input
    }
}

void
m_show_post_roll_menu(mGameData* mGame) 
{
    if(!mGame)
    {
        // handle error
        return;
    }

    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    while (mGame->mCurrentState == PHASE_POST_ROLL) 
    {
        mActions action = m_get_player_post_roll_choice();
        mBoardSquareType current_square_type = m_get_square_type(current_player->uPosition);

        switch(action) 
        {
            case VIEW_STATUS:
            {
                m_show_player_status(current_player);
                break;
            }
            case VIEW_PROPERTIES:
            {
                m_show_props_owned(current_player);
                break;
            }
            case PROPERTY_MANAGMENT:
            {
                m_show_building_managment_menu(mGame);
                break;
            }
            case PROPOSE_TRADE:
            {
                m_enter_trade_phase(mGame);
                break;
            }
            case BUY_SQUARE:
            {
                if(current_square_type == PROPERTY_SQUARE_TYPE)
                {
                    m_buy_property(&mGame->mGameProperties[m_property_landed_on(current_player->uPosition)], current_player);
                }
                else if(current_square_type == RAILROAD_SQUARE_TYPE)
                {
                    m_buy_railroad(&mGame->mGameRailroads[m_railroad_landed_on(current_player->uPosition)], current_player);
                }
                else if(current_square_type == UTILITY_SQUARE_TYPE)
                {
                    m_buy_utility(&mGame->mGameUtilities[m_utility_landed_on(current_player->uPosition)], current_player);
                }
                else
                {
                    printf("Square cannot be purchased\n");
                }
                break;
            }
            case END_TURN:
            {
                mGame->mCurrentState = PHASE_END_TURN;
                break;
            }
            default:
            {
                // Handle unexpected actions
                break;
            }
        }
    }
}

void
m_show_building_managment_menu(mGameData* mGame)
{
    mActions subAction = m_get_building_managment_choice();
    switch(subAction)
    {
        case SELL_HOUSES:
        {
            // Handle selling houses
            break;
        }
        case BUY_HOUSES:
        {
            // Handle buying houses
            break;
        }
        case SELL_HOTELS:
        {
            // Handle selling hotels
            break;
        }
        case BUY_HOTELS:
        {
            // Handle buying hotels
            break;
        }
        case MORTGAGE_PROPERTY:
        {
            break;
        }
        case UNMORTGAGE_PROPERTY:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

mActions
m_get_building_managment_choice()
{
    printf("\n=== Building Management Menu ===\n");
    printf("1: Sell houses\n");
    printf("2: Buy houses\n");
    printf("3: Sell hotels\n");
    printf("4: Buy hotels\n");
    printf("5: Mortgage property\n");
    printf("6: Unmortgage property\n");
    printf("7: Cancel\n");
    printf("> ");

    int choice;
    scanf("%d", &choice);
    while(getchar() != '\n');

    switch(choice) 
    {
        case 1:  return SELL_HOUSES;
        case 2:  return BUY_HOUSES;
        case 3:  return SELL_HOTELS;
        case 4:  return BUY_HOTELS;
        case 5:  return MORTGAGE_PROPERTY;
        case 6:  return UNMORTGAGE_PROPERTY;
        default: return CANCEL;
    }
}


// ==================== SPECIAL PHASES ==================== //
void
m_enter_trade_phase(mGameData* mGame)
{
    // TODO: complete
}

// ==================== END TURN ACTIONS ==================== //

// ==================== FINANCIAL MANAGEMENT ==================== //
bool
m_can_player_afford(mPlayer* mCurrentPlayer, uint8_t uExpense)
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
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++) {
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

// TODO:
bool
m_player_has_forced_actions(mGameData* mGame)
{
    // TODO: handle logic here
    return true;
}

void
m_handle_emergency_actions(mGameData* mGame)
{
    // TODO: handle logic here
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

