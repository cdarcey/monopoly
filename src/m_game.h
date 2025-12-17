
#ifndef M_GAME_H
#define M_GAME_H

#include <stdint.h>
#include <stdbool.h>

// forward declarations
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

// function pointer type for phase functions
typedef ePhaseResult (*fPhaseFunc)(void* pData, float fDeltaTime, struct _mGameFlow* pFlow);

// phase data for pre-roll menu and actions
typedef struct _mPreRollData
{
    mPlayer* pPlayer;
    bool     bShowedMenu;
    int      iMenuSelection;
} mPreRollData;

// phase data for post-roll square handling
typedef struct _mPostRollData
{
    mPlayer* pPlayer;
    bool     bProcessedSquare;
    bool     bShowedMenu;
    int      iMenuSelection;
} mPostRollData;

// phase data for jail turn handling
typedef struct _mJailData
{
    mPlayer* pPlayer;
    bool     bShowedMenu;
    int      iChoice;
    bool     bRolled;
} mJailData;

// phase data for property auction
typedef struct _mAuctionData
{
    void*    pAsset;        // property, railroad, or utility pointer
    uint8_t  uAssetType;    // 0=property, 1=railroad, 2=utility
    uint32_t uHighestBid;
    int      iBidOwner;
    bool     bWaitingForBids;
} mAuctionData;

// phase data for trade negotiation
typedef struct _mTradeData
{
    mPlayer* pInitiator;
    mPlayer* pPartner;
    bool     bNegotiating;
} mTradeData;

// phase data for property management menu
typedef struct _mPropertyManagementData
{
    mPlayer* pPlayer;
    bool     bShowedMenu;
    int      iSelection;
} mPropertyManagementData;

// manages game phases, input, and phase stack
typedef struct _mGameFlow 
{
    fPhaseFunc pfCurrentPhase;
    void*      pCurrentPhaseData;
    
    // stack for nested phases like auctions and trades
    fPhaseFunc apReturnStack[16];
    void*      apReturnDataStack[16];
    int        iStackDepth;
    
    // reference to main game data
    mGameData* pGameData;
    
    // platform-specific input context (e.g. sdl window)
    void* pInputContext;
    bool  bInputReceived;
    int   iInputValue;
    char  szInputString[256];
    
    // accumulated time for animations
    float fAccumulatedTime;
    
} mGameFlow;

// ==================== PHASE SYSTEM FUNCTIONS ==================== //

// phase management
void         m_init_game_flow   (mGameFlow* pFlow, mGameData* pGame, void* pInputContext);
void         m_push_phase       (mGameFlow* pFlow, fPhaseFunc pfNewPhase, void* pNewData);
void         m_pop_phase        (mGameFlow* pFlow);
void         m_run_current_phase(mGameFlow* pFlow, float fDeltaTime);

// input handling
void m_set_input_int   (mGameFlow* pFlow, int iValue);
void m_set_input_string(mGameFlow* pFlow, const char* szValue);
void m_clear_input     (mGameFlow* pFlow);
bool m_is_waiting_input(mGameFlow* pFlow);

// main game phases
ePhaseResult m_phase_pre_roll       (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_post_roll      (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_end_turn       (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_jail           (void* pData, float fDeltaTime, mGameFlow* pFlow);

// sub-phases for nested operations
ePhaseResult m_phase_auction        (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_trade          (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_property_mgmt  (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_pay_rent       (void* pData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_draw_card      (void* pData, float fDeltaTime, mGameFlow* pFlow);

// ==================== GAME STATE ENUMS ==================== //

typedef enum _eGameState
{
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

// ==================== CORE GAME LOGIC ==================== //

void m_next_player_turn(mGameData* pGame);
void m_game_over_check (mGameData* pGame);
void m_roll_dice       (mDice* pDice);

// jail mechanics
bool m_attempt_jail_escape(mGameData* pGame);
bool m_use_jail_free_card (mPlayer* pPlayer);

// financial operations
bool m_can_player_afford        (mPlayer* pPlayer, uint32_t uExpense);
bool m_attempt_emergency_payment(mGameData* pGame, mPlayer* pPlayer, uint32_t uAmount);

// utility functions
eBoardSquareType m_get_square_type          (uint32_t uPlayerPosition);
uint8_t          m_get_empty_prop_owned_slot(mPlayer* pPlayer);
uint8_t          m_get_empty_rail_owned_slot(mPlayer* pPlayer);
uint8_t          m_get_empty_util_owned_slot(mPlayer* pPlayer);

// forced actions when player can't afford payments
void m_forced_release           (mGameData* pGame);
bool m_player_has_forced_actions(mGameData* pGame);
void m_handle_emergency_actions (mGameData* pGame);

#endif // M_GAME_H
