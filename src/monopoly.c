#include "monopoly.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// ==================== PHASE SYSTEM CORE ==================== //

void 
m_init_game_flow(mGameFlow* pFlow, mGameData* pGame, void* pInputContext)
{
    if(!pFlow || !pGame) return;

    memset(pFlow, 0, sizeof(mGameFlow));
    pFlow->pGame         = pGame;
    pFlow->pInputContext = pInputContext;
    pFlow->iStackDepth   = 0;
    
    mPreRollData* pPreRoll = malloc(sizeof(mPreRollData));
    memset(pPreRoll, 0, sizeof(mPreRollData));
    
    pFlow->pfCurrentPhase = m_phase_pre_roll;
    pFlow->pCurrentPhaseData = pPreRoll;
    
    pGame->bShowPrerollMenu = false;
}

void 
m_push_phase(mGameFlow* pFlow, fPhaseFunc pfNewPhase, void* pNewData)
{
    if(!pFlow || pFlow->iStackDepth >= 16) return;

    pFlow->apPhaseStack[pFlow->iStackDepth]     = pFlow->pfCurrentPhase;
    pFlow->apPhaseDataStack[pFlow->iStackDepth] = pFlow->pCurrentPhaseData;
    pFlow->iStackDepth++;

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

    pFlow->iStackDepth--;
    pFlow->pfCurrentPhase    = pFlow->apPhaseStack[pFlow->iStackDepth];
    pFlow->pCurrentPhaseData = pFlow->apPhaseDataStack[pFlow->iStackDepth];
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
    strncpy(pFlow->szInputString, szValue, sizeof(pFlow->szInputString) - 1);
    pFlow->szInputString[sizeof(pFlow->szInputString) - 1] = '\0';
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

// ==================== DICE ==================== //

void
m_roll_dice(mDice* pDice)
{
    pDice->uDie1 = (rand() % 6) + 1;
    pDice->uDie2 = (rand() % 6) + 1;
}

// ==================== MOVEMENT ==================== //

void
m_move_player(mPlayer* pPlayer, mDice* pDice, mGameData* pGame)
{
    uint8_t uSpacesToMove = pDice->uDie1 + pDice->uDie2;
    uint8_t uOldPosition = pPlayer->uPosition;
    
    pPlayer->uPosition = (pPlayer->uPosition + uSpacesToMove) % TOTAL_BOARD_SQUARES;
    
    if(pPlayer->uPosition < uOldPosition || pPlayer->uPosition == 0)
    {
        pPlayer->uMoney += GO_MONEY;
        if(pGame)
            m_set_notification(pGame, "Passed GO! Collected $%d", GO_MONEY);
    }
}

void
m_move_player_to(mPlayer* pPlayer, uint8_t uPosition)
{
    if(uPosition >= TOTAL_BOARD_SQUARES) return;
    pPlayer->uPosition = uPosition;
}

// ==================== TURN MANAGEMENT ==================== //

void
m_next_player_turn(mGameData* pGame)
{
    uint8_t uAttempts = 0;
    
    while(uAttempts < pGame->uPlayerCount)
    {
        pGame->uCurrentPlayerIndex = (pGame->uCurrentPlayerIndex + 1) % pGame->uPlayerCount;
        
        // increment round counter when we loop back to player 0
        if(pGame->uCurrentPlayerIndex == 0)
        {
            pGame->uRoundCount++;
        }
        
        // skip bankrupt players
        if(!pGame->amPlayers[pGame->uCurrentPlayerIndex].bIsBankrupt)
        {
            return;
        }
        
        uAttempts++;
    }
    
    // all players bankrupt
    pGame->bIsRunning = false;
}

// ==================== PROPERTY BUYING ==================== //

bool
m_can_afford(mPlayer* pPlayer, uint32_t uAmount)
{
    return pPlayer->uMoney >= uAmount;
}

bool
m_buy_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // check if already owned (not owned by bank)
    if(pProp->uOwnerIndex != BANK_PLAYER_INDEX) return false;
    
    // check if player can afford
    if(!m_can_afford(pPlayer, pProp->uPrice)) return false;
    
    // transfer ownership
    pPlayer->uMoney -= pProp->uPrice;
    pProp->uOwnerIndex = uPlayerIndex;
    
    // add to player's property list
    if(pPlayer->uPropertyCount < PROPERTY_ARRAY_SIZE)
    {
        pPlayer->auPropertiesOwned[pPlayer->uPropertyCount] = uPropertyIndex;
        pPlayer->uPropertyCount++;
    }
    
    return true;
}

// ==================== RENT PAYMENT ==================== //

uint8_t
m_count_properties_of_color(mGameData* pGame, uint8_t uPlayerIndex, ePropertyColor eColor)
{
    uint8_t uCount = 0;
    
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].eColor == eColor && 
           pGame->amProperties[i].uOwnerIndex == uPlayerIndex)
        {
            uCount++;
        }
    }
    
    return uCount;
}

uint8_t
m_get_color_set_size(ePropertyColor eColor)
{
    switch(eColor)
    {
        case COLOR_BROWN:
        case COLOR_DARK_BLUE:
            return 2;
        case COLOR_RAILROAD:
            return 4;
        case COLOR_UTILITY:
            return 2;
        default:
            return 3;
    }
}

bool
m_owns_color_set(mGameData* pGame, uint8_t uPlayerIndex, ePropertyColor eColor)
{
    uint8_t uOwned = m_count_properties_of_color(pGame, uPlayerIndex, eColor);
    uint8_t uNeeded = m_get_color_set_size(eColor);
    
    return uOwned == uNeeded;
}

uint32_t
m_calculate_rent(mGameData* pGame, uint8_t uPropertyIndex)
{
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    
    // property owned by bank (not owned)
    if(pProp->uOwnerIndex == BANK_PLAYER_INDEX) return 0;
    
    // mortgaged property
    if(pProp->bIsMortgaged) return 0;
    
    // railroads: rent based on count owned
    if(pProp->eType == PROPERTY_TYPE_RAILROAD)
    {
        uint8_t uRailroadsOwned = m_count_properties_of_color(pGame, pProp->uOwnerIndex, COLOR_RAILROAD);
        return pProp->uRentBase * (1 << (uRailroadsOwned - 1)); // doubles for each owned: 25, 50, 100, 200
    }
    
    // utilities: rent based on dice roll (handled separately in gameplay)
    if(pProp->eType == PROPERTY_TYPE_UTILITY)
    {
        uint8_t uUtilitiesOwned = m_count_properties_of_color(pGame, pProp->uOwnerIndex, COLOR_UTILITY);
        // multiplier will be applied to dice roll: 4x for 1, 10x for 2
        return (uUtilitiesOwned == 2) ? 10 : 4;
    }
    
    // streets: check for monopoly
    if(m_owns_color_set(pGame, pProp->uOwnerIndex, pProp->eColor))
    {
        return pProp->uRentMonopoly;
    }
    
    return pProp->uRentBase;
}

bool
m_pay_rent(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPayer = &pGame->amPlayers[uPayerIndex];
    
    // no owner (bank owns) or payer is owner
    if(pProp->uOwnerIndex == BANK_PLAYER_INDEX || pProp->uOwnerIndex == uPayerIndex) return false;
    
    mPlayer* pOwner = &pGame->amPlayers[pProp->uOwnerIndex];
    
    uint32_t uRent = m_calculate_rent(pGame, uPropertyIndex);
    
    // special case for utilities: multiply by dice roll
    if(pProp->eType == PROPERTY_TYPE_UTILITY)
    {
        uRent = uRent * (pGame->tDice.uDie1 + pGame->tDice.uDie2);
    }
    
    // transfer money
    if(pPayer->uMoney >= uRent)
    {
        pPayer->uMoney -= uRent;
        pOwner->uMoney += uRent;
        return true;
    }
    else
    {
        // player can't afford rent - goes bankrupt
        pOwner->uMoney += pPayer->uMoney;
        pPayer->uMoney = 0;
        pPayer->bIsBankrupt = true;
        pGame->uActivePlayers--;
        return false;
    }
}

// ==================== PROPERTY LOOKUP ==================== //

uint8_t
m_get_property_at_position(mGameData* pGame, uint8_t uBoardPosition)
{
    // find property with matching board position
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].uPosition == uBoardPosition)
        {
            return i;
        }
    }
    
    return BANK_PLAYER_INDEX; // not a property square
}

eSquareType
m_get_square_type(uint8_t uPosition)
{
    // special squares
    if(uPosition == 0) return SQUARE_GO;
    if(uPosition == 10) return SQUARE_JAIL;
    if(uPosition == 20) return SQUARE_FREE_PARKING;
    if(uPosition == 30) return SQUARE_GO_TO_JAIL;
    
    // tax squares
    if(uPosition == 4) return SQUARE_INCOME_TAX;
    if(uPosition == 38) return SQUARE_LUXURY_TAX;
    
    // chance squares
    if(uPosition == 7 || uPosition == 22 || uPosition == 36) return SQUARE_CHANCE;
    
    // community chest squares
    if(uPosition == 2 || uPosition == 17 || uPosition == 33) return SQUARE_COMMUNITY_CHEST;
    
    // everything else is a property (street, railroad, or utility)
    return SQUARE_PROPERTY;
}

// ==================== JAIL ==================== //

bool
m_use_jail_free_card(mPlayer* pPlayer)
{
    if(!pPlayer->bHasJailFreeCard) return false;
    
    pPlayer->bHasJailFreeCard = false;
    pPlayer->uJailTurns = 0;
    
    return true;
}

// ==================== MORTGAGING ==================== //

bool
m_mortgage_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // check ownership and mortgage status
    if(pProp->uOwnerIndex != uPlayerIndex) return false;
    if(pProp->bIsMortgaged) return false;
    
    // mortgage property
    pProp->bIsMortgaged = true;
    pPlayer->uMoney += pProp->uMortgageValue;
    
    return true;
}

bool
m_unmortgage_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // check ownership and mortgage status
    if(pProp->uOwnerIndex != uPlayerIndex) return false;
    if(!pProp->bIsMortgaged) return false;
    
    // calculate unmortgage cost (mortgage value + 10%)
    uint32_t uCost = pProp->uMortgageValue + (pProp->uMortgageValue / 10);
    
    // check if player can afford
    if(!m_can_afford(pPlayer, uCost)) return false;
    
    // unmortgage property
    pProp->bIsMortgaged = false;
    pPlayer->uMoney -= uCost;
    
    return true;
}

// ==================== NOTIFICATIONS ==================== //

// TODO: is this worth using or should it be more explict passing a string that is formatted before func call
void
m_set_notification(mGameData* pGame, const char* pcFormat, ...) 
{
    va_list args;
    va_start(args, pcFormat);
    vsnprintf(pGame->acNotification, sizeof(pGame->acNotification), pcFormat, args);
    va_end(args);
    pGame->bShowNotification = true;
    pGame->fNotificationTimer = 3.0f;  // show for 3 seconds
}

void
m_clear_notification(mGameData* pGame)
{
    pGame->bShowNotification = false;
    pGame->fNotificationTimer = 0.0f;
    pGame->acNotification[0] = '\0';
}

// ==================== CARD EXECUTION ==================== //

void
m_execute_chance_card(mGameData* pGame, uint8_t uCardIdx)
{
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    switch(uCardIdx)
    {
        case 0: // advance to go
        {
            pPlayer->uPosition = 0;
            pPlayer->uMoney += GO_MONEY;
            break;
        }
        
        case 1: // advance to illinois avenue (position 24)
        {
            if(pPlayer->uPosition > 24)
                pPlayer->uMoney += GO_MONEY;
            pPlayer->uPosition = 24;
            break;
        }
        
        case 2: // advance to st. charles place (position 11)
        {
            if(pPlayer->uPosition > 11)
                pPlayer->uMoney += GO_MONEY;
            pPlayer->uPosition = 11;
            break;
        }
        
        case 3: // advance to nearest utility
        {
            if(pPlayer->uPosition > 12 && pPlayer->uPosition < 28)
            {
                pPlayer->uPosition = 28;
            }
            else
            {
                if(pPlayer->uPosition > 28)
                    pPlayer->uMoney += GO_MONEY;
                pPlayer->uPosition = 12;
            }
            break;
        }
        
        case 4: // advance to nearest railroad
        {
            uint8_t uCurrent = pPlayer->uPosition;
            if(uCurrent < 5 || uCurrent >= 35)
            {
                if(uCurrent >= 35)
                    pPlayer->uMoney += GO_MONEY;
                pPlayer->uPosition = 5;
            }
            else if(uCurrent < 15)
                pPlayer->uPosition = 15;
            else if(uCurrent < 25)
                pPlayer->uPosition = 25;
            else
                pPlayer->uPosition = 35;
            break;
        }
        
        case 5: // bank pays dividend $50
        {
            pPlayer->uMoney += 50;
            break;
        }
        
        case 6: // get out of jail free
        {
            pPlayer->bHasJailFreeCard = true;
            break;
        }
        
        case 7: // go back 3 spaces
        {
            if(pPlayer->uPosition < 3)
                pPlayer->uPosition = 40 + pPlayer->uPosition - 3;
            else
                pPlayer->uPosition -= 3;
            break;
        }
        
        case 8: // go to jail
        {
            pPlayer->uPosition = 10;
            pPlayer->uJailTurns = 1;
            break;
        }
        
        case 9: // general repairs (skip for now - needs houses/hotels)
        {
            // TODO: implement when houses/hotels are added
            break;
        }
        
        case 10: // pay poor tax $15
        {
            if(pPlayer->uMoney >= 15)
            {
                pPlayer->uMoney -= 15;
            }
            else
            {
                pPlayer->bIsBankrupt = true;
                pGame->uActivePlayers--;
            }
            break;
        }
        
        case 11: // reading railroad (position 5)
        {
            if(pPlayer->uPosition > 5)
                pPlayer->uMoney += GO_MONEY;
            pPlayer->uPosition = 5;
            break;
        }
        
        case 12: // boardwalk (position 39)
        {
            pPlayer->uPosition = 39;
            break;
        }
        
        case 13: // elected chairman - pay each player $50
        {
            for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
            {
                if(i == pGame->uCurrentPlayerIndex)
                    continue;
                if(pGame->amPlayers[i].bIsBankrupt)
                    continue;
                
                if(pPlayer->uMoney >= 50)
                {
                    pPlayer->uMoney -= 50;
                    pGame->amPlayers[i].uMoney += 50;
                }
                else
                {
                    pPlayer->bIsBankrupt = true;
                    pGame->uActivePlayers--;
                    break;
                }
            }
            break;
        }
        
        case 14: // building loan matures $150
        {
            pPlayer->uMoney += 150;
            break;
        }
        
        case 15: // crossword competition $100
        {
            pPlayer->uMoney += 100;
            break;
        }
    }
}

void
m_execute_community_chest_card(mGameData* pGame, uint8_t uCardIdx)
{
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    switch(uCardIdx)
    {
        case 0: // advance to go
        {
            pPlayer->uPosition = 0;
            pPlayer->uMoney += GO_MONEY;
            break;
        }
        
        case 1: // bank error $200
        {
            pPlayer->uMoney += 200;
            break;
        }
        
        case 2: // doctor's fees $50
        {
            if(pPlayer->uMoney >= 50)
            {
                pPlayer->uMoney -= 50;
            }
            else
            {
                pPlayer->bIsBankrupt = true;
                pGame->uActivePlayers--;
            }
            break;
        }
        
        case 3: // stock sale $50
        {
            pPlayer->uMoney += 50;
            break;
        }
        
        case 4: // get out of jail free
        {
            pPlayer->bHasJailFreeCard = true;
            break;
        }
        
        case 5: // go to jail
        {
            pPlayer->uPosition = 10;
            pPlayer->uJailTurns = 1;
            break;
        }
        
        case 6: // grand opera night - collect $50 from each player
        {
            for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
            {
                if(i == pGame->uCurrentPlayerIndex)
                    continue;
                if(pGame->amPlayers[i].bIsBankrupt)
                    continue;
                
                if(pGame->amPlayers[i].uMoney >= 50)
                {
                    pGame->amPlayers[i].uMoney -= 50;
                    pPlayer->uMoney += 50;
                }
                else
                {
                    pPlayer->uMoney += pGame->amPlayers[i].uMoney;
                    pGame->amPlayers[i].uMoney = 0;
                    pGame->amPlayers[i].bIsBankrupt = true;
                    pGame->uActivePlayers--;
                }
            }
            break;
        }
        
        case 7: // holiday fund $100
        {
            pPlayer->uMoney += 100;
            break;
        }
        
        case 8: // income tax refund $20
        {
            pPlayer->uMoney += 20;
            break;
        }
        
        case 9: // birthday - collect $10 from each player
        {
            for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
            {
                if(i == pGame->uCurrentPlayerIndex)
                    continue;
                if(pGame->amPlayers[i].bIsBankrupt)
                    continue;
                
                if(pGame->amPlayers[i].uMoney >= 10)
                {
                    pGame->amPlayers[i].uMoney -= 10;
                    pPlayer->uMoney += 10;
                }
                else
                {
                    pPlayer->uMoney += pGame->amPlayers[i].uMoney;
                    pGame->amPlayers[i].uMoney = 0;
                    pGame->amPlayers[i].bIsBankrupt = true;
                    pGame->uActivePlayers--;
                }
            }
            break;
        }
        
        case 10: // life insurance $100
        {
            pPlayer->uMoney += 100;
            break;
        }
        
        case 11: // hospital fees $100
        {
            if(pPlayer->uMoney >= 100)
            {
                pPlayer->uMoney -= 100;
            }
            else
            {
                pPlayer->bIsBankrupt = true;
                pGame->uActivePlayers--;
            }
            break;
        }
        
        case 12: // school fees $150
        {
            if(pPlayer->uMoney >= 150)
            {
                pPlayer->uMoney -= 150;
            }
            else
            {
                pPlayer->bIsBankrupt = true;
                pGame->uActivePlayers--;
            }
            break;
        }
        
        case 13: // consultancy fee $25
        {
            pPlayer->uMoney += 25;
            break;
        }
        
        case 14: // street repairs (skip for now - needs houses/hotels)
        {
            // TODO: implement when houses/hotels are added
            break;
        }
        
        case 15: // beauty contest $10
        {
            pPlayer->uMoney += 10;
            break;
        }
    }
}

uint8_t
m_draw_chance_card(mGameData* pGame)
{
    mDeckState* pDeck = &pGame->tChanceDeck;
    
    // if deck exhausted, reshuffle
    if(pDeck->uCurrentIndex >= 16)
    {
        m_shuffle_deck(pDeck);
    }
    
    uint8_t uCardIdx = pDeck->auIndices[pDeck->uCurrentIndex];
    pDeck->uCurrentIndex++;
    
    return uCardIdx;
}

uint8_t
m_draw_community_chest_card(mGameData* pGame)
{
    mDeckState* pDeck = &pGame->tCommunityChestDeck;
    
    // if deck exhausted, reshuffle
    if(pDeck->uCurrentIndex >= 16)
    {
        m_shuffle_deck(pDeck);
    }
    
    uint8_t uCardIdx = pDeck->auIndices[pDeck->uCurrentIndex];
    pDeck->uCurrentIndex++;
    
    return uCardIdx;
}

// ==================== MAIN TURN PHASES ==================== //

ePhaseResult
m_phase_pre_roll(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mPreRollData* pPreRoll = (mPreRollData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    if(pPlayer->uJailTurns > 0)
    {
        mJailData* pJail = malloc(sizeof(mJailData));
        memset(pJail, 0, sizeof(mJailData));
        
        free(pPreRoll);
        pFlow->pCurrentPhaseData = pJail;
        pFlow->pfCurrentPhase = m_phase_jail;
        
        return PHASE_RUNNING;
    }
    
    if(!pPreRoll->bShowedMenu)
    {
        pGame->bShowPrerollMenu = true;
        pPreRoll->bShowedMenu = true;
        
        return PHASE_RUNNING;
    }
    
    if(!pFlow->bInputReceived)
    {
        return PHASE_RUNNING;
    }
    
    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);
    
    switch(iChoice)
    {
        case 1:
        {
            m_set_notification(pGame, "Property management not yet implemented");
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }
        
        case 2:
        {
            m_set_notification(pGame, "Trading not yet implemented");
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }
        
        case 3:
        {
            pGame->bShowPrerollMenu = false;
            
            m_roll_dice(&pGame->tDice);
            
            mPostRollData* pPostRoll = malloc(sizeof(mPostRollData));
            memset(pPostRoll, 0, sizeof(mPostRollData));
            
            // free current phase data and set new phase
            free(pPreRoll);
            pFlow->pCurrentPhaseData = pPostRoll;
            pFlow->pfCurrentPhase = m_phase_post_roll;
            
            return PHASE_RUNNING;
        }
        
        default:
        {
            pPreRoll->bShowedMenu = false;
            return PHASE_RUNNING;
        }
    }
}

ePhaseResult
m_phase_post_roll(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mPostRollData* pPostRoll = (mPostRollData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // move player if not already done
    if(!pPostRoll->bMovedPlayer)
    {
        m_move_player(pPlayer, &pGame->tDice, pGame);
        
        pPostRoll->bMovedPlayer = true;
        pPostRoll->eSquareType = m_get_square_type(pPlayer->uPosition);
        
        // if property square, find which property
        if(pPostRoll->eSquareType == SQUARE_PROPERTY)
        {
            pPostRoll->uPropertyIndex = m_get_property_at_position(pGame, pPlayer->uPosition);
        }
        
        // don't advance yet - need to handle landing
        return PHASE_RUNNING;
    }
    
    // handle landing if not done yet
    if(!pPostRoll->bHandledLanding)
    {
        switch(pPostRoll->eSquareType)
        {
            case SQUARE_GO:
            {
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_PROPERTY:
            {
                if(pPostRoll->uPropertyIndex == BANK_PLAYER_INDEX)
                {
                    pPostRoll->bHandledLanding = true;
                    break;
                }
                
                mProperty* pProp = &pGame->amProperties[pPostRoll->uPropertyIndex];
                
                // unowned property - offer to buy or auction
                if(pProp->uOwnerIndex == BANK_PLAYER_INDEX)
                {
                    // wait for input from UI
                    if(!pFlow->bInputReceived)
                        return PHASE_RUNNING;
                    
                    int iChoice = pFlow->iInputValue;
                    m_clear_input(pFlow);
                    
                    if(iChoice == 1)
                    {
                        if(m_buy_property(pGame, pPostRoll->uPropertyIndex, pGame->uCurrentPlayerIndex))
                        {
                            m_set_notification(pGame, "Bought %s for $%d", pProp->cName, pProp->uPrice);
                        }
                        else
                        {
                            m_set_notification(pGame, "Cannot afford %s", pProp->cName);
                        }
                    }
                    else
                    {
                        m_set_notification(pGame, "Passed on %s - auction not yet implemented", pProp->cName);
                        // TODO: push auction phase
                    }
                    
                    pPostRoll->bHandledLanding = true;
                }
                // owned by current player
                else if(pProp->uOwnerIndex == pGame->uCurrentPlayerIndex)
                {
                    pPostRoll->bHandledLanding = true;
                }
                // owned by another player - pay rent
                else
                {
                    uint32_t uRent = m_calculate_rent(pGame, pPostRoll->uPropertyIndex);
                    
                    // special handling for utilities (rent based on dice)
                    if(pProp->eType == PROPERTY_TYPE_UTILITY)
                    {
                        uRent = uRent * (pGame->tDice.uDie1 + pGame->tDice.uDie2);
                    }
                    
                    if(m_pay_rent(pGame, pPostRoll->uPropertyIndex, pGame->uCurrentPlayerIndex))
                    {
                        m_set_notification(pGame, "Paid $%d rent to Player %d", uRent, pProp->uOwnerIndex + 1);
                    }
                    else
                    {
                        m_set_notification(pGame, "BANKRUPT! Cannot afford $%d rent", uRent);
                        // TODO: bankruptcy phase
                    }
                    
                    pPostRoll->bHandledLanding = true;
                }
                break;
            }
            
            case SQUARE_INCOME_TAX:
            {
                if(pPlayer->uMoney >= INCOME_TAX)
                {
                    pPlayer->uMoney -= INCOME_TAX;
                    m_set_notification(pGame, "Paid $%d Income Tax", INCOME_TAX);
                }
                else
                {
                    pPlayer->bIsBankrupt = true;
                    pGame->uActivePlayers--;
                    m_set_notification(pGame, "BANKRUPT! Cannot afford $%d tax", INCOME_TAX);
                    // TODO: bankruptcy phase
                }
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_LUXURY_TAX:
            {
                if(pPlayer->uMoney >= LUXURY_TAX)
                {
                    pPlayer->uMoney -= LUXURY_TAX;
                    m_set_notification(pGame, "Paid $%d Luxury Tax", LUXURY_TAX);
                }
                else
                {
                    pPlayer->bIsBankrupt = true;
                    pGame->uActivePlayers--;
                    m_set_notification(pGame, "BANKRUPT! Cannot afford $%d tax", LUXURY_TAX);
                    // TODO:: bankruptcy phase
                }
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_CHANCE:
            {
                // draw card
                uint8_t uCardIdx = m_draw_chance_card(pGame);
                mChanceCard* pCard = &pGame->amChanceCards[uCardIdx];

                // show card to player
                m_set_notification(pGame, "Chance: %s", pCard->cDescription);

                // execute card effect immediately
                m_execute_chance_card(pGame, uCardIdx);

                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_COMMUNITY_CHEST:
            {
                // draw card
                uint8_t uCardIdx = m_draw_community_chest_card(pGame);  // was m_draw_chance_card
                mCommunityChestCard* pCard = &pGame->amCommunityChestCards[uCardIdx];  // was mChanceCard

                // show card to player
                m_set_notification(pGame, "Community Chest: %s", pCard->cDescription);  // was "Community Chance"

                // execute card effect immediately
                m_execute_community_chest_card(pGame, uCardIdx);

                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_FREE_PARKING:
            {
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_JAIL:
            {
                // just visiting, no action needed
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_GO_TO_JAIL:
            {
                pPlayer->uPosition = 10;  // jail position
                pPlayer->uJailTurns = 1;
                m_set_notification(pGame, "Go to Jail!");
                pPostRoll->bHandledLanding = true;
                break;
            }
            
            default:
                pPostRoll->bHandledLanding = true;
                break;
        }
        
        // if not done handling, return and wait
        if(!pPostRoll->bHandledLanding)
            return PHASE_RUNNING;
    }
    
    m_next_player_turn(pGame);
    
    mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
    memset(pNextPreRoll, 0, sizeof(mPreRollData));
    
    free(pPostRoll);
    pFlow->pCurrentPhaseData = pNextPreRoll;
    pFlow->pfCurrentPhase = m_phase_pre_roll;
    m_clear_input(pFlow);
    
    return PHASE_RUNNING;
}

// ==================== JAIL PHASE ==================== //

ePhaseResult
m_phase_jail(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mJailData* pJail = (mJailData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // show menu first time
    if(!pJail->bShowedMenu)
    {
        pJail->uAttemptNumber = pPlayer->uJailTurns;
        pJail->bShowedMenu = true;
        
        // set flag for UI to show jail menu
        pGame->bShowPrerollMenu = true;  // reuse this flag for jail menu
        
        return PHASE_RUNNING;
    }
    
    // wait for input
    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;
    
    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);
    
    switch(iChoice)
    {
        case 1: // pay fine
        {
            if(pPlayer->uMoney >= pGame->uJailFine)
            {
                pPlayer->uMoney -= pGame->uJailFine;
                pPlayer->uJailTurns = 0;
                m_set_notification(pGame, "Paid $50 fine - released from jail!");
                
                // end turn after paying fine
                m_next_player_turn(pGame);
                pGame->bShowPrerollMenu = false;
                
                mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
                memset(pNextPreRoll, 0, sizeof(mPreRollData));
                
                free(pJail);
                pFlow->pCurrentPhaseData = pNextPreRoll;
                pFlow->pfCurrentPhase = m_phase_pre_roll;
                
                return PHASE_RUNNING;
            }
            else
            {
                m_set_notification(pGame, "Cannot afford $50 fine!");
                pJail->bShowedMenu = false;
                return PHASE_RUNNING;
            }
        }
        
        case 2: // use get out of jail free card
        {
            if(pPlayer->bHasJailFreeCard)
            {
                pPlayer->bHasJailFreeCard = false;
                pPlayer->uJailTurns = 0;
                m_set_notification(pGame, "Used Get Out of Jail Free card!");
                
                // end turn after using card
                m_next_player_turn(pGame);
                pGame->bShowPrerollMenu = false;
                
                mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
                memset(pNextPreRoll, 0, sizeof(mPreRollData));
                
                free(pJail);
                pFlow->pCurrentPhaseData = pNextPreRoll;
                pFlow->pfCurrentPhase = m_phase_pre_roll;
                
                return PHASE_RUNNING;
            }
            else
            {
                m_set_notification(pGame, "You don't have a Get Out of Jail Free card!");
                pJail->bShowedMenu = false;
                return PHASE_RUNNING;
            }
        }
        
        case 3: // roll for doubles
        {
            if(!pJail->bRolledDice)
            {
                m_roll_dice(&pGame->tDice);
                pJail->bRolledDice = true;
                
                // check for doubles
                if(pGame->tDice.uDie1 == pGame->tDice.uDie2)
                {
                    pPlayer->uJailTurns = 0;
                    m_set_notification(pGame, "Rolled doubles (%d+%d)! Released and moving...", pGame->tDice.uDie1, pGame->tDice.uDie2);
                    
                    // transition to post-roll to move with the doubles roll
                    pGame->bShowPrerollMenu = false;
                    mPostRollData* pPostRoll = malloc(sizeof(mPostRollData));
                    memset(pPostRoll, 0, sizeof(mPostRollData));
                    
                    free(pJail);
                    pFlow->pCurrentPhaseData = pPostRoll;
                    pFlow->pfCurrentPhase = m_phase_post_roll;
                    
                    return PHASE_RUNNING;
                }
                else
                {
                    // didn't roll doubles
                    pPlayer->uJailTurns++;
                    
                    // check if this was third attempt
                    if(pPlayer->uJailTurns > 3)
                    {
                        // must pay fine on third failed attempt
                        if(pPlayer->uMoney >= pGame->uJailFine)
                        {
                            pPlayer->uMoney -= pGame->uJailFine;
                            pPlayer->uJailTurns = 0;
                            m_set_notification(pGame, "Failed 3 attempts - paid $50 fine");
                            
                            // transition back to pre-roll
                            pGame->bShowPrerollMenu = false;
                            mPreRollData* pPreRoll = malloc(sizeof(mPreRollData));
                            memset(pPreRoll, 0, sizeof(mPreRollData));
                            
                            free(pJail);
                            pFlow->pCurrentPhaseData = pPreRoll;
                            pFlow->pfCurrentPhase = m_phase_pre_roll;
                            
                            return PHASE_RUNNING;
                        }
                        else
                        {
                            // can't afford fine after 3 attempts - bankruptcy
                            m_set_notification(pGame, "Cannot afford jail fine - BANKRUPT!");
                            pPlayer->bIsBankrupt = true;
                            pGame->uActivePlayers--;
                            
                            // end turn
                            m_next_player_turn(pGame);
                            pGame->bShowPrerollMenu = false;
                            
                            mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
                            memset(pNextPreRoll, 0, sizeof(mPreRollData));
                            
                            free(pJail);
                            pFlow->pCurrentPhaseData = pNextPreRoll;
                            pFlow->pfCurrentPhase = m_phase_pre_roll;
                            
                            return PHASE_RUNNING;
                        }
                    }
                    else
                    {
                        // still have attempts left - end turn
                        m_set_notification(pGame, "Didn't roll doubles - stay in jail (attempt %d/3)", pPlayer->uJailTurns);
                        
                        m_next_player_turn(pGame);
                        pGame->bShowPrerollMenu = false;
                        
                        mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
                        memset(pNextPreRoll, 0, sizeof(mPreRollData));
                        
                        free(pJail);
                        pFlow->pCurrentPhaseData = pNextPreRoll;
                        pFlow->pfCurrentPhase = m_phase_pre_roll;
                        
                        return PHASE_RUNNING;
                    }
                }
            }
            
            return PHASE_RUNNING;
        }
        
        default:
        {
            pJail->bShowedMenu = false;
            return PHASE_RUNNING;
        }
    }
}

