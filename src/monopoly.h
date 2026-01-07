#ifndef MONOPOLY_H
#define MONOPOLY_H

#include <stdint.h> // uint
#include <stdbool.h> // bool

// ==================== CONSTANTS ==================== //

#define MAX_PLAYERS 6
#define TOTAL_BOARD_SQUARES 40
#define TOTAL_PROPERTIES 28
#define TOTAL_CHANCE_CARDS 16
#define TOTAL_COMMUNITY_CHEST_CARDS 16
#define GO_MONEY 200
#define LUXURY_TAX 100
#define INCOME_TAX 200

// property ownership array sizes (with buffer for trading/selling)
#define PROPERTY_ARRAY_SIZE 35

// ==================== ENUMS ==================== //

// board square types
typedef enum _eSquareType
{
    SQUARE_GO,
    SQUARE_PROPERTY,
    SQUARE_CHANCE,
    SQUARE_COMMUNITY_CHEST,
    SQUARE_INCOME_TAX,
    SQUARE_LUXURY_TAX,
    SQUARE_JAIL,
    SQUARE_GO_TO_JAIL,
    SQUARE_FREE_PARKING
} eSquareType;

// property color sets
typedef enum _ePropertyColor
{
    COLOR_BROWN,
    COLOR_LIGHT_BLUE,
    COLOR_PINK,
    COLOR_ORANGE,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_DARK_BLUE,
    COLOR_RAILROAD,
    COLOR_UTILITY,
    COLOR_NONE
} ePropertyColor;

// property types
typedef enum _ePropertyType
{
    PROPERTY_TYPE_STREET,
    PROPERTY_TYPE_RAILROAD,
    PROPERTY_TYPE_UTILITY
} ePropertyType;

typedef enum _ePropertyArrayIndex
{
    // brown properties
    MEDITERRANEAN_AVENUE_PROPERTY_ARRAY_INDEX = 0,
    BALTIC_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // light blue properties
    ORIENTAL_AVENUE_PROPERTY_ARRAY_INDEX,
    VERMONT_AVENUE_PROPERTY_ARRAY_INDEX,
    CONNECTICUT_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // pink properties
    ST_CHARLES_PLACE_PROPERTY_ARRAY_INDEX,
    STATES_AVENUE_PROPERTY_ARRAY_INDEX,
    VIRGINIA_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // orange properties
    ST_JAMES_PLACE_PROPERTY_ARRAY_INDEX,
    TENNESSEE_AVENUE_PROPERTY_ARRAY_INDEX,
    NEW_YORK_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // red properties
    KENTUCKY_AVENUE_PROPERTY_ARRAY_INDEX,
    INDIANA_AVENUE_PROPERTY_ARRAY_INDEX,
    ILLINOIS_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // yellow properties
    ATLANTIC_AVENUE_PROPERTY_ARRAY_INDEX,
    VENTNOR_AVENUE_PROPERTY_ARRAY_INDEX,
    MARVIN_GARDENS_PROPERTY_ARRAY_INDEX,
    
    // green properties
    PACIFIC_AVENUE_PROPERTY_ARRAY_INDEX,
    NORTH_CAROLINA_AVENUE_PROPERTY_ARRAY_INDEX,
    PENNSYLVANIA_AVENUE_PROPERTY_ARRAY_INDEX,
    
    // dark blue properties
    PARK_PLACE_PROPERTY_ARRAY_INDEX,
    BOARDWALK_PROPERTY_ARRAY_INDEX,
    
    // railroads
    READING_RAILROAD_PROPERTY_ARRAY_INDEX,
    PENNSYLVANIA_RAILROAD_PROPERTY_ARRAY_INDEX,
    BO_RAILROAD_PROPERTY_ARRAY_INDEX,
    SHORT_LINE_RAILROAD_PROPERTY_ARRAY_INDEX,
    
    // utilities
    ELECTRIC_COMPANY_PROPERTY_ARRAY_INDEX,
    WATER_WORKS_PROPERTY_ARRAY_INDEX
} ePropertyArrayIndex;

// game states
typedef enum _eGameState
{
    GAME_STATE_STARTUP,
    GAME_STATE_RUNNING,
    GAME_STATE_GAME_OVER
} eGameState;

// phase results for game flow system
typedef enum _ePhaseResult
{
    PHASE_RUNNING,
    PHASE_COMPLETE
} ePhaseResult;

// player pieces
typedef enum _ePlayerPiece
{
    PIECE_BATTLESHIP,
    PIECE_RACE_CAR,
    PIECE_TOP_HAT,
    PIECE_BOOT,
    PIECE_THIMBLE,
    PIECE_IRON,
    PIECE_NONE
} ePlayerPiece;

typedef enum _ePlayerArrayIndex
{
    PLAYER_ONE_ARRAY_INDEX,
    PLAYER_TWO_ARRAY_INDEX,
    PLAYER_THREE_ARRAY_INDEX,
    PLAYER_FOUR_ARRAY_INDEX,
    PLAYER_FIVE_ARRAY_INDEX,
    PLAYER_SIX_ARRAY_INDEX
} ePlayerArrayIndex;

// special player indices
#define BANK_PLAYER_INDEX 255

// ==================== STRUCTS ==================== //

// dice state
typedef struct _mDice
{
    uint8_t uDie1;
    uint8_t uDie2;
} mDice;

// property data
typedef struct _mProperty
{
    char           cName[32];
    uint32_t       auRentWithHouses[6];
    uint32_t       uPrice;
    uint32_t       uRentBase;
    uint32_t       uRentMonopoly;        // rent when owning full color set
    uint32_t       uMortgageValue;
    uint32_t       uHouseCost;
    uint32_t       uHotelCost;
    uint8_t        uPosition;            // board position (0-39)
    uint8_t        uHouses;
    ePropertyType  eType;
    ePropertyColor eColor;
    uint8_t        uOwnerIndex;          // index into players array, 255 = unowned / banker "BANK_PLAYER_INDEX"
    bool           bIsMortgaged;
    bool           bHasHotel;
} mProperty;

// player data
typedef struct _mPlayer
{
    uint32_t     uMoney;
    uint8_t      uPosition;              // board position (0-39)
    uint8_t      uJailTurns;             // turns spent in jail (0 = not in jail)
    uint8_t      auPropertiesOwned[PROPERTY_ARRAY_SIZE];  // indices into game properties array, 255 = banker "BANK_PLAYER_INDEX"
    uint8_t      uPropertyCount;         // actual count of owned properties
    ePlayerPiece ePiece;
    bool         bHasJailFreeCard;
    bool         bIsBankrupt;
} mPlayer;

// chance card
typedef struct _mChanceCard
{
    char     cDescription[128];
    uint32_t uCardID;
} mChanceCard;

// community chest card
typedef struct _mCommunityChestCard
{
    char     cDescription[128];
    uint32_t uCardID;
} mCommunityChestCard;

// deck shuffling state
typedef struct _mDeckState
{
    uint8_t auIndices[16]; // shuffled indices
    uint8_t uCurrentIndex; // next card to draw
} mDeckState;

// forward declaration for phase function pointer
typedef struct _mGameData mGameData;
typedef struct _mGameFlow mGameFlow;

// phase function pointer type
typedef ePhaseResult (*fPhaseFunc)(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);

// game flow state (phase system)
typedef struct _mGameFlow
{
    fPhaseFunc pfCurrentPhase;
    void*      pCurrentPhaseData;
    
    // phase stack for nested operations (auctions, trades, etc)
    fPhaseFunc apPhaseStack[16];
    void*      apPhaseDataStack[16];
    int        iStackDepth;
    
    // reference to game data
    mGameData* pGame;
    
    // input state
    void* pInputContext; // platform-specific (e.g. window handle)
    bool  bInputReceived;
    int   iInputValue;
    char  szInputString[256];
    
    // timing
    float fAccumulatedTime;
} mGameFlow;

// pre-roll phase data
typedef struct _mPreRollData
{
    bool bShowedMenu;
} mPreRollData;

// post-roll phase data
typedef struct _mPostRollData
{
    bool        bMovedPlayer;
    bool        bHandledLanding;
    eSquareType eSquareType;
    uint8_t     uPropertyIndex;  // if landed on property
} mPostRollData;

// jail phase data
typedef struct _mJailData
{
    bool    bShowedMenu;
    bool    bRolledDice;
    uint8_t uAttemptNumber;  // 0 not in jail, auto release on turn 3 
} mJailData;

// trade phase data 
typedef enum _eTradeStep
{
    TRADE_STEP_SELECT_PLAYER,
    TRADE_STEP_BUILD_OFFER,
    TRADE_STEP_AWAITING_RESPONSE
} eTradeStep;

typedef struct _mTradeData
{
    eTradeStep eStep;
    uint8_t    uTargetPlayer;                             // who we're trading with
    uint8_t    auOfferedProperties[PROPERTY_ARRAY_SIZE];  // properties we're offering
    uint8_t    uOfferedPropertyCount;
    uint32_t   uOfferedMoney;
    uint8_t    auRequestedProperties[PROPERTY_ARRAY_SIZE]; // properties we want
    uint8_t    uRequestedPropertyCount;
    uint32_t   uRequestedMoney;
    bool       bShowedMenu;
} mTradeData;

// auction phase data
typedef struct _mAuctionData
{
    ePropertyArrayIndex ePropertyIndex;
    uint8_t             uHighestBidder;      // 255 = no bids yet
    uint32_t            uHighestBid;
    uint8_t             uCurrentBidder;      // whose turn to bid
    bool                abPlayersPassed[MAX_PLAYERS];  // track who passed
    uint8_t             uConsecutivePasses;  // end auction when = active players
    bool                bShowedMenu;
} mAuctionData;

// property management phase data
typedef struct _mPropertyManagementData
{
    bool bShowedMenu;
} mPropertyManagementData;

typedef struct _mBankruptcyData
{
    ePlayerArrayIndex eBankruptPlayer;
    uint8_t           uCreditor;
    uint32_t          uAmountOwed;
} mBankruptcyData;

// main game state
typedef struct _mGameData
{
    mPlayer             amPlayers[MAX_PLAYERS];
    mProperty           amProperties[TOTAL_PROPERTIES];
    mChanceCard         amChanceCards[TOTAL_CHANCE_CARDS];
    mCommunityChestCard amCommunityChestCards[TOTAL_COMMUNITY_CHEST_CARDS];
    mDeckState          tChanceDeck;
    mDeckState          tCommunityChestDeck;
    mDice               tDice;
    
    uint8_t             uPlayerCount;
    uint8_t             uCurrentPlayerIndex;
    uint8_t             uActivePlayers;  // non-bankrupt players
    uint64_t            uRoundCount;
    uint32_t            uJailFine;
    uint32_t            uGlobalHouseSupply;
    uint32_t            uGlobalHotelSupply;
    eGameState          eState;
    bool                bIsRunning;
    
    // ui state flags
    bool bShowPrerollMenu;
    bool bShowPropertyMenu;
    bool bShowJailMenu;
    bool bShowAuctionMenu;
    bool bShowTradeMenu;
    
    // notification system
    char  acNotification[256];
    bool  bShowNotification;
    float fNotificationTimer;
} mGameData;

// game initialization settings
typedef struct _mGameSettings
{
    uint32_t uStartingMoney;
    uint32_t uJailFine;
    uint8_t  uPlayerCount;
} mGameSettings;

// ==================== PHASE SYSTEM FUNCTIONS ==================== //

// phase management
void m_init_game_flow(mGameFlow* pFlow, mGameData* pGame, void* pInputContext);
void m_push_phase(mGameFlow* pFlow, fPhaseFunc pfNewPhase, void* pNewData);
void m_pop_phase(mGameFlow* pFlow);
void m_run_current_phase(mGameFlow* pFlow, float fDeltaTime);

// input handling
void m_set_input_int(mGameFlow* pFlow, int iValue);
void m_set_input_string(mGameFlow* pFlow, const char* szValue);
void m_clear_input(mGameFlow* pFlow);
bool m_is_waiting_input(mGameFlow* pFlow);

// ==================== GAME LOGIC FUNCTIONS ==================== //

// dice
void m_roll_dice(mDice* pDice);

// movement
void m_move_player(mPlayer* pPlayer, mDice* pDice, mGameData* pGame);
void m_move_player_to(mPlayer* pPlayer, uint8_t uPosition);

// turn management
void m_next_player_turn(mGameData* pGame);

// property buying
bool m_can_afford(mPlayer* pPlayer, uint32_t uAmount);
bool m_buy_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);

// building houses/hotels
bool m_can_build_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_build_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_can_build_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_build_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_can_sell_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_sell_house(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_can_sell_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_sell_hotel(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);

// rent payment
uint8_t  m_count_properties_of_color(mGameData* pGame, uint8_t uPlayerIndex, ePropertyColor eColor);
uint8_t  m_get_color_set_size(ePropertyColor eColor);
bool     m_owns_color_set(mGameData* pGame, uint8_t uPlayerIndex, ePropertyColor eColor);
uint32_t m_calculate_rent(mGameData* pGame, uint8_t uPropertyIndex);
bool     m_pay_rent(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPayerIndex);

// property lookup
uint8_t     m_get_property_at_position(mGameData* pGame, uint8_t uBoardPosition);
eSquareType m_get_square_type(uint8_t uPosition);
const char* m_get_square_name(mGameData* pGame, uint8_t uPosition);

// jail
bool m_use_jail_free_card(mPlayer* pPlayer);

// mortgaging
bool m_mortgage_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);
bool m_unmortgage_property(mGameData* pGame, uint8_t uPropertyIndex, uint8_t uPlayerIndex);

// notifications
void m_set_notification(mGameData* pGame, const char* pcFormat, ...);
void m_clear_notification(mGameData* pGame);

// card execution
void    m_execute_chance_card(mGameData* pGame, uint8_t uCardIdx);
void    m_execute_community_chest_card(mGameData* pGame, uint8_t uCardIdx);
void    m_shuffle_deck(mDeckState* pDeck);
uint8_t m_draw_chance_card(mGameData* pGame);
uint8_t m_draw_community_chest_card(mGameData* pGame);

//property management
int32_t m_calculate_net_worth(mGameData* pGame, uint8_t uPlayerIndex); // int because can be nagative

// bankruptcy 
void m_transfer_assets_to_player(mGameData* pGame, uint8_t uFromPlayer, uint8_t uToPlayer);
void m_transfer_assets_to_bank(mGameData* pGame, uint8_t uFromPlayer);

// ==================== PHASE FUNCTIONS ==================== //

// phases
ePhaseResult m_phase_pre_roll(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_post_roll(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_jail(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_property_management(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_auction(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_bankruptcy(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);
ePhaseResult m_phase_trade(void* pPhaseData, float fDeltaTime, mGameFlow* pFlow);

// helper for trading 
void m_transfer_property(mGameData* pGame, uint8_t uPropIdx, uint8_t uFromPlayer, uint8_t uToPlayer);
#endif // MONOPOLY_H
