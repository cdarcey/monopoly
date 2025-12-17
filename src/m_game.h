#ifndef M_GAME_H
#define M_GAME_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct _mGameData mGameData;
typedef struct _mDice mDice;
typedef struct _mPlayer mPlayer;
typedef struct _mProperty mProperty;
typedef struct _mRailroad mRailroad;
typedef struct _mUtility mUtility;

// ==================== PHASE SYSTEM ==================== //

typedef enum _ePhaseResult
{
    PHASE_RUNNING,
    PHASE_COMPLETE
} ePhaseResult;

// Phase function pointer type
typedef ePhaseResult (*fPhaseFunc)(void* pData, float fDeltaTime, struct _mGameFlow* pFlow);

// Phase data structures
typedef struct _mPreRollData
{
    mPlayer* pPlayer;
    bool     bWaitingForRoll;
} mPreRollData;

typedef struct _mPostRollData
{
    mPlayer* pPlayer;
    bool     bFirstCall;
} mPostRollData;

// Game flow system
typedef struct _mGameFlow 
{
    fPhaseFunc pfCurrentPhase;
    void*      pCurrentPhaseData;
    
    // Phase stack for nested operations
    fPhaseFunc apReturnStack[16];
    void* apReturnDataStack[16];
    int iStackDepth;
    
    // Game data & glfw window 
    mGameData* pGameData;
    
    // Input system
    void* pInputWindow; // void for now TODO: properly track when graphics are added
    bool bInputReceived;
    int  iInputValue;
    char szInputString[256];
} mGameFlow;

// ==================== PHASE SYSTEM FUNCTIONS ==================== //

// Core phase management
void m_push_phase       (mGameFlow* tFlow, fPhaseFunc tNewPhase, void* pNewData);
void m_pop_phase        (mGameFlow* tFlow);
void m_run_current_phase(mGameFlow* tFlow, float fDeltaTime);

// Input helpers TODO: replace with proper input handling once graphics are decided on
bool m_check_input_int   (int* pOut, void* tContextWindow);
bool m_check_input_yes_no(bool* pOut, void* tContextWindow);
bool m_check_input_space (void* tContextWindow);
bool m_check_input_enter (void* tContextWindow);
void m_clear_input       (mGameFlow* tFlow);

// Phase functions
ePhaseResult m_phase_pre_roll (void* pData, float fDeltaTime, mGameFlow* tGameFlow);
ePhaseResult m_phase_post_roll(void* pData, float fDeltaTime, mGameFlow* tGameFlow);

// ==================== GAME STATE ENUMS ==================== //

typedef enum _eGameState{
    GAME_STARTUP,
    GAME_RUNNING,
    GAME_OVER
} eGameState;

typedef enum _eBoardSquareType
{
    GO_SQUARE_TYPE,
    PROPERTY_SQUARE_TYPE,
    RAILROAD_SQUARE_TYPE,
    UTILITY_SQUARE_TYPE,
    INCOME_TAX_SQUARE_TYPE,
    LUXURY_TAX_SQUARE_TYPE,
    CHANCE_CARD_SQUARE_TYPE,
    COMMUNITY_CHEST_SQUARE_TYPE,
    FREE_PARKING_SQUARE_TYPE,
    JAIL_SQUARE_TYPE,
    GO_TO_JAIL_SQUARE_TYPE
} eBoardSquareType;

// ==================== CORE GAME LOGIC (KEEP THESE) ==================== //

void m_next_player_turn(mGameData* mGame);
void m_game_over_check (mGameData* mGame);
void m_roll_dice       (mDice* game_dice);

// Jail helpers
bool m_attempt_jail_escape(mGameData* mGame);
bool m_use_jail_free_card (mPlayer* mPlayerInJail);

// Financial helpers
bool m_can_player_afford        (mPlayer* mCurrentPlayer, uint32_t uExpense);
bool m_attempt_emergency_payment(mGameData* mGame, mPlayer* mCcurrentPlayer, uint32_t uAmount);

// Helpers
eBoardSquareType m_get_square_type          (uint32_t uPlayerPosition);
uint8_t          m_get_empty_prop_owned_slot(mPlayer* mPlayer);
uint8_t          m_get_empty_rail_owned_slot(mPlayer* mPlayer);
uint8_t          m_get_empty_util_owned_slot(mPlayer* mPlayer);

// Forced actions
void m_forced_release           (mGameData* mGame);
bool m_player_has_forced_actions(mGameData* mGame);
void m_handle_emergency_actions (mGameData* mGame);

#endif // M_GAME_H