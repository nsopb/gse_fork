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

#include "dll/playtime.h"

#include <limits>

#include "dll/settings.h"  
#include <curl/curl.h>

PlaytimeCounter::PlaytimeCounter(Local_Storage* local_storage, Settings* settings)  
   : local_storage(local_storage), settings(settings), last_tick(std::chrono::steady_clock::now())  
{  
    load();  
}

PlaytimeCounter::~PlaytimeCounter()
{
    save();
}

void PlaytimeCounter::tick()
{
    auto now = std::chrono::steady_clock::now();

    if (!initialized) {
        load();
        std::lock_guard<std::mutex> lock(mutex);
        last_tick = now;
        initialized = true;
        return;
    }

    bool need_save = false;
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - last_tick).count();
        if (delta <= 0) return;

        uint64_t inc = static_cast<uint64_t>(delta);
        const uint64_t maxv = std::numeric_limits<uint64_t>::max();
        if (playtime_seconds > maxv - inc) {
            playtime_seconds = maxv;
        } else {
            playtime_seconds += inc;
        }
        
        last_tick = now;

        since_save += delta;
        if (since_save >= 60) {
            since_save = 0;
            need_save = true;
        }
    }

    if (need_save) {
        save();
    }
}

void PlaytimeCounter::load()
{
    std::lock_guard<std::mutex> lock(mutex);

    playtime_seconds = 0;

    std::string data(32, '\0');
    if (local_storage->get_data("", playtime_filename, data.data(), static_cast<unsigned int>(data.size()), 0) > 0) {
        try {
            playtime_seconds = std::stoull(data);
        } catch (...) {}
    }

    initialized = true;
}

void PlaytimeCounter::save()  
{  
    PRINT_DEBUG("PlaytimeCounter::save() called, playtime_seconds=%llu", playtime_seconds);  
    std::lock_guard<std::mutex> lock(mutex);  
  
    std::string data = std::to_string(playtime_seconds);  
    local_storage->store_data("", playtime_filename, data.data(), static_cast<unsigned int>(data.size()));  
      
    send_to_api();  
}

void PlaytimeCounter::send_to_api()  
{  
    PRINT_DEBUG("send_to_api() called");  
      
    if (!settings || !settings->send_playtime_to_api) {  
        PRINT_DEBUG("send_to_api: disabled or settings is null");  
        return;  
    }  
      
    PRINT_DEBUG("send_to_api: attempting to send data");  
      
    CURL* curl = curl_easy_init();  
    if (!curl) {  
        PRINT_DEBUG("send_to_api: curl_easy_init() failed");  
        return;  
    }  
      
    uint32_t appid = settings->get_local_game_id().AppID();  
    std::string json_data = "{\"appid\":" + std::to_string(appid) +   
                           ",\"user_id\":" + std::to_string(settings->playtime_api_user_id) +   
                           ",\"playtime_seconds\":" + std::to_string(playtime_seconds) + "}";  
      
    PRINT_DEBUG("send_to_api: JSON data: %s", json_data.c_str());  
    PRINT_DEBUG("send_to_api: endpoint: %s", settings->playtime_api_endpoint.c_str());  
      
    struct curl_slist* headers = nullptr;  
    headers = curl_slist_append(headers, "Content-Type: application/json");  
      
    curl_easy_setopt(curl, CURLOPT_URL, settings->playtime_api_endpoint.c_str());  
    curl_easy_setopt(curl, CURLOPT_POST, 1L);  
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());  
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);  
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);  
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);  
      
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  
      
    CURLcode res = curl_easy_perform(curl);  
    PRINT_DEBUG("send_to_api: curl_easy_perform() result: %d (%s)", res, curl_easy_strerror(res));  
      
    curl_slist_free_all(headers);  
    curl_easy_cleanup(curl);  
}

uint64_t PlaytimeCounter::seconds() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return playtime_seconds;
}
