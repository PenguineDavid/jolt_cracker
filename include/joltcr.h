/*
 * Copyright (C) 2026 David S
 *
 * This file is part of Joltcr.
 *
 * Joltcr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Joltcr is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */


#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace joltcr
{
    struct video_time
    {
        uint32_t hours;
        uint32_t minutes;
        uint32_t seconds;
    };

    struct query
    {
        int32_t hunger;
        bool low_health;
        std::string pattern;
    };

    video_time hud_ticks_to_vid_time(int32_t hud_ticks);
    std::vector<std::vector<int32_t>> batch_solve(std::vector<query> inputs, int32_t max_tick);
    std::vector<int32_t> solve(int32_t hunger, bool low_health, std::string pattern, int32_t max_tick);
}