

#include "m_init_game.h"


// ==================== CARD FUNCTIONS ==================== //
void 
m_execute_chance_card(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    bool bGetOutOfJailFreeCardOwned = m_get_out_jail_card_owned(mGame);
    mChanceCard* m_card_pulled = m_draw_chance_card(mGame->mGameChanceCards, &mGame->mGameDeckIndexBuffers[CHANCE_INDEX_BUFFER], bGetOutOfJailFreeCardOwned);

    switch(m_card_pulled->uID)
    {
        case 1:
        {
           m_move_player_to(current_player, GO_SQUARE);
           current_player->uMoney += 200;
        }
        break;
        case 2:
        {
            if(current_player->uPosition > ILLINOIS_AVENUE_SQUARE)
            {
                current_player->uMoney += 200;
            }
            m_move_player_to(current_player, ILLINOIS_AVENUE_SQUARE);
        }
        break;
        case 3:
            if(current_player->uPosition > ST_CHARLES_PLACE_SQUARE)
            {
                current_player->uMoney += 200;
            }
            m_move_player_to(current_player, ST_CHARLES_PLACE_SQUARE);
        break;
        case 4:
        {
            // TODO: needs to be tested
            if(current_player->uPosition < ELECTRIC_COMPANY_SQUARE || current_player->uPosition > WATER_WORKS_SQUARE)
            {
                m_move_player_to(current_player, ELECTRIC_COMPANY_SQUARE);
                if(current_player->ePlayerTurnPosition == mGame->mGameUtilities[ELECTRIC_COMPANY].eOwner)
                {
                    return;
                }
                else if(current_player->ePlayerTurnPosition != mGame->mGameUtilities[ELECTRIC_COMPANY].eOwner)
                {
                    m_pay_rent_utility(mGame->mGamePlayers[mGame->mGameUtilities[ELECTRIC_COMPANY].eOwner], current_player, &mGame->mGameUtilities[ELECTRIC_COMPANY], mGame->mGameDice);
                }
                else if(mGame->mGameUtilities[ELECTRIC_COMPANY].bOwned == false)
                {
                    m_buy_utility(&mGame->mGameUtilities[ELECTRIC_COMPANY], current_player);
                }
                else
                {
                    printf("control path missed on chance card 4\n");
                }
            }
            else if(current_player->uPosition > ELECTRIC_COMPANY && current_player->uPosition < WATER_WORKS)
            {
                m_move_player_to(current_player, WATER_WORKS_SQUARE);
                if(current_player->ePlayerTurnPosition == mGame->mGameUtilities[WATER_WORKS].eOwner)
                {
                    return;
                }
                else if(current_player->ePlayerTurnPosition != mGame->mGameUtilities[WATER_WORKS].eOwner)
                {
                    m_pay_rent_utility(mGame->mGamePlayers[mGame->mGameUtilities[WATER_WORKS].eOwner], current_player, &mGame->mGameUtilities[WATER_WORKS], mGame->mGameDice);
                }
                else if(mGame->mGameUtilities[WATER_WORKS].bOwned == false)
                {
                    m_buy_utility(&mGame->mGameUtilities[WATER_WORKS], current_player);
                }
                else
                {
                    printf("control path missed on chance card 4\n");
                }
            }
        }
        break;
        case 5:
        {
            // TODO: needs to be tested
            if(current_player->uPosition < READING_RAILROAD_SQUARE || current_player->uPosition > SHORT_LINE_RAILROAD_SQUARE)
            {
                m_move_player_to(current_player, READING_RAILROAD_SQUARE);
                if(mGame->mGameRailroads[READING_RAILROAD].bOwned == false)
                {
                    m_buy_railroad(&mGame->mGameRailroads[READING_RAILROAD], current_player);
                }
                else if(mGame->mGameRailroads[READING_RAILROAD].bOwned == true)
                {
                    // TODO: not elegant way to pay twice the rent
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[READING_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[READING_RAILROAD]);
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[READING_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[READING_RAILROAD]);
                }
            }
            else if(current_player->uPosition >= READING_RAILROAD_SQUARE && current_player->uPosition <= PENNSYLVANIA_RAILROAD_SQUARE)
            {
                m_move_player_to(current_player, PENNSYLVANIA_RAILROAD_SQUARE);
                if(mGame->mGameRailroads[PENNSYLVANIA_RAILROAD].bOwned == false)
                {
                    m_buy_railroad(&mGame->mGameRailroads[PENNSYLVANIA_RAILROAD], current_player);
                }
                else if(mGame->mGameRailroads[PENNSYLVANIA_RAILROAD].bOwned == true)
                {
                    // TODO: not elegant way to pay twice the rent
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[PENNSYLVANIA_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[PENNSYLVANIA_RAILROAD]);
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[PENNSYLVANIA_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[PENNSYLVANIA_RAILROAD]);
                }
            }
            else if(current_player->uPosition >= PENNSYLVANIA_RAILROAD_SQUARE && current_player->uPosition <= B_AND_O_RAILROAD_SQUARE)
            {
                m_move_player_to(current_player, B_AND_O_RAILROAD_SQUARE);
                if(mGame->mGameRailroads[B_AND_O_RAILROAD].bOwned == false)
                {
                    m_buy_railroad(&mGame->mGameRailroads[B_AND_O_RAILROAD], current_player);
                }
                else if(mGame->mGameRailroads[B_AND_O_RAILROAD].bOwned == true)
                {
                    // TODO: not elegant way to pay twice the rent
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[B_AND_O_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[B_AND_O_RAILROAD]);
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[B_AND_O_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[B_AND_O_RAILROAD]);
                }
            }
            else
            {
                m_move_player_to(current_player, SHORT_LINE_RAILROAD_SQUARE);
                if(mGame->mGameRailroads[SHORT_LINE_RAILROAD].bOwned == false)
                {
                    m_buy_railroad(&mGame->mGameRailroads[SHORT_LINE_RAILROAD], current_player);
                }
                else if(mGame->mGameRailroads[SHORT_LINE_RAILROAD].bOwned == true)
                {
                    // TODO: not elegant way to pay twice the rent
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[SHORT_LINE_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[SHORT_LINE_RAILROAD]);
                    m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[SHORT_LINE_RAILROAD].eOwner], current_player, &mGame->mGameRailroads[SHORT_LINE_RAILROAD]);
                }
            }
        }
        break;
        case 6:
        {
            current_player->uMoney += 50;
        }
        break;
        case 7:
        {
            current_player->uMoney += 100;
        }
        break;
        case 8:
        {
            if(current_player->uPosition >= 3)
            {
                current_player->uPosition -= 3;
            }
            else
            {
                current_player->uPosition = 40 - current_player->uPosition;
            }
        }
        break;
        case 9:
        {
            m_move_player_to(current_player, JAIL_SQUARE);
            current_player->bInJail = true;
        }
        break;
        case 10:
        {
            // TODO: "Make general repairs on all your property. Pay $25 for each house and $100 for each hotel you own."
        }
        break;
        case 11:
        {
            if(current_player->uMoney <= 15)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;
            }
            else
            {
                current_player->uMoney -= 15;
            }
        }
        break;
        case 12:
        {
            if(current_player->uPosition > READING_RAILROAD_SQUARE)
            {
                current_player->uMoney += 200;
            }
            m_move_player_to(current_player, READING_RAILROAD_SQUARE);
        }
        break;
        case 13:
        {
            m_move_player_to(current_player, BOARDWALK_SQUARE);
        }
        break;
        case 14:
        {
            uint8_t uAmountToPay = (mGame->uActivePlayers - 1) * 50;
            if(!m_can_player_afford(current_player, uAmountToPay))
            {
                if(!m_attempt_emergency_payment(mGame, current_player, uAmountToPay))
                {
                    m_trigger_bankruptcy(mGame, current_player->ePlayerTurnPosition, false, NO_PLAYER);
                }
            }
            else
            {
                current_player->uMoney -= (mGame->uActivePlayers - 1) * 50;

                for(uint8_t uPlayersToPay = 0; uPlayersToPay < mGame->uStartingPlayerCount; uPlayersToPay++)
                {
                    if(current_player->ePlayerTurnPosition != mGame->mGamePlayers[uPlayersToPay]->ePlayerTurnPosition && mGame->mGamePlayers[uPlayersToPay]->bBankrupt != false)
                    {
                        mGame->mGamePlayers[uPlayersToPay]->uMoney += 50;
                    }
                }
            }
        }
        break;
        case 15:
        {
            current_player->uMoney += 150;
        }
        break;
        case 16:
        {
            current_player->bGetOutOfJailFreeCard = true;
        }
        break;
    }
}

void
m_execute_community_chest_card(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    bool bGetOutOfJailFreeCardOwned = m_get_out_jail_card_owned(mGame);
    mCommunityChestCard* m_card_pulled = m_draw_community_chest_card(mGame->mGameCommChesCards, &mGame->mGameDeckIndexBuffers[COMM_CHEST_INDEX_BUFFER], bGetOutOfJailFreeCardOwned);

    switch(m_card_pulled->uID)
    {
        case 1:
        {
            m_move_player_to(current_player, GO_SQUARE);
            current_player->uMoney += UNIVERSAL_BASIC_INCOME;
        }
        break;
        case 2:
        {
            current_player->uMoney += 200;
        }
        break;
        case 3:
        {
            if(current_player->uMoney <= 100)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;
                return;
            }
            else
            {
            current_player->uMoney -= 50;
            }
        }
        break;
        case 4:
        {
            current_player->uMoney += 50;
        }
        break;
        case 5:
        {
            current_player->uMoney += 100;
        }
        break;
        case 6:
        {
            m_move_player_to(current_player, JAIL_SQUARE);
            current_player->bInJail = true;
        }
        break;
        case 7:
        {
            current_player->uMoney += 100;
        }
        break;
        case 8:
        {
            current_player->uMoney += 20;
        }
        break;
        case 9:
        {
            // "It is your birthday. Collect $10 from every player."
            // TODO: needs to be tested
            uint8_t uPlayersWithMoney = 0;

            for (uint8_t i = 0; i < mGame->uStartingPlayerCount; i++) 
            {
                if (mGame->uCurrentPlayer == i || mGame->mGamePlayers[i]->bBankrupt) 
                {
                    continue;
                }

                if (mGame->mGamePlayers[i]->uMoney >= 10)
                {
                    mGame->mGamePlayers[i]->uMoney -= 10;
                    uPlayersWithMoney++;
                } 
                else 
                {
                    mGame->mGamePlayers[i]->uMoney = 0;
                    mGame->mGamePlayers[i]->bBankrupt = true;
                }
            }
            current_player->uMoney += uPlayersWithMoney * 10;
        }
        break;
        case 10:
        {
            current_player->uMoney += 100;
        }
        break;
        case 11:
        {
            if(current_player->uMoney <= 100)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;
            }
            else
            {
            current_player->uMoney -= 100;
            }
        }
        break;
        case 12:
        {
            if(current_player->uMoney <= 50)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;
                return;
            }
            else
            {
            current_player->uMoney -= 50;
            }
        }
        break;
        case 13:
        {
            current_player->uMoney += 25;
        }
        break;
        case 14:
        {
            // TODO: "You are assessed for street repairs. Pay $40 per house and $115 per hotel you own."
            // TODO: needs to be tested
            uint8_t uHousesOwned = 0;
            uint8_t uHotelsOwned = 0;
            for(uint16_t i = 0; i < PROPERTY_TOTAL; i++)
            {
                for(uint8_t j = 0; j < MAX_HOUSES; j++)
                {
                    if(mGame->mGameProperties[j].eOwner == current_player->ePlayerTurnPosition)
                    {
                        uHousesOwned += mGame->mGameProperties[j].uNumberOfHouses;
                        if(j > 2)
                        {
                            uHotelsOwned += mGame->mGameProperties[j].uNumberOfHotels;
                        }
                    }
                }
            }
            uint8_t uPayment = (uHousesOwned * 40) + (uHotelsOwned * 115);
            if(current_player->uMoney <= uPayment)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;
            }
            else if(current_player->uMoney > uPayment)
            {
                current_player->uMoney -= uPayment;
            }
            else
            {
                printf("control path missed on comm chest card 14\n");
            }
        }
        break;
        case 15:
        {
            current_player->uMoney += 10;
        }
        break;
        case 16:
        {
            current_player->bGetOutOfJailFreeCard = true;
        }
        break;
    }
}


// ==================== HELPERS ==================== //
mDeckIndexBuffer 
m_create_deck_index_buffer() 
{
    mDeckIndexBuffer newDeckIndexBuffer;

    for (uint8_t i = 0; i < TOTAL_CHANCE_CARDS; i++) // chance and comm chest cards equal
    {
        newDeckIndexBuffer.uIndexBuffer[i] = i;
    }

    for (uint8_t i = TOTAL_CHANCE_CARDS - 1; i > 0; i--) 
    {
        uint8_t j = rand() % (i + 1);
        uint8_t temp = newDeckIndexBuffer.uIndexBuffer[i];
        newDeckIndexBuffer.uIndexBuffer[i] = newDeckIndexBuffer.uIndexBuffer[j];
        newDeckIndexBuffer.uIndexBuffer[j] = temp;
    }

    newDeckIndexBuffer.uCurrentCard = 0;
    return newDeckIndexBuffer;
}

mCommunityChestCard* 
m_draw_community_chest_card(mCommunityChestCard* mCurrentCommChestDeck, mDeckIndexBuffer* mDeckIndex, bool bJailFreeOwned)
{
    if (!mCurrentCommChestDeck || !mDeckIndex) {
        return NULL;
    }

    uint8_t activeCards = TOTAL_COMM_CHEST_CARDS - (bJailFreeOwned ? 1 : 0); // 15 if jail free card is owned
    if (mDeckIndex->uCurrentCard >= activeCards) 
    { 
        for (uint8_t i = activeCards - 1; i > 0; i--) 
        {
            uint8_t j = rand() % (i + 1);
            uint8_t temp = mDeckIndex->uIndexBuffer[i];
            mDeckIndex->uIndexBuffer[i] = mDeckIndex->uIndexBuffer[j];
            mDeckIndex->uIndexBuffer[j] = temp;
        }
        mDeckIndex->uCurrentCard = 0;
    }

    uint8_t uCardDrawIndex = mDeckIndex->uIndexBuffer[mDeckIndex->uCurrentCard];
    mDeckIndex->uCurrentCard++;
    return &mCurrentCommChestDeck[uCardDrawIndex];
}

mChanceCard*
m_draw_chance_card(mChanceCard* mCurrentChanceDeck, mDeckIndexBuffer* mDeckIndex, bool bJailFreeOwned)
{
    if (!mCurrentChanceDeck || !mDeckIndex) {
        return NULL;
    }

    uint8_t activeCards = TOTAL_CHANCE_CARDS - (bJailFreeOwned ? 1 : 0); // 15 if jail free card is owned
    if (mDeckIndex->uCurrentCard >= activeCards)
    {
        for (uint8_t i = activeCards - 1; i > 0; i--)
        {
            uint8_t j = rand() % (i + 1);
            uint8_t temp = mDeckIndex->uIndexBuffer[i];
            mDeckIndex->uIndexBuffer[i] = mDeckIndex->uIndexBuffer[j];
            mDeckIndex->uIndexBuffer[j] = temp;
        }
        mDeckIndex->uCurrentCard = 0;
    }

    uint8_t uCardDrawIndex = mDeckIndex->uIndexBuffer[mDeckIndex->uCurrentCard];
    mDeckIndex->uCurrentCard++;
    return &mCurrentChanceDeck[uCardDrawIndex];
}

bool
m_get_out_jail_card_owned(mGameData* mGame)
{
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
    {
        if(mGame->mGamePlayers[i] == NULL)
        {
            return false;
        }
        else if(mGame->mGamePlayers[i]->bGetOutOfJailFreeCard)
        {
            return true;
        }
    }
    return false;
}