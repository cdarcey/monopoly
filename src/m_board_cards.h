
#ifndef M_BOARD_CARDS_H
#define M_BOARD_CARDS_H

#include <stdint.h> // uint
#include <stdbool.h> // bool

typedef struct _mPlayer mPlayer;
typedef struct _mGameData mGameData;


// ==================== ENUMS ==================== //

typedef enum _mIndexBufferNames
{
    CHANCE_INDEX_BUFFER,
    COMM_CHEST_INDEX_BUFFER,
    TOTAL_INDEX_BUFFERS
} mIndexBufferNames;


// ==================== STRUCTS ==================== //
typedef struct _mChanceCard
{
    char        cDescription[200];
    uint32_t    uID;
} mChanceCard;

typedef struct _mCommunityChestCard
{
    char        cDescription[200];
    uint32_t    uID;
} mCommunityChestCard;

typedef struct _mDeckIndexBuffer
{
    uint8_t uIndexBuffer[16];
    uint8_t uCurrentCard;
} mDeckIndexBuffer;


// ==================== CARD FUNCTIONS ==================== //
void m_execute_community_chest_card(mGameData* mGame);
void m_execute_chance_card         (mGameData* mGame);


// ==================== HELPERS ==================== //
mDeckIndexBuffer     m_create_deck_index_buffer (void);
mChanceCard*         m_draw_chance_card         (mChanceCard* mCurrentChanceDeck, mDeckIndexBuffer* mDeckIndex, bool bJailFreeOwned);
mCommunityChestCard* m_draw_community_chest_card(mCommunityChestCard* mCurrentCommChestDeck, mDeckIndexBuffer* mDeckIndex, bool bJailFreeOwned);
bool                 m_get_out_jail_card_owned  (mGameData* mGame);

#endif