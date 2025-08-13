

#include <string.h>

#include "m_init_game.h"


// ==================== PLAYER CREATION ==================== //
mPlayer*
m_create_player(uint8_t uPlayerPositionInIndex, mGameStartSettings mSettings)
{

    mPlayer* newPlayer = malloc(sizeof(mPlayer));
    memset(newPlayer, 0, sizeof(mPlayer));

    newPlayer->ePlayerTurnPosition   = uPlayerPositionInIndex;
    newPlayer->bGetOutOfJailFreeCard = false;
    newPlayer->bInJail               = false;
    newPlayer->bBankrupt             = false;
    newPlayer->bPlayerIsAI           = false;
    newPlayer->bActiveInAuction      = false;
    newPlayer->ePiece                = NO_PIECE_ASSIGNED;
    newPlayer->uMoney                = mSettings.uStartingMoney;
    newPlayer->uPosition             = GO_SQUARE;
    newPlayer->uJailTurns            = 0;

    for(uint8_t uPropCount = 0; uPropCount < PROP_OWNED_WITH_BUFFER; uPropCount++)
    {
        newPlayer->ePropertyOwned[uPropCount] = NO_PROPERTY;
    }
    for(uint8_t uRailroadCount = 0; uRailroadCount < RAIL_OWNED_WITH_BUFFER; uRailroadCount++)
    {
        newPlayer->eRailroadOwned[uRailroadCount] = NO_RAILROAD;
    }
    for(uint8_t uUtilityCount = 0; uUtilityCount < UTIL_OWNED_WITH_BUFFER; uUtilityCount++)
    {
        newPlayer->eUtilityOwned[uUtilityCount] = NO_UTILITY;
    }
    return newPlayer;
}

void
m_set_player_piece(mGameData* mGame, mPlayerPiece playerPiece, mPlayerNumber playerTurnPosition)
{
    mGame->mGamePlayers[playerTurnPosition]->ePiece = playerPiece;
}

void
m_set_player_as_AI(mGameData* mGame, mPlayerNumber playerTurnPosition)
{
    mGame->mGamePlayers[playerTurnPosition]->bPlayerIsAI = true;
}

// ==================== MOVEMENT ==================== //
void
m_move_player(mPlayer* mPlayerToMove, mDice* mRolledDice)
{
    uint8_t uSpacesToMove = mRolledDice->dice_one + mRolledDice->dice_two;

    // Calculate new position (handling wrap-around)
    uint8_t uNewPosition = mPlayerToMove->uPosition + uSpacesToMove;

    if (uNewPosition >= TOTAL_BOARD_SQUARES) 
    {
        uNewPosition %= TOTAL_BOARD_SQUARES; // Wrap around to 0 if >= 40
    }

    mPlayerToMove->uPosition = uNewPosition;
}

// only set position any triggerable action will need to be done elsewhere
void
m_move_player_to(mPlayer* currentPlayer, mBoardSquares destination)
{
    if(!currentPlayer && destination > TOTAL_BOARD_SQUARES)
    {
        __debugbreak();
        return;
    }
    currentPlayer->uPosition = destination;
}

// ==================== FINANCIAL ==================== //
// playerOwed will be NO_PLAYER if bank is owed
void
m_trigger_bankruptcy(mGameData* mGame, mPlayerNumber playerBankrupting, mGameState entityOwed, mPlayerNumber playerOwed)
{
    mPlayer* current_player = mGame->mGamePlayers[playerBankrupting];

    switch(entityOwed)
    {
        case OWE_PLAYER: 
        {
            m_transfer_assets_to_player(mGame, mGame->uCurrentPlayer, playerOwed);
            break;
        }
        case OWE_BANK:
        {
            m_return_assets_to_bank(mGame, mGame->uCurrentPlayer);
            break;
        }
    }
    current_player->bBankrupt = true;
    current_player->uMoney = 0;
    mGame->uActivePlayers--;

    m_game_over_check(mGame);
}

// ==================== OTHER ==================== //
void
m_show_player_status(mPlayer* mCurrentPlayer)
{
    printf("\n Player Status\n");

    printf("You have $%d\n", mCurrentPlayer->uMoney);
    m_show_props_owned(mCurrentPlayer);
    m_show_rails_owned(mCurrentPlayer);
    m_show_utils_owned(mCurrentPlayer);

    if(mCurrentPlayer->bGetOutOfJailFreeCard)
    {
        printf("You own a get out of jail free card\n");
    }
    if(mCurrentPlayer->bInJail)
    {
        printf("You are in jail\n");
    }
    printf("You are on square %s\n", m_player_position_to_string(mCurrentPlayer->uPosition));
}

void
m_show_player_assets(mPlayer* mCurrentPlayer)
{
    printf("\n Player %d Assets\n", mCurrentPlayer->ePlayerTurnPosition + 1);

    printf("You have $%d\n", mCurrentPlayer->uMoney);
    m_show_props_owned(mCurrentPlayer);
    m_show_rails_owned(mCurrentPlayer);
    m_show_utils_owned(mCurrentPlayer);

    if(mCurrentPlayer->bGetOutOfJailFreeCard)
    {
        printf("You own a get out of jail free card\n");
    }

}

const char*
m_player_position_to_string(uint8_t uPosition)
{
    if (uPosition < 0 || uPosition >= TOTAL_BOARD_SQUARES) 
    {
        return "INVALID_POSITION";
    }

    switch ((mBoardSquares)uPosition) 
    {
        case GO_SQUARE:                    return "GO";
        case MEDITERRANEAN_AVENUE_SQUARE:  return "Mediterranean Avenue";
        case COMMUNITY_CHEST_SQUARE_1:     return "Community Chest (1)";
        case BALTIC_AVENUE_SQUARE:         return "Baltic Avenue";
        case INCOME_TAX_SQUARE:            return "Income Tax";
        case READING_RAILROAD_SQUARE:      return "Reading Railroad";
        case ORIENTAL_AVENUE_SQUARE:       return "Oriental Avenue";
        case CHANCE_SQUARE_1:              return "Chance (1)";
        case VERMONT_AVENUE_SQUARE:        return "Vermont Avenue";
        case CONNECTICUT_AVENUE_SQUARE:    return "Connecticut Avenue";
        case JAIL_SQUARE:                  return "Jail";
        case ST_CHARLES_PLACE_SQUARE:      return "St. Charles Place";
        case ELECTRIC_COMPANY_SQUARE:      return "Electric Company";
        case STATES_AVENUE_SQUARE:         return "States Avenue";
        case VIRGINIA_AVENUE_SQUARE:       return "Virginia Avenue";
        case PENNSYLVANIA_RAILROAD_SQUARE: return "Pennsylvania Railroad";
        case ST_JAMES_PLACE_SQUARE:        return "St. James Place";
        case COMMUNITY_CHEST_SQUARE_2:     return "Community Chest (2)";
        case TENNESSEE_AVENUE_SQUARE:      return "Tennessee Avenue";
        case NEW_YORK_AVENUE_SQUARE:       return "New York Avenue";
        case FREE_PARKING_SQUARE:          return "Free Parking";
        case KENTUCKY_AVENUE_SQUARE:       return "Kentucky Avenue";
        case CHANCE_SQUARE_2:              return "Chance (2)";
        case INDIANA_AVENUE_SQUARE:        return "Indiana Avenue";
        case ILLINOIS_AVENUE_SQUARE:       return "Illinois Avenue";
        case B_AND_O_RAILROAD_SQUARE:      return "B. & O. Railroad";
        case ATLANTIC_AVENUE_SQUARE:       return "Atlantic Avenue";
        case VENTNOR_AVENUE_SQUARE:        return "Ventnor Avenue";
        case WATER_WORKS_SQUARE:           return "Water Works";
        case MARVIN_GARDENS_SQUARE:        return "Marvin Gardens";
        case GO_TO_JAIL_SQUARE:            return "Go to Jail";
        case PACIFIC_AVENUE_SQUARE:        return "Pacific Avenue";
        case NORTH_CAROLINA_AVENUE_SQUARE: return "North Carolina Avenue";
        case COMMUNITY_CHEST_SQUARE_3:     return "Community Chest (3)";
        case PENNSYLVANIA_AVENUE_SQUARE:   return "Pennsylvania Avenue";
        case SHORT_LINE_RAILROAD_SQUARE:   return "Short Line Railroad";
        case CHANCE_SQUARE_3:              return "Chance (3)";
        case PARK_PLACE_SQUARE:            return "Park Place";
        case LUXURY_TAX_SQUARE:            return "Luxury Tax";
        case BOARDWALK_SQUARE:             return "Boardwalk";
        default:                           return "UNKNOWN_SQUARE";
    }
}

void 
m_defrag_asset_arrays(mPlayer *mPlayer) 
{
    uint8_t uWritePos = 0;
    for (uint8_t uReadPos = 0; uReadPos < PROP_OWNED_WITH_BUFFER; uReadPos++) 
    {
        if (mPlayer->ePropertyOwned[uReadPos] != NO_PROPERTY) 
        {
            mPlayer->ePropertyOwned[uWritePos] = mPlayer->ePropertyOwned[uReadPos];
            uWritePos++;
        }
    }
    while (uWritePos < PROP_OWNED_WITH_BUFFER) 
    {
        mPlayer->ePropertyOwned[uWritePos] = NO_PROPERTY;
        uWritePos++;
    }

    uWritePos = 0;
    for (uint8_t uReadPos = 0; uReadPos < UTIL_OWNED_WITH_BUFFER; uReadPos++) 
    {
        if (mPlayer->eUtilityOwned[uReadPos] != NO_UTILITY) 
        {
            mPlayer->eUtilityOwned[uWritePos] = mPlayer->eUtilityOwned[uReadPos];
            uWritePos++;
        }
    }
    while (uWritePos < UTIL_OWNED_WITH_BUFFER) 
    {
        mPlayer->eUtilityOwned[uWritePos] = NO_UTILITY;
        uWritePos++;
    }
    uWritePos = 0;
    for (uint8_t uReadPos = 0; uReadPos < RAIL_OWNED_WITH_BUFFER; uReadPos++) 
    {
        if (mPlayer->eRailroadOwned[uReadPos] != NO_RAILROAD) 
        {
            mPlayer->eRailroadOwned[uWritePos] = mPlayer->eRailroadOwned[uReadPos];
            uWritePos++;
        }
    }
    while (uWritePos < RAIL_OWNED_WITH_BUFFER) 
    {
        mPlayer->eRailroadOwned[uWritePos] = NO_RAILROAD;
        uWritePos++;
    }
}