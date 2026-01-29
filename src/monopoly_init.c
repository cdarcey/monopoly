
#include <string.h> // memset etc...
#include <time.h> // srand
#include <stdio.h> // printf
#include <stdarg.h>  // va_copy, va_start, va_end 

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

#include "monopoly_init.h"


static ePropertyColor
m_string_to_color_enum(const char* cColor)
{
    if(strcmp(cColor, "brown")      == 0) return COLOR_BROWN;
    if(strcmp(cColor, "light_blue") == 0) return COLOR_LIGHT_BLUE;
    if(strcmp(cColor, "pink")       == 0) return COLOR_PINK;
    if(strcmp(cColor, "orange")     == 0) return COLOR_ORANGE;
    if(strcmp(cColor, "red")        == 0) return COLOR_RED;
    if(strcmp(cColor, "yellow")     == 0) return COLOR_YELLOW;
    if(strcmp(cColor, "green")      == 0) return COLOR_GREEN;
    if(strcmp(cColor, "dark_blue")  == 0) return COLOR_DARK_BLUE;
    if(strcmp(cColor, "railroad")   == 0) return COLOR_RAILROAD;
    if(strcmp(cColor, "utility")    == 0) return COLOR_UTILITY;
    return COLOR_BROWN; // default
}

static ePropertyType
m_string_to_type_enum(const char* cType)
{
    if(strcmp(cType, "street")   == 0) return PROPERTY_TYPE_STREET;
    if(strcmp(cType, "railroad") == 0) return PROPERTY_TYPE_RAILROAD;
    if(strcmp(cType, "utility")  == 0) return PROPERTY_TYPE_UTILITY;
    return PROPERTY_TYPE_STREET; // default
}

// ==================== DECK INITIALIZATION ==================== //

void
m_shuffle_deck(mDeckState* pDeck)
{
    // initialize indices (16 cards 0-15)
    for(uint8_t i = 0; i < 16; i++)
    {
        pDeck->auIndices[i] = i;
    }

    // fisher-yates shuffle
    for(uint8_t i = 15; i > 0; i--)
    {
        uint8_t j = rand() % (i + 1);
        uint8_t temp = pDeck->auIndices[i];
        pDeck->auIndices[i] = pDeck->auIndices[j];
        pDeck->auIndices[j] = temp;
    }

    pDeck->uCurrentIndex = 0;
}

// ==================== PLAYER INITIALIZATION ==================== //

static void
m_init_players(mPlayer* pPlayers, uint8_t uPlayerCount, uint32_t uStartingMoney)
{
    for(uint8_t i = 0; i < uPlayerCount; i++)
    {
        pPlayers[i].uMoney = uStartingMoney;
        pPlayers[i].uPosition = 0;
        pPlayers[i].uJailTurns = 0;
        pPlayers[i].uPropertyCount = 0;
        pPlayers[i].ePiece = PIECE_NONE;
        pPlayers[i].bHasJailFreeCard = false;
        pPlayers[i].bIsBankrupt = false;

        // initialize property array with empty slots
        for(uint8_t j = 0; j < PROPERTY_ARRAY_SIZE; j++)
        {
            pPlayers[i].auPropertiesOwned[j] = BANK_PLAYER_INDEX;
        }
    }
}

// ==================== GAME INITIALIZATION ==================== //

mGameData*
m_init_game(mGameSettings tSettings)
{
    // seed random number generator (for dice)
    srand((unsigned int)time(NULL));

    // allocate and zero-initialize game data
    mGameData* pGame = calloc(1, sizeof(mGameData));
    if(!pGame)
    {
        printf("Failed to allocate game data\n");
        return NULL;
    }

    // ==================== LOAD PROPERTIES ==================== //
    FILE* jsonFileProperties = fopen("../../monopoly/game_data/properties.json", "r");
    if(!jsonFileProperties)
    {
        printf("Failed to open properties.json\n");
        free(pGame);
        return NULL;
    }

    char jsonBufferProperties[8000] = {0};
    fread(jsonBufferProperties, 1, 8000, jsonFileProperties);
    fclose(jsonFileProperties);

    plJsonObject* tRootProperties = NULL;
    pl_load_json(jsonBufferProperties, &tRootProperties);

    uint32_t uPropertyCount = 0;
    plJsonObject* tPropertyArray = pl_json_array_member(tRootProperties, "properties", &uPropertyCount);

    for(uint32_t i = 0; i < uPropertyCount && i < TOTAL_PROPERTIES; i++)
    {
        plJsonObject* tProp = pl_json_member_by_index(tPropertyArray, i);
        
        char cName[50] = {0};
        char cType[20] = {0};
        char cColor[20] = {0};
        
        pl_json_string_member(tProp, "name", cName, 50);
        pl_json_string_member(tProp, "type", cType, 20);
        pl_json_string_member(tProp, "color", cColor, 20);
        
        strncpy(pGame->amProperties[i].cName, cName, 49);
        pGame->amProperties[i].eType = m_string_to_type_enum(cType);
        pGame->amProperties[i].eColor = m_string_to_color_enum(cColor);
        pGame->amProperties[i].uPrice = pl_json_uint_member(tProp, "price", 0);
        pGame->amProperties[i].uPosition = (uint8_t)pl_json_uint_member(tProp, "position", 0);
        pGame->amProperties[i].uMortgageValue = pl_json_uint_member(tProp, "mortgage", 0);
        pGame->amProperties[i].uHouseCost = pl_json_uint_member(tProp, "house_cost", 0);
        
        // read rent array
        uint32_t uRentCount = 6;
        int32_t aiRent[6] = {0}; // function requires int type 
        pl_json_int_array_member(tProp, "rent", aiRent, &uRentCount);
        
        for(uint8_t j = 0; j < 6; j++)
        {
            pGame->amProperties[i].auRentWithHouses[j] = (uint32_t)aiRent[j];
        }
        
        // set base rent and monopoly rent from rent array
        pGame->amProperties[i].uRentBase = pGame->amProperties[i].auRentWithHouses[0];
        pGame->amProperties[i].uRentMonopoly = pGame->amProperties[i].auRentWithHouses[0] * 2;
        
        // initialize default values
        pGame->amProperties[i].uOwnerIndex = BANK_PLAYER_INDEX;
        pGame->amProperties[i].bIsMortgaged = false;
        pGame->amProperties[i].uHouses = 0;
        pGame->amProperties[i].bHasHotel = false;
    }

    pl_unload_json(&tRootProperties);

    // ==================== LOAD CHANCE CARDS ==================== //
    FILE* jsonFileChance = fopen("../../monopoly/game_data/chance_cards.json", "r");
    if(!jsonFileChance)
    {
        printf("Failed to open chance_cards.json\n");
        free(pGame);
        return NULL;
    }

    char jsonBufferChance[4000] = {0};
    fread(jsonBufferChance, 1, 4000, jsonFileChance);
    fclose(jsonFileChance);

    plJsonObject* tRootChance = NULL;
    pl_load_json(jsonBufferChance, &tRootChance);

    uint32_t uChanceCount = 0;
    plJsonObject* tChanceArray = pl_json_array_member(tRootChance, "chance_cards", &uChanceCount);

    for(uint32_t i = 0; i < uChanceCount && i < 16; i++)
    {
        plJsonObject* tCard = pl_json_member_by_index(tChanceArray, i);
        
        pGame->amChanceCards[i].uCardID = pl_json_uint_member(tCard, "id", 0);
        pl_json_string_member(tCard, "description", pGame->amChanceCards[i].cDescription, 199);
    }

    pl_unload_json(&tRootChance);

    // ==================== LOAD COMMUNITY CHEST CARDS ==================== //
    FILE* jsonFileCommChest = fopen("../../monopoly/game_data/community_chest_cards.json", "r");
    if(!jsonFileCommChest)
    {
        printf("Failed to open community_chest_cards.json\n");
        free(pGame);
        return NULL;
    }

    char jsonBufferCommChest[4000] = {0};
    fread(jsonBufferCommChest, 1, 4000, jsonFileCommChest);
    fclose(jsonFileCommChest);

    plJsonObject* tRootCommChest = NULL;
    pl_load_json(jsonBufferCommChest, &tRootCommChest);

    uint32_t uCommChestCount = 0;
    plJsonObject* tCommChestArray = pl_json_array_member(tRootCommChest, "community_chest_cards", &uCommChestCount);

    for(uint32_t i = 0; i < uCommChestCount && i < 16; i++)
    {
        plJsonObject* tCard = pl_json_member_by_index(tCommChestArray, i);
        
        pGame->amCommunityChestCards[i].uCardID = pl_json_uint_member(tCard, "id", 0);
        pl_json_string_member(tCard, "description", pGame->amCommunityChestCards[i].cDescription, 199);
    }

    pl_unload_json(&tRootCommChest);

    // ==================== INITIALIZE OTHER COMPONENTS ==================== //
    
    // initialize game state
    m_shuffle_deck(&pGame->tChanceDeck);
    m_shuffle_deck(&pGame->tCommunityChestDeck);
    m_init_players(pGame->amPlayers, tSettings.uPlayerCount, tSettings.uStartingMoney);
    pGame->tDice.uDie1 = 1;
    pGame->tDice.uDie2 = 1;
    pGame->uPlayerCount = tSettings.uPlayerCount;
    pGame->uCurrentPlayerIndex = PLAYER_ONE_ARRAY_INDEX;
    pGame->uActivePlayers = tSettings.uPlayerCount;
    pGame->uRoundCount = 0;
    pGame->uJailFine = tSettings.uJailFine;
    pGame->eState = GAME_STATE_RUNNING;
    pGame->bIsRunning = true;
    pGame->uGlobalHotelSupply = 12;
    pGame->uGlobalHouseSupply = 32;

    return pGame;
}

// ==================== MEMORY CLEANUP ==================== //

void
m_free_game(mGameData* pGame)
{
    if(pGame)
    {
        free(pGame);
    }
}