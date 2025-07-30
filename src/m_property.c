

#include <string.h>
#include <math.h> // min function


#include "m_property.h"
#include "m_player.h"
#include "m_init_game.h"


// ==================== PROPERTY MANAGEMENT ==================== //
// Core Property Transactions
void
m_buy_property(mProperty* propertyToBuy, mPlayer* playerBuying)
{
    // TODO: handle not enough money with new finace management functions
    if(propertyToBuy->bOwned == true)
    {
        printf("property is owned and cannot be bought\n");
        return;
    }
    else if(!m_can_player_afford(playerBuying, propertyToBuy->uPrice))
    {
        printf("not enough money to buy property\n");
        return;
    }
    else
    {
        playerBuying->uMoney = playerBuying->uMoney - propertyToBuy->uPrice;
        propertyToBuy->bOwned = true;
        propertyToBuy->eOwner = playerBuying->ePlayerTurnPosition;
        for(uint32_t propertyOwnedIndex = 0; propertyOwnedIndex < PROPERTY_TOTAL; propertyOwnedIndex++)
        {
            if(playerBuying->ePropertyOwned[propertyOwnedIndex] == NO_PROPERTY)
            {
                playerBuying->ePropertyOwned[propertyOwnedIndex] = m_string_to_property(propertyToBuy->cName);
                break;
            }
        }
    }
}

void
m_buy_utility(mUtility* utilityToBuy, mPlayer* playerBuying)
{
    // TODO: handle not enough money with new finace management functions 
    if(utilityToBuy->bOwned == true)
    {
        printf("utility is owned and cannot be bought\n");
        return;
    }
    else if(utilityToBuy->uPrice > playerBuying->uMoney)
    {
        printf("not enough money to buy utility\n");
        return;
    }
    else
    {
        playerBuying->uMoney -= utilityToBuy->uPrice;
        utilityToBuy->bOwned = true;
        utilityToBuy->eOwner = playerBuying->ePlayerTurnPosition;
        for(uint32_t UtilityOwnedIndex = 0; UtilityOwnedIndex < UTILITY_TOTAL; UtilityOwnedIndex++)
        {
            if(playerBuying->eUtilityOwned[UtilityOwnedIndex] == NO_UTILITY)
            {
                playerBuying->eUtilityOwned[UtilityOwnedIndex] = m_string_to_utility(utilityToBuy->cName);
                break;
            }
        }
    }
}

void
m_buy_railroad(mRailroad* railroadToBuy, mPlayer* playerBuying)
{
    // TODO: handle not enough money with new finace management functions 
    if(railroadToBuy->bOwned == true)
    {
        printf("railroad owned and cannot be bought\n");
        return;
    }
    else if(railroadToBuy->uPrice > playerBuying->uMoney)
    {
        printf("not enough money to buy railroad\n");
        return;
    }
    else
    {
        playerBuying->uMoney = playerBuying->uMoney - railroadToBuy->uPrice;
        railroadToBuy->bOwned = true;
        railroadToBuy->eOwner = playerBuying->ePlayerTurnPosition;
        for(uint32_t railroadOwnedIndex = 0; railroadOwnedIndex < RAILROAD_TOTAL; railroadOwnedIndex++)
        {
            if(playerBuying->eRailroadsOwned[railroadOwnedIndex] == NO_RAILROAD)
            {
                playerBuying->eRailroadsOwned[railroadOwnedIndex] = m_string_to_railroad(railroadToBuy->cName);
                break;
            }
        }
    }
}

void
m_buy_house(mProperty* mPropertyToAddHouse, mPlayer* mPropertyOwner, bool bHouseCanBeAdded)
{
    assert(bHouseCanBeAdded == false);
    assert(mPropertyToAddHouse->eOwner != mPropertyOwner->ePlayerTurnPosition && mPropertyToAddHouse->eOwner != NO_PLAYER);

    if(mPropertyToAddHouse->uHouseCost > mPropertyOwner->uMoney)
    {
        printf("not enout money to buy house\n");
        return;
    }

    else
    {
        if(bHouseCanBeAdded == true)
        {
            mPropertyToAddHouse->uNumberOfHouses++;
            mPropertyOwner->uMoney -= mPropertyToAddHouse->uHouseCost;
        }

    }
}

void
m_buy_hotel(mProperty* mPropertyToAddHotel, mPlayer* mPropertyOwner, bool bHotelCanBeAdded)
{
    assert(bHotelCanBeAdded == false);
    assert(mPropertyToAddHotel->eOwner != mPropertyOwner->ePlayerTurnPosition && mPropertyToAddHotel->eOwner != NO_PLAYER);

    if(mPropertyToAddHotel->uHouseCost > mPropertyOwner->uMoney)
    {
        printf("not enout money to buy hotel\n");
        return;
    }

    else
    {
        if(mPropertyToAddHotel == true)
        {
            mPropertyToAddHotel->uNumberOfHotels++;
            mPropertyOwner->uMoney -= mPropertyToAddHotel->uRent[5];
        }

    }
}


// Rent Handling
void
m_pay_rent_property(mPlayer* mPayee, mPlayer* mPayer, mProperty* mPropertyForRent, bool bColorSetOwned)
{
    if(!mPayee || !mPayer || !mPropertyForRent ||
        mPayee->ePlayerTurnPosition == mPayer->ePlayerTurnPosition ||
        mPropertyForRent->bOwned == false)
    {
        // handle error
        return;
    }

    if(mPropertyForRent->bMortgaged)
    {
        return; // cannot collect rent on mortgaged property
    }

    uint8_t uRent = 0;
    if(bColorSetOwned && mPropertyForRent->uNumberOfHouses == 0)
    {
        uRent = mPropertyForRent->uRent[mPropertyForRent->uNumberOfHouses] * 2;
    }
    else if(mPropertyForRent->uNumberOfHotels == 0 || mPropertyForRent->uNumberOfHouses == 0)
    {
        uRent = mPropertyForRent->uRent[mPropertyForRent->uNumberOfHouses];
    }
    else if(mPropertyForRent->uNumberOfHouses > 0)
    {
        uRent = mPropertyForRent->uRent[mPropertyForRent->uNumberOfHouses];
    }
    else if(mPropertyForRent->uNumberOfHotels > 0)
    {
        uRent = mPropertyForRent->uRent[mPropertyForRent->uNumberOfHouses + mPropertyForRent->uNumberOfHotels];
    }

    if(!m_can_player_afford(mPayer, uRent))
    {
        // TODO: cannot afford
    }
    else 
        mPayer->uMoney -= uRent;
        mPayee->uMoney += uRent;


}

void
m_pay_rent_railroad(mPlayer* mPayee, mPlayer* mPayer, mRailroad* mRailroadForRent)
{
    if(mPayer->ePlayerTurnPosition == mRailroadForRent->eOwner)
    {
        return;
    }

    // rent doubles for every railroad owned
    uint8_t uRentArrayIndex = 0;
    for(uint8_t uRailroadsChecked = 0; uRailroadsChecked < RAILROAD_TOTAL; uRailroadsChecked++)
    {
        if(mPayee->eRailroadsOwned[uRailroadsChecked] != NO_RAILROAD)
        {
            uRentArrayIndex++;
        }
    }

    if (mPayer->uMoney > mRailroadForRent->uRent[uRentArrayIndex])
    {
        mPayer->uMoney = mPayer->uMoney - mRailroadForRent->uRent[uRentArrayIndex];
        mPayee->uMoney = mPayee->uMoney + mRailroadForRent->uRent[uRentArrayIndex];
    }
    else if(mPayer->uMoney <= mRailroadForRent->uRent[uRentArrayIndex])
    {
        mPayer->bBankrupt = true;
        mPayer->uMoney = 0;
        return;
    }
    else
    {
        printf("control path missed on railroad rent pay");
    }

}

void
m_pay_rent_utility(mPlayer* mPayee, mPlayer* mPayer, mUtility* mUtilityForRent, mDice* mDiceResult)
{
    if(mPayer->ePlayerTurnPosition == mUtilityForRent->eOwner)
    {
        return;
    }

    // rent multiplies 4x for owning one utitily and 10x for 2 owned
    uint8_t uRentArrayIndex = 0;
    uint8_t uRentAfterMultiplier = 0;
    for(uint8_t uUtilitiesChecked = 0; uUtilitiesChecked < UTILITY_TOTAL; uUtilitiesChecked++)
    {
        if(mPayee->eUtilityOwned[uUtilitiesChecked] != NO_UTILITY)
        {
            uRentArrayIndex++;
        }
    }
    if(uRentArrayIndex == 1)
    {
        uRentAfterMultiplier = (mDiceResult->dice_one + mDiceResult->dice_two) * 4;
    }
    else if(uRentArrayIndex == 2)
    {
        uRentAfterMultiplier = (mDiceResult->dice_one + mDiceResult->dice_two) * 10;
    }

    if(mPayer->uMoney > uRentAfterMultiplier)
    {
        mPayer->uMoney = mPayer->uMoney - uRentAfterMultiplier;
        mPayee->uMoney = mPayee->uMoney + uRentAfterMultiplier;
    }
    else if(mPayer->uMoney <= uRentAfterMultiplier)
    {
        mPayer->uMoney = 0;
        mPayee->bBankrupt = true;
    }
    else
    {
        printf("control path missed on utility rent function\n");
    }
}

void
m_return_assets_to_bank(mGameData* mGame, mPlayerNumber playerIndex)
{
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)
    {
        if(mGame->mGameProperties[i].eOwner == playerIndex)
        {
            mGame->mGameProperties[i].eOwner = NO_PLAYER;
            mGame->mGameProperties[i].uNumberOfHouses = 0;
            mGame->mGameProperties[i].uNumberOfHouses = 0;
            mGame->mGameProperties[i].bMortgaged = false;
            mGame->mGameProperties[i].bOwned = false;
        }
    }
}

void
m_transfer_assets_to_player(mGameData* mGame, mPlayerNumber losingAssets, mPlayerNumber gainingAssets)
{
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)
    {
        if (mGame->mGameProperties[i].eOwner == losingAssets) 
        {
            // Transfer property
            mGame->mGameProperties[i].eOwner = gainingAssets;

            // Inherit buildings/mortgage status
            if (mGame->mGameProperties[i].bMortgaged) 
            {
                if(m_can_player_afford(mGame->mGamePlayers[gainingAssets], mGame->mGameProperties[i].uPrice))
                {
                    mGame->mGamePlayers[gainingAssets]->uMoney -= mGame->mGameProperties[i].uPrice / 2; // Pay unmortgage cost
                }
                else
                {
                    // TODO: prop needs to be auctioned and if auction fails returned to bank
                    mGame->mGameProperties[i].eOwner = NO_PLAYER;
                    mGame->mGameProperties[i].uNumberOfHouses = 0;
                    mGame->mGameProperties[i].uNumberOfHouses = 0;
                    mGame->mGameProperties[i].bMortgaged = false;
                    mGame->mGameProperties[i].bOwned = false;
                }
            }
        }
    }


    if(mGame->mGamePlayers[losingAssets]->bGetOutOfJailFreeCard == true)
    {
        mGame->mGamePlayers[losingAssets]->bGetOutOfJailFreeCard = false;
        mGame->mGamePlayers[gainingAssets]->bGetOutOfJailFreeCard = true;
    }
}


// ==================== FINANCIAL ACTIONS ==================== //
bool
m_player_has_mortgagable_props(mPlayer* mCurrentPlayer, mProperty* mGameProperties)
{
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)
    {
        if (mGameProperties[i].eOwner == mCurrentPlayer->ePlayerTurnPosition && mGameProperties[i].bMortgaged == false) 
        {
            return true;
        }
    }
    return false;
}

bool
m_player_has_buildings(mPlayer* mCurrentPlayer, mProperty* mGameProperties)
{
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)
    {
        if (mGameProperties[i].eOwner != mCurrentPlayer->ePlayerTurnPosition) 
        continue;

        if (mGameProperties[i].uNumberOfHotels > 0 || mGameProperties[i].uNumberOfHouses > 0)
        {
            return true;
        }
    }
    return false;
}

void
m_execute_mortgage_flow(mProperty* mPropToMortgage, mPlayer* mPlayerMortgaging)
{
    if(!mPropToMortgage || !mPlayerMortgaging)
    {
        __debugbreak();
        return;
    }

    if(mPropToMortgage->eOwner != mPlayerMortgaging->ePlayerTurnPosition)
    {
        printf("you do not own property\n");
        return;
    }
    else if(mPropToMortgage->bMortgaged == true)
    {
        printf("property is mortgaged\n");
        return;
    }
    else if(mPropToMortgage->uNumberOfHotels > 0 || mPropToMortgage->uNumberOfHouses > 0)
    {
        printf("cannot mortgage property with buildings\n");
        return;
    }
    else
    {
        mPropToMortgage->bMortgaged = true;
        mPlayerMortgaging->uMoney += mPropToMortgage->uPrice / 2;
    }
}

void
m_execute_house_sale(mGameData* mGame, mProperty* mPropWithHouses, mPlayer* mPlayerSelling) 
{
    assert(mPropWithHouses->uNumberOfHouses == 0);
    assert(mPropWithHouses->eOwner != mPlayerSelling->ePlayerTurnPosition);
    assert(!mPropWithHouses || !mPlayerSelling);

    if(!m_house_can_be_sold(mGame, mPlayerSelling, mPropWithHouses->eColor))
    {
        printf("House cannot be sold\n");
    }
    else
    {
        mPropWithHouses->uNumberOfHouses--;
        mPlayerSelling->uMoney += mPropWithHouses->uHouseCost / 2;
    }
}

void
m_execute_hotel_sale(mProperty* mPropWithHotels, mPlayer* mPlayerSelling)
{

    if(mPropWithHotels->uNumberOfHotels == 0)
    {
        printf("no hotel to sell\n");
        return;
    }
    else
    {
        mPropWithHotels->uNumberOfHotels = 0;
        mPlayerSelling->uMoney += (mPropWithHotels->uHouseCost * 4) / 2;
    }
}



// ==================== HELPERS ==================== //
const char*
m_color_enum_to_string(mPropertyColor color)
{
    switch(color) {
        case PURPLE:     return "purple";
        case LIGHT_BLUE: return "light blue";
        case PINK:       return "pink";
        case ORANGE:     return "orange";
        case RED:        return "red";
        case YELLOW:     return "yellow";
        case GREEN:      return "green";
        case DARK_BLUE:  return "dark blue";
        default:         return "unknown";
    }
}

mPropertyName
m_string_to_property(const char* property_str)
{

    if (strcmp(property_str, "Mediterranean Avenue") == 0)  return MEDITERRANEAN_AVENUE;
    if (strcmp(property_str, "Baltic Avenue") == 0)         return BALTIC_AVENUE;
    if (strcmp(property_str, "Oriental Avenue") == 0)       return ORIENTAL_AVENUE;
    if (strcmp(property_str, "Vermont Avenue") == 0)        return VERMONT_AVENUE;
    if (strcmp(property_str, "Connecticut Avenue") == 0)    return CONNECTICUT_AVENUE;
    if (strcmp(property_str, "St Charles place") == 0)      return ST_CHARLES_PLACE;
    if (strcmp(property_str, "States Avenue") == 0)         return STATES_AVENUE;
    if (strcmp(property_str, "Virginia Avenue") == 0)       return VIRGINIA_AVENUE;
    if (strcmp(property_str, "St James Place") == 0)        return ST_JAMES_PLACE;
    if (strcmp(property_str, "Tennessee Avenue") == 0)      return TENNESSEE_AVENUE;
    if (strcmp(property_str, "New York Avenue") == 0)       return NEW_YORK_AVENUE;
    if (strcmp(property_str, "Kentucky Avenue") == 0)       return KENTUCKY_AVENUE;
    if (strcmp(property_str, "Indiana Avenue") == 0)        return INDIANA_AVENUE;
    if (strcmp(property_str, "Illinois Avenue") == 0)       return ILLINOIS_AVENUE;
    if (strcmp(property_str, "Atlantic Avenue") == 0)       return ATLANTIC_AVENUE;
    if (strcmp(property_str, "Ventnor Avenue") == 0)        return VENTNOR_AVENUE;
    if (strcmp(property_str, "Marvin Gardens") == 0)        return MARVIN_GARDENS;
    if (strcmp(property_str, "Pacific Avenue") == 0)        return PACIFIC_AVENUE;
    if (strcmp(property_str, "North carolina Avenue") == 0) return NORTH_CAROLINA_AVENUE;
    if (strcmp(property_str, "Pennsylvania Avenue") == 0)   return PENNSYLVANIA_AVENUE;
    if (strcmp(property_str, "Park Place") == 0)            return PARK_PLACE;
    if (strcmp(property_str, "Boardwalk") == 0)             return BOARDWALK;
    return NO_PROPERTY;
}

const char* 
m_property_enum_to_string(mPropertyName mProperty) 
{
    switch (mProperty) {
        case MEDITERRANEAN_AVENUE:  return "Mediterranean Avenue";
        case BALTIC_AVENUE:         return "Baltic Avenue";
        case ORIENTAL_AVENUE:       return "Oriental Avenue";
        case VERMONT_AVENUE:        return "Vermont Avenue";
        case CONNECTICUT_AVENUE:    return "Connecticut Avenue";
        case ST_CHARLES_PLACE:      return "St. Charles Place";
        case STATES_AVENUE:         return "States Avenue";
        case VIRGINIA_AVENUE:       return "Virginia Avenue";
        case ST_JAMES_PLACE:        return "St. James Place";
        case TENNESSEE_AVENUE:      return "Tennessee Avenue";
        case NEW_YORK_AVENUE:       return "New York Avenue";
        case KENTUCKY_AVENUE:       return "Kentucky Avenue";
        case INDIANA_AVENUE:        return "Indiana Avenue";
        case ILLINOIS_AVENUE:       return "Illinois Avenue";
        case ATLANTIC_AVENUE:       return "Atlantic Avenue";
        case VENTNOR_AVENUE:        return "Ventnor Avenue";
        case MARVIN_GARDENS:        return "Marvin Gardens";
        case PACIFIC_AVENUE:        return "Pacific Avenue";
        case NORTH_CAROLINA_AVENUE: return "North Carolina Avenue";
        case PENNSYLVANIA_AVENUE:   return "Pennsylvania Avenue";
        case PARK_PLACE:            return "Park Place";
        case BOARDWALK:             return "Boardwalk";
        case PROPERTY_TOTAL:        return "Property Total";
        case NO_PROPERTY:           return "No Property";
        default:                    return "Unknown Property";
    }
}

const char*
m_railroad_enum_to_string(mRailroadName mRailroad) 
{
    switch (mRailroad) {
        case READING_RAILROAD:      return "Reading Railroad";
        case PENNSYLVANIA_RAILROAD: return "Pennsylvania Railroad";
        case B_AND_O_RAILROAD:      return "B & O Railroad";
        case SHORT_LINE_RAILROAD:   return "Short Line Railroad";
        default:                    return "Unknown Railroad";
    }
}

const char*
m_utility_enum_to_string(mUtilityName mUtility) 
{
    switch (mUtility) {
        case ELECTRIC_COMPANY:    return "Electric Company";
        case WATER_WORKS:         return "Water Works";
        default:                  return "Unknown Utility";
    }
}

mUtilityName 
m_string_to_utility(const char* utility_str)
{
    if (strcmp(utility_str, "Electric Company") == 0)  return ELECTRIC_COMPANY;
    if (strcmp(utility_str, "Water Works") == 0)       return WATER_WORKS;
    return NO_UTILITY;
}

mRailroadName 
m_string_to_railroad(const char* railroad_str)
{
    if (strcmp(railroad_str, "Reading Railroad") == 0)      return READING_RAILROAD;
    if (strcmp(railroad_str, "Pennsylvania Railroad") == 0) return PENNSYLVANIA_RAILROAD;
    if (strcmp(railroad_str, "B. & O. Railroad") == 0)      return B_AND_O_RAILROAD;
    if (strcmp(railroad_str, "Short Line") == 0)            return SHORT_LINE_RAILROAD;
    return NO_RAILROAD;
}

mPropertyColor 
m_string_color_to_enum(const char* colorStr)
{
    if (strcmp(colorStr, "PURPLE") == 0)     return PURPLE;
    if (strcmp(colorStr, "LIGHT_BLUE") == 0) return LIGHT_BLUE;
    if (strcmp(colorStr, "PINK") == 0)       return PINK;
    if (strcmp(colorStr, "ORANGE") == 0)     return ORANGE;
    if (strcmp(colorStr, "RED") == 0)        return RED;
    if (strcmp(colorStr, "YELLOW") == 0)     return YELLOW;
    if (strcmp(colorStr, "GREEN") == 0)      return GREEN;
    if (strcmp(colorStr, "DARK_BLUE") == 0)  return DARK_BLUE;
    return NO_COLOR; 
}

bool
m_color_set_owned(mGameData* mGame, mPlayer* mPlayerBuyingHouse, mPropertyColor eColorOfSet)
{
    switch(eColorOfSet)
    {
        case PURPLE:
        {
            if (mGame->mGameProperties[MEDITERRANEAN_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition && 
                mGame->mGameProperties[BALTIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case LIGHT_BLUE:
        {
            if(mGame->mGameProperties[ORIENTAL_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[VERMONT_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[CONNECTICUT_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case PINK:
        {
            if(mGame->mGameProperties[ST_CHARLES_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[STATES_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[VIRGINIA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case ORANGE:
        {
            if(mGame->mGameProperties[ST_JAMES_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[TENNESSEE_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[NEW_YORK_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case RED:
        {
            if(mGame->mGameProperties[ILLINOIS_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[INDIANA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[KENTUCKY_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case YELLOW:
        {
            if(mGame->mGameProperties[MARVIN_GARDENS].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[VENTNOR_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[ATLANTIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case GREEN:
        {
            if(mGame->mGameProperties[PENNSYLVANIA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[NORTH_CAROLINA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[PACIFIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        case DARK_BLUE:
        {
            if (mGame->mGameProperties[BOARDWALK].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[PARK_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                return true;
            }
            return false;
        }
        break;
        default:
        {
            printf("error in checking if property set is owned function\n");
            return false;
        }
    }
}

bool
m_house_can_be_added(mGameData* mGame, mPlayer* mPlayerBuyingHouse, mPropertyColor eColorOfSet)
{
    switch(eColorOfSet)
    {
        case PURPLE:  
        {
            if (mGame->mGameProperties[MEDITERRANEAN_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[BALTIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t medHouses = mGame->mGameProperties[MEDITERRANEAN_AVENUE].uNumberOfHouses;
                uint8_t balHouses = mGame->mGameProperties[BALTIC_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(medHouses, balHouses);
                return (medHouses <= minHouses + 1) &&
                       (balHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case LIGHT_BLUE:
        {
            if(mGame->mGameProperties[ORIENTAL_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[VERMONT_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[CONNECTICUT_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t oriHouses = mGame->mGameProperties[ORIENTAL_AVENUE].uNumberOfHouses;
                uint8_t verHouses = mGame->mGameProperties[VERMONT_AVENUE].uNumberOfHouses;
                uint8_t conHouses = mGame->mGameProperties[CONNECTICUT_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(oriHouses, verHouses), conHouses);
                return (oriHouses <= minHouses + 1) && 
                       (verHouses <= minHouses + 1) && 
                       (conHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case PINK:
        {
            if(mGame->mGameProperties[ST_CHARLES_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[STATES_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[VIRGINIA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t stcHouses = mGame->mGameProperties[ST_CHARLES_PLACE].uNumberOfHouses;
                uint8_t staHouses = mGame->mGameProperties[STATES_AVENUE].uNumberOfHouses;
                uint8_t virHouses = mGame->mGameProperties[VIRGINIA_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(stcHouses, staHouses), virHouses);
                return (stcHouses <= minHouses + 1) && 
                       (staHouses <= minHouses + 1) && 
                       (virHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case ORANGE:
        {
            if(mGame->mGameProperties[ST_JAMES_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[TENNESSEE_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[NEW_YORK_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t stjHouses = mGame->mGameProperties[ST_JAMES_PLACE].uNumberOfHouses;
                uint8_t tenHouses = mGame->mGameProperties[TENNESSEE_AVENUE].uNumberOfHouses;
                uint8_t nyHouses = mGame->mGameProperties[NEW_YORK_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(stjHouses, tenHouses), nyHouses);
                return (stjHouses <= minHouses + 1) && 
                       (tenHouses <= minHouses + 1) && 
                       (nyHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case RED:
        {
            if(mGame->mGameProperties[ILLINOIS_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[INDIANA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[KENTUCKY_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t illHouses = mGame->mGameProperties[ILLINOIS_AVENUE].uNumberOfHouses;
                uint8_t indHouses = mGame->mGameProperties[INDIANA_AVENUE].uNumberOfHouses;
                uint8_t kenHouses = mGame->mGameProperties[KENTUCKY_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(illHouses, indHouses), kenHouses);
                return (illHouses <= minHouses + 1) && 
                       (indHouses <= minHouses + 1) && 
                       (kenHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case YELLOW:
        {
            if(mGame->mGameProperties[MARVIN_GARDENS].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[VENTNOR_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[ATLANTIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t marHouses = mGame->mGameProperties[MARVIN_GARDENS].uNumberOfHouses;
                uint8_t venHouses = mGame->mGameProperties[VENTNOR_AVENUE].uNumberOfHouses;
                uint8_t atlHouses = mGame->mGameProperties[ATLANTIC_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(marHouses, venHouses), atlHouses);
                return (marHouses <= minHouses + 1) &&
                       (venHouses <= minHouses + 1) &&
                       (atlHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case GREEN:
        {
            if(mGame->mGameProperties[PENNSYLVANIA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[NORTH_CAROLINA_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition&&
                mGame->mGameProperties[PACIFIC_AVENUE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t penHouses = mGame->mGameProperties[PENNSYLVANIA_AVENUE].uNumberOfHouses;
                uint8_t norHouses = mGame->mGameProperties[NORTH_CAROLINA_AVENUE].uNumberOfHouses;
                uint8_t pacHouses = mGame->mGameProperties[PACIFIC_AVENUE].uNumberOfHouses;
                uint8_t minHouses = min(min(penHouses, norHouses), pacHouses);
                return (penHouses <= minHouses + 1) && 
                       (norHouses <= minHouses + 1) && 
                       (pacHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        case DARK_BLUE:
        {
            if (mGame->mGameProperties[BOARDWALK].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[PARK_PLACE].eOwner == mPlayerBuyingHouse->ePlayerTurnPosition)
            {
                uint8_t boaHouses = mGame->mGameProperties[BOARDWALK].uNumberOfHouses;
                uint8_t parHouses = mGame->mGameProperties[PARK_PLACE].uNumberOfHouses;
                uint8_t minHouses = min(boaHouses, parHouses);
                return (boaHouses <= minHouses + 1) &&
                       (parHouses <= minHouses + 1);
            }
            return false;
        }
        break;
        default:
        {
            printf("error in checking if property set is owned function\n");
            return false;
        }
    }
}

bool 
m_house_can_be_sold(mGameData* mGame, mPlayer* mPlayerSellingHouse, mPropertyColor eColorOfSet) 
{
    switch(eColorOfSet) 
    {
        case PURPLE:  
        {
            if (mGame->mGameProperties[MEDITERRANEAN_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[BALTIC_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t medHouses = mGame->mGameProperties[MEDITERRANEAN_AVENUE].uNumberOfHouses;
                uint8_t balHouses = mGame->mGameProperties[BALTIC_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (medHouses > balHouses) ? medHouses : balHouses;
                return (medHouses == maxHouses) || (balHouses == maxHouses);
            }
            return false;
        }
        break;

        case LIGHT_BLUE:
        {
            if(mGame->mGameProperties[ORIENTAL_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[VERMONT_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[CONNECTICUT_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t oriHouses = mGame->mGameProperties[ORIENTAL_AVENUE].uNumberOfHouses;
                uint8_t verHouses = mGame->mGameProperties[VERMONT_AVENUE].uNumberOfHouses;
                uint8_t conHouses = mGame->mGameProperties[CONNECTICUT_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (oriHouses > verHouses) ? 
                                   ((oriHouses > conHouses) ? oriHouses : conHouses) :
                                   ((verHouses > conHouses) ? verHouses : conHouses);
                return (oriHouses == maxHouses) || 
                       (verHouses == maxHouses) || 
                       (conHouses == maxHouses);
            }
            return false;
        }
        break;

        case PINK:
        {
            if(mGame->mGameProperties[ST_CHARLES_PLACE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[STATES_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[VIRGINIA_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t stcHouses = mGame->mGameProperties[ST_CHARLES_PLACE].uNumberOfHouses;
                uint8_t staHouses = mGame->mGameProperties[STATES_AVENUE].uNumberOfHouses;
                uint8_t virHouses = mGame->mGameProperties[VIRGINIA_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (stcHouses > staHouses) ? 
                                   ((stcHouses > virHouses) ? stcHouses : virHouses) :
                                   ((staHouses > virHouses) ? staHouses : virHouses);
                return (stcHouses == maxHouses) || 
                       (staHouses == maxHouses) || 
                       (virHouses == maxHouses);
            }
            return false;
        }
        break;

        case ORANGE:
        {
            if(mGame->mGameProperties[ST_JAMES_PLACE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[TENNESSEE_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[NEW_YORK_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t stjHouses = mGame->mGameProperties[ST_JAMES_PLACE].uNumberOfHouses;
                uint8_t tenHouses = mGame->mGameProperties[TENNESSEE_AVENUE].uNumberOfHouses;
                uint8_t nyHouses = mGame->mGameProperties[NEW_YORK_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (stjHouses > tenHouses) ? 
                                   ((stjHouses > nyHouses) ? stjHouses : nyHouses) :
                                   ((tenHouses > nyHouses) ? tenHouses : nyHouses);
                return (stjHouses == maxHouses) || 
                       (tenHouses == maxHouses) || 
                       (nyHouses == maxHouses);
            }
            return false;
        }
        break;

        case RED:
        {
            if(mGame->mGameProperties[ILLINOIS_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[INDIANA_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[KENTUCKY_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t illHouses = mGame->mGameProperties[ILLINOIS_AVENUE].uNumberOfHouses;
                uint8_t indHouses = mGame->mGameProperties[INDIANA_AVENUE].uNumberOfHouses;
                uint8_t kenHouses = mGame->mGameProperties[KENTUCKY_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (illHouses > indHouses) ? 
                                   ((illHouses > kenHouses) ? illHouses : kenHouses) :
                                   ((indHouses > kenHouses) ? indHouses : kenHouses);
                return (illHouses == maxHouses) || 
                       (indHouses == maxHouses) || 
                       (kenHouses == maxHouses);
            }
            return false;
        }
        break;

        case YELLOW:
        {
            if(mGame->mGameProperties[MARVIN_GARDENS].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[VENTNOR_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[ATLANTIC_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t marHouses = mGame->mGameProperties[MARVIN_GARDENS].uNumberOfHouses;
                uint8_t venHouses = mGame->mGameProperties[VENTNOR_AVENUE].uNumberOfHouses;
                uint8_t atlHouses = mGame->mGameProperties[ATLANTIC_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (marHouses > venHouses) ? 
                                   ((marHouses > atlHouses) ? marHouses : atlHouses) :
                                   ((venHouses > atlHouses) ? venHouses : atlHouses);
                return (marHouses == maxHouses) || 
                       (venHouses == maxHouses) || 
                       (atlHouses == maxHouses);
            }
            return false;
        }
        break;

        case GREEN:
        {
            if(mGame->mGameProperties[PENNSYLVANIA_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[NORTH_CAROLINA_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
               mGame->mGameProperties[PACIFIC_AVENUE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t penHouses = mGame->mGameProperties[PENNSYLVANIA_AVENUE].uNumberOfHouses;
                uint8_t norHouses = mGame->mGameProperties[NORTH_CAROLINA_AVENUE].uNumberOfHouses;
                uint8_t pacHouses = mGame->mGameProperties[PACIFIC_AVENUE].uNumberOfHouses;
                uint8_t maxHouses = (penHouses > norHouses) ? 
                                   ((penHouses > pacHouses) ? penHouses : pacHouses) :
                                   ((norHouses > pacHouses) ? norHouses : pacHouses);
                return (penHouses == maxHouses) || 
                       (norHouses == maxHouses) || 
                       (pacHouses == maxHouses);
            }
            return false;
        }
        break;

        case DARK_BLUE:
        {
            if (mGame->mGameProperties[BOARDWALK].eOwner == mPlayerSellingHouse->ePlayerTurnPosition &&
                mGame->mGameProperties[PARK_PLACE].eOwner == mPlayerSellingHouse->ePlayerTurnPosition) 
            {
                uint8_t boaHouses = mGame->mGameProperties[BOARDWALK].uNumberOfHouses;
                uint8_t parHouses = mGame->mGameProperties[PARK_PLACE].uNumberOfHouses;
                uint8_t maxHouses = (boaHouses > parHouses) ? boaHouses : parHouses;
                return (boaHouses == maxHouses) || (parHouses == maxHouses);
            }
            return false;
        }
        break;

        default:
        {
            printf("Error: Invalid property color set in m_house_can_be_sold()\n");
            return false;
        }
    }
}

bool
m_is_property_owned(mProperty* mPropertyToCheck)
{
    return(mPropertyToCheck->bOwned);
}

bool
m_is_property_owner(mPlayer* mPlayerToCheck, mProperty* mPropertyToCheck)
{
    return(mPlayerToCheck->ePlayerTurnPosition == mPropertyToCheck->eOwner);
}

mPropertyName
m_property_landed_on(uint32_t uPosition)
{
    switch(uPosition)
    {
        case MEDITERRANEAN_AVENUE_SQUARE:  return MEDITERRANEAN_AVENUE;
        case BALTIC_AVENUE_SQUARE:         return BALTIC_AVENUE;
        case ORIENTAL_AVENUE_SQUARE:       return ORIENTAL_AVENUE;
        case VERMONT_AVENUE_SQUARE:        return VERMONT_AVENUE;
        case CONNECTICUT_AVENUE_SQUARE:    return CONNECTICUT_AVENUE;
        case ST_CHARLES_PLACE_SQUARE:      return ST_CHARLES_PLACE;
        case STATES_AVENUE_SQUARE:         return STATES_AVENUE;
        case VIRGINIA_AVENUE_SQUARE:       return VIRGINIA_AVENUE;
        case ST_JAMES_PLACE_SQUARE:        return ST_JAMES_PLACE;
        case TENNESSEE_AVENUE_SQUARE:      return TENNESSEE_AVENUE;
        case NEW_YORK_AVENUE_SQUARE:       return NEW_YORK_AVENUE;
        case KENTUCKY_AVENUE_SQUARE:       return KENTUCKY_AVENUE;
        case INDIANA_AVENUE_SQUARE:        return INDIANA_AVENUE;
        case ILLINOIS_AVENUE_SQUARE:       return ILLINOIS_AVENUE;
        case ATLANTIC_AVENUE_SQUARE:       return ATLANTIC_AVENUE;
        case VENTNOR_AVENUE_SQUARE:        return VENTNOR_AVENUE;
        case MARVIN_GARDENS_SQUARE:        return MARVIN_GARDENS;
        case PACIFIC_AVENUE_SQUARE:        return PACIFIC_AVENUE;
        case NORTH_CAROLINA_AVENUE_SQUARE: return NORTH_CAROLINA_AVENUE;
        case PENNSYLVANIA_AVENUE_SQUARE:   return PENNSYLVANIA_AVENUE;
        case PARK_PLACE_SQUARE:            return PARK_PLACE;
        case BOARDWALK_SQUARE:             return BOARDWALK;
        default:                          return NO_PROPERTY;
    }
}

bool
m_is_railroad_owned(mRailroad* mRailroadToCheck)
{
    return(mRailroadToCheck->bOwned);
}

bool
m_is_railroad_owner(mPlayer* mPlayerToCheck, mRailroad* mRailroadToCheck)
{
    return(mPlayerToCheck->ePlayerTurnPosition == mRailroadToCheck->eOwner);
}

mRailroadName
m_railroad_landed_on(uint32_t uPosition)
{
    switch(uPosition)
    {
        case READING_RAILROAD_SQUARE:      return READING_RAILROAD;
        case PENNSYLVANIA_RAILROAD_SQUARE: return PENNSYLVANIA_RAILROAD;
        case B_AND_O_RAILROAD_SQUARE:      return B_AND_O_RAILROAD;
        case SHORT_LINE_RAILROAD_SQUARE:   return SHORT_LINE_RAILROAD;
        default:                           return NO_RAILROAD;
    }
}

bool
m_is_utility_owned(mUtility* mUtilityToCheck)
{
    return (mUtilityToCheck->bOwned);
}

bool
m_is_utility_owner(mPlayer* mPlayerToCheck, mUtility* mUtilityToCheck)
{
    return (mPlayerToCheck->ePlayerTurnPosition == mUtilityToCheck->eOwner);
}

mUtilityName
m_utility_landed_on(uint32_t uPosition)
{
    switch(uPosition) 
    {
        case ELECTRIC_COMPANY_SQUARE: return ELECTRIC_COMPANY;
        case WATER_WORKS_SQUARE:     return WATER_WORKS;
        default:                    return NO_UTILITY;
    }
}

void
m_show_props_owned(mPlayer* currentPlayer) 
{
    uint8_t ownedCount = 0;
    for (uint8_t i = 0; i < PROPERTY_TOTAL; i++)
    {
        mPropertyName prop = currentPlayer->ePropertyOwned[i];

        if(currentPlayer->ePropertyOwned[i] == NO_PROPERTY)
        {
            break;
        }
        if(i == 0)
        {
            printf("Properties Owned :\n");
        }
        const char* propName = m_property_enum_to_string(prop);
        printf("- %s\n", propName);
        ownedCount++;

    }

    if (ownedCount == 0)
    {
        printf("(No properties owned)\n");
    }
}

void
m_show_rails_owned(mPlayer* currentPlayer)
{
    uint8_t ownedCount = 0;
    for (int i = 0; i < RAILROAD_TOTAL; i++)
    {
        mRailroadName rail = currentPlayer->eRailroadsOwned[i];

        if(currentPlayer->eRailroadsOwned[i] == NO_RAILROAD)
        {
            break;
        }
        if(i == 0)
        {
            printf("Railroads Owned :\n");
        }
        const char* railName = m_railroad_enum_to_string(rail);
        printf("- %s\n", railName);
        ownedCount++;

    }

    if (ownedCount == 0) 
    {
        printf("(No railroads owned)\n");
    }
}

void
m_show_utils_owned(mPlayer* currentPlayer)
{
    uint8_t ownedCount = 0;
    for (uint8_t i = 0; i < UTILITY_TOTAL; i++)
    {
        mUtilityName utils = currentPlayer->eUtilityOwned[i];

        if(currentPlayer->eUtilityOwned[i] == NO_UTILITY)
        {
            break;
        }
        if(i == 0)
        {
            printf("Utilities Owned :\n");
        }
        const char* utilName = m_utility_enum_to_string(utils);
        printf("- %s\n", utilName);
        ownedCount++;

    }

    if (ownedCount == 0) {
        printf("(No utilities owned)\n");
    }
}

mPropertyName 
m_get_player_property(mPlayer* mCurrentPlayer) 
{
    m_show_props_owned(mCurrentPlayer);
    printf("\nSelect a property (enter number): ");

    int choice;
    scanf("%d", &choice);
    switch(choice) 
    {
        case 1:  return MEDITERRANEAN_AVENUE;
        case 2:  return BALTIC_AVENUE;
        case 3:  return ORIENTAL_AVENUE;
        case 4:  return VERMONT_AVENUE;
        case 5:  return CONNECTICUT_AVENUE;
        case 6:  return ST_CHARLES_PLACE;
        case 7:  return STATES_AVENUE;
        case 8:  return VIRGINIA_AVENUE;
        case 9:  return ST_JAMES_PLACE;
        case 10: return TENNESSEE_AVENUE;
        case 11: return NEW_YORK_AVENUE;
        case 12: return ILLINOIS_AVENUE;
        case 13: return INDIANA_AVENUE;
        case 14: return KENTUCKY_AVENUE;
        case 15: return MARVIN_GARDENS;
        case 16: return VENTNOR_AVENUE;
        case 17: return ATLANTIC_AVENUE;
        case 18: return PENNSYLVANIA_AVENUE;
        case 19: return NORTH_CAROLINA_AVENUE;
        case 20: return PACIFIC_AVENUE;
        case 21: return PARK_PLACE;
        case 22: return BOARDWALK;
        default: return NO_PROPERTY;
    }
}