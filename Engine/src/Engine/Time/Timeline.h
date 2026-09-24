#pragma once

#include <iostream>
#include <cstdint>
#include <mutex>

namespace Engine 
{
    class Timeline
    {
        public:
        Timeline(Timeline *anchor, int64_t tic);
        Timeline();
        int64_t getTime(); // this can be game or system time implementation
        void pause();
        void unpause();
        void changeTic(int tic); // optional
        bool isPaused(); // optional

        private:
        std::mutex m; // if tics can change size and the game is multithreaded
        
        int64_t start_time; // the time of the *anchor when created
        int64_t elapsed_paused_time; 
        int64_t last_paused_time;

        int64_t tic; // units of anchor timeline per step
        int64_t time_offset = 0; // save previous time when changing tic 
        
        bool paused = false;
        Timeline *anchor; // for most general game time, system library pointer

        int64_t getTimeUnlocked();
    };
}