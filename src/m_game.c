#include "m_init_game.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ==================== PHASE SYSTEM CORE ==================== //

void 
m_init_game_flow(mGameFlow* pFlow, mGameData* pGame, void* pInputContext)
{
    if(!pFlow || !pGame) return;

    memset(pFlow, 0, sizeof(mGameFlow));
    pFlow->pGameData     = pGame;
    pFlow->pInputContext = pInputContext;
    pFlow->iStackDepth   = 0;

    // start with pre-roll phase
    mPreRollData* pPreRoll   = malloc(sizeof(mPreRollData));
    pPreRoll->pPlayer        = pGame->mGamePlayers[pGame->uCurrentPlayer];
    pPreRoll->bShowedMenu    = false;
    pPreRoll->iMenuSelection = -1;

    pFlow->pfCurrentPhase = m_phase_pre_roll;
    pFlow->pCurrentPhaseData = pPreRoll;
}

void 
m_push_phase(mGameFlow* pFlow, fPhaseFunc pfNewPhase, void* pNewData)
{
    if(!pFlow || pFlow->iStackDepth >= 16) return;

    // save current phase to stack
    pFlow->apReturnStack[pFlow->iStackDepth]     = pFlow->pfCurrentPhase;
    pFlow->apReturnDataStack[pFlow->iStackDepth] = pFlow->pCurrentPhaseData;
    pFlow->iStackDepth++;

    // set new phase
    pFlow->pfCurrentPhase    = pfNewPhase;
    pFlow->pCurrentPhaseData = pNewData;
    m_clear_input(pFlow);
}

void 
m_pop_phase(mGameFlow* pFlow)
{
    if(!pFlow || pFlow->iStackDepth <= 0) return;

    // free current phase data
    if(pFlow->pCurrentPhaseData)
    {
        free(pFlow->pCurrentPhaseData);
        pFlow->pCurrentPhaseData = NULL;
    }

    // Restore previous phase
    pFlow->iStackDepth--;
    pFlow->pfCurrentPhase    = pFlow->apReturnStack[pFlow->iStackDepth];
    pFlow->pCurrentPhaseData = pFlow->apReturnDataStack[pFlow->iStackDepth];
    m_clear_input(pFlow);
}

void 
m_run_current_phase(mGameFlow* pFlow, float fDeltaTime)
{
    if(!pFlow || !pFlow->pfCurrentPhase) return;

    pFlow->fAccumulatedTime += fDeltaTime;

    ePhaseResult tResult = pFlow->pfCurrentPhase(pFlow->pCurrentPhaseData, fDeltaTime, pFlow);

    if(tResult == PHASE_COMPLETE)
    {
        m_pop_phase(pFlow);
    }
}

// ==================== INPUT SYSTEM ==================== //

void 
m_set_input_int(mGameFlow* pFlow, int iValue)
{
    if(!pFlow) return;
    pFlow->iInputValue    = iValue;
    pFlow->bInputReceived = true;
}

void 
m_set_input_string(mGameFlow* pFlow, const char* szValue)
{
    if(!pFlow || !szValue) return;
    strncpy(pFlow->szInputString, szValue, sizeof(pFlow->szInputString) - 1); // leave room for null terminator
    pFlow->szInputString[sizeof(pFlow->szInputString) - 1] = '\0'; // add back in case input was too long and cut off
    pFlow->bInputReceived = true;
}

void 
m_clear_input(mGameFlow* pFlow)
{
    if(!pFlow) return;
    pFlow->bInputReceived   = false;
    pFlow->iInputValue      = 0;
    pFlow->szInputString[0] = '\0';
}

bool 
m_is_waiting_input(mGameFlow* pFlow)
{
    return pFlow && !pFlow->bInputReceived;
}

// ==================== PHASE: PRE-ROLL ==================== //

ePhaseResult 
m_phase_pre_roll(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    mPreRollData* pPreRoll = (mPreRollData*)pData;
    mGameData* pGame       = pFlow->pGameData;
    mPlayer* pPlayer       = pGame->mGamePlayers[pGame->uCurrentPlayer];

    // handle jail if player is in jail
    if(pPlayer->bInJail)
    {
        pPlayer->uJailTurns++;
        if(pPlayer->uJailTurns >= 3) // release forced after 3 turns
        {
            m_forced_release(pGame);
            m_next_player_turn(pGame);
            return PHASE_COMPLETE; // turn ends on release
        }

        // push jail phase TODO: if phase is pushed here it needs to break from loop once resolved - handle in jail phase 
        // so that player does not get an extra turn by coming back to this point
        mJailData* pJail = malloc(sizeof(mJailData));
        pJail->pPlayer     = pPlayer;
        pJail->bShowedMenu = false;
        pJail->iChoice     = -1;
        pJail->bRolled     = false;
        m_push_phase(pFlow, m_phase_jail, pJail);
        return PHASE_RUNNING;
    }

    // show menu first time
    if(!pPreRoll->bShowedMenu)
    {
        printf("\n=== Player %d's Turn ===\n", pPlayer->ePlayerTurnPosition + 1);
        printf("1. View status\n");
        printf("2. View properties\n");
        printf("3. Property management\n");
        printf("4. Propose trade\n");
        printf("5. Roll dice\n");
        printf("> ");
        fflush(stdout);
        pPreRoll->bShowedMenu = true;
        return PHASE_RUNNING;
    }

    // wait for input
    if(!pFlow->bInputReceived)
    {
        return PHASE_RUNNING;
    }

    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow); // reset input buffer once copied to temp in scope buffer "iChoice"

    switch(iChoice)
    {
        case 0: return PHASE_RUNNING; // TODO: should iChoice be 0 by default to make sure to pass this switch statment when idling waiting 
                                    // on player choice
        case 1: // view status
        {
            m_show_player_status(pPlayer);
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }

        case 2: // view properties TODO: status contains this so this is likely redundant 
        {
            m_show_props_owned(pPlayer);
            m_show_rails_owned(pPlayer);
            m_show_utils_owned(pPlayer);
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }

        case 3: // property management
        {
            mPropertyManagementData* pMgmt = malloc(sizeof(mPropertyManagementData));
            pMgmt->pPlayer     = pPlayer;
            pMgmt->bShowedMenu = false;
            pMgmt->iSelection  = -1;
            m_push_phase(pFlow, m_phase_property_mgmt, pMgmt);
            return PHASE_RUNNING;
        }

        case 4: // propose trade
        {
            // TODO: implement trade phase
            printf("Trade system not yet implemented in phase system\n");
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }

        case 5: // roll dice
        {
            m_roll_dice(pGame->mGameDice);
            printf("\nRolled: %d + %d = %d\n", pGame->mGameDice->dice_one, pGame->mGameDice->dice_two, 
                    pGame->mGameDice->dice_one + pGame->mGameDice->dice_two);

            // move to post-roll phase
            mPostRollData* pPostRoll = malloc(sizeof(mPostRollData));
            pPostRoll->pPlayer          = pPlayer;
            pPostRoll->bProcessedSquare = false;
            pPostRoll->bShowedMenu      = false;
            pPostRoll->iMenuSelection   = -1;

            // free current phase data and set new phase
            free(pPreRoll);
            pFlow->pCurrentPhaseData = pPostRoll;
            pFlow->pfCurrentPhase = m_phase_post_roll;
            return PHASE_RUNNING;
        }

        default:
        {
            printf("Invalid choice\n");
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }
    }
}

// ==================== PHASE: POST-ROLL ==================== //

ePhaseResult 
m_phase_post_roll(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    mPostRollData* pPostRoll = (mPostRollData*)pData;
    mGameData* pGame         = pFlow->pGameData;
    mPlayer* pPlayer         = pGame->mGamePlayers[pGame->uCurrentPlayer];

    // move player if not already done
    if(!pPostRoll->bProcessedSquare)
    {
        m_move_player(pPlayer, pGame->mGameDice);
        printf("\nMoved to: %s\n", m_player_position_to_string(pPlayer->uPosition));
        pPostRoll->bProcessedSquare = true;
    }

    // handle square type
    eBoardSquareType tSquareType = m_get_square_type((uint32_t)pPlayer->uPosition);

    switch(tSquareType)
    {
        case GO_SQUARE_TYPE:
            if(pGame->uGameRoundCount >= 1)
            {
                pPlayer->uMoney += 200;
                printf("Collected $200 for landing on GO\n");
            }
            return PHASE_COMPLETE;

        case PROPERTY_SQUARE_TYPE:
        {
            mPropertyName tPropName = m_property_landed_on(pPlayer->uPosition);
            mProperty* pProp       = &pGame->mGameProperties[tPropName];

            if(m_is_property_owned(pProp))
            {
                if(!m_is_property_owner(pPlayer, pProp))
                {
                    bool bColorSet = m_color_set_owned(pGame, pPlayer, pProp->eColor);
                    m_pay_rent_property(pGame->mGamePlayers[pProp->eOwner], pPlayer, pProp, bColorSet);
                }
                return PHASE_COMPLETE;
            }
            else
            {
                // Offer to buy or auction
                if (!pPostRoll->bShowedMenu)
                {
                    printf("\n%s is available for $%d\n", pProp->cName, pProp->uPrice);
                    printf("1. Buy property\n");
                    printf("2. Pass (auction)\n");
                    printf("> ");
                    fflush(stdout);
                    pPostRoll->bShowedMenu = true;
                    return PHASE_RUNNING;
                }

                if(!pFlow->bInputReceived)
                    return PHASE_RUNNING;

                int iChoice = pFlow->iInputValue;
                m_clear_input(pFlow);

                if(iChoice == 1)
                {
                    m_buy_property(pProp, pPlayer);
                }
                else
                {
                    // start auction
                    mAuctionData* pAuction = malloc(sizeof(mAuctionData));
                    pAuction->pAsset          = pProp;
                    pAuction->uAssetType      = 0; // property TODO: dont like this
                    pAuction->uHighestBid     = 0;
                    pAuction->iBidOwner       = -1;
                    pAuction->bWaitingForBids = false;
                    m_push_phase(pFlow, m_phase_auction, pAuction);
                    return PHASE_RUNNING;
                }
                return PHASE_COMPLETE;
            }
        }

        case RAILROAD_SQUARE_TYPE:
        {
            mRailroadName tRailName = m_railroad_landed_on(pPlayer->uPosition);
            mRailroad* pRail        = &pGame->mGameRailroads[tRailName];

            if(m_is_railroad_owned(pRail))
            {
                if(!m_is_railroad_owner(pPlayer, pRail))
                {
                    m_pay_rent_railroad(pGame->mGamePlayers[pRail->eOwner], pPlayer, pRail);
                }
                return PHASE_COMPLETE;
            }
            else
            {
                if(!pPostRoll->bShowedMenu)
                {
                    printf("\n%s is available for $%d\n", pRail->cName, pRail->uPrice);
                    printf("1. Buy railroad\n");
                    printf("2. Pass (auction)\n");
                    printf("> ");
                    fflush(stdout);
                    pPostRoll->bShowedMenu = true;
                    return PHASE_RUNNING;
                }

                if(!pFlow->bInputReceived)
                    return PHASE_RUNNING;

                int iChoice = pFlow->iInputValue;
                m_clear_input(pFlow);

                if(iChoice == 1)
                {
                    m_buy_railroad(pRail, pPlayer);
                }
                else
                {
                    mAuctionData* pAuction = malloc(sizeof(mAuctionData));
                    pAuction->pAsset          = pRail;
                    pAuction->uAssetType      = 1; // railroad
                    pAuction->uHighestBid     = 0;
                    pAuction->iBidOwner       = -1;
                    pAuction->bWaitingForBids = false;
                    m_push_phase(pFlow, m_phase_auction, pAuction);
                    return PHASE_RUNNING;
                }
                return PHASE_COMPLETE;
            }
        }

        case UTILITY_SQUARE_TYPE:
        {
            mUtilityName tUtilName = m_utility_landed_on(pPlayer->uPosition);
            mUtility* pUtil        = &pGame->mGameUtilities[tUtilName];

            if(m_is_utility_owned(pUtil))
            {
                if(!m_is_utility_owner(pPlayer, pUtil))
                {
                    m_pay_rent_utility(pGame->mGamePlayers[pUtil->eOwner], pPlayer, pUtil, pGame->mGameDice);
                }
                return PHASE_COMPLETE;
            }
            else
            {
                if(!pPostRoll->bShowedMenu)
                {
                    printf("\n%s is available for $%d\n", pUtil->cName, pUtil->uPrice);
                    printf("1. Buy utility\n");
                    printf("2. Pass (auction)\n");
                    printf("> ");
                    fflush(stdout);
                    pPostRoll->bShowedMenu = true;
                    return PHASE_RUNNING;
                }

                if (!pFlow->bInputReceived)
                    return PHASE_RUNNING;

                int iChoice = pFlow->iInputValue;
                m_clear_input(pFlow);

                if(iChoice == 1)
                {
                    m_buy_utility(pUtil, pPlayer);
                }
                else
                {
                    mAuctionData* pAuction = malloc(sizeof(mAuctionData));
                    pAuction->pAsset          = pUtil;
                    pAuction->uAssetType      = 2; // utility
                    pAuction->uHighestBid     = 0;
                    pAuction->iBidOwner       = -1;
                    pAuction->bWaitingForBids = false;
                    m_push_phase(pFlow, m_phase_auction, pAuction);
                    return PHASE_RUNNING;
                }
                return PHASE_COMPLETE;
            }
        }

        case INCOME_TAX_SQUARE_TYPE:
            if(!m_can_player_afford(pPlayer, INCOME_TAX))
            {
                if (!m_attempt_emergency_payment(pGame, pPlayer, INCOME_TAX))
                {
                    m_trigger_bankruptcy(pGame, pGame->uCurrentPlayer, false, NO_PLAYER);
                }
            }
            else
            {
                pPlayer->uMoney -= INCOME_TAX;
                printf("Paid $%d income tax\n", INCOME_TAX);
            }
            return PHASE_COMPLETE;

        case LUXURY_TAX_SQUARE_TYPE:
            if(!m_can_player_afford(pPlayer, LUXURY_TAX))
            {
                if (!m_attempt_emergency_payment(pGame, pPlayer, LUXURY_TAX))
                {
                    m_trigger_bankruptcy(pGame, pGame->uCurrentPlayer, false, NO_PLAYER);
                }
            }
            else
            {
                pPlayer->uMoney -= LUXURY_TAX;
                printf("Paid $%d luxury tax\n", LUXURY_TAX);
            }
            return PHASE_COMPLETE;

        case CHANCE_CARD_SQUARE_TYPE:
        {
            m_execute_chance_card(pGame);
            return PHASE_COMPLETE;
        }
        case COMMUNITY_CHEST_SQUARE_TYPE:
        {
            m_execute_community_chest_card(pGame);
            return PHASE_COMPLETE;
        }
        case FREE_PARKING_SQUARE_TYPE:
        {
            printf("Just visiting Free Parking\n");
            return PHASE_COMPLETE;
        }
        case JAIL_SQUARE_TYPE:
            if (!pPlayer->bInJail)
            {
                printf("Just visiting Jail\n");
            }
            return PHASE_COMPLETE;

        case GO_TO_JAIL_SQUARE_TYPE:
        {
            m_move_player_to(pPlayer, JAIL_SQUARE);
            pPlayer->bInJail = true;
            printf("Go to Jail!\n");
            return PHASE_COMPLETE;
        }
        default:
            return PHASE_COMPLETE;
    }
}

// ==================== PHASE: END TURN ==================== //

ePhaseResult 
m_phase_end_turn(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    m_next_player_turn(pFlow->pGameData);

    // start new turn with pre-roll phase
    mPreRollData* pPreRoll = malloc(sizeof(mPreRollData));
    pPreRoll->pPlayer        = pFlow->pGameData->mGamePlayers[pFlow->pGameData->uCurrentPlayer];
    pPreRoll->bShowedMenu    = false;
    pPreRoll->iMenuSelection = -1;

    free(pData); // old phase
    pFlow->pCurrentPhaseData = pPreRoll;
    pFlow->pfCurrentPhase    = m_phase_pre_roll;

    return PHASE_RUNNING;
}

// ==================== PHASE: JAIL ==================== //

ePhaseResult 
m_phase_jail(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    mJailData* pJail = (mJailData*)pData;
    mGameData* pGame = pFlow->pGameData;
    mPlayer* pPlayer = pJail->pPlayer;

    if(!pJail->bShowedMenu)
    {
        printf("\n=== Jail (Turn %d/3) ===\n", pPlayer->uJailTurns);
        printf("1. Use Get Out of Jail Free card\n");
        printf("2. Pay $50 fine\n");
        printf("3. Roll for doubles\n");
        printf("> ");
        fflush(stdout);
        pJail->bShowedMenu = true;
        return PHASE_RUNNING;
    }

    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;

    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);

    switch(iChoice)
    {
        case 1: // use jail free card
        {
            if(m_use_jail_free_card(pPlayer))
            {
                printf("Used Get Out of Jail Free card\n");
                return PHASE_COMPLETE;
            }
            else
            {
                printf("You don't have a Get Out of Jail Free card\n");
                pJail->bShowedMenu = false;
                return PHASE_RUNNING;
            }
        }
        case 2: // pay fine
        {
            if(m_can_player_afford(pPlayer, pGame->uJailFine))
            {
                pPlayer->uMoney -= pGame->uJailFine;
                pPlayer->bInJail = false;
                printf("Paid $%d fine and left jail\n", pGame->uJailFine);
                return PHASE_COMPLETE;
            }
            else
            {
                printf("Not enough money to pay fine\n");
                pJail->bShowedMenu = false;
                return PHASE_RUNNING;
            }
        }
        case 3: // roll for doubles
        {
            if(!pJail->bRolled)
            {
                m_roll_dice(pGame->mGameDice);
                printf("Rolled: %d and %d\n", pGame->mGameDice->dice_one, pGame->mGameDice->dice_two);
                pJail->bRolled = true;

                if(pGame->mGameDice->dice_one == pGame->mGameDice->dice_two)
                {
                    pPlayer->bInJail = false;
                    pPlayer->uJailTurns = 0;
                    printf("Rolled doubles! You're free!\n");
                    return PHASE_COMPLETE;
                }
                else
                {
                    printf("Failed to roll doubles\n");
                    return PHASE_COMPLETE;
                }
            }
            return PHASE_RUNNING;
        }
        default:
        {
            printf("Invalid choice\n");
            pJail->bShowedMenu = false;
            return PHASE_RUNNING;
        }
    }
}

// ==================== PHASE: AUCTION ==================== //

ePhaseResult 
m_phase_auction(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    mAuctionData* pAuction = (mAuctionData*)pData;
    mGameData* pGame       = pFlow->pGameData;

    // TODO: implement full auction system
    // for now, just return the asset to the bank
    printf("Auction system not fully implemented - property returned to bank\n");

    if(pAuction->uAssetType == 0) // property
    {
        mProperty* pProp = (mProperty*)pAuction->pAsset;
        pProp->bOwned    = false;
        pProp->eOwner    = NO_PLAYER;
    }
    else if(pAuction->uAssetType == 1) // railroad
    {
        mRailroad* pRail = (mRailroad*)pAuction->pAsset;
        pRail->bOwned    = false;
        pRail->eOwner    = NO_PLAYER;
    }
    else if(pAuction->uAssetType == 2) // utility
    {
        mUtility* pUtil = (mUtility*)pAuction->pAsset;
        pUtil->bOwned   = false;
        pUtil->eOwner   = NO_PLAYER;
    }

    return PHASE_COMPLETE;
}

// ==================== PHASE: PROPERTY MANAGEMENT ==================== //

ePhaseResult 
m_phase_property_mgmt(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    mPropertyManagementData* pMgmt = (mPropertyManagementData*)pData;

    if(!pMgmt->bShowedMenu)
    {
        printf("\n=== Property Management ===\n");
        printf("1. Buy houses\n");
        printf("2. Sell houses\n");
        printf("3. Buy hotels\n");
        printf("4. Sell hotels\n");
        printf("5. Mortgage property\n");
        printf("6. Unmortgage property\n");
        printf("7. Back\n");
        printf("> ");
        fflush(stdout);
        pMgmt->bShowedMenu = true;
        return PHASE_RUNNING;
    }

    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;

    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);

    if (iChoice == 7)
    {
        return PHASE_COMPLETE;
    }

    // TODO: implement each property management option
    printf("Property management option %d not yet implemented\n", iChoice);
    pMgmt->bShowedMenu = false;
    return PHASE_RUNNING;
}

// ==================== PHASE: TRADE (STUB) ==================== //

ePhaseResult 
m_phase_trade(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    // TODO: implement trade negotiation system
    printf("Trade system not yet implemented\n");
    return PHASE_COMPLETE;
}

// TODO: do we even need these or is inline fine -> maybe for graphics could be useful
// ==================== PHASE: PAY RENT (STUB) ==================== //

ePhaseResult 
m_phase_pay_rent(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    // rent payment is handled inline for now
    return PHASE_COMPLETE;
}

// ==================== PHASE: DRAW CARD (STUB) ==================== //

ePhaseResult 
m_phase_draw_card(void* pData, float fDeltaTime, mGameFlow* pFlow)
{
    // card drawing is handled inline for now
    return PHASE_COMPLETE;
}

// ==================== CORE GAME FUNCTIONS (Keep existing implementations) ==================== //

void
m_next_player_turn(mGameData* mGame)
{
    if(mGame->mGamePlayers[mGame->uCurrentPlayer]->bBankrupt)
    {
        mGame->uActivePlayers--;
    }

    const uint8_t uMaxAttempts = mGame->uStartingPlayerCount; 
    uint8_t uAttempts = 0;

    while(uAttempts < uMaxAttempts)
    {
        mGame->uCurrentPlayer = (mGame->uCurrentPlayer + 1) % mGame->uStartingPlayerCount;

        if(mGame->uCurrentPlayer == 0) 
        {
            mGame->uGameRoundCount++;
        }

        if(mGame->uCurrentPlayer >= PLAYER_PIECE_TOTAL) 
        {
            mGame->uCurrentPlayer = 0;
            return;
        }

        if(!mGame->mGamePlayers[mGame->uCurrentPlayer]->bBankrupt) 
        {
            return;
        }
        uAttempts++;
    }
    mGame->bRunning = false;
}

void
m_game_over_check(mGameData* mGame)
{
    if(!mGame)
    {
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
        return true;
    }
    return false;
}

bool
m_use_jail_free_card(mPlayer* mPlayerInJail)
{
    if(mPlayerInJail->bGetOutOfJailFreeCard == true)
    {
        mPlayerInJail->bGetOutOfJailFreeCard = false;
        mPlayerInJail->bInJail               = false;
        mPlayerInJail->uJailTurns            = 0;
        mPlayerInJail->uPosition             = JAIL_SQUARE;
        return true;
    }
    return false;
}

void
m_forced_release(mGameData* mGame) 
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];

    if(current_player->bGetOutOfJailFreeCard)
    {
        current_player->bGetOutOfJailFreeCard = false;
    }

    if(m_can_player_afford(current_player, mGame->uJailFine) == false)
    {
        if(m_attempt_emergency_payment(mGame, current_player, mGame->uJailFine) == false)
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
m_can_player_afford(mPlayer* mCurrentPlayer, uint32_t uExpense)
{
    if(mCurrentPlayer->uMoney >= uExpense)
    {
        return true;
    }
    return false;
}

// TODO: needs to be phase so player can decide not be forced into certain actions 
bool
m_attempt_emergency_payment(mGameData* mGame, mPlayer* currentPlayer, uint32_t uAmount) 
{
    if(!mGame || !currentPlayer || uAmount == 0) return false;

    uint32_t uMoneyNeeded = uAmount;

    for(uint8_t i = 0; i < PROPERTY_TOTAL; i++) 
    {
        if(mGame->mGameProperties[i].eOwner != currentPlayer->ePlayerTurnPosition) continue;

        while(mGame->mGameProperties[i].uNumberOfHotels > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        {
            currentPlayer->uMoney += (mGame->mGameProperties[i].uHouseCost * 4) / 2;
            mGame->mGameProperties[i].uNumberOfHotels--;
        }

        while(mGame->mGameProperties[i].uNumberOfHouses > 0 && currentPlayer->uMoney < uMoneyNeeded) 
        {
            currentPlayer->uMoney += mGame->mGameProperties[i].uHouseCost / 2;
            mGame->mGameProperties[i].uNumberOfHouses--;
        }
    }

    for(uint8_t i = 0; i < PROPERTY_TOTAL && currentPlayer->uMoney < uMoneyNeeded; i++) 
    {
        if(mGame->mGameProperties[i].eOwner != currentPlayer->ePlayerTurnPosition || mGame->mGameProperties[i].bMortgaged) continue;

        currentPlayer->uMoney += mGame->mGameProperties[i].uPrice / 2;
        mGame->mGameProperties[i].bMortgaged = true;
    }

    return(currentPlayer->uMoney >= uMoneyNeeded);
}

bool
m_player_has_forced_actions(mGameData* mGame)
{
    mGame->uCurrentPlayer += 0;
    return true;
}

void
m_handle_emergency_actions(mGameData* mGame)
{
    mGame->uCurrentPlayer += 0;
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
            return GO_SQUARE_TYPE;
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
