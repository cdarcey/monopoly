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

// move player with dice
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

// for sending player to specific location
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
    uint8_t uStartingPlayer = pGame->uCurrentPlayerIndex;
    uint8_t uAttempts = 0;
    
    while(uAttempts < pGame->uPlayerCount)
    {
        pGame->uCurrentPlayerIndex = (pGame->uCurrentPlayerIndex + 1) % pGame->uPlayerCount;
        // skip bankrupt players
        if(!pGame->amPlayers[pGame->uCurrentPlayerIndex].bIsBankrupt)
        {
            // increment round if we've wrapped back to the first active player after a full cycle
            // this happens when current player has a lower index than where we started
            // protects from player one bankrupting messing up round counter
            if(pGame->uCurrentPlayerIndex < uStartingPlayer)
            {
                pGame->uRoundCount++;
            }
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

// ==================== BUILDING HOUSES/HOTELS ==================== //

bool
m_can_build_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    
    if(pProp->eType != PROPERTY_TYPE_STREET) return false; // must be a street property (not railroad or utility)
    if(pProp->uOwnerIndex != uPlayerIndex) return false; // must own the property
    if(pProp->bIsMortgaged) return false; // can't build on mortgaged property
    if(pProp->bHasHotel) return false; // already has hotel
    if(pProp->uHouses >= 4) return false; // already has 4 houses
    if(!m_owns_color_set(pGame, uPlayerIndex, pProp->eColor)) return false; // must own complete color set
    
    // no mortgaged properties in the color set
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].eColor == pProp->eColor && 
           pGame->amProperties[i].uOwnerIndex == uPlayerIndex &&
           pGame->amProperties[i].bIsMortgaged)
        {
            return false;
        }
    }
    
    // check even building rule - can't have more than 1 house difference
    uint8_t uMinHouses = 255;
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].eColor == pProp->eColor && pGame->amProperties[i].uOwnerIndex == uPlayerIndex)
        {
            if(pGame->amProperties[i].uHouses < uMinHouses)
                uMinHouses = pGame->amProperties[i].uHouses;
        }
    }
    
    // this property can't have more houses than the minimum + 1
    if(pProp->uHouses > uMinHouses) return false;
    
    // check house supply
    if(pGame->uGlobalHouseSupply == 0) return false;
    
    // check if player can afford
    if(!m_can_afford(pPlayer, pProp->uHouseCost)) return false;
    
    return true;
}

bool
m_build_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(!m_can_build_house(pGame, uPropertyIndex, uPlayerIndex)) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // charge player
    pPlayer->uMoney -= pProp->uHouseCost;
    pProp->uHouses++;
    pGame->uGlobalHouseSupply--;
    
    return true;
}

bool
m_can_build_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    if(pProp->eType != PROPERTY_TYPE_STREET) return false; // must be a street property
    if(pProp->uOwnerIndex != uPlayerIndex) return false; // must own the property
    if(pProp->bIsMortgaged) return false; // can't build on mortgaged property
    if(pProp->bHasHotel) return false; // already has hotel
    if(pProp->uHouses != 4) return false; // must have exactly 4 houses
    if(!m_owns_color_set(pGame, uPlayerIndex, pProp->eColor)) return false; // must own complete color set
    if(pGame->uGlobalHotelSupply == 0) return false; // check hotel supply
    
    // check if player can afford
    if(!m_can_afford(pPlayer, pProp->uHouseCost)) return false;
    
    return true;
}

bool
m_build_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(!m_can_build_hotel(pGame, uPropertyIndex, uPlayerIndex)) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // charge player (same as house cost)
    pPlayer->uMoney -= pProp->uHouseCost;
    
    // remove 4 houses, add hotel
    pProp->uHouses = 0;
    pProp->bHasHotel = true;
    
    // update supply (return 4 houses, remove 1 hotel)
    pGame->uGlobalHouseSupply += 4;
    pGame->uGlobalHotelSupply--;
    
    return true;
}

bool
m_can_sell_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    
    if(pProp->eType != PROPERTY_TYPE_STREET) return false; // must be a street property
    if(pProp->uOwnerIndex != uPlayerIndex) return false; // must own the property
    if(pProp->uHouses == 0) return false; // must have at least 1 house
    
    // check even selling rule - can't have more than 1 house difference after sale
    uint8_t uMaxHouses = 0;
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].eColor == pProp->eColor && pGame->amProperties[i].uOwnerIndex == uPlayerIndex)
        {
            uint8_t uHouses = pGame->amProperties[i].uHouses;
            if(pGame->amProperties[i].bHasHotel) uHouses = 5; // hotel counts as 5 for comparison
            
            if(uHouses > uMaxHouses)
                uMaxHouses = uHouses;
        }
    }
    
    // this property can't have fewer houses than maximum - 1
    if(pProp->uHouses != uMaxHouses) return false;
    
    return true;
}

bool
m_sell_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(!m_can_sell_house(pGame, uPropertyIndex, uPlayerIndex)) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // give player half the build cost
    pPlayer->uMoney += (pProp->uHouseCost / 2);
    pProp->uHouses--;
    pGame->uGlobalHouseSupply++;
    
    return true;
}

bool
m_can_sell_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(uPropertyIndex >= TOTAL_PROPERTIES) return false;
    if(uPlayerIndex >= pGame->uPlayerCount) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    if(pProp->eType != PROPERTY_TYPE_STREET) return false; // must be a street property
    if(pProp->uOwnerIndex != uPlayerIndex) return false; // must own the property
    if(!pProp->bHasHotel) return false; // must have a hotel
    if(pGame->uGlobalHouseSupply < 4) return false; // need 4 houses available to convert hotel back
    
    return true;
}

bool
m_sell_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex)
{
    if(!m_can_sell_hotel(pGame, uPropertyIndex, uPlayerIndex)) return false;
    
    mProperty* pProp = &pGame->amProperties[uPropertyIndex];
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    
    // give player half the build cost
    pPlayer->uMoney += (pProp->uHouseCost / 2);
    
    // convert hotel to 4 houses
    pProp->bHasHotel = false;
    pProp->uHouses = 4;
    
    // update supply (remove 4 houses, return 1 hotel)
    pGame->uGlobalHouseSupply -= 4;
    pGame->uGlobalHotelSupply++;
    
    return true;
}

// ==================== RENT PAYMENT ==================== //

uint8_t
m_count_properties_of_color(mGameData* pGame, uint8_t uPlayerIndex, ePropertyColor eColor)
{
    uint8_t uCount = 0;
    
    for(uint8_t i = 0; i < TOTAL_PROPERTIES; i++)
    {
        if(pGame->amProperties[i].eColor == eColor && pGame->amProperties[i].uOwnerIndex == uPlayerIndex)
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
    
    if(pProp->uOwnerIndex == BANK_PLAYER_INDEX) return 0; // property owned by bank (not owned)
    if(pProp->bIsMortgaged) return 0; // mortgaged property
    
    // railroads: rent based on count owned
    if(pProp->eType == PROPERTY_TYPE_RAILROAD)
    {
        uint8_t uRailroadsOwned = m_count_properties_of_color(pGame, pProp->uOwnerIndex, COLOR_RAILROAD);
        // rent doubles for each railroad owned
        uint32_t uMultiplier = 1;
        for(uint8_t i = 1; i < uRailroadsOwned; i++)
        {
            uMultiplier *= 2;
        }
    
        return pProp->uRentBase * uMultiplier;
    }
    
    // utilities: rent based on dice roll (multiplier returned, actual calculation in m_pay_rent)
    if(pProp->eType == PROPERTY_TYPE_UTILITY)
    {
        uint8_t uUtilitiesOwned = m_count_properties_of_color(pGame, pProp->uOwnerIndex, COLOR_UTILITY);
        return (uUtilitiesOwned == 2) ? 10 : 4; // multiplier will be applied to dice roll
    }

    // streets: check for houses/hotels first
    if(pProp->eType == PROPERTY_TYPE_STREET)
    {
        if(pProp->bHasHotel) // hotel rent
        {
            return pProp->auRentWithHouses[5]; // index 5 = hotel
        }
        if(pProp->uHouses > 0) // house rent (1-4 houses)
        {
            return pProp->auRentWithHouses[pProp->uHouses]; // index 1-4 = houses
        }
        if(m_owns_color_set(pGame, pProp->uOwnerIndex, pProp->eColor)) // no buildings - check for monopoly
        {
            return pProp->uRentMonopoly; // double rent for monopoly with no buildings
        }
        return pProp->uRentBase; // base rent (no monopoly, no buildings)
    }
    
    return 0;
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

const char*
m_get_square_name(mGameData* pGame, uint8_t uPosition)
{
    // special squares
    if(uPosition == 0) return "GO";
    if(uPosition == 10) return "Jail (Visiting)";
    if(uPosition == 20) return "Free Parking";
    if(uPosition == 30) return "Go To Jail";
    
    // tax squares
    if(uPosition == 4) return "Income Tax";
    if(uPosition == 38) return "Luxury Tax";
    
    // chance squares
    if(uPosition == 7 || uPosition == 22 || uPosition == 36) return "Chance";
    
    // community chest squares
    if(uPosition == 2 || uPosition == 17 || uPosition == 33) return "Community Chest";
    
    // property - find the property name
    uint8_t uPropIdx = m_get_property_at_position(pGame, uPosition);
    if(uPropIdx != BANK_PLAYER_INDEX)
    {
        return pGame->amProperties[uPropIdx].cName;
    }
    
    return "Unknown";
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

// Helper function to trigger bankruptcy from cards
void
m_trigger_card_bankruptcy(mGameData* pGame, mGameFlow* pFlow, uint32_t uAmountOwed)
{
    mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
    memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
    pBankruptcyData->eBankruptPlayer = pGame->uCurrentPlayerIndex;
    pBankruptcyData->uCreditor = BANK_PLAYER_INDEX;
    pBankruptcyData->uAmountOwed = uAmountOwed;
    m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
}

void
m_execute_chance_card(mGameData* pGame, uint8_t uCardIdx, mGameFlow* pFlow)
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
            uint8_t uIllinoisPos = pGame->amProperties[ILLINOIS_AVENUE_PROPERTY_ARRAY_INDEX].uPosition;
            if(pPlayer->uPosition > uIllinoisPos)
                pPlayer->uMoney += GO_MONEY;
            pPlayer->uPosition = uIllinoisPos;
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
            uint8_t uElectricPos = pGame->amProperties[ELECTRIC_COMPANY_PROPERTY_ARRAY_INDEX].uPosition;
            uint8_t uWaterPos = pGame->amProperties[WATER_WORKS_PROPERTY_ARRAY_INDEX].uPosition;
            
            if(pPlayer->uPosition > uElectricPos && pPlayer->uPosition < uWaterPos)
            {
                pPlayer->uPosition = uWaterPos;
            }
            else
            {
                if(pPlayer->uPosition > uWaterPos)
                    pPlayer->uMoney += GO_MONEY;
                pPlayer->uPosition = uElectricPos;
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
        
        case 9: // general repairs - pay $25 per house, $100 per hotel
        {
            uint32_t uTotalCost = 0;
            for(uint8_t uBuildingCount = 0; uBuildingCount < pPlayer->uPropertyCount; uBuildingCount++)
            {
                uint8_t uPropIdx = pPlayer->auPropertiesOwned[uBuildingCount];
                if(uPropIdx == BANK_PLAYER_INDEX)
                    break;

                mProperty* pProp = &pGame->amProperties[uPropIdx];

                if(pProp->bHasHotel)
                {
                    uTotalCost += 100;
                }
                else
                {
                    uTotalCost += pProp->uHouses * 25;
                }
            }

            if(pPlayer->uMoney >= uTotalCost)
            {
                pPlayer->uMoney -= uTotalCost;
            }
            else
            {
                m_trigger_card_bankruptcy(pGame, pFlow, uTotalCost);
            }
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
                m_trigger_card_bankruptcy(pGame, pFlow, 15);
            }
            break;
        }
        
        case 11: // reading railroad (position 5)
        {
            uint8_t uReadingPos = pGame->amProperties[READING_RAILROAD_PROPERTY_ARRAY_INDEX].uPosition;
            if(pPlayer->uPosition > uReadingPos)
                pPlayer->uMoney += GO_MONEY;
            pPlayer->uPosition = uReadingPos;
            break;
        }
        
        case 12: // boardwalk (position 39)
        {
            pPlayer->uPosition = 39;
            break;
        }
        
        case 13: // elected chairman - pay each player $50
        {
            uint32_t uTotalCost = 0;
            uint8_t uActivePlayers = 0;
            
            for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
            {
                if(i == pGame->uCurrentPlayerIndex)
                    continue;
                if(pGame->amPlayers[i].bIsBankrupt)
                    continue;
                
                uActivePlayers++;
                uTotalCost += 50;
            }
            
            if(pPlayer->uMoney >= uTotalCost)
            {
                for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
                {
                    if(i == pGame->uCurrentPlayerIndex)
                        continue;
                    if(pGame->amPlayers[i].bIsBankrupt)
                        continue;
                    
                    pPlayer->uMoney -= 50;
                    pGame->amPlayers[i].uMoney += 50;
                }
            }
            else
            {
                m_trigger_card_bankruptcy(pGame, pFlow, uTotalCost);
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
m_execute_community_chest_card(mGameData* pGame, uint8_t uCardIdx, mGameFlow* pFlow)
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
                m_trigger_card_bankruptcy(pGame, pFlow, 50);
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
                    // other player goes bankrupt, current player is creditor
                    pPlayer->uMoney += pGame->amPlayers[i].uMoney;
                    pGame->amPlayers[i].uMoney = 0;
                    
                    mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
                    memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
                    pBankruptcyData->eBankruptPlayer = i;
                    pBankruptcyData->uCreditor = pGame->uCurrentPlayerIndex;
                    pBankruptcyData->uAmountOwed = 50;
                    m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
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
                    // other player goes bankrupt, current player is creditor
                    pPlayer->uMoney += pGame->amPlayers[i].uMoney;
                    pGame->amPlayers[i].uMoney = 0;
                    
                    mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
                    memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
                    pBankruptcyData->eBankruptPlayer = i;
                    pBankruptcyData->uCreditor = pGame->uCurrentPlayerIndex;
                    pBankruptcyData->uAmountOwed = 10;
                    m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
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
                m_trigger_card_bankruptcy(pGame, pFlow, 100);
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
                m_trigger_card_bankruptcy(pGame, pFlow, 150);
            }
            break;
        }
        
        case 13: // consultancy fee $25
        {
            pPlayer->uMoney += 25;
            break;
        }
        
        case 14: // street repairs - pay $40 per house, $115 per hotel
        {
            uint32_t uTotalCost = 0;
            for(uint8_t uBuildingCount = 0; uBuildingCount < pPlayer->uPropertyCount; uBuildingCount++)
            {
                uint8_t uPropIdx = pPlayer->auPropertiesOwned[uBuildingCount];
                if(uPropIdx == BANK_PLAYER_INDEX)
                    break;
                
                mProperty* pProp = &pGame->amProperties[uPropIdx];
                
                if(pProp->bHasHotel)
                {
                    uTotalCost += 115;
                }
                else
                {
                    uTotalCost += pProp->uHouses * 40;
                }
            }
            
            if(pPlayer->uMoney >= uTotalCost)
            {
                pPlayer->uMoney -= uTotalCost;
            }
            else
            {
                m_trigger_card_bankruptcy(pGame, pFlow, uTotalCost);
            }
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

// property managment 
int32_t
m_calculate_net_worth(mGameData* pGame, uint8_t uPlayerIndex)
{
    if(uPlayerIndex >= pGame->uPlayerCount) return 0;
    
    mPlayer* pPlayer = &pGame->amPlayers[uPlayerIndex];
    int32_t iNetWorth = (int32_t)pPlayer->uMoney;
    
    for(uint8_t i = 0; i < pPlayer->uPropertyCount; i++)
    {
        uint8_t uPropIdx = pPlayer->auPropertiesOwned[i];
        if(uPropIdx == BANK_PLAYER_INDEX)
            break;
        
        mProperty* pProp = &pGame->amProperties[uPropIdx];
        
        if(pProp->bIsMortgaged)
        {
            // subtract mortgage debt (mortgage value + 10%)
            uint32_t uDebt = pProp->uMortgageValue + (pProp->uMortgageValue / 10);
            iNetWorth -= (int32_t)uDebt;
        }
        else
        {
            // add full property value
            iNetWorth += (int32_t)pProp->uPrice;
        }
    }
    
    return iNetWorth;
}

void
m_transfer_assets_to_player(mGameData* pGame, uint8_t uFromPlayer, uint8_t uToPlayer)
{
    mPlayer* pBankruptPlayer = &pGame->amPlayers[uFromPlayer];
    mPlayer* pCreditor = &pGame->amPlayers[uToPlayer];
    
    // transfer all remaining money
    pCreditor->uMoney += pBankruptPlayer->uMoney;
    pBankruptPlayer->uMoney = 0;
    
    // transfer get out of jail free card if they have it
    if(pBankruptPlayer->bHasJailFreeCard)
    {
        pCreditor->bHasJailFreeCard = true;
        pBankruptPlayer->bHasJailFreeCard = false;
    }
    
    // transfer all properties
    for(uint8_t i = 0; i < pBankruptPlayer->uPropertyCount; i++)
    {
        uint8_t uPropIdx = pBankruptPlayer->auPropertiesOwned[i];
        if(uPropIdx == BANK_PLAYER_INDEX)
            break;
        
        mProperty* pProp = &pGame->amProperties[uPropIdx];
        
        // change ownership
        pProp->uOwnerIndex = uToPlayer;
        
        // add to creditor's property list
        if(pCreditor->uPropertyCount < PROPERTY_ARRAY_SIZE)
        {
            pCreditor->auPropertiesOwned[pCreditor->uPropertyCount] = uPropIdx;
            pCreditor->uPropertyCount++;
        }
    }
    
    // clear bankrupt player's property list
    pBankruptPlayer->uPropertyCount = 0;
    for(uint8_t i = 0; i < PROPERTY_ARRAY_SIZE; i++)
    {
        pBankruptPlayer->auPropertiesOwned[i] = BANK_PLAYER_INDEX;
    }
}

void
m_transfer_assets_to_bank(mGameData* pGame, uint8_t uFromPlayer)
{
    mPlayer* pBankruptPlayer = &pGame->amPlayers[uFromPlayer];
    
    // money is lost (bank has inifinate money no need to return)
    pBankruptPlayer->uMoney = 0;
    
    // return get out of jail free card to deck if they have it
    if(pBankruptPlayer->bHasJailFreeCard)
    {
        pBankruptPlayer->bHasJailFreeCard = false;
        // card goes back into deck rotation naturally since we track by player ownership
    }
    
    // return all properties to bank
    for(uint8_t i = 0; i < pBankruptPlayer->uPropertyCount; i++)
    {
        uint8_t uPropIdx = pBankruptPlayer->auPropertiesOwned[i];
        if(uPropIdx == BANK_PLAYER_INDEX)
            break;
        
        mProperty* pProp = &pGame->amProperties[uPropIdx];
        
        // sell all houses/hotels back to bank
        while(pProp->uHouses > 0)
        {
            pProp->uHouses--;
            pGame->uGlobalHouseSupply++;
        }
        
        if(pProp->bHasHotel)
        {
            pProp->bHasHotel = false;
            pGame->uGlobalHotelSupply++;
        }
        
        // unmortgage property (bank takes it back fresh)
        pProp->bIsMortgaged = false;
        
        // return to bank ownership
        pProp->uOwnerIndex = BANK_PLAYER_INDEX;
    }
    
    // clear bankrupt player's property list
    pBankruptPlayer->uPropertyCount = 0;
    for(uint8_t i = 0; i < PROPERTY_ARRAY_SIZE; i++)
    {
        pBankruptPlayer->auPropertiesOwned[i] = BANK_PLAYER_INDEX;
    }
}

// ==================== PHASES ==================== //

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
            mPropertyManagementData* pPropMgmt = malloc(sizeof(mPropertyManagementData));
            memset(pPropMgmt, 0, sizeof(mPropertyManagementData));

            m_push_phase(pFlow, m_phase_property_management, pPropMgmt);
            pGame->bShowPrerollMenu = false;
            pGame->bShowPropertyMenu = false;

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
        
        if(pPostRoll->eSquareType == SQUARE_PROPERTY)
        {
            pPostRoll->uPropertyIndex = m_get_property_at_position(pGame, pPlayer->uPosition);
        }
        
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
                if(pProp->uOwnerIndex == BANK_PLAYER_INDEX && !pPostRoll->bHandledLanding)
                {
                    // wait for input from UI
                    if(!pFlow->bInputReceived)
                        return PHASE_RUNNING;

                    int iChoice = pFlow->iInputValue;
                    m_clear_input(pFlow);

                    if(iChoice == 1) // buy property
                    {
                        if(m_buy_property(pGame, pPostRoll->uPropertyIndex, pGame->uCurrentPlayerIndex))
                        {
                            m_set_notification(pGame, "Bought %s for $%d", pProp->cName, pProp->uPrice);
                        }
                        else
                        {
                            m_set_notification(pGame, "Cannot afford %s", pProp->cName);
                        }
                        pPostRoll->bHandledLanding = true;
                        return PHASE_RUNNING;
                    }
                    else if(iChoice == 2) // pass - start auction
                    {
                        mAuctionData* pAuction = malloc(sizeof(mAuctionData));
                        memset(pAuction, 0, sizeof(mAuctionData));

                        pAuction->ePropertyIndex = pPostRoll->uPropertyIndex;

                        m_push_phase(pFlow, m_phase_auction, pAuction);
                        pPostRoll->bHandledLanding = true;  
                        return PHASE_RUNNING;
                    }
                    else if(iChoice == 3) // manage properties
                    {
                        mPropertyManagementData* pPropMgmt = malloc(sizeof(mPropertyManagementData));
                        memset(pPropMgmt, 0, sizeof(mPropertyManagementData));
                        m_push_phase(pFlow, m_phase_property_management, pPropMgmt);
                        // DON'T set bHandledLanding - return to property decision after managing
                    }
                    else if(iChoice == 4) // propose trade (from property menu)
                    {
                        mTradeData* pTradeData = malloc(sizeof(mTradeData));
                        memset(pTradeData, 0, sizeof(mTradeData));
                        pTradeData->eStep = TRADE_STEP_SELECT_PLAYER;
                        m_push_phase(pFlow, m_phase_trade, pTradeData);
                        // dont set bHandledLanding - return to property decision after trade
                    }
                    
                    return PHASE_RUNNING;
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
                        // trigger bankruptcy phase
                        mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
                        memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
                        pBankruptcyData->eBankruptPlayer = pGame->uCurrentPlayerIndex;
                        pBankruptcyData->uCreditor = pProp->uOwnerIndex;
                        pBankruptcyData->uAmountOwed = uRent;
                        m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
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
                    // trigger bankruptcy phase
                    mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
                    memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
                    pBankruptcyData->eBankruptPlayer = pGame->uCurrentPlayerIndex;
                    pBankruptcyData->uCreditor = BANK_PLAYER_INDEX;
                    pBankruptcyData->uAmountOwed = INCOME_TAX;
                    m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
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
                    // trigger bankruptcy phase
                    mBankruptcyData* pBankruptcyData = malloc(sizeof(mBankruptcyData));
                    memset(pBankruptcyData, 0, sizeof(mBankruptcyData));
                    pBankruptcyData->eBankruptPlayer = pGame->uCurrentPlayerIndex;
                    pBankruptcyData->uCreditor = BANK_PLAYER_INDEX;
                    pBankruptcyData->uAmountOwed = LUXURY_TAX;
                    m_push_phase(pFlow, m_phase_bankruptcy, pBankruptcyData);
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
                m_execute_chance_card(pGame, uCardIdx, pFlow);

                pPostRoll->bHandledLanding = true;
                break;
            }
            
            case SQUARE_COMMUNITY_CHEST:
            {
                // draw card
                uint8_t uCardIdx = m_draw_community_chest_card(pGame); 
                mCommunityChestCard* pCard = &pGame->amCommunityChestCards[uCardIdx]; 

                // show card to player
                m_set_notification(pGame, "Community Chest: %s", pCard->cDescription);

                // execute card effect immediately
                m_execute_community_chest_card(pGame, uCardIdx, pFlow);

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
    
    // after handling landing, show end-of-turn options
    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;

    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);

    if(iChoice == 1) // manage properties
    {
        mPropertyManagementData* pPropMgmt = malloc(sizeof(mPropertyManagementData));
        memset(pPropMgmt, 0, sizeof(mPropertyManagementData));
        m_push_phase(pFlow, m_phase_property_management, pPropMgmt);
        return PHASE_RUNNING;
    }
    else if(iChoice == 2) // propose trade (end-of-turn menu)
    {
        mTradeData* pTradeData = malloc(sizeof(mTradeData));
        memset(pTradeData, 0, sizeof(mTradeData));
        pTradeData->eStep = TRADE_STEP_SELECT_PLAYER;
        m_push_phase(pFlow, m_phase_trade, pTradeData);
        return PHASE_RUNNING;
    }
    else if(iChoice == 3) // end turn
    {
        // fall through to next player turn logic below
    }
    
    m_next_player_turn(pGame);
    
    mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
    memset(pNextPreRoll, 0, sizeof(mPreRollData));
    
    free(pPostRoll);
    pFlow->pCurrentPhaseData = pNextPreRoll;
    pFlow->pfCurrentPhase = m_phase_pre_roll;
    pGame->bShowPrerollMenu = true;
    m_clear_input(pFlow);
    
    return PHASE_RUNNING;
}

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
        
        pGame->bShowJailMenu = true;
        
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
                pGame->bShowJailMenu = false;
                
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
                pGame->bShowJailMenu = false;
                
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
                    pGame->bShowJailMenu = false;
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
                        if(m_can_afford(pPlayer, pGame->uJailFine))
                        {
                            pPlayer->uMoney -= pGame->uJailFine;
                            pPlayer->uJailTurns = 0;
                            m_set_notification(pGame, "Failed 3 attempts - paid $50 fine");

                            // end turn after forced payment
                            m_next_player_turn(pGame);
                            pGame->bShowJailMenu = false;

                            mPreRollData* pNextPreRoll = malloc(sizeof(mPreRollData));
                            memset(pNextPreRoll, 0, sizeof(mPreRollData));

                            free(pJail);
                            pFlow->pCurrentPhaseData = pNextPreRoll;
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

ePhaseResult
m_phase_property_management(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mPropertyManagementData* pPropMgmt = (mPropertyManagementData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    if(!pPropMgmt->bShowedMenu)
    {
        pGame->bShowPropertyMenu = true;
        pPropMgmt->bShowedMenu = true;
        return PHASE_RUNNING;
    }
    
    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;
    
    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);
    
    // exit
    if(iChoice == 0)
    {
        pGame->bShowPropertyMenu = false;
        
        // reset pre-roll menu so it shows again
        mPreRollData* pPreRoll = (mPreRollData*)pFlow->apPhaseDataStack[pFlow->iStackDepth - 1];
        pPreRoll->bShowedMenu = false;
        
        m_pop_phase(pFlow);
        return PHASE_RUNNING;
    }
    
    uint8_t uPropArrayIdx = 0;
    uint8_t uPropIdx = 0;
    mProperty* pProp = NULL;
    
    // build house (100-199)
    if(iChoice >= 100 && iChoice < 200)
    {
        uPropArrayIdx = (uint8_t)(iChoice - 100);
        if(uPropArrayIdx < pPlayer->uPropertyCount)
        {
            uPropIdx = pPlayer->auPropertiesOwned[uPropArrayIdx];
            if(m_build_house(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                pProp = &pGame->amProperties[uPropIdx];
                m_set_notification(pGame, "Built house on %s ($%d)", pProp->cName, pProp->uHouseCost);
            }
            else
            {
                m_set_notification(pGame, "Cannot build house on this property");
            }
        }
        pPropMgmt->bShowedMenu = false;
        return PHASE_RUNNING;
    }
    
    // build hotel (200-299)
    if(iChoice >= 200 && iChoice < 300)
    {
        uPropArrayIdx = (uint8_t)(iChoice - 200);
        if(uPropArrayIdx < pPlayer->uPropertyCount)
        {
            uPropIdx = pPlayer->auPropertiesOwned[uPropArrayIdx];
            if(m_build_hotel(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                pProp = &pGame->amProperties[uPropIdx];
                m_set_notification(pGame, "Built hotel on %s ($%d)", pProp->cName, pProp->uHouseCost);
            }
            else
            {
                m_set_notification(pGame, "Cannot build hotel on this property");
            }
        }
        pPropMgmt->bShowedMenu = false;
        return PHASE_RUNNING;
    }
    
    // sell house (300-399)
    if(iChoice >= 300 && iChoice < 400)
    {
        uPropArrayIdx = (uint8_t)(iChoice - 300);
        if(uPropArrayIdx < pPlayer->uPropertyCount)
        {
            uPropIdx = pPlayer->auPropertiesOwned[uPropArrayIdx];
            pProp = &pGame->amProperties[uPropIdx];
            uint32_t uRefund = pProp->uHouseCost / 2;
            
            if(m_sell_house(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                m_set_notification(pGame, "Sold house from %s (+$%d)", pProp->cName, uRefund);
            }
            else
            {
                m_set_notification(pGame, "Cannot sell house from this property");
            }
        }
        pPropMgmt->bShowedMenu = false;
        return PHASE_RUNNING;
    }
    
    // sell hotel (400-499)
    if(iChoice >= 400 && iChoice < 500)
    {
        uPropArrayIdx = (uint8_t)(iChoice - 400);
        if(uPropArrayIdx < pPlayer->uPropertyCount)
        {
            uPropIdx = pPlayer->auPropertiesOwned[uPropArrayIdx];
            pProp = &pGame->amProperties[uPropIdx];
            uint32_t uRefund = pProp->uHouseCost / 2;
            
            if(m_sell_hotel(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                m_set_notification(pGame, "Sold hotel from %s (+$%d)", pProp->cName, uRefund);
            }
            else
            {
                m_set_notification(pGame, "Cannot sell hotel from this property");
            }
        }
        pPropMgmt->bShowedMenu = false;
        return PHASE_RUNNING;
    }
    
    // mortgage/unmortgage (1-99)
    if(iChoice >= 1 && iChoice < 100)
    {
        uPropArrayIdx = (uint8_t)(iChoice - 1);
        
        if(uPropArrayIdx >= pPlayer->uPropertyCount)
        {
            m_set_notification(pGame, "Invalid property selection");
            pPropMgmt->bShowedMenu = false;
            return PHASE_RUNNING;
        }
        
        uPropIdx = pPlayer->auPropertiesOwned[uPropArrayIdx];
        pProp = &pGame->amProperties[uPropIdx];
        
        // toggle mortgage status
        if(pProp->bIsMortgaged)
        {
            if(m_unmortgage_property(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                uint32_t uCost = pProp->uMortgageValue + (pProp->uMortgageValue / 10);
                m_set_notification(pGame, "Unmortgaged %s for $%d", pProp->cName, uCost);
            }
            else
            {
                m_set_notification(pGame, "Cannot afford to unmortgage %s", pProp->cName);
            }
        }
        else
        {
            if(m_mortgage_property(pGame, uPropIdx, pGame->uCurrentPlayerIndex))
            {
                m_set_notification(pGame, "Mortgaged %s for $%d", pProp->cName, pProp->uMortgageValue);
            }
            else
            {
                m_set_notification(pGame, "Cannot mortgage %s", pProp->cName);
            }
        }
        
        pPropMgmt->bShowedMenu = false;
        return PHASE_RUNNING;
    }
    
    pPropMgmt->bShowedMenu = false;
    return PHASE_RUNNING;
}

ePhaseResult
m_phase_auction(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mAuctionData* pAuction = (mAuctionData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    
    // show menu first time
    if(!pAuction->bShowedMenu)
    {
        pGame->bShowAuctionMenu = true;
        pAuction->bShowedMenu = true;
        
        // start with player after current player
        pAuction->uCurrentBidder = (pGame->uCurrentPlayerIndex + 1) % pGame->uPlayerCount;
        
        // skip to first non-bankrupt player
        while(pGame->amPlayers[pAuction->uCurrentBidder].bIsBankrupt && 
              pAuction->uCurrentBidder != pGame->uCurrentPlayerIndex)
        {
            pAuction->uCurrentBidder = (pAuction->uCurrentBidder + 1) % pGame->uPlayerCount;
        }
        
        pAuction->uHighestBidder = BANK_PLAYER_INDEX;
        pAuction->uHighestBid = 0;
        pAuction->uConsecutivePasses = 0;
        
        return PHASE_RUNNING;
    }
    
    // wait for input
    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;
    
    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);
    
    mPlayer* pCurrentBidder = &pGame->amPlayers[pAuction->uCurrentBidder];
    
    // pass
    if(iChoice == 0)
    {
        if(!pAuction->abPlayersPassed[pAuction->uCurrentBidder])
        {
            pAuction->abPlayersPassed[pAuction->uCurrentBidder] = true;
            pAuction->uConsecutivePasses++;
        }
        
        // count how many players have passed
        uint8_t uPlayersPassed = 0;
        for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
        {
            if(pGame->amPlayers[i].bIsBankrupt)
                continue;
            if(pAuction->abPlayersPassed[i])
                uPlayersPassed++;
        }
        
        // auction ends when everyone has passed or when everyone except the highest bidder has passed
        bool bAuctionOver = (uPlayersPassed >= pGame->uActivePlayers) || 
                            (pAuction->uHighestBidder != BANK_PLAYER_INDEX && 
                             uPlayersPassed >= pGame->uActivePlayers - 1);
        
        if(bAuctionOver)
        {
            pGame->bShowAuctionMenu = false;
            
            // award property to highest bidder
            if(pAuction->uHighestBidder != BANK_PLAYER_INDEX)
            {
                mProperty* pProp = &pGame->amProperties[pAuction->ePropertyIndex];
                mPlayer* pWinner = &pGame->amPlayers[pAuction->uHighestBidder];
                
                pWinner->uMoney -= pAuction->uHighestBid;
                pProp->uOwnerIndex = pAuction->uHighestBidder;
                
                // add to winner's property list
                if(pWinner->uPropertyCount < PROPERTY_ARRAY_SIZE)
                {
                    pWinner->auPropertiesOwned[pWinner->uPropertyCount] = pAuction->ePropertyIndex;
                    pWinner->uPropertyCount++;
                }
                
                m_set_notification(pGame, "Player %d won %s for $%d!", 
                    pAuction->uHighestBidder + 1, pProp->cName, pAuction->uHighestBid);
            }
            else
            {
                m_set_notification(pGame, "No bids - property remains unowned");
            }
            
            mPostRollData* pPostRoll = (mPostRollData*)pFlow->apPhaseDataStack[pFlow->iStackDepth - 1];
            pPostRoll->bHandledLanding = true;
            
            m_pop_phase(pFlow);
            return PHASE_RUNNING;
        }
        
        // move to next bidder
        do {
            pAuction->uCurrentBidder = (pAuction->uCurrentBidder + 1) % pGame->uPlayerCount;
        } while(pGame->amPlayers[pAuction->uCurrentBidder].bIsBankrupt);
        
        return PHASE_RUNNING;
    }
    
    // bid amount
    uint32_t uBidAmount = (uint32_t)iChoice;
    
    // validate bid is higher than current
    if(uBidAmount <= pAuction->uHighestBid)
    {
        m_set_notification(pGame, "Bid must be higher than $%d", pAuction->uHighestBid);
        return PHASE_RUNNING;
    }
    
    // validate player can afford bid
    if(uBidAmount > pCurrentBidder->uMoney)
    {
        m_set_notification(pGame, "Cannot afford bid of $%d", uBidAmount);
        return PHASE_RUNNING;
    }
    
    // accept bid
    pAuction->uHighestBid = uBidAmount;
    pAuction->uHighestBidder = pAuction->uCurrentBidder;
    pAuction->abPlayersPassed[pAuction->uCurrentBidder] = false;
    pAuction->uConsecutivePasses = 0;
    
    // move to next bidder
    do {
        pAuction->uCurrentBidder = (pAuction->uCurrentBidder + 1) % pGame->uPlayerCount;
    } while(pGame->amPlayers[pAuction->uCurrentBidder].bIsBankrupt);
    
    return PHASE_RUNNING;
}

ePhaseResult 
m_phase_bankruptcy(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mBankruptcyData* pBankruptcy = (mBankruptcyData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pBankruptPlayer = &pGame->amPlayers[pBankruptcy->eBankruptPlayer];
    
    uint32_t uDebtOwed = pBankruptcy->uAmountOwed;
    uint32_t uMoneyRaised = pBankruptPlayer->uMoney;
    
    // step 1: sell all hotels back to 4 houses
    for(uint8_t i = 0; i < pBankruptPlayer->uPropertyCount; i++)
    {
        uint8_t uPropIdx = pBankruptPlayer->auPropertiesOwned[i];
        if(uPropIdx == BANK_PLAYER_INDEX) break;
        
        mProperty* pProp = &pGame->amProperties[uPropIdx];
        
        if(pProp->bHasHotel && pGame->uGlobalHouseSupply >= 4)
        {
            m_sell_hotel(pGame, uPropIdx, pBankruptcy->eBankruptPlayer);
            uMoneyRaised += pProp->uHouseCost / 2;
            
            if(uMoneyRaised >= uDebtOwed)
            {
                // paid off debt through hotel sales
                pBankruptPlayer->uMoney = uMoneyRaised - uDebtOwed;
                
                // pay the creditor
                if(pBankruptcy->uCreditor != BANK_PLAYER_INDEX)
                {
                    pGame->amPlayers[pBankruptcy->uCreditor].uMoney += uDebtOwed;
                }
                
                m_pop_phase(pFlow);
                return PHASE_RUNNING;
            }
        }
    }
    
    // step 2: sell all houses
    bool bSoldHouse = true;
    while(bSoldHouse && uMoneyRaised < uDebtOwed)
    {
        bSoldHouse = false;
        
        for(uint8_t i = 0; i < pBankruptPlayer->uPropertyCount; i++)
        {
            uint8_t uPropIdx = pBankruptPlayer->auPropertiesOwned[i];
            if(uPropIdx == BANK_PLAYER_INDEX) break;
            
            if(m_can_sell_house(pGame, uPropIdx, pBankruptcy->eBankruptPlayer))
            {
                mProperty* pProp = &pGame->amProperties[uPropIdx];
                m_sell_house(pGame, uPropIdx, pBankruptcy->eBankruptPlayer);
                uMoneyRaised += pProp->uHouseCost / 2;
                bSoldHouse = true;
                
                if(uMoneyRaised >= uDebtOwed)
                {
                    // paid off debt through house sales
                    pBankruptPlayer->uMoney = uMoneyRaised - uDebtOwed;
                    
                    // pay the creditor
                    if(pBankruptcy->uCreditor != BANK_PLAYER_INDEX)
                    {
                        pGame->amPlayers[pBankruptcy->uCreditor].uMoney += uDebtOwed;
                    }
                    
                    m_pop_phase(pFlow);
                    return PHASE_RUNNING;
                }
            }
        }
    }
    
    // step 3: mortgage all unmortgaged properties
    for(uint8_t i = 0; i < pBankruptPlayer->uPropertyCount; i++)
    {
        uint8_t uPropIdx = pBankruptPlayer->auPropertiesOwned[i];
        if(uPropIdx == BANK_PLAYER_INDEX) break;
        
        mProperty* pProp = &pGame->amProperties[uPropIdx];
        
        if(!pProp->bIsMortgaged)
        {
            m_mortgage_property(pGame, uPropIdx, pBankruptcy->eBankruptPlayer);
            uMoneyRaised += pProp->uMortgageValue;
            
            if(uMoneyRaised >= uDebtOwed)
            {
                // paid off debt through mortgages
                pBankruptPlayer->uMoney = uMoneyRaised - uDebtOwed;
                
                // pay the creditor
                if(pBankruptcy->uCreditor != BANK_PLAYER_INDEX)
                {
                    pGame->amPlayers[pBankruptcy->uCreditor].uMoney += uDebtOwed;
                }
                
                m_pop_phase(pFlow);
                return PHASE_RUNNING;
            }
        }
    }
    
    // step 4: still can't pay - declare bankruptcy
    pBankruptPlayer->bIsBankrupt = true;
    pGame->uActivePlayers--;

    // transfer assets based on creditor type
    if(pBankruptcy->uCreditor == BANK_PLAYER_INDEX)
    {
        // creditor is bank - return all properties to bank
        m_transfer_assets_to_bank(pGame, pBankruptcy->eBankruptPlayer);
        m_set_notification(pGame, "Player %d is bankrupt! Properties returned to the bank.", 
            pBankruptcy->eBankruptPlayer + 1);
    }
    else
    {
        // creditor is another player - transfer everything
        m_transfer_assets_to_player(pGame, pBankruptcy->eBankruptPlayer, pBankruptcy->uCreditor);
        m_set_notification(pGame, "Player %d is bankrupt! Assets transferred to Player %d.", 
            pBankruptcy->eBankruptPlayer + 1, pBankruptcy->uCreditor + 1);
    }

    // check if game is over (only 1 player left)
    if(pGame->uActivePlayers == 1)
    {
        pGame->bIsRunning = false;
        
        // find the winner
        for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
        {
            if(!pGame->amPlayers[i].bIsBankrupt)
            {
                m_set_notification(pGame, "GAME OVER! Player %d wins!", i + 1);
                break;
            }
        }
    }
    else
    {
        // game continues - move to next player's turn
        m_next_player_turn(pGame);
    }

    m_pop_phase(pFlow);
    return PHASE_RUNNING;
}

ePhaseResult
m_phase_trade(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow)
{
    mTradeData* pTrade = (mTradeData*)pPhaseData;
    mGameData* pGame = pFlow->pGame;
    mPlayer* pCurrentPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // show menu once per step
    if(!pTrade->bShowedMenu)
    {
        pGame->bShowTradeMenu = true;
        pTrade->bShowedMenu = true;
        return PHASE_RUNNING;
    }
    
    if(!pFlow->bInputReceived)
        return PHASE_RUNNING;
    
    int iChoice = pFlow->iInputValue;
    m_clear_input(pFlow);
    
    switch(pTrade->eStep)
    {
        case TRADE_STEP_SELECT_PLAYER:
        {
            if(iChoice == 0) // cancel
            {
                pGame->bShowTradeMenu = false;
                m_pop_phase(pFlow);
                return PHASE_RUNNING;
            }
            
            // player selection (1-6 maps to player indices 0-5)
            if(iChoice >= 1 && iChoice <= 6)
            {
                uint8_t uSelectedPlayer = (uint8_t)(iChoice - 1);
                
                // validate selection
                if(uSelectedPlayer == pGame->uCurrentPlayerIndex)
                {
                    m_set_notification(pGame, "Cannot trade with yourself");
                    pTrade->bShowedMenu = false;
                    return PHASE_RUNNING;
                }
                
                if(uSelectedPlayer >= pGame->uPlayerCount || pGame->amPlayers[uSelectedPlayer].bIsBankrupt)
                {
                    m_set_notification(pGame, "Invalid player selection");
                    pTrade->bShowedMenu = false;
                    return PHASE_RUNNING;
                }
                
                pTrade->uTargetPlayer = uSelectedPlayer;
                pTrade->eStep = TRADE_STEP_BUILD_OFFER;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            break;
        }
        
        case TRADE_STEP_BUILD_OFFER:
        {
            if(iChoice == 0) // back to player selection
            {
                pTrade->eStep = TRADE_STEP_SELECT_PLAYER;
                pTrade->uOfferedPropertyCount = 0;
                pTrade->uRequestedPropertyCount = 0;
                pTrade->uOfferedMoney = 0;
                pTrade->uRequestedMoney = 0;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            if(iChoice == 1) // send offer
            {
                // validate offer isn't empty
                if(pTrade->uOfferedPropertyCount == 0 && pTrade->uOfferedMoney == 0 &&
                   pTrade->uRequestedPropertyCount == 0 && pTrade->uRequestedMoney == 0)
                {
                    m_set_notification(pGame, "Trade cannot be empty");
                    pTrade->bShowedMenu = false;
                    return PHASE_RUNNING;
                }
                
                pTrade->eStep = TRADE_STEP_AWAITING_RESPONSE;
                pTrade->bShowedMenu = false;
                m_set_notification(pGame, "Trade offer sent to Player %d", pTrade->uTargetPlayer + 1);
                return PHASE_RUNNING;
            }
            
            // toggle offered properties (100-199)
            if(iChoice >= 100 && iChoice < 200)
            {
                uint8_t uPropArrayIdx = (uint8_t)(iChoice - 100);
                if(uPropArrayIdx < pCurrentPlayer->uPropertyCount)
                {
                    uint8_t uPropIdx = pCurrentPlayer->auPropertiesOwned[uPropArrayIdx];
                    
                    // check if already in offer
                    bool bFound = false;
                    uint8_t uFoundIdx = 0;
                    for(uint8_t i = 0; i < pTrade->uOfferedPropertyCount; i++)
                    {
                        if(pTrade->auOfferedProperties[i] == uPropIdx)
                        {
                            bFound = true;
                            uFoundIdx = i;
                            break;
                        }
                    }
                    
                    if(bFound)
                    {
                        // remove from offer
                        for(uint8_t i = uFoundIdx; i < pTrade->uOfferedPropertyCount - 1; i++)
                        {
                            pTrade->auOfferedProperties[i] = pTrade->auOfferedProperties[i + 1];
                        }
                        pTrade->uOfferedPropertyCount--;
                    }
                    else
                    {
                        // add to offer
                        if(pTrade->uOfferedPropertyCount < PROPERTY_ARRAY_SIZE)
                        {
                            pTrade->auOfferedProperties[pTrade->uOfferedPropertyCount] = uPropIdx;
                            pTrade->uOfferedPropertyCount++;
                        }
                    }
                }
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            // toggle requested properties (200-299)
            if(iChoice >= 200 && iChoice < 300)
            {
                uint8_t uPropArrayIdx = (uint8_t)(iChoice - 200);
                mPlayer* pTargetPlayer = &pGame->amPlayers[pTrade->uTargetPlayer];
                
                if(uPropArrayIdx < pTargetPlayer->uPropertyCount)
                {
                    uint8_t uPropIdx = pTargetPlayer->auPropertiesOwned[uPropArrayIdx];
                    
                    // check if already in request
                    bool bFound = false;
                    uint8_t uFoundIdx = 0;
                    for(uint8_t i = 0; i < pTrade->uRequestedPropertyCount; i++)
                    {
                        if(pTrade->auRequestedProperties[i] == uPropIdx)
                        {
                            bFound = true;
                            uFoundIdx = i;
                            break;
                        }
                    }
                    
                    if(bFound)
                    {
                        // remove from request
                        for(uint8_t i = uFoundIdx; i < pTrade->uRequestedPropertyCount - 1; i++)
                        {
                            pTrade->auRequestedProperties[i] = pTrade->auRequestedProperties[i + 1];
                        }
                        pTrade->uRequestedPropertyCount--;
                    }
                    else
                    {
                        // add to request
                        if(pTrade->uRequestedPropertyCount < PROPERTY_ARRAY_SIZE)
                        {
                            pTrade->auRequestedProperties[pTrade->uRequestedPropertyCount] = uPropIdx;
                            pTrade->uRequestedPropertyCount++;
                        }
                    }
                }
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            // adjust offered money (300-399: subtract $100, 400-499: add $100)
            if(iChoice >= 300 && iChoice < 400)
            {
                if(pTrade->uOfferedMoney >= 100)
                    pTrade->uOfferedMoney -= 100;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            if(iChoice >= 400 && iChoice < 500)
            {
                if(pTrade->uOfferedMoney + 100 <= pCurrentPlayer->uMoney)
                    pTrade->uOfferedMoney += 100;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            // adjust requested money (500-599: subtract $100, 600-699: add $100)
            if(iChoice >= 500 && iChoice < 600)
            {
                if(pTrade->uRequestedMoney >= 100)
                    pTrade->uRequestedMoney -= 100;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            if(iChoice >= 600 && iChoice < 700)
            {
                mPlayer* pTargetPlayer = &pGame->amPlayers[pTrade->uTargetPlayer];
                if(pTrade->uRequestedMoney + 100 <= pTargetPlayer->uMoney)
                    pTrade->uRequestedMoney += 100;
                pTrade->bShowedMenu = false;
                return PHASE_RUNNING;
            }
            
            break;
        }
        
        case TRADE_STEP_AWAITING_RESPONSE:
        {
            if(iChoice == 1) // accept
            {
                // execute trade
                mPlayer* pTargetPlayer = &pGame->amPlayers[pTrade->uTargetPlayer];
                
                // transfer offered properties
                for(uint8_t i = 0; i < pTrade->uOfferedPropertyCount; i++)
                {
                    uint8_t uPropIdx = pTrade->auOfferedProperties[i];
                    m_transfer_property(pGame, uPropIdx, pGame->uCurrentPlayerIndex, pTrade->uTargetPlayer);
                }
                
                // transfer requested properties
                for(uint8_t i = 0; i < pTrade->uRequestedPropertyCount; i++)
                {
                    uint8_t uPropIdx = pTrade->auRequestedProperties[i];
                    m_transfer_property(pGame, uPropIdx, pTrade->uTargetPlayer, pGame->uCurrentPlayerIndex);
                }
                
                // transfer money
                pCurrentPlayer->uMoney -= pTrade->uOfferedMoney;
                pTargetPlayer->uMoney += pTrade->uOfferedMoney;
                
                pCurrentPlayer->uMoney += pTrade->uRequestedMoney;
                pTargetPlayer->uMoney -= pTrade->uRequestedMoney;
                
                m_set_notification(pGame, "Trade completed!");
                
                pGame->bShowTradeMenu = false;
                m_pop_phase(pFlow);
                return PHASE_RUNNING;
            }
            else if(iChoice == 2) // reject
            {
                m_set_notification(pGame, "Trade rejected");
                pGame->bShowTradeMenu = false;
                m_pop_phase(pFlow);
                return PHASE_RUNNING;
            }
            break;
        }
    }
    
    pTrade->bShowedMenu = false;
    return PHASE_RUNNING;
}

void
m_transfer_property(mGameData* pGame, uint8_t uPropIdx, uint8_t uFromPlayer, uint8_t uToPlayer)
{
    mProperty* pProp = &pGame->amProperties[uPropIdx];
    mPlayer* pFrom = &pGame->amPlayers[uFromPlayer];
    mPlayer* pTo = &pGame->amPlayers[uToPlayer];
    
    // remove from old owner
    for(uint8_t i = 0; i < pFrom->uPropertyCount; i++)
    {
        if(pFrom->auPropertiesOwned[i] == uPropIdx)
        {
            // shift remaining properties down
            for(uint8_t j = i; j < pFrom->uPropertyCount - 1; j++)
            {
                pFrom->auPropertiesOwned[j] = pFrom->auPropertiesOwned[j + 1];
            }
            pFrom->auPropertiesOwned[pFrom->uPropertyCount - 1] = BANK_PLAYER_INDEX;
            pFrom->uPropertyCount--;
            break;
        }
    }
    
    // add to new owner
    pTo->auPropertiesOwned[pTo->uPropertyCount] = uPropIdx;
    pTo->uPropertyCount++;
    
    // update property ownership
    pProp->uOwnerIndex = uToPlayer;
}

// Add this to monopoly.c

bool
m_check_game_over(mGameData* pGame)
{
    // check if only one player remains
    if(pGame->uActivePlayers <= 1)
    {
        pGame->bIsRunning = false;
        
        // find and announce the winner
        for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
        {
            if(!pGame->amPlayers[i].bIsBankrupt)
            {
                m_set_notification(pGame, "GAME OVER! Player %d wins!", i + 1);
                return true;
            }
        }
        
        // edge case: all players bankrupt somehow (shouldn't happen)
        m_set_notification(pGame, "GAME OVER! No winners.");
        return true;
    }
    
    return false;
}