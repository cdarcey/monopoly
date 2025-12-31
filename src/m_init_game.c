

#include <stdlib.h> // srand()
#include <time.h> // time
#include <stdio.h> // printf
#include <string.h> // strings
#include <Windows.h> // debugbreak


#include "m_init_game.h"



// ==================== GAME INITIALIZATION ==================== //
mGameData*
m_init_game(mGameStartSettings mSettings)
{
    // seeding for random number gen in some game functions
    srand((unsigned int)time(NULL));
    mGameData* mGame = calloc(1, sizeof(mGameData));  // Using calloc to zero-initialize

    // ==================== PROPERTIES ==================== //
    // Open and read JSON file
    FILE* jsonFileProperties = fopen("../../monopoly/game_data/properties.json", "r");
    if (!jsonFileProperties) 
    {
        printf("Failed to open properties.json\n");
        free(mGame);
        return NULL;
    }

    char jsonFileBufferProperties[6000] = {0};
    fread(jsonFileBufferProperties, 1, 6000, jsonFileProperties);
    fclose(jsonFileProperties);

    // load in root object
    plJsonObject* tRootObjBoardSquares = NULL;
    pl_load_json(jsonFileBufferProperties, &tRootObjBoardSquares);

    // ===== PROPERTIES =====
    uint32_t uPropCount = 0;
    plJsonObject* tProperty = pl_json_array_member(tRootObjBoardSquares, "properties", &uPropCount);

    for(uint32_t uPropertyIndex = 0; uPropertyIndex < uPropCount; uPropertyIndex++)
    {
        if (uPropertyIndex >= PROPERTY_TOTAL)
        {
            printf("Warning: Too many properties in JSON (max %d)\n", PROPERTY_TOTAL);
            break;
        }

        char cBufferForEnumConversion[30] = {0};
        plJsonObject* tPropElement = pl_json_member_by_index(tProperty, uPropertyIndex);
        pl_json_string_member(tPropElement, "name", mGame->mGameProperties[uPropertyIndex].cName, 30);
        pl_json_string_member(tPropElement, "color", cBufferForEnumConversion, 30);
        mGame->mGameProperties[uPropertyIndex].eColor = m_string_color_to_enum(cBufferForEnumConversion);
        mGame->mGameProperties[uPropertyIndex].uPrice = pl_json_uint_member(tPropElement, "price", 0);
        uint32_t rentCount = 6;
        pl_json_int_array_member(tPropElement, "rent", mGame->mGameProperties[uPropertyIndex].iRent, &rentCount);
        mGame->mGameProperties[uPropertyIndex].uHouseCost = pl_json_uint_member(tPropElement, "house_cost", 0);
        mGame->mGameProperties[uPropertyIndex].uMortgage = pl_json_uint_member(tPropElement, "mortgage", 0);
        mGame->mGameProperties[uPropertyIndex].uPropertiesInSet = pl_json_uint_member(tPropElement, "set_size", 0);

        // Initialize default values
        mGame->mGameProperties[uPropertyIndex].uNumberOfHotels = 0; 
        mGame->mGameProperties[uPropertyIndex].uNumberOfHouses = 0;
        mGame->mGameProperties[uPropertyIndex].eOwner = NO_PLAYER;
        mGame->mGameProperties[uPropertyIndex].bOwned = false;
        mGame->mGameProperties[uPropertyIndex].bMortgaged = false;

    }

    // ===== RAILROADS =====
    uint32_t uRailroadCount = 0;
    plJsonObject* tRailroad = pl_json_array_member(tRootObjBoardSquares, "railroads", &uRailroadCount);

    for(uint32_t uRailroadIndex = 0; uRailroadIndex < uRailroadCount; uRailroadIndex++)
    {
        if (uRailroadIndex >= RAILROAD_TOTAL)
        {
            printf("Warning: Too many railroads in JSON (max %d)\n", RAILROAD_TOTAL);
            break;
        }

        plJsonObject* tRailElement = pl_json_member_by_index(tRailroad, uRailroadIndex);
        pl_json_string_member(tRailElement, "name", mGame->mGameRailroads[uRailroadIndex].cName, 50);
        uint32_t rentCount = 4;
        pl_json_int_array_member(tRailElement, "rent", mGame->mGameRailroads[uRailroadIndex].iRent, &rentCount);
        mGame->mGameRailroads[uRailroadIndex].uPrice = pl_json_uint_member(tRailElement, "price", 0);
        mGame->mGameRailroads[uRailroadIndex].uMortgage = pl_json_uint_member(tRailElement, "mortgage", 0);

        // Initialize default values
        mGame->mGameRailroads[uRailroadIndex].eOwner = NO_PLAYER;
        mGame->mGameRailroads[uRailroadIndex].bOwned = false;
    }

    // ===== UTILITIES =====
    uint32_t uUtilityCount = 0;
    plJsonObject* tUtility = pl_json_array_member(tRootObjBoardSquares, "utilities", &uUtilityCount);

    for(uint32_t uUtilityIndex = 0; uUtilityIndex < uUtilityCount; uUtilityIndex++)
    {
        if (uUtilityIndex >= UTILITY_TOTAL)
        {
            printf("Warning: Too many utilities in JSON (max %d)\n", UTILITY_TOTAL);
            break;
        }

        plJsonObject* tUtilElement = pl_json_member_by_index(tUtility, uUtilityIndex);
        pl_json_string_member(tUtilElement, "name", mGame->mGameUtilities[uUtilityIndex].cName, 50);
        mGame->mGameUtilities[uUtilityIndex].uPrice = pl_json_uint_member(tUtilElement, "price", 0);
        mGame->mGameUtilities[uUtilityIndex].uMortgage = pl_json_uint_member(tUtilElement, "mortgage", 0);

        // Initialize default values
        mGame->mGameUtilities[uUtilityIndex].eOwner = NO_PLAYER;
        mGame->mGameUtilities[uUtilityIndex].bOwned = false;
    }

    pl_unload_json(&tRootObjBoardSquares);

    // ==================== CHANCE CARDS ==================== //
    FILE* jsonFileChanceCards = fopen("../../monopoly/game_data/chance_cards.json", "r");
    if (!jsonFileChanceCards) {
        printf("Failed to open chance_cards.json\n");
        return NULL;
    }

    char jsonFileBufferChanceCards[4000] = {0};
    fread(jsonFileBufferChanceCards, 1, 4000, jsonFileChanceCards);
    fclose(jsonFileChanceCards);

    plJsonObject* tRootObjChanceCards = NULL;
    pl_load_json(jsonFileBufferChanceCards, &tRootObjChanceCards);

    uint32_t uChanceCardCount = 0;
    plJsonObject* tChanceCard = pl_json_array_member(tRootObjChanceCards, "chance_cards", &uChanceCardCount);

    for(uint32_t uChanceCardIndex = 0; uChanceCardIndex < uChanceCardCount; uChanceCardIndex++)
    {
        if (uChanceCardIndex >= TOTAL_CHANCE_CARDS) 
        {
            printf("Warning: Too many chance cards in JSON (max 16)\n");
            break;
        }

        plJsonObject* tChanCarElement = pl_json_member_by_index(tChanceCard, uChanceCardIndex);
        mGame->mGameChanceCards[uChanceCardIndex].uID = pl_json_uint_member(tChanCarElement, "id", 0);
        pl_json_string_member(tChanCarElement, "description", mGame->mGameChanceCards[uChanceCardIndex].cDescription, 200);
    }
    pl_unload_json(&tRootObjChanceCards);

    // ==================== COMMUNITY CHEST CARDS ==================== //
    FILE* jsonFileCommChest = fopen("../../monopoly/game_data/community_chest_cards.json", "r");
    if (!jsonFileCommChest) 
    {
        printf("Failed to open community_chest_cards.json\n");
        return NULL;
    }

    char jsonFileBufferCommChest[4000] = {0};
    fread(jsonFileBufferCommChest, 1, 4000, jsonFileCommChest);
    fclose(jsonFileCommChest);

    plJsonObject* tRootObjCommChest = NULL;
    pl_load_json(jsonFileBufferCommChest, &tRootObjCommChest);

    uint32_t uCommChestCount = 0;
    plJsonObject* tCommChest = pl_json_array_member(tRootObjCommChest, "community_chest_cards", &uCommChestCount);

    for(uint32_t uCommChestIndex = 0; uCommChestIndex < uCommChestCount; uCommChestIndex++) 
    {
        if (uCommChestIndex >= TOTAL_COMM_CHEST_CARDS) 
        {
            printf("Warning: Too many community chest cards in JSON (max 16)\n");
            break;
        }

        plJsonObject* tCommChestElement = pl_json_member_by_index(tCommChest, uCommChestIndex);
        mGame->mGameCommChesCards[uCommChestIndex].uID = pl_json_uint_member(tCommChestElement, "id", 0);
        pl_json_string_member(tCommChestElement, "description", mGame->mGameCommChesCards[uCommChestIndex].cDescription, 200);
    }
    pl_unload_json(&tRootObjCommChest);

    // ==================== PLAYERS ==================== //
    for(uint8_t i = 0; i < mSettings.uStartingPlayerCount; i++)
    {
        mGame->mGamePlayers[i] = m_create_player(i, mSettings);
    }

    // ==================== GAME COMPONENTS ==================== //
    mGame->mGameDice = malloc(sizeof(mDice));
    if (!mGame->mGameDice) {
        printf("Failed to allocate memory for dice\n");
        return NULL;
    }

    mGame->mGameDeckIndexBuffers[CHANCE_INDEX_BUFFER] = m_create_deck_index_buffer();
    mGame->mGameDeckIndexBuffers[COMM_CHEST_INDEX_BUFFER] = m_create_deck_index_buffer();

    // ==================== GAME STATE ==================== //
    mGame->uStartingPlayerCount = mSettings.uStartingPlayerCount;
    mGame->uActivePlayers = mSettings.uStartingPlayerCount;
    mGame->uCurrentPlayer = PLAYER_ONE;
    mGame->uGameRoundCount = 0;
    mGame->bRunning = true;
    mGame->uJailFine = mSettings.uJailFine;


    return mGame;
}


// ==================== MEMORY ==================== //
void m_free_game_data(mGameData* mGame)
{
    if (!mGame)
    {
        return;
    }
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
    {
        if(mGame->mGamePlayers[i] == NULL)
        {
            break;
        }
        else
        {
            free(mGame->mGamePlayers[i]);
        }
    }
    if (mGame->mGameDice != NULL)
    {
        free(mGame->mGameDice);
    }
    free(mGame);
}

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"
