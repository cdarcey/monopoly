#include "monopoly_init.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ==================== HARDCODED PROPERTY DATA ==================== //

static void
m_init_properties(mProperty* pProperties)
{
    // brown properties
    pProperties[MEDITERRANEAN_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Mediterranean Avenue",
        .uPrice         = 60,
        .uRentBase      = 2,
        .uRentMonopoly  = 4,
        .uMortgageValue = 30,
        .uPosition      = 1,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_BROWN,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 50,
        .auRentWithHouses = {2, 10, 30, 90, 160, 250}
    };

    pProperties[BALTIC_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Baltic Avenue",
        .uPrice         = 60,
        .uRentBase      = 4,
        .uRentMonopoly  = 8,
        .uMortgageValue = 30,
        .uPosition      = 3,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_BROWN,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 50,
        .auRentWithHouses = {4, 20, 60, 180, 320, 450}
    };

    // light blue properties
    pProperties[ORIENTAL_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Oriental Avenue",
        .uPrice         = 100,
        .uRentBase      = 6,
        .uRentMonopoly  = 12,
        .uMortgageValue = 50,
        .uPosition      = 6,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_LIGHT_BLUE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 50,
        .auRentWithHouses = {6, 30, 90, 270, 400, 550}
    };

    pProperties[VERMONT_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Vermont Avenue",
        .uPrice         = 100,
        .uRentBase      = 6,
        .uRentMonopoly  = 12,
        .uMortgageValue = 50,
        .uPosition      = 8,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_LIGHT_BLUE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 50,
        .auRentWithHouses = {6, 30, 90, 270, 400, 550}
    };

    pProperties[CONNECTICUT_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Connecticut Avenue",
        .uPrice         = 120,
        .uRentBase      = 8,
        .uRentMonopoly  = 16,
        .uMortgageValue = 60,
        .uPosition      = 9,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_LIGHT_BLUE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 50,
        .auRentWithHouses = {8, 40, 100, 300, 450, 600}
    };

    // pink properties
    pProperties[ST_CHARLES_PLACE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "St. Charles Place",
        .uPrice         = 140,
        .uRentBase      = 10,
        .uRentMonopoly  = 20,
        .uMortgageValue = 70,
        .uPosition      = 11,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_PINK,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {10, 50, 150, 450, 625, 750}
    };

    pProperties[STATES_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "States Avenue",
        .uPrice         = 140,
        .uRentBase      = 10,
        .uRentMonopoly  = 20,
        .uMortgageValue = 70,
        .uPosition      = 13,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_PINK,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {10, 50, 150, 450, 625, 750}
    };

    pProperties[VIRGINIA_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Virginia Avenue",
        .uPrice         = 160,
        .uRentBase      = 12,
        .uRentMonopoly  = 24,
        .uMortgageValue = 80,
        .uPosition      = 14,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_PINK,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {12, 60, 180, 500, 700, 900}
    };

    // orange properties
    pProperties[ST_JAMES_PLACE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "St. James Place",
        .uPrice         = 180,
        .uRentBase      = 14,
        .uRentMonopoly  = 28,
        .uMortgageValue = 90,
        .uPosition      = 16,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_ORANGE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {14, 70, 200, 550, 750, 950}
    };

    pProperties[TENNESSEE_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Tennessee Avenue",
        .uPrice         = 180,
        .uRentBase      = 14,
        .uRentMonopoly  = 28,
        .uMortgageValue = 90,
        .uPosition      = 18,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_ORANGE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {14, 70, 200, 550, 750, 950}
    };

    pProperties[NEW_YORK_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "New York Avenue",
        .uPrice         = 200,
        .uRentBase      = 16,
        .uRentMonopoly  = 32,
        .uMortgageValue = 100,
        .uPosition      = 19,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_ORANGE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 100,
        .auRentWithHouses = {16, 80, 220, 600, 800, 1000}
    };

    // red properties
    pProperties[KENTUCKY_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Kentucky Avenue",
        .uPrice         = 220,
        .uRentBase      = 18,
        .uRentMonopoly  = 36,
        .uMortgageValue = 110,
        .uPosition      = 21,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_RED,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {18, 90, 250, 700, 875, 1050}
    };

    pProperties[INDIANA_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Indiana Avenue",
        .uPrice         = 220,
        .uRentBase      = 18,
        .uRentMonopoly  = 36,
        .uMortgageValue = 110,
        .uPosition      = 23,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_RED,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {18, 90, 250, 700, 875, 1050}
    };

    pProperties[ILLINOIS_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Illinois Avenue",
        .uPrice         = 240,
        .uRentBase      = 20,
        .uRentMonopoly  = 40,
        .uMortgageValue = 120,
        .uPosition      = 24,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_RED,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {20, 100, 300, 750, 925, 1100}
    };

    // yellow properties
    pProperties[ATLANTIC_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Atlantic Avenue",
        .uPrice         = 260,
        .uRentBase      = 22,
        .uRentMonopoly  = 44,
        .uMortgageValue = 130,
        .uPosition      = 26,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_YELLOW,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {22, 110, 330, 800, 975, 1150}
    };

    pProperties[VENTNOR_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Ventnor Avenue",
        .uPrice         = 260,
        .uRentBase      = 22,
        .uRentMonopoly  = 44,
        .uMortgageValue = 130,
        .uPosition      = 27,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_YELLOW,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {22, 110, 330, 800, 975, 1150}
    };

    pProperties[MARVIN_GARDENS_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Marvin Gardens",
        .uPrice         = 280,
        .uRentBase      = 24,
        .uRentMonopoly  = 48,
        .uMortgageValue = 140,
        .uPosition      = 29,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_YELLOW,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 150,
        .auRentWithHouses = {24, 120, 360, 850, 1025, 1200}
    };

    // green properties
    pProperties[PACIFIC_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Pacific Avenue",
        .uPrice         = 300,
        .uRentBase      = 26,
        .uRentMonopoly  = 52,
        .uMortgageValue = 150,
        .uPosition      = 31,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_GREEN,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 200,
        .auRentWithHouses = {26, 130, 390, 900, 1100, 1275}
    };

    pProperties[NORTH_CAROLINA_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "North Carolina Avenue",
        .uPrice         = 300,
        .uRentBase      = 26,
        .uRentMonopoly  = 52,
        .uMortgageValue = 150,
        .uPosition      = 32,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_GREEN,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 200,
        .auRentWithHouses = {26, 130, 390, 900, 1100, 1275}
    };

    pProperties[PENNSYLVANIA_AVENUE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Pennsylvania Avenue",
        .uPrice         = 320,
        .uRentBase      = 28,
        .uRentMonopoly  = 56,
        .uMortgageValue = 160,
        .uPosition      = 34,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_GREEN,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 200,
        .auRentWithHouses = {28, 150, 450, 1000, 1200, 1400}
    };

    // dark blue properties
    pProperties[PARK_PLACE_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Park Place",
        .uPrice         = 350,
        .uRentBase      = 35,
        .uRentMonopoly  = 70,
        .uMortgageValue = 175,
        .uPosition      = 37,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_DARK_BLUE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 200,
        .auRentWithHouses = {35, 175, 500, 1100, 1300, 1500}
    };

    pProperties[BOARDWALK_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Boardwalk",
        .uPrice         = 400,
        .uRentBase      = 50,
        .uRentMonopoly  = 100,
        .uMortgageValue = 200,
        .uPosition      = 39,
        .eType          = PROPERTY_TYPE_STREET,
        .eColor         = COLOR_DARK_BLUE,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 200,
        .auRentWithHouses = {50, 200, 600, 1400, 1700, 2000}
    };

    // railroads - no houses/hotels
    pProperties[READING_RAILROAD_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Reading Railroad",
        .uPrice         = 200,
        .uRentBase      = 25,
        .uRentMonopoly  = 0,
        .uMortgageValue = 100,
        .uPosition      = 5,
        .eType          = PROPERTY_TYPE_RAILROAD,
        .eColor         = COLOR_RAILROAD,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };

    pProperties[PENNSYLVANIA_RAILROAD_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Pennsylvania Railroad",
        .uPrice         = 200,
        .uRentBase      = 25,
        .uRentMonopoly  = 0,
        .uMortgageValue = 100,
        .uPosition      = 15,
        .eType          = PROPERTY_TYPE_RAILROAD,
        .eColor         = COLOR_RAILROAD,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };

    pProperties[BO_RAILROAD_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "B&O Railroad",
        .uPrice         = 200,
        .uRentBase      = 25,
        .uRentMonopoly  = 0,
        .uMortgageValue = 100,
        .uPosition      = 25,
        .eType          = PROPERTY_TYPE_RAILROAD,
        .eColor         = COLOR_RAILROAD,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };

    pProperties[SHORT_LINE_RAILROAD_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Short Line Railroad",
        .uPrice         = 200,
        .uRentBase      = 25,
        .uRentMonopoly  = 0,
        .uMortgageValue = 100,
        .uPosition      = 35,
        .eType          = PROPERTY_TYPE_RAILROAD,
        .eColor         = COLOR_RAILROAD,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };

    // utilities - no houses/hotels
    pProperties[ELECTRIC_COMPANY_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Electric Company",
        .uPrice         = 150,
        .uRentBase      = 0,
        .uRentMonopoly  = 0,
        .uMortgageValue = 75,
        .uPosition      = 12,
        .eType          = PROPERTY_TYPE_UTILITY,
        .eColor         = COLOR_UTILITY,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };

    pProperties[WATER_WORKS_PROPERTY_ARRAY_INDEX] = (mProperty){
        .cName          = "Water Works",
        .uPrice         = 150,
        .uRentBase      = 0,
        .uRentMonopoly  = 0,
        .uMortgageValue = 75,
        .uPosition      = 28,
        .eType          = PROPERTY_TYPE_UTILITY,
        .eColor         = COLOR_UTILITY,
        .uOwnerIndex    = BANK_PLAYER_INDEX,
        .bIsMortgaged   = false,
        .uHouses        = 0,
        .bHasHotel      = false,
        .uHouseCost     = 0,
        .auRentWithHouses = {0}
    };
}

// ==================== HARDCODED CARD DATA ==================== //

static void
m_init_chance_cards(mChanceCard* pCards)
{
    strcpy(pCards[0].cDescription, "Advance to Go (Collect $200)");
    pCards[0].uCardID = 0;

    strcpy(pCards[1].cDescription, "Advance to Illinois Avenue");
    pCards[1].uCardID = 1;

    strcpy(pCards[2].cDescription, "Advance to St. Charles Place");
    pCards[2].uCardID = 2;

    strcpy(pCards[3].cDescription, "Advance token to nearest Utility");
    pCards[3].uCardID = 3;

    strcpy(pCards[4].cDescription, "Advance token to nearest Railroad");
    pCards[4].uCardID = 4;

    strcpy(pCards[5].cDescription, "Bank pays you dividend of $50");
    pCards[5].uCardID = 5;

    strcpy(pCards[6].cDescription, "Get Out of Jail Free");
    pCards[6].uCardID = 6;

    strcpy(pCards[7].cDescription, "Go Back 3 Spaces");
    pCards[7].uCardID = 7;

    strcpy(pCards[8].cDescription, "Go to Jail");
    pCards[8].uCardID = 8;

    strcpy(pCards[9].cDescription, "Make general repairs on all your property");
    pCards[9].uCardID = 9;

    strcpy(pCards[10].cDescription, "Pay poor tax of $15");
    pCards[10].uCardID = 10;

    strcpy(pCards[11].cDescription, "Take a trip to Reading Railroad");
    pCards[11].uCardID = 11;

    strcpy(pCards[12].cDescription, "Take a walk on the Boardwalk");
    pCards[12].uCardID = 12;

    strcpy(pCards[13].cDescription, "You have been elected Chairman of the Board");
    pCards[13].uCardID = 13;

    strcpy(pCards[14].cDescription, "Your building loan matures - Collect $150");
    pCards[14].uCardID = 14;

    strcpy(pCards[15].cDescription, "You have won a crossword competition - Collect $100");
    pCards[15].uCardID = 15;
}

static void
m_init_community_chest_cards(mCommunityChestCard* pCards)
{
    strcpy(pCards[0].cDescription, "Advance to Go (Collect $200)");
    pCards[0].uCardID = 0;

    strcpy(pCards[1].cDescription, "Bank error in your favor - Collect $200");
    pCards[1].uCardID = 1;

    strcpy(pCards[2].cDescription, "Doctor's fees - Pay $50");
    pCards[2].uCardID = 2;

    strcpy(pCards[3].cDescription, "From sale of stock you get $50");
    pCards[3].uCardID = 3;

    strcpy(pCards[4].cDescription, "Get Out of Jail Free");
    pCards[4].uCardID = 4;

    strcpy(pCards[5].cDescription, "Go to Jail");
    pCards[5].uCardID = 5;

    strcpy(pCards[6].cDescription, "Grand Opera Night - Collect $50 from every player");
    pCards[6].uCardID = 6;

    strcpy(pCards[7].cDescription, "Holiday Fund matures - Receive $100");
    pCards[7].uCardID = 7;

    strcpy(pCards[8].cDescription, "Income tax refund - Collect $20");
    pCards[8].uCardID = 8;

    strcpy(pCards[9].cDescription, "It is your birthday - Collect $10 from every player");
    pCards[9].uCardID = 9;

    strcpy(pCards[10].cDescription, "Life insurance matures - Collect $100");
    pCards[10].uCardID = 10;

    strcpy(pCards[11].cDescription, "Hospital fees - Pay $100");
    pCards[11].uCardID = 11;

    strcpy(pCards[12].cDescription, "School fees - Pay $150");
    pCards[12].uCardID = 12;

    strcpy(pCards[13].cDescription, "Receive $25 consultancy fee");
    pCards[13].uCardID = 13;

    strcpy(pCards[14].cDescription, "You are assessed for street repairs");
    pCards[14].uCardID = 14;

    strcpy(pCards[15].cDescription, "You have won second prize in a beauty contest - Collect $10");
    pCards[15].uCardID = 15;
}

// ==================== DECK INITIALIZATION ==================== //

void
m_shuffle_deck(mDeckState* pDeck)
{
    // initialize indices 0-15
    for(uint8_t i = 0; i < 16; i++)
    {
        pDeck->auIndices[i] = i;
    }

    // fisher-yates shuffle
    for(uint8_t i = 15; i > 0; i--)
    {
        uint8_t j = rand() % (i + 1);
        uint8_t temp = pDeck->auIndices[i];
        pDeck->auIndices[i] = pDeck->auIndices[j];
        pDeck->auIndices[j] = temp;
    }

    pDeck->uCurrentIndex = 0;
}

// ==================== PLAYER INITIALIZATION ==================== //

static void
m_init_players(mPlayer* pPlayers, uint8_t uPlayerCount, uint32_t uStartingMoney)
{
    for(uint8_t i = 0; i < uPlayerCount; i++)
    {
        pPlayers[i].uMoney = uStartingMoney;
        pPlayers[i].uPosition = 0;
        pPlayers[i].uJailTurns = 0;
        pPlayers[i].uPropertyCount = 0;
        pPlayers[i].ePiece = PIECE_NONE;
        pPlayers[i].bHasJailFreeCard = false;
        pPlayers[i].bIsBankrupt = false;

        // initialize property array with empty slots (bank owns = no property in this slot)
        for(uint8_t j = 0; j < PROPERTY_ARRAY_SIZE; j++)
        {
            pPlayers[i].auPropertiesOwned[j] = BANK_PLAYER_INDEX;
        }
    }
}

// ==================== GAME INITIALIZATION ==================== //

mGameData*
m_init_game(mGameSettings tSettings)
{
    // seed random number generator
    srand((unsigned int)time(NULL));

    // allocate and zero-initialize game data
    mGameData* pGame = calloc(1, sizeof(mGameData));
    if(!pGame)
    {
        return NULL;
    }

    // initialize properties
    m_init_properties(pGame->amProperties);

    // initialize cards
    m_init_chance_cards(pGame->amChanceCards);
    m_init_community_chest_cards(pGame->amCommunityChestCards);

    // shuffle decks
    m_shuffle_deck(&pGame->tChanceDeck);
    m_shuffle_deck(&pGame->tCommunityChestDeck);

    // initialize players
    m_init_players(pGame->amPlayers, tSettings.uPlayerCount, tSettings.uStartingMoney);

    // initialize dice
    pGame->tDice.uDie1 = 1;
    pGame->tDice.uDie2 = 1;

    // initialize game state
    pGame->uPlayerCount = tSettings.uPlayerCount;
    pGame->uCurrentPlayerIndex = PLAYER_ONE_ARRAY_INDEX;
    pGame->uActivePlayers = tSettings.uPlayerCount;
    pGame->uRoundCount = 0;
    pGame->uJailFine = tSettings.uJailFine;
    pGame->eState = GAME_STATE_RUNNING;
    pGame->bIsRunning = true;
    pGame->uGlobalHotelSupply = 12;
    pGame->uGlobalHouseSupply = 32;

    return pGame;
}

// ==================== MEMORY CLEANUP ==================== //

void
m_free_game(mGameData* pGame)
{
    if(pGame)
    {
        free(pGame);
    }
}
