

#ifndef M_GAME_H
#define M_GAME_H


typedef struct _mGameData mGameData;
typedef struct _mDice mDice;
typedef struct _mPlayer mPlayer;
typedef struct _mProperty mProperty;


// ==================== ENUMS ==================== //
typedef enum _mGameState 
{
    // meta states
    GAME_STARTUP,
    GAME_OVER,

    // turn phases
    PHASE_PRE_ROLL,
    PHASE_POST_ROLL,
    PHASE_END_TURN,

    // special states
    AUCTION_IN_PROGRESS,
    TRADE_NEGOTIATION,

    // emergency payment states
    OWE_PLAYER,
    OWE_BANK

} mGameState;

typedef enum _mJailAction
{
    // jail choices
    USE_JAIL_CARD,
    PAY_JAIL_FINE,
    ROLL_FOR_DOUBLES,
    MAX_JAIL_OPTIONS
} mJailAction;

typedef enum _mActions
{
    // can do at any time
    VIEW_STATUS,
    VIEW_PROPERTIES,

    PROPOSE_TRADE,

    // property managment
    PROPERTY_MANAGMENT,
    SELL_HOUSES,
    BUY_HOUSES,
    SELL_HOTELS,
    BUY_HOTELS,
    MORTGAGE_PROPERTY,
    UNMORTGAGE_PROPERTY,
    CANCEL,

    // pre roll actions
    ROLL_DICE,

    // post roll
    BUY_PROPERTY,
    BUY_RAILROAD,
    BUY_UTILITY,
    BUY_SQUARE,
    END_TURN,

    // forced actions
    FORCED_MORTGAGE_PROPERTIES,
    FORCED_SELL_HOUSES,
    FORCED_DECLARE_BANKRUPTCY,
    MAX_FORCED_ACTIONS,

    // trade actions
    TRADE_PROPERTIES, 

    MAX_ACTIONS
} mActions;


typedef enum _mBoardSquareType 
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
} mBoardSquareType;

typedef struct _mTradeOffer
{
    uint32_t      uCash;
    uint8_t       uPropsInOffer;
    uint8_t       uRailsInOffer;
    uint8_t       uUtilsInOffer;
    mPropertyName mPropsToTrade[PROPERTY_TOTAL];
    mUtilityName  mUtilsToTrade[UTILITY_TOTAL];
    mRailroadName mRailsToTrade[RAILROAD_TOTAL];

} mTradeOffer;

// ==================== CORE GAME FLOW ==================== //
void m_player_turn     (mGameData* mGame);
void m_next_player_turn(mGameData* mGame);
void m_game_over_check (mGameData* mGame);
void m_roll_dice       (mDice* game_dice);

// ==================== JAIL MECHANICS ==================== //
void        m_jail_subphase         (mGameData* mGame);
bool        m_attempt_jail_escape   (mGameData* mGame);
bool        m_use_jail_free_card    (mPlayer* mPlayerInJail);
mJailAction m_get_player_jail_choice(void);

// ==================== PRE-ROLL ACTIONS ==================== //
void     m_pre_roll_phase            (mGameData* mGame);
void     m_show_pre_roll_menu        (mGameData* mGame);
mActions m_get_player_pre_roll_choice(void);

// ==================== POST-ROLL ACTIONS ==================== //
void     m_phase_post_roll              (mGameData* mGame);
void     m_show_post_roll_menu          (mGameData* mGame);
mActions m_get_player_post_roll_choice  (void);
void     m_show_building_managment_menu (mGameData* mGame);
mActions m_get_building_managment_choice(void);

// ==================== SPECIAL PHASES ==================== //
void m_enter_trade_phase(mGameData* mGame);

// ==================== END TURN ACTIONS ==================== //

// ==================== FINANCIAL MANAGEMENT ==================== //
bool m_can_player_afford        (mPlayer* mCurrentPlayer, uint32_t uExpense);
bool m_attempt_emergency_payment(mGameData* mGame, mPlayer* currentPlayer, uint32_t uAmount);

// ==================== FORCED ACTIONS ==================== //
void m_forced_release           (mGameData* mGame);
bool m_player_has_forced_actions(mGameData* mGame);
void m_handle_emergency_actions (mGameData* mGame);


// ==================== HELPERS ==================== //
mBoardSquareType m_get_square_type(uint32_t uPlayerPosition);

mActions m_show_trade_menu  (mGameData* mGame);
void     m_propose_trade    (mGameData* mGame, mPlayer* mCurrentPlayer, mPlayer* mTradePartener);
mPlayer* m_get_trade_partner(mGameData* mGame);
void     m_add_to_offer     (mGameData* mGame, mPlayer* mPlayer, mTradeOffer* mOffer); 
void     m_add_to_request   (mPlayer* partner, mTradeOffer* request);
bool     m_review_trade     (mGameData* mGame, mPlayer* mCurrentPlayer, mPlayer* mTradePartner, mTradeOffer* mOffer, mTradeOffer* mRequest);
void     m_execute_trade    (mGameData* mGame, mPlayer* mPlayerFrom, mPlayer* mPlayerTo, mTradeOffer* mOffer, mTradeOffer* mRequest);
uint8_t  m_get_empty_prop_owned_slot(mPlayer* mPlayer);
uint8_t  m_get_empty_rail_owned_slot(mPlayer* mPlayer);
uint8_t  m_get_empty_util_owned_slot(mPlayer* mPlayer);

#endif