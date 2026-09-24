#include "Timeline.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
// #include <SDL3_ttf/SDL_ttf.h>

namespace Engine {
    Timeline::Timeline() 
    : start_time(0),
    elapsed_paused_time(0),
    last_paused_time(0),
    tic(1),
    time_offset(0),
    paused(),
    anchor(nullptr)
    {
        start_time = SDL_GetTicks();
    }

    Timeline::Timeline(Timeline* anchor, int64_t tic) 
    : start_time(0),
    elapsed_paused_time(0),
    last_paused_time(0),
    tic(tic),
    time_offset(0),
    paused(),
    anchor(anchor)
    {
        if (anchor != nullptr) {
            start_time = anchor->getTime();
        } else {
            start_time = SDL_GetTicks();
        }
    }

    int64_t Timeline::getTime() 
    {
        std::lock_guard<std::mutex> lock(m);
        return getTimeUnlocked();
    }

    int64_t Timeline::getTimeUnlocked()
    {
        int64_t currentTime;

        if (paused)
        {
            currentTime = last_paused_time;
        }
        else if (anchor != nullptr)
        {
            currentTime = anchor->getTime();
        }
        else
        {
            currentTime = SDL_GetTicks();
        }

        return time_offset + (currentTime - start_time - elapsed_paused_time) / tic;
    }

    void Timeline::pause() 
    {
        std::lock_guard<std::mutex> lock(m);
        if (!paused) 
        {
            if (anchor != nullptr) 
            {
                last_paused_time = anchor -> getTime();
            }
            else
            {
                last_paused_time = SDL_GetTicks();
            }

            paused = true;
        }
    }

    void Timeline::unpause() 
    {
        std::lock_guard<std::mutex> lock(m);
        if (paused) 
        {
            int64_t currentTime;
            if (anchor != nullptr) 
            {
                currentTime = anchor -> getTime();
            }
            else
            {
                currentTime = SDL_GetTicks();
            }
            elapsed_paused_time += currentTime - last_paused_time;

            paused = false;
        }
    }

    bool Timeline::isPaused()
    {
        std::lock_guard<std::mutex> lock(m);
        if (paused) return true;
        else return false;
    }

    void Timeline::changeTic(int newTic)
    {

        // std::cout << "=== before change ===\n";
        // std::cout << "startTime: " << start_time << std::endl;
        // std::cout << "pausedTime: " << elapsed_paused_time << std::endl;
        // std::cout << "tic: " << tic << std::endl;
        // std::cout << "offset: " << time_offset << std::endl;

        if (newTic <= 0)
            return;

        std::lock_guard<std::mutex> lock(m);

        int64_t currentTimelineTime = getTimeUnlocked();

        int64_t currentAnchorTime;

        if (anchor != nullptr)
            currentAnchorTime = anchor->getTime();
        else
            currentAnchorTime = SDL_GetTicks();

        time_offset = currentTimelineTime;
        start_time = currentAnchorTime;
        elapsed_paused_time = 0;

        tic = newTic;
        // std::cout << "=== after change ===\n";
        // std::cout << "startTime: " << start_time << '\n';
        // std::cout << "new tic: " << tic << '\n';
        // std::cout << "offset: " << time_offset << '\n';
    }
}