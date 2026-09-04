#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

const int WINDOW_WIDTH = 512;  /*  Width of the game window to be created*/
const int WINDOW_HEIGHT = 512; /*  Height of the game window to be created*/
const int FRAME_COUNT = 8;     /*  Number of frames in the spritesheet */
const int FRAME_WIDTH = 512;   /*  Width of the frame in the spritesheet */
const int FRAME_HEIGHT = 512;  /*  Height of the frame in the spritesheet */
const int ANIMATION_DELAY = 100;/* Number of iterations between the animation frames (determines delay) */

/* Struct to store the current state*/
struct AppState {
    SDL_Texture *Texture = nullptr;
    int currentFrame = 0;
    Uint32 lastFrameTime = 0;
};

int main(int argc, char *argv[])
{
    // Initialize the SDL library
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create window and renderer
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    
    // Initialize the window and renderer using SDL method
    if (!SDL_CreateWindowAndRenderer("Feeling Loopy", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Load the texture from the assets folder
    AppState state;

    /* Load the texture into state.Texture */
    // YOUR CODE HERE 

    // Test to ensure texture was loaded


    state.currentFrame = 0;
    state.lastFrameTime = 1;

    // Main game loop condition variable
    bool running = true;

    // SDL_Event to capture event of window being closed
    SDL_Event event;

    // The main game loop
    while (running) {

        // Handle event of window close i.e. quit
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Update animation
        state.lastFrameTime += 1;

        // Change the frame if enough interations performed since the last
        // YOUR CODE HERE 
        // YOUR CODE HERE
        
        // Set Background color to white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // Clear screen
        SDL_RenderClear(renderer);


        // Draw the sprite onto the window

	    // Source (src) Rectangle is capturing the image from the spritesheet
        SDL_FRect srcRect = { /* x(position), y(position), width, height */ 
            (float)(state.currentFrame * FRAME_WIDTH), 
            0.0f, 
            (float)FRAME_WIDTH, 
            (float)FRAME_HEIGHT 
        };

        // Destination (dst) Rectangle is drawing the image on the window
        SDL_FRect dstRect = { /* x(position), y(position), width, height */
            0.0f, 
            0.0f, 
            FRAME_WIDTH, 
            FRAME_HEIGHT 
        };

        SDL_RenderTexture(renderer, state.Texture, &srcRect, &dstRect);

        SDL_RenderPresent(renderer);
    }

    // Cleaning up the objects
    if (state.Texture) {
        SDL_DestroyTexture(state.Texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
