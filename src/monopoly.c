#include "monopoly.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ==================== PHASE SYSTEM CORE ==================== //

void 
m_init_game_flow(mGameFlow* pFlow, mGameData* pGame, void* pInputContext)
{
    if(!pFlow || !pGame) return;

    memset(pFlow, 0, sizeof(mGameFlow));
    pFlow->pGame         = pGame;
    pFlow->pInputContext = pInputContext;
    pFlow->iStackDepth   = 0;

    // start with simple turn phase for testing TODO: replace this once more phases added
    mSimpleTurnData* pTurnData = malloc(sizeof(mSimpleTurnData));
    pTurnData->bShowedPrompt = false;
    pTurnData->bRolled = false;

    pFlow->pfCurrentPhase = m_phase_simple_turn;
    pFlow->pCurrentPhaseData = pTurnData;
}

void 
m_push_phase(mGameFlow* pFlow, fPhaseFunc pfNewPhase, void* pNewData)
{
    if(!pFlow || pFlow->iStackDepth >= 16) return;

    // save current phase to stack
    pFlow->apPhaseStack[pFlow->iStackDepth]     = pFlow->pfCurrentPhase;
    pFlow->apPhaseDataStack[pFlow->iStackDepth] = pFlow->pCurrentPhaseData;
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

    // restore previous phase
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
m_move_player(mPlayer* pPlayer, mDice* pDice)
{
    uint8_t uSpacesToMove = pDice->uDie1 + pDice->uDie2;
    uint8_t uOldPosition = pPlayer->uPosition;

    pPlayer->uPosition = (pPlayer->uPosition + uSpacesToMove) % TOTAL_BOARD_SQUARES;

    // collect $200 if passed or landed on go
    if(pPlayer->uPosition < uOldPosition || pPlayer->uPosition == 0)
    {
        pPlayer->uMoney += GO_MONEY;
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
        // TODO: add ability to raise money if possible 
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

// ==================== SIMPLE PHASE FOR TESTING ==================== //

// TODO: create real phases and test each 
ePhaseResult
m_phase_simple_turn(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mSimpleTurnData* pTurnData = (mSimpleTurnData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];

    // show prompt first time
    if(!pTurnData->bShowedPrompt)
    {
        printf("\n=== Player %d's Turn ===\n", pGame->uCurrentPlayerIndex + 1);
        printf("Money: $%d\n", pPlayer->uMoney);
        printf("Position: %d\n", pPlayer->uPosition);
        printf("Press 1 to roll dice\n> ");
        fflush(stdout);
        pTurnData->bShowedPrompt = true;
        return PHASE_RUNNING;
    }

    // wait for input
    if(!pFlow->bInputReceived)
    {
        return PHASE_RUNNING;
    }

    // only accept '1' to roll
    if(pFlow->iInputValue != 1)
    {
        m_clear_input(pFlow);
        printf("Invalid input. Press 1 to roll.\n> ");
        fflush(stdout);
        return PHASE_RUNNING;
    }

    // roll and move
    if(!pTurnData->bRolled)
    {
        m_roll_dice(&pGame->tDice);
        printf("\nRolled: %d + %d = %d\n", 
               pGame->tDice.uDie1, pGame->tDice.uDie2, 
               pGame->tDice.uDie1 + pGame->tDice.uDie2);

        m_move_player(pPlayer, &pGame->tDice);
        printf("Moved to position %d\n", pPlayer->uPosition);
        printf("Money: $%d\n\n", pPlayer->uMoney);

        pTurnData->bRolled = true;
        m_clear_input(pFlow);

        // move to next player
        m_next_player_turn(pGame);

        // check if game is over
        if(!pGame->bIsRunning)
        {
            printf("Game Over!\n");
            return PHASE_COMPLETE;
        }

        // create new phase data for next turn
        mSimpleTurnData* pNextTurn = malloc(sizeof(mSimpleTurnData));
        pNextTurn->bShowedPrompt = false;
        pNextTurn->bRolled = false;

        // free current and set new
        free(pTurnData);
        pFlow->pCurrentPhaseData = pNextTurn;

        return PHASE_RUNNING;
    }
    return PHASE_RUNNING;
}
