/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

#pragma once

#include "local_storage.h"
#include <cstdint>
#include <chrono>
#include <mutex>
#include <string>

class PlaytimeCounter {  
public:  
    explicit PlaytimeCounter(Local_Storage* local_storage, Settings* settings);  
    ~PlaytimeCounter();  
  
    void tick();  
    void load();  
    void save();  
    uint64_t seconds() const;  
  
private:  
    void send_to_api();  
      
    Local_Storage* local_storage{};  
    Settings* settings{};  
    const std::string playtime_filename = "playtime.txt";  
    std::chrono::steady_clock::time_point last_tick{};  
    uint64_t playtime_seconds = 0;  
    mutable std::mutex mutex;  
    bool initialized = false;  
    uint64_t since_save = 0;  
};
