# Monopoly - Function Inventory for Coroutine Conversion

## Directory-Style Function Tree

```
m_game.c
├── CORE GAME FLOW
│   ├── m_next_player_turn()              [✓ KEPT - No blocking]
│   ├── m_game_over_check()               [✓ KEPT - No blocking]
│   └── m_roll_dice()                     [✓ KEPT - No blocking]
│
├── JAIL MECHANICS
│   ├── m_jail_subphase()                 [⚠ STRIPPED - Called m_get_player_jail_choice()]
│   ├── m_attempt_jail_escape()           [✓ KEPT - No blocking]
│   ├── m_use_jail_free_card()            [✓ KEPT - No blocking]
│   └── m_get_player_jail_choice()        [✗ REMOVED - scanf_s blocking]
│
├── PRE-ROLL ACTIONS
│   ├── m_pre_roll_phase()                [⚠ STRIPPED - Called m_show_pre_roll_menu()]
│   ├── m_show_pre_roll_menu()            [✗ REMOVED - while loop + scanf_s]
│   ├── m_get_player_pre_roll_choice()    [✗ REMOVED - scanf_s blocking]
│   └── m_get_trade_partner()             [✗ REMOVED - scanf_s blocking]
│
├── POST-ROLL ACTIONS
│   ├── m_phase_post_roll()               [⚠ STRIPPED - Called m_show_post_roll_menu()]
│   ├── m_show_post_roll_menu()           [✗ REMOVED - while loop + scanf_s]
│   ├── m_get_player_post_roll_choice()   [✗ REMOVED - scanf_s blocking]
│   └── m_show_building_managment_menu()  [✗ REMOVED - Called m_get_player_property()]
│       └── m_get_building_managment_choice() [✗ REMOVED - scanf_s blocking]
│
├── AUCTION SYSTEM (All removed - complex blocking loops)
│   ├── m_enter_auction_prop()            [✗ REMOVED - while + scanf_s loop]
│   ├── m_enter_auction_rail()            [✗ REMOVED - while + scanf_s loop]
│   ├── m_enter_auction_util()            [✗ REMOVED - while + scanf_s loop]
│   └── m_exit_auction()                  [✓ KEPT - Helper logic only]
│
├── TRADE SYSTEM (All removed - complex blocking negotiation)
│   ├── m_enter_trade_phase()             [✓ KEPT - Just sets state]
│   ├── m_propose_trade()                 [✗ REMOVED - while + scanf_s loop]
│   ├── m_add_to_offer()                  [✗ REMOVED - scanf_s blocking]
│   ├── m_add_to_request()                [✗ REMOVED - scanf_s blocking]
│   ├── m_review_trade()                  [✗ REMOVED - scanf_s blocking]
│   └── m_execute_trade()                 [✓ KEPT - Pure logic, no blocking]
│
├── FINANCIAL MANAGEMENT
│   ├── m_can_player_afford()             [✓ KEPT - No blocking]
│   └── m_attempt_emergency_payment()     [✓ KEPT - Auto sells, no choice]
│
├── FORCED ACTIONS
│   ├── m_forced_release()                [✓ KEPT - No blocking]
│   ├── m_player_has_forced_actions()     [✓ KEPT - Stub only]
│   └── m_handle_emergency_actions()      [✓ KEPT - Stub only]
│
└── HELPERS
    ├── m_get_square_type()               [✓ KEPT - No blocking]
    ├── m_get_empty_prop_owned_slot()     [✓ KEPT - No blocking]
    ├── m_get_empty_rail_owned_slot()     [✓ KEPT - No blocking]
    └── m_get_empty_util_owned_slot()     [✓ KEPT - No blocking]
```

---

## Function Flow Patterns to Recreate

### 1. JAIL DECISION FLOW
```
OLD VERSION:
m_jail_subphase()
  └─> m_get_player_jail_choice()  [BLOCKING scanf_s]
       └─> switch(choice):
            ├─> USE_JAIL_CARD -> m_use_jail_free_card()
            ├─> PAY_JAIL_FINE -> deduct money, set bInJail = false
            └─> ROLL_FOR_DOUBLES -> m_attempt_jail_escape()

NEED TO RECREATE:
- Present 3 options to player
- Yield/await player choice
- Resume with chosen action
```

### 2. PRE-ROLL MENU FLOW
```
OLD VERSION:
m_pre_roll_phase()
  └─> m_show_pre_roll_menu()  [BLOCKING while loop]
       └─> while (state == PHASE_PRE_ROLL):
            ├─> m_get_player_pre_roll_choice()  [BLOCKING scanf_s]
            └─> switch(action):
                 ├─> VIEW_STATUS -> m_show_player_status()
                 ├─> VIEW_PROPERTIES -> m_show_props_owned()
                 ├─> PROPERTY_MANAGMENT -> m_show_building_managment_menu()
                 ├─> PROPOSE_TRADE -> m_get_trade_partner() + m_propose_trade()
                 └─> ROLL_DICE -> m_roll_dice(), state = PHASE_POST_ROLL

NEED TO RECREATE:
- Menu loop that can be interrupted
- Each action should yield back to main loop
- Continue until ROLL_DICE chosen
```

### 3. POST-ROLL MENU FLOW
```
OLD VERSION:
m_phase_post_roll()
  └─> m_show_post_roll_menu()  [BLOCKING while loop]
       └─> while (state == PHASE_POST_ROLL):
            ├─> m_get_player_post_roll_choice()  [BLOCKING scanf_s]
            └─> switch(action):
                 ├─> VIEW_STATUS -> m_show_player_status()
                 ├─> VIEW_PROPERTIES -> m_show_props_owned()
                 ├─> PROPERTY_MANAGMENT -> m_show_building_managment_menu()
                 ├─> PROPOSE_TRADE -> m_enter_trade_phase()
                 ├─> BUY_SQUARE -> m_buy_property/railroad/utility()
                 └─> END_TURN -> state = PHASE_END_TURN

NEED TO RECREATE:
- Menu loop with multiple options
- Purchase decision for unowned properties
- Continue until END_TURN chosen
```

### 4. BUILDING MANAGEMENT FLOW
```
OLD VERSION:
m_show_building_managment_menu()
  ├─> m_get_player_property()  [BLOCKING scanf_s - property selection]
  └─> m_get_building_managment_choice()  [BLOCKING scanf_s - action choice]
       └─> switch(action):
            ├─> SELL_HOUSES -> m_execute_house_sale()
            ├─> BUY_HOUSES -> m_buy_house()
            ├─> SELL_HOTELS -> m_execute_hotel_sale()
            ├─> BUY_HOTELS -> m_buy_hotel()
            ├─> MORTGAGE_PROPERTY -> m_execute_mortgage_flow()
            ├─> UNMORTGAGE_PROPERTY -> m_execute_unmortgage_flow()
            └─> CANCEL -> return

NEED TO RECREATE:
- Two-step process: select property, then select action
- Yield at each decision point
- Return to previous menu when done
```

### 5. AUCTION FLOW
```
OLD VERSION:
m_enter_auction_prop/rail/util()
  ├─> Initialize auction (mark players active/inactive)
  └─> while (bAuctionInProgress):  [BLOCKING LOOP]
       ├─> for each active player:
       │    ├─> scanf_s("%d", &bid)  [BLOCKING]
       │    ├─> if (bid == 0) -> mark inactive
       │    └─> if (bid > highest) -> update highest
       └─> if (m_exit_auction()) -> assign to winner

NEED TO RECREATE:
- Round-robin bidding system
- Players can pass (bid 0)
- Continue until 1 or 0 active bidders remain
- Award to highest bidder OR return to bank
```

### 6. TRADE FLOW
```
OLD VERSION:
m_propose_trade()
  ├─> m_get_trade_partner()  [BLOCKING scanf_s]
  └─> while (bTrading):  [BLOCKING LOOP]
       ├─> scanf_s for menu choice  [BLOCKING]
       └─> switch(choice):
            ├─> 1 -> m_add_to_offer()  [BLOCKING scanf_s for items]
            ├─> 2 -> m_add_to_request()  [BLOCKING scanf_s for items]
            ├─> 3 -> m_review_trade()  [BLOCKING scanf_s accept/reject]
            └─> 4 -> cancel

m_add_to_offer/request():
  ├─> scanf_s for category (prop/rail/util/cash)  [BLOCKING]
  ├─> m_show_owned_items()
  └─> scanf_s for item selection  [BLOCKING]

m_review_trade():
  └─> scanf_s for accept/reject/counter  [BLOCKING]
       └─> if accept -> m_execute_trade()

NEED TO RECREATE:
- Multi-step negotiation process
- Building offers/requests incrementally
- Partner can accept/reject/counter
- Must validate buildings on properties before trade
```

---

## Key Decision Points Requiring Coroutines

### Priority 1 - Essential Gameplay
1. **Jail Choice** (3 options)
   - Use card / Pay fine / Roll for doubles
   
2. **Property Purchase** (Yes/No)
   - After landing on unowned property
   
3. **Pre-Roll Action Menu** (5 options)
   - View status / View properties / Property mgmt / Propose trade / Roll dice

4. **Post-Roll Action Menu** (6 options)
   - View status / View properties / Property mgmt / Propose trade / Buy square / End turn

### Priority 2 - Property Management
5. **Property Selection** (List)
   - Choose which property to manage
   
6. **Building Management** (7 options)
   - Buy/sell houses/hotels, mortgage, unmortgage, cancel

### Priority 3 - Advanced Features
7. **Auction System**
   - Round-robin bidding
   - Multiple rounds until 1/0 active bidders
   
8. **Trade System**
   - Partner selection
   - Build offer (multi-step)
   - Build request (multi-step)
   - Review and accept/reject/counter

---

## Coroutine Yield Points (Where Old Code Had scanf_s)

```c
// Example transformation:

// OLD:
int choice;
scanf_s("%d", &choice);
return choice;

// NEW:
typedef struct {
    CoroutineState state;
    int* result_ptr;
} MenuCoroutine;

// Yield to get input
YIELD_FOR_INPUT(MENU_CHOICE);
// Resume here when input arrives
int choice = *coro->result_ptr;
```

### All Yield Points Needed:
1. `m_get_player_jail_choice()` - 1 yield
2. `m_get_player_pre_roll_choice()` - 1 yield (loop until ROLL_DICE)
3. `m_get_player_post_roll_choice()` - 1 yield (loop until END_TURN)
4. `m_get_player_property()` - 1 yield
5. `m_get_building_managment_choice()` - 1 yield
6. `m_get_trade_partner()` - 1 yield
7. `m_add_to_offer()` - 2 yields (category + item)
8. `m_add_to_request()` - 2 yields (category + item)
9. `m_review_trade()` - 1 yield
10. Auction bid input - 1 yield per player per round

---

## State Machine Additions Needed

Your current states:
```c
typedef enum _mGameState {
    GAME_STARTUP,
    GAME_OVER,
    PHASE_PRE_ROLL,
    PHASE_POST_ROLL,
    PHASE_END_TURN,
    AUCTION_IN_PROGRESS,
    TRADE_NEGOTIATION,
    OWE_PLAYER,
    OWE_BANK
} mGameState;
```

**New sub-states you may need:**
```c
// Jail sub-states
JAIL_AWAITING_CHOICE,

// Menu sub-states
PRE_ROLL_MENU_ACTIVE,
POST_ROLL_MENU_ACTIVE,

// Property management sub-states
PROP_MGMT_SELECT_PROPERTY,
PROP_MGMT_SELECT_ACTION,

// Auction sub-states
AUCTION_AWAITING_BID,
AUCTION_PROCESSING_ROUND,

// Trade sub-states
TRADE_SELECT_PARTNER,
TRADE_BUILD_OFFER,
TRADE_BUILD_REQUEST,
TRADE_AWAIT_REVIEW,
```

---

## Summary: Functions by Action Required

### ✓ KEEP AS-IS (No modification needed)
- All financial logic functions
- Property validation functions
- Card execution functions
- Movement functions
- Helper functions

### ⚠ MODIFY (Remove blocking, add yield points)
- `m_jail_subphase()` - Add yield for choice
- `m_pre_roll_phase()` - Add yield for menu
- `m_phase_post_roll()` - Add yield for menu

### ✗ RECREATE (Completely rewrite as coroutines)
- All `m_get_*_choice()` functions
- All `m_show_*_menu()` functions
- Entire auction system
- Entire trade system

### ➕ CREATE NEW
- Coroutine infrastructure
- Event/message system for input
- State persistence between yields
- Menu rendering system (non-blocking)
