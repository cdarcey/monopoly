

#include "m_ai.h"


void 
m_ai_player_turn(mGameData* mGame)
{
    mPlayer* current_player = mGame->mGamePlayers[mGame->uCurrentPlayer];
    switch (current_player->uPosition)
    {
        case GO_SQUARE:
            {
                if(mGame->uGameRoundCount == 1)
                {
                    return;
                }
                else
                {
                    current_player->uMoney += 200;
                }
            }
            break;

        case MEDITERRANEAN_AVENUE_SQUARE:
        {
            mProperty* mediterranean_ave = &mGame->mGameProperties[MEDITERRANEAN_AVENUE];
            if(mediterranean_ave->bOwned == true && current_player->ePlayerTurnPosition != mediterranean_ave->eOwner)
            {   
                bool bColor = m_color_set_owned(mGame, current_player, mediterranean_ave->eColor);
                m_pay_rent_property(mGame->mGamePlayers[mediterranean_ave->eOwner], current_player, mediterranean_ave, bColor);
            }
            else if(mediterranean_ave->bOwned == false)
            {
                m_buy_property(mediterranean_ave, current_player);
            }

            if(mediterranean_ave->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(mediterranean_ave, current_player, m_color_set_owned(mGame, current_player, mediterranean_ave->eColor));
            }
        }
        break;

        case COMMUNITY_CHEST_SQUARE_1:
        {
            m_execute_community_chest_card(mGame);
        }
        break;

        case BALTIC_AVENUE_SQUARE:
        {
            mProperty* baltic_avenue = &mGame->mGameProperties[BALTIC_AVENUE];
            if(baltic_avenue->bOwned == true && current_player->ePlayerTurnPosition != baltic_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, baltic_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[baltic_avenue->eOwner], current_player, baltic_avenue, bColor);
            }
            else if(baltic_avenue->bOwned == false)
            {
                m_buy_property(baltic_avenue, current_player);
            }
            
            if(baltic_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(baltic_avenue, current_player, m_color_set_owned(mGame, current_player, baltic_avenue->eColor));
            }
        }
        break;

        case INCOME_TAX_SQUARE:
        {
            if(current_player->uMoney <= INCOME_TAX)
            {
                current_player->bBankrupt = true; 
                current_player->uMoney = 0; 
                return;
            }
            else if(current_player->uMoney > INCOME_TAX)
            {
                current_player->uMoney -= INCOME_TAX;
            }

            else
            {
                printf("control path missed on income tax square\n");
            }
        }
        break;

        case READING_RAILROAD_SQUARE:
        {
            mRailroad* reading_railroad = &mGame->mGameRailroads[READING_RAILROAD];
            if(reading_railroad->bOwned == true && current_player->ePlayerTurnPosition != reading_railroad->eOwner)
            {
                m_pay_rent_railroad(mGame->mGamePlayers[reading_railroad->eOwner], current_player, reading_railroad);
            }
            else
            {
                m_buy_railroad(reading_railroad, current_player);
            }
        }
        break;

        case ORIENTAL_AVENUE_SQUARE:
        {
            mProperty* oriental_avenue = &mGame->mGameProperties[ORIENTAL_AVENUE];
            if(oriental_avenue->bOwned == true && current_player->ePlayerTurnPosition != oriental_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, oriental_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[oriental_avenue->eOwner], current_player, oriental_avenue, bColor);
            }
            else if(oriental_avenue->bOwned == false)
            {
                m_buy_property(oriental_avenue, current_player);
            }
            
            if(oriental_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(oriental_avenue, current_player, m_color_set_owned(mGame, current_player, oriental_avenue->eColor));
            }
        }
        break;

        case CHANCE_SQUARE_1:
        {
            m_execute_chance_card(mGame);
        }
        break;

        case VERMONT_AVENUE_SQUARE:
        {
            mProperty* vermont_avenue = &mGame->mGameProperties[VERMONT_AVENUE];
            if(vermont_avenue->bOwned == true && current_player->ePlayerTurnPosition != vermont_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, vermont_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[vermont_avenue->eOwner],current_player, vermont_avenue, bColor);
            }
            else if(vermont_avenue->bOwned == false)
            {
                m_buy_property(vermont_avenue, current_player);
            }
            
            if(vermont_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(vermont_avenue, current_player, m_color_set_owned(mGame, current_player, vermont_avenue->eColor));
            }
        }
        break;

        case CONNECTICUT_AVENUE_SQUARE:
        {
            mProperty* connecticut_avenue = &mGame->mGameProperties[CONNECTICUT_AVENUE];
            if(connecticut_avenue->bOwned == true && current_player->ePlayerTurnPosition != connecticut_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, connecticut_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[connecticut_avenue->eOwner], current_player, connecticut_avenue, bColor);
            }
            else if(connecticut_avenue->bOwned == false)
            {
                m_buy_property(connecticut_avenue, current_player);
            }

            if(connecticut_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(connecticut_avenue, current_player, m_color_set_owned(mGame, current_player, connecticut_avenue->eColor));
            }
        }
        break;

        case JAIL_SQUARE:
        {
            return; // no actions req when landed on via dice roll
        }
        break;

        case ST_CHARLES_PLACE_SQUARE:
        {
            mProperty* st_charles_place = &mGame->mGameProperties[ST_CHARLES_PLACE];
            if(st_charles_place->bOwned == true && current_player->ePlayerTurnPosition != st_charles_place->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, st_charles_place->eColor);
                m_pay_rent_property(mGame->mGamePlayers[st_charles_place->eOwner], current_player, st_charles_place, bColor);
            }
            else if(st_charles_place->bOwned == false)
            {
                m_buy_property(st_charles_place, current_player);
            }
            
            if(st_charles_place->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(st_charles_place, current_player, m_color_set_owned(mGame, current_player, st_charles_place->eColor));
            }
        }
        break;

        case ELECTRIC_COMPANY_SQUARE:
        {
            mUtility* electric_company = &mGame->mGameUtilities[ELECTRIC_COMPANY];
            if(electric_company->bOwned == true && current_player->ePlayerTurnPosition != electric_company->eOwner)
            {
                m_pay_rent_utility(mGame->mGamePlayers[electric_company->eOwner], current_player, electric_company, mGame->mGameDice);
            }
            else
            {
                m_buy_utility(electric_company, current_player);
            }
        }
        break;

        case STATES_AVENUE_SQUARE:
        {
            mProperty* states_avenue = &mGame->mGameProperties[STATES_AVENUE];
            if(states_avenue->bOwned == true && current_player->ePlayerTurnPosition != states_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, states_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[states_avenue->eOwner], current_player, states_avenue, bColor);
            }
            else if(states_avenue->bOwned == false)
            {
                m_buy_property(states_avenue, current_player);
            }
            
            if(states_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(states_avenue, current_player, m_color_set_owned(mGame, current_player, states_avenue->eColor));
            }
        }
        break;

        case VIRGINIA_AVENUE_SQUARE:
        {
            mProperty* virginia_avenue = &mGame->mGameProperties[VIRGINIA_AVENUE];
            if(virginia_avenue->bOwned == true && current_player->ePlayerTurnPosition != virginia_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, virginia_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[virginia_avenue->eOwner], current_player, virginia_avenue, bColor);
            }
            else if(virginia_avenue->bOwned == false)
            {
                m_buy_property(virginia_avenue, current_player);
            }
            
            if(virginia_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(virginia_avenue, current_player, m_color_set_owned(mGame, current_player, virginia_avenue->eColor));
            }
        }
        break;

        case PENNSYLVANIA_RAILROAD_SQUARE:
        {
            mRailroad* pennsylvania_railroad = &mGame->mGameRailroads[PENNSYLVANIA_AVENUE];
            if(pennsylvania_railroad->bOwned == true && current_player->ePlayerTurnPosition !=pennsylvania_railroad->eOwner)
            {
                m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[PENNSYLVANIA_RAILROAD].eOwner], current_player, pennsylvania_railroad);
            }
            else
            {
                m_buy_railroad(pennsylvania_railroad, current_player);
            }
        }
        break;

        case ST_JAMES_PLACE_SQUARE:
        {
            mProperty* st_james_place = &mGame->mGameProperties[ST_JAMES_PLACE];
            if(st_james_place->bOwned == true && current_player->ePlayerTurnPosition != st_james_place->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, st_james_place->eColor);
                m_pay_rent_property(mGame->mGamePlayers[st_james_place->eOwner], current_player, st_james_place, bColor);
            }
            else if(st_james_place->bOwned == false)
            {
                m_buy_property(st_james_place, current_player);
            }
            
            if(st_james_place->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(st_james_place, current_player, m_color_set_owned(mGame, current_player, st_james_place->eColor));
            }
        }
        break;

        case COMMUNITY_CHEST_SQUARE_2:
        {
            m_execute_community_chest_card(mGame);
        }
        break;

        case TENNESSEE_AVENUE_SQUARE:
        {
            mProperty* tennessee_avenue = &mGame->mGameProperties[TENNESSEE_AVENUE];
            if(tennessee_avenue->bOwned == true && current_player->ePlayerTurnPosition != tennessee_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, tennessee_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[tennessee_avenue->eOwner], current_player, tennessee_avenue, bColor);
            }
            else if(tennessee_avenue->bOwned == false)
            {
                m_buy_property(tennessee_avenue, current_player);
            }
            
            if(tennessee_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(tennessee_avenue, current_player, m_color_set_owned(mGame, current_player, tennessee_avenue->eColor));
            }
        }
        break;

        case NEW_YORK_AVENUE_SQUARE:
        {
            mProperty* new_york_avenue = &mGame->mGameProperties[NEW_YORK_AVENUE];
            if(new_york_avenue->bOwned == true && current_player->ePlayerTurnPosition != new_york_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, new_york_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[new_york_avenue->eOwner], current_player, new_york_avenue, bColor);
            }
            else if(new_york_avenue->bOwned == false)
            {
                m_buy_property(new_york_avenue, current_player);
            }
            
            if(new_york_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(new_york_avenue, current_player, m_color_set_owned(mGame, current_player, new_york_avenue->eColor));
            }
        }
        break;

        case FREE_PARKING_SQUARE:
        {
            return; // no actions required 
        }
        break;

        case KENTUCKY_AVENUE_SQUARE:
        {
            mProperty* kentucky_avenue = &mGame->mGameProperties[KENTUCKY_AVENUE];
            if(kentucky_avenue->bOwned == true && current_player->ePlayerTurnPosition != kentucky_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, kentucky_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[kentucky_avenue->eOwner], current_player, kentucky_avenue, bColor);
            }
            else if(kentucky_avenue->bOwned == false)
            {
                m_buy_property(kentucky_avenue, current_player);
            }
            
            if(kentucky_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(kentucky_avenue, current_player, m_color_set_owned(mGame, current_player, kentucky_avenue->eColor));
            }
        }
        break;

        case CHANCE_SQUARE_2:
        {
            m_execute_chance_card(mGame);
        }
        break;

        case INDIANA_AVENUE_SQUARE:
        {
            mProperty* indiana_avenue = &mGame->mGameProperties[INDIANA_AVENUE];
            if(indiana_avenue->bOwned == true && current_player->ePlayerTurnPosition != indiana_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, indiana_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[indiana_avenue->eOwner], current_player, indiana_avenue, bColor);
            }
            else if(indiana_avenue->bOwned == false)
            {
                m_buy_property(indiana_avenue, current_player);
            }
            
            if(indiana_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(indiana_avenue, current_player, m_color_set_owned(mGame, current_player, indiana_avenue->eColor));
            }
        }
        break;

        case ILLINOIS_AVENUE_SQUARE:
        {
            mProperty* illinois_avenue = &mGame->mGameProperties[ILLINOIS_AVENUE];
            if(illinois_avenue->bOwned == true && current_player->ePlayerTurnPosition != illinois_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, illinois_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[illinois_avenue->eOwner], current_player, illinois_avenue, bColor);
            }
            else if(illinois_avenue->bOwned == false)
            {
                m_buy_property(illinois_avenue, current_player);
            }
            
            if(illinois_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(illinois_avenue, current_player, m_color_set_owned(mGame, current_player, illinois_avenue->eColor));
            }
        }
        break;

        case B_AND_O_RAILROAD_SQUARE:
        {
            mRailroad* b_and_o_railroad = &mGame->mGameRailroads[B_AND_O_RAILROAD];
            if(b_and_o_railroad->bOwned == true && current_player->ePlayerTurnPosition != b_and_o_railroad->eOwner)
            {
                m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[B_AND_O_RAILROAD].eOwner], current_player, b_and_o_railroad);
            }
            else
            {
                m_buy_railroad(b_and_o_railroad, current_player);
            }
        }
        break;

        case ATLANTIC_AVENUE_SQUARE:
        {
            mProperty* atlantic_avenue = &mGame->mGameProperties[ATLANTIC_AVENUE];
            if(atlantic_avenue->bOwned == true && current_player->ePlayerTurnPosition != atlantic_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, atlantic_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[atlantic_avenue->eOwner], current_player, atlantic_avenue, bColor);
            }
            else if(atlantic_avenue->bOwned == false)
            {
                m_buy_property(atlantic_avenue, current_player);
            }
            
            if(atlantic_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(atlantic_avenue, current_player, m_color_set_owned(mGame, current_player, atlantic_avenue->eColor));
            }
        }
        break;

        case VENTNOR_AVENUE_SQUARE:
        {
            mProperty* ventnor_avenue = &mGame->mGameProperties[VENTNOR_AVENUE];
            if(ventnor_avenue->bOwned == true && current_player->ePlayerTurnPosition != ventnor_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, ventnor_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[ventnor_avenue->eOwner], current_player, ventnor_avenue, bColor);
            }
            else if(ventnor_avenue->bOwned == false)
            {
                m_buy_property(ventnor_avenue, current_player);
            }
            
            if(ventnor_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(ventnor_avenue, current_player, m_color_set_owned(mGame, current_player, ventnor_avenue->eColor));
            }
        }
        break;

        case WATER_WORKS_SQUARE:
        {
            mUtility* water_works = &mGame->mGameUtilities[WATER_WORKS];
            if(water_works->bOwned == true && current_player->ePlayerTurnPosition != water_works->eOwner)
            {
                m_pay_rent_utility(mGame->mGamePlayers[mGame->mGameUtilities[WATER_WORKS].eOwner], current_player, water_works, mGame->mGameDice);
            }
            else
            {
                m_buy_utility(water_works, current_player);
            }
        }
        break;

        case MARVIN_GARDENS_SQUARE:
        {
            mProperty* marvin_gardens = &mGame->mGameProperties[MARVIN_GARDENS];
            if(marvin_gardens->bOwned == true && current_player->ePlayerTurnPosition != marvin_gardens->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, marvin_gardens->eColor);
                m_pay_rent_property(mGame->mGamePlayers[marvin_gardens->eOwner], current_player, marvin_gardens, bColor);
            }
            else if(marvin_gardens->bOwned == false)
            {
                m_buy_property(marvin_gardens, current_player);
            }
            
            if(marvin_gardens->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(marvin_gardens, current_player, m_color_set_owned(mGame, current_player, marvin_gardens->eColor));
            }
        }
        break;

        case GO_TO_JAIL_SQUARE:
        {
            current_player->uPosition = JAIL_SQUARE;
            current_player->bInJail = true;           
        }
        break;

        case PACIFIC_AVENUE_SQUARE:
        {
            mProperty* pacific_avenue = &mGame->mGameProperties[PACIFIC_AVENUE];
            if(pacific_avenue->bOwned == true && current_player->ePlayerTurnPosition != pacific_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, pacific_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[pacific_avenue->eOwner], current_player, pacific_avenue, bColor);
            }
            else if(pacific_avenue->bOwned == false)
            {
                m_buy_property(pacific_avenue, current_player);
            }
            
            if(pacific_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(pacific_avenue, current_player, m_color_set_owned(mGame, current_player, pacific_avenue->eColor));
            }
        }
        break;

        case NORTH_CAROLINA_AVENUE_SQUARE:
        {
            mProperty* north_carolina_avenue = &mGame->mGameProperties[NORTH_CAROLINA_AVENUE];
            if(north_carolina_avenue->bOwned == true && current_player->ePlayerTurnPosition != north_carolina_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, north_carolina_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[north_carolina_avenue->eOwner], current_player, north_carolina_avenue, bColor);
            }
            else if(north_carolina_avenue->bOwned == false)
            {
                m_buy_property(north_carolina_avenue, current_player);
            }
            
            if(north_carolina_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(north_carolina_avenue, current_player, m_color_set_owned(mGame, current_player, north_carolina_avenue->eColor));
            }
        }
        break;

        case COMMUNITY_CHEST_SQUARE_3:
        {
            m_execute_community_chest_card(mGame);
        }
        break;

     case PENNSYLVANIA_AVENUE_SQUARE:
        {
            mProperty* pennsylvania_avenue = &mGame->mGameProperties[PENNSYLVANIA_AVENUE];
            if(pennsylvania_avenue->bOwned == true && current_player->ePlayerTurnPosition != pennsylvania_avenue->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, pennsylvania_avenue->eColor);
                m_pay_rent_property(mGame->mGamePlayers[pennsylvania_avenue->eOwner], current_player, pennsylvania_avenue, bColor);
            }
            else if(pennsylvania_avenue->bOwned == false)
            {
                m_buy_property(pennsylvania_avenue, current_player);
            }
            
            if(pennsylvania_avenue->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(pennsylvania_avenue, current_player, m_color_set_owned(mGame, current_player, pennsylvania_avenue->eColor));
            }
        }
        break;

        case SHORT_LINE_RAILROAD_SQUARE:
        {
            mRailroad* short_line_railroad = &mGame->mGameRailroads[SHORT_LINE_RAILROAD];
            if(short_line_railroad->bOwned == true && current_player->ePlayerTurnPosition != short_line_railroad->eOwner)
            {
                m_pay_rent_railroad(mGame->mGamePlayers[mGame->mGameRailroads[SHORT_LINE_RAILROAD].eOwner], current_player, short_line_railroad);
            }
            else
            {
                m_buy_railroad(short_line_railroad, current_player);
            }
        }
        break;

        case CHANCE_SQUARE_3:
        {
            m_execute_chance_card(mGame);
        }
        break;

        case PARK_PLACE_SQUARE:
        {
            mProperty* park_place = &mGame->mGameProperties[PARK_PLACE];
            if(park_place->bOwned == true && current_player->ePlayerTurnPosition != park_place->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, park_place->eColor);
                m_pay_rent_property(mGame->mGamePlayers[park_place->eOwner], current_player, park_place, bColor);
            }
            else if(park_place->bOwned == false)
            {
                m_buy_property(park_place, current_player);
            }

            if(park_place->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(park_place, current_player, m_color_set_owned(mGame, current_player, park_place->eColor));
            }
        }
        break;

        case LUXURY_TAX_SQUARE:
        {
            if(current_player->uMoney <= LUXURY_TAX)
            {
                current_player->bBankrupt = true;
                current_player->uMoney = 0;  
            }
            else if(current_player->uMoney > LUXURY_TAX)
            {
                current_player->uMoney -= INCOME_TAX;
            }
            else
            {
                printf("control path missed on luxary tax square\n");
            }
        }
        break;

        case BOARDWALK_SQUARE:
        {
            mProperty* boardwalk = &mGame->mGameProperties[BOARDWALK];
            if(boardwalk->bOwned == true && current_player->ePlayerTurnPosition != boardwalk->eOwner)
            {
                bool bColor = m_color_set_owned(mGame, current_player, boardwalk->eColor);
                m_pay_rent_property(mGame->mGamePlayers[boardwalk->eOwner], current_player, boardwalk, bColor);
            }
            else if(boardwalk->bOwned == false)
            {
                m_buy_property(boardwalk, current_player);
            }
            
            if(boardwalk->eOwner == current_player->ePlayerTurnPosition)
            {
                m_buy_house(boardwalk, current_player, m_color_set_owned(mGame, current_player, boardwalk->eColor));
            }
        }
        break;
        default: 
        {
            printf("player position is greater than squares\n");
        }    
        break;
    }
}