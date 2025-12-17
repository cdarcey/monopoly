#ifndef M_INIT_GAME_H
#define M_INIT_GAME_H

#include <assert.h>


#include "pl_json.h"
#include "m_property.h"
#include "m_board_cards.h"
#include "m_player.h"
#include "m_game.h"
#include <GLFW/glfw3.h>


#define MAX_HOUSES 4
#define TOTAL_CHANCE_CARDS 16
#define TOTAL_COMM_CHEST_CARDS TOTAL_CHANCE_CARDS
#define LUXURY_TAX 100
#define INCOME_TAX 100
#define UNIVERSAL_BASIC_INCOME 200


// ==================== ENUMS ==================== //
typedef enum _mBoardSquares
{
    GO_SQUARE,
    MEDITERRANEAN_AVENUE_SQUARE,
    COMMUNITY_CHEST_SQUARE_1,
    BALTIC_AVENUE_SQUARE,
    INCOME_TAX_SQUARE,
    READING_RAILROAD_SQUARE,
    ORIENTAL_AVENUE_SQUARE,
    CHANCE_SQUARE_1,
    VERMONT_AVENUE_SQUARE,
    CONNECTICUT_AVENUE_SQUARE,
    JAIL_SQUARE,
    ST_CHARLES_PLACE_SQUARE,
    ELECTRIC_COMPANY_SQUARE,
    STATES_AVENUE_SQUARE,
    VIRGINIA_AVENUE_SQUARE,
    PENNSYLVANIA_RAILROAD_SQUARE,
    ST_JAMES_PLACE_SQUARE,
    COMMUNITY_CHEST_SQUARE_2,
    TENNESSEE_AVENUE_SQUARE,
    NEW_YORK_AVENUE_SQUARE,
    FREE_PARKING_SQUARE,
    KENTUCKY_AVENUE_SQUARE,
    CHANCE_SQUARE_2,
    INDIANA_AVENUE_SQUARE,
    ILLINOIS_AVENUE_SQUARE,
    B_AND_O_RAILROAD_SQUARE,
    ATLANTIC_AVENUE_SQUARE,
    VENTNOR_AVENUE_SQUARE,
    WATER_WORKS_SQUARE,
    MARVIN_GARDENS_SQUARE,
    GO_TO_JAIL_SQUARE,
    PACIFIC_AVENUE_SQUARE,
    NORTH_CAROLINA_AVENUE_SQUARE,
    COMMUNITY_CHEST_SQUARE_3,
    PENNSYLVANIA_AVENUE_SQUARE,
    SHORT_LINE_RAILROAD_SQUARE,
    CHANCE_SQUARE_3,
    PARK_PLACE_SQUARE,
    LUXURY_TAX_SQUARE,
    BOARDWALK_SQUARE,
    TOTAL_BOARD_SQUARES
} mBoardSquares;


// ==================== STRUCTS ==================== //
typedef struct _mDice
{
    uint8_t dice_one;
    uint8_t dice_two;
} mDice;

typedef struct _mGameData
{
    mPlayer*            mGamePlayers[PLAYER_PIECE_TOTAL];
    mDice*              mGameDice;
    mProperty           mGameProperties[PROPERTY_TOTAL];
    mRailroad           mGameRailroads[RAILROAD_TOTAL];
    mUtility            mGameUtilities[UTILITY_TOTAL];
    mChanceCard         mGameChanceCards[TOTAL_CHANCE_CARDS];
    mCommunityChestCard mGameCommChesCards[TOTAL_COMM_CHEST_CARDS];
    mDeckIndexBuffer    mGameDeckIndexBuffers[TOTAL_INDEX_BUFFERS];

    // internal
    uint64_t             uGameRoundCount;
    uint32_t             uJailFine;
    uint8_t              uStartingPlayerCount;
    uint8_t              uCurrentPlayer;
    uint8_t              uActivePlayers; 
    eGameState           mCurrentState;
    bool                 bRunning;
} mGameData;

typedef struct _mGameStartSettings
{
    uint32_t uStartingMoney;
    uint16_t uStartingPlayerCount;
    uint16_t uJailFine;
} mGameStartSettings;


// ==================== GAME INITIALIZATION ==================== //
mGameData* m_init_game(mGameStartSettings mSettings);

// ==================== MEMORY ==================== //
void m_free_game_data(mGameData* mGame);


#endif