


#include "m_init_game.h"

int main(void)
{
    srand((unsigned int)time(NULL));
    
    // Initialize GLFW (just for input)
    if (!glfwInit()) {
        printf("Failed to initialize GLFW!\n");
        return -1;
    }
    
    // Create invisible window for input handling only at this point
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Hidden window
    GLFWwindow* inputWindow = glfwCreateWindow(1, 1, "Input", NULL, NULL);
    
    // Initialize game
    mGameStartSettings settings = {
        .uStartingMoney       = 1500,
        .uStartingPlayerCount = 3,
        .uJailFine            = 50
    };
    
    mGameData* game = m_init_game(settings);
    if(!game) return 1;
    
    // Initialize game flow system
    mGameFlow flow = {0};
    flow.pGameData = game;
    flow.pInputWindow = inputWindow;  // Store for input checking
    
    // Start first phase
    mPreRollData* firstTurn    = malloc(sizeof(mPreRollData));
    firstTurn->pPlayer         = game->mGamePlayers[0];
    firstTurn->bWaitingForRoll = false;
    m_push_phase(&flow, m_phase_pre_roll, firstTurn);
    
    printf("=== MONOPOLY GAME STARTED ===\n");
    
    // Main loop
    float deltaTime = 0.016f;
    
    while(flow.pfCurrentPhase && game->bRunning && !glfwWindowShouldClose(inputWindow))
    {
        glfwPollEvents();  // Get input
        
        m_run_current_phase(&flow, deltaTime);
        
        // Simple timing
        glfwWaitEventsTimeout(0.016);  // ~60fps
    }
    
    printf("\n=== GAME ENDED ===\n");
    
    // Cleanup
    m_free_game_data(game);
    glfwDestroyWindow(inputWindow);
    glfwTerminate();
    
    return 0;
}