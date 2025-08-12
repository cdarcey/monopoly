
#ifndef M_PROPERTY_H
#define M_PROPERTY_H

#include <stdint.h> // uint
#include <stdbool.h> // bool
#include <stdio.h> // printf


// ==================== FORWARD DECLERATIONS ==================== //
typedef enum _mPlayerNumber mPlayerNumber;
typedef struct _mPlayer mPlayer;
typedef struct _mDice mDice;
typedef struct _mGameData mGameData;


// ==================== ENUMS ==================== //
typedef enum _mPropertyColor {
    PURPLE,
    LIGHT_BLUE,
    PINK,
    ORANGE,
    RED,
    YELLOW,
    GREEN,
    DARK_BLUE,
    PROPERTY_COLOR_TOAL,
    NO_COLOR = PROPERTY_COLOR_TOAL
} mPropertyColor;

typedef enum _mPropertyName // order matters
{
    MEDITERRANEAN_AVENUE,
    BALTIC_AVENUE,
    ORIENTAL_AVENUE,
    VERMONT_AVENUE,
    CONNECTICUT_AVENUE,
    ST_CHARLES_PLACE,
    STATES_AVENUE,
    VIRGINIA_AVENUE,
    ST_JAMES_PLACE,
    TENNESSEE_AVENUE,
    NEW_YORK_AVENUE,
    KENTUCKY_AVENUE,
    INDIANA_AVENUE,
    ILLINOIS_AVENUE,
    ATLANTIC_AVENUE,
    VENTNOR_AVENUE,
    MARVIN_GARDENS,
    PACIFIC_AVENUE,
    NORTH_CAROLINA_AVENUE,
    PENNSYLVANIA_AVENUE,
    PARK_PLACE,
    BOARDWALK,
    PROPERTY_TOTAL,
    NO_PROPERTY,
    PROP_OWNED_WITH_BUFFER = 30

} mPropertyName;

typedef enum _mUtilityName // order matters
{
    ELECTRIC_COMPANY,
    WATER_WORKS,
    UTILITY_TOTAL,
    NO_UTILITY,
    UTIL_OWNED_WITH_BUFFER = 6
} mUtilityName;

typedef enum _mRailroadName // order matters
{
    READING_RAILROAD,
    PENNSYLVANIA_RAILROAD,
    B_AND_O_RAILROAD,
    SHORT_LINE_RAILROAD,
    RAILROAD_TOTAL,
    NO_RAILROAD,
    RAIL_OWNED_WITH_BUFFER = 8
} mRailroadName;


// ==================== STRUCTS ==================== //
typedef struct _mProperty 
{
    char           cName[30];
    uint32_t       uPrice;
    uint32_t       uRent[6];
    uint32_t       uHouseCost;
    uint32_t       uMortgage;
    uint32_t       uNumberOfHouses;
    uint32_t       uNumberOfHotels;
    uint32_t       uPropertiesInSet;
    mPropertyColor eColor;
    mPlayerNumber  eOwner;
    bool           bOwned;
    bool           bMortgaged;
} mProperty;

typedef struct _mRailroad 
{
    char          cName[30];
    uint32_t      uPrice;
    uint32_t      uRent[4];
    uint32_t      uMortgage;
    mPlayerNumber eOwner;
    bool          bOwned;
} mRailroad;  

typedef struct _mUtility 
{
    char          cName[30];
    uint32_t      uPrice;
    uint32_t      uMortgage;
    mPlayerNumber eOwner;
    bool          bOwned;
} mUtility;


// ==================== PROPERTY MANAGEMENT ==================== //
// Core Property Transactions
void m_buy_property(mProperty* propertyToBuy, mPlayer* playerBuying);
void m_buy_railroad(mRailroad* railroadToBuy, mPlayer* playerBuying);
void m_buy_utility (mUtility* utilityToBuy, mPlayer* playerBuying);
void m_buy_house   (mProperty* mPropertyToAddHouse, mPlayer* mPropertyOwner, bool bHouseCanBeAdded);
void m_buy_hotel   (mProperty* mPropertyToAddHotel, mPlayer* mPropertyOwner, bool bHotelCanBeAdded);

// Rent Handling
void m_pay_rent_property(mPlayer* mPayee, mPlayer* mPayer, mProperty* mPropertyForRent, bool bColorSetOwned);
void m_pay_rent_railroad(mPlayer* mPayee, mPlayer* mPayer, mRailroad* mRailroadForRent);
void m_pay_rent_utility (mPlayer* mPayee, mPlayer* mPayer, mUtility* mUtilityForRent, mDice* mDiceResult);

// Asset Transfers
void m_return_assets_to_bank    (mGameData* mGame, mPlayerNumber playerIndex);
void m_transfer_assets_to_player(mGameData* mGame, mPlayerNumber losingAssets, mPlayerNumber gainingAssets);


// ==================== FINANCIAL ACTIONS ==================== //
// Mortgage & Building Management
bool m_player_has_mortgagable_props(mPlayer* mCurrentPlayer, mProperty* mGameProperties);
bool m_player_has_buildings        (mPlayer* mCurrentPlayermProperty, mProperty* mGameProperties);
void m_execute_mortgage_flow       (mProperty* mPropToMortgage, mPlayer* mPlayerMortgaging);
void m_execute_unmortgage_flow     (mProperty* mPropToMortgage, mPlayer* mPlayerMortgaging);
void m_execute_house_sale          (mGameData* mGame, mProperty* mPropWithHouses, mPlayer* mPlayerSelling);
void m_execute_hotel_sale          (mProperty* mPropWithHouses, mPlayer* mPlayerSelling);


// ==================== PROPERTY HELPERS ==================== //
// Enum/String Conversions
const char*    m_color_enum_to_string      (mPropertyColor color);
mPropertyColor m_string_color_to_enum      (const char* colorStr);
mPropertyName  m_string_to_property        (const char* property_str);
mRailroadName  m_string_to_railroad        (const char* railroad_str);
mUtilityName   m_string_to_utility         (const char* utility_str);
const char*    m_property_enum_to_string   (mPropertyName property);
const char*    m_railroad_enum_to_string   (mRailroadName mRailroad);
const char*    m_utility_enum_to_string    (mUtilityName mUtility);
void           m_show_props_owned          (mPlayer* currentPlayer);
void           m_show_rails_owned          (mPlayer* currentPlayer);
void           m_show_utils_owned          (mPlayer* currentPlayer);

// Property Rules & Validation
bool          m_color_set_owned   (mGameData* mGame, mPlayer* mPlayerBuyingHouse, mPropertyColor eColorOfSet);
bool          m_house_can_be_added(mGameData* mGame, mProperty* mCurrentProperty, mPlayer* mPlayerBuyingHouse, mPropertyColor eColorOfSet);
bool          m_hotel_can_be_added(mGameData* mGame, mPropertyColor eColorOfSet);
bool          m_house_can_be_sold (mGameData* mGame, mPlayer* mPlayerSellingHouse, mPropertyColor eColorOfSet); 
bool          m_is_property_owned (mProperty* mPropertyToCheck);
bool          m_is_property_owner (mPlayer* mPlayerToCheck, mProperty* mPropertyToCheck);
mPropertyName m_property_landed_on(uint32_t uPosition);
bool          m_is_railroad_owned (mRailroad* mRailroadToCheck);
bool          m_is_railroad_owner (mPlayer* mPlayerToCheck, mRailroad* mRailroadToCheck);
mRailroadName m_railroad_landed_on(uint32_t uPosition);
bool          m_is_utility_owned  (mUtility* mUtilityToCheck);
bool          m_is_utility_owner  (mPlayer* mPlayerToCheck, mUtility* mUtilityToCheck);
mUtilityName  m_utility_landed_on (uint32_t uPosition);

mPropertyName m_get_player_property(mPlayer* mCurrentPlayer);


#endif