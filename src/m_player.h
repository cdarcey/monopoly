
#ifndef M_PLAYER_H
#define M_PLAYER_H

#include <stdint.h> // uint
#include <stdlib.h> // malloc
#include <stdbool.h> // bool



// ==================== FORWARD DECLERATIONS ==================== //
typedef struct _mGameStartSettings mGameStartSettings;
typedef struct _mDice mDice;
typedef enum _mPropertyName mPropertyName; 
typedef enum _mRailroadName mRailroadName;
typedef enum _mUtilityName  mUtilityName;
typedef enum _mBoardSquares mBoardSquares;


// ==================== ENUMS ==================== //
typedef enum _mPlayerPiece
{
    BATTLESHIP,
    RACE_CAR,
    TOP_HAT,
    BOOT,
    THIMBLE,
    IRON,
    CANNON,
    LANTERN,
    PURSE,
    ROCKING_HORSE,
    PLAYER_PIECE_TOTAL,
    NO_PIECE_ASSIGNED,
    NOT_PLAYING
} mPlayerPiece;

typedef enum mPlayerNumber
{
    PLAYER_ONE,
    PLAYER_TWO,
    PLAYER_THREE,
    PLAYER_FOUR,
    PLAYER_FIVE,
    PLAYER_SIX,
    PLAYER_SEVEN,
    PLAYER_EIGHT,
    PLAYER_NINE,
    PLAYER_TEN,
    MAX_NUMBER_OF_PLAYERS,
    NO_PLAYER
} mPlayerNumber;

// ==================== STRUCTS ==================== //
typedef struct _mPlayer
{
    uint32_t       uMoney;
    uint8_t        uPosition;
    uint8_t        uJailTurns;
    mPropertyName  ePropertyOwned[PROP_OWNED_WITH_BUFFER]; // more than total prop count as a buffer for trade system
    mRailroadName  eRailroadOwned[UTIL_OWNED_WITH_BUFFER]; // more than total rail count as a buffer for trade system
    mUtilityName   eUtilityOwned[RAIL_OWNED_WITH_BUFFER]; // more than total util count as a buffer for trade system
    mPlayerPiece   ePiece;
    mPlayerNumber  ePlayerTurnPosition;
    bool           bInJail;
    bool           bGetOutOfJailFreeCard;
    bool           bBankrupt;
    bool           bPlayerIsAI;
} mPlayer;

// ==================== PLAYER CREATION ==================== //
mPlayer* m_create_player   (uint8_t uPlayerPositionInIndex, mGameStartSettings mSettings); 
void     m_set_player_piece(mGameData* mGame, mPlayerPiece playerPiece, mPlayerNumber playerTurnPosition);
void     m_set_player_as_AI (mGameData* mGame, mPlayerNumber playerTurnPosition);

// ==================== MOVEMENT ==================== //
void m_move_player   (mPlayer* mPlayer, mDice* mRolledDice);
void m_move_player_to(mPlayer* currentPlayer, mBoardSquares destination);

// ==================== FINANCIAL ==================== //
void m_trigger_bankruptcy(mGameData* mGame, mPlayerNumber playerBankrupting, bool bOwesPlayer, mPlayerNumber playerOwed);

// ==================== HELPERS ==================== //
void        m_show_player_status       (mPlayer* mCurrentPlayer);
void        m_show_player_assets       (mPlayer* mCurrentPlayer);
const char* m_player_position_to_string(uint8_t uPosition);
void        m_defrag_asset_arrays      (mPlayer *mPlayer);


#endif