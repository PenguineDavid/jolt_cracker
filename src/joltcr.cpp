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
 * along with this program.  If not, see <https://www.gnu.org/licenses/lgpl-3.0.en.html>.
 */


#include "joltcr.h"

#include <thread>

namespace joltcr
{
    namespace 
    {
        enum class pattern_token
        {
            up,
            down,
            still,
            unknown
        };

                /*
            * basic implementation of java.util.Random random algorithm with set_seed next_bits and next_int methods
            * java.util.Random uses a LCG (Linear congruential generator) algorithm to generate pseudo random numbers. The algorithm's internal state relies on a 48bit seed. With every request for a new random number the seed is updated using this equation:
            * seed[n+1] = (seed[n] * multiplier + addend) & (mod mask)
            * multiplier: 0x5DEECE66D
            * addend: 0xB
            * mod mask: (1 << 48) - 1
            * then the requesting methods in our case just next_bits and next_int will use the updated seed to generate a new random number. The next_bits method will return the upper bits of the seed, while the next_int method will return a random integer within a specified range.
        */
        class java_random
        {
        private:
            uint64_t seed;
            static constexpr uint64_t multiplier = 0x5DEECE66DULL;
            static constexpr uint64_t addend = 0xBULL;
            static constexpr uint64_t mask = (1ULL << 48) - 1;

        public:
            java_random()
            {
                // Initialize the seed with a default value (can be changed later using set_seed)
                seed = 0;
            }

            void set_seed(int64_t seed_value)
            {
                // java uses xor with multiplier to initialize the seed instead of just setting it directly
                seed = (static_cast<uint64_t>(seed_value) ^ multiplier) & mask;
            }

            int32_t next_bits(int bits)
            {
                seed = (seed * multiplier + addend) & mask;
                return static_cast<int32_t>(seed >> (48 - bits));
            }

            int32_t next_int(int32_t bound)
            {
                if (bound <= 0)
                {
                    throw std::invalid_argument("bound must be positive");
                }

                if ((bound & -bound) == bound) // Check if bound is a power of 2
                {
                    return static_cast<int32_t>(
                        (static_cast<int64_t>(bound) * next_bits(31)) >> 31);
                }

                int32_t bits, val;
                int64_t rejection;
                do
                {
                    bits = next_bits(31);
                    val = bits % bound;
                    rejection = static_cast<int64_t>(bits) - val +
                                (static_cast<int64_t>(bound) - 1);
                } while (rejection >= (1LL << 31)); // Reject if the Java int calculation would overflow

                return val;
            }
        };

        // times in a jolt code like "-+--000-+++" and outputs an array of pattern tokens
        std::vector<pattern_token> parse_pattern_string(const std::string& pattern)
        {
            std::vector<pattern_token> tokens;
            for (char c : pattern)
            {
                switch (c)
                {
                    case '+':
                        tokens.push_back(pattern_token::up);
                        break;
                    case '-':
                        tokens.push_back(pattern_token::down);
                        break;
                    case '0':
                        tokens.push_back(pattern_token::still);
                        break;
                    case '?':
                        tokens.push_back(pattern_token::unknown);
                        break;
                    default:
                        throw std::invalid_argument("Invalid character in pattern string: " + std::string(1, c));
                }
            }

            std::reverse(tokens.begin(), tokens.end());

            if(tokens.size() != 10)
            {
                throw std::invalid_argument("joltcr::solve pattern argument must be exactly 10 characters");
            }

            return tokens;
        }

        // reproduces the pattern minecraft would give for a given hud tick so as tolater brute force the hud tick from the hunger pattern
        std::vector<pattern_token> reproduce_pattern(int32_t hud_tick, bool low_health, java_random* rand_gen)
        {
            rand_gen->set_seed(static_cast<int32_t>(static_cast<int64_t>(hud_tick) * 312871)); // 312871 is a little random hud seed multiplier used by mojang (see mcsrc.dev 1.16.5 net/minecraft/client/gui/Gui ln 734)
            if (low_health)
            {
                for (int i = 0; i < 10; i++) 
                {
                    // 10 times (once for every hunger shank)
                    rand_gen->next_int(2);
                }
            }

            std::vector<pattern_token> offsets;

            for (int i = 0; i < 10; i++)
            {
                switch(rand_gen->next_int(3))
                {
                    case 0:
                        offsets.push_back(pattern_token::up);
                        break;
                    case 1:
                        offsets.push_back(pattern_token::still);
                        break;
                    case 2:
                        offsets.push_back(pattern_token::down);
                        break;
                }
            }

            return offsets;
        }

        // calculates the time between hunger jolts so as not to check ticks that don't even have jolts the formula minecraft uses for this is hudTick mod (hunger * 3 + 1) == 0
        int32_t between_time(int32_t hunger)
        {
            return hunger * 3 + 1;
        }

        // checks if two arrays of pattern tokens match taking into account any unknown shanks
        bool pattern_matches(const std::vector<pattern_token>& pattern,
                     const std::vector<pattern_token>& produced)
        {
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                if (pattern[i] != pattern_token::unknown && pattern[i] != produced[i])
                {
                    return false;
                }
            }
            return true;
        }

        // Work descriptor shared by all threads handling one group.
        struct group_work
        {
            int32_t hunger;
            bool low_health;
            const std::vector<std::vector<pattern_token>>* tokens;
        };

        // Each thread gets its own java_random (it's stateful) and its own output
        // vectors, so the hot loop never needs a lock. Ticks handled by a chunk are
        // [begin_iter, end_iter) in the group's progression: tick = iter * stride.
        void run_group_chunk(const group_work& work,
                             size_t begin_iter,
                             size_t end_iter,
                             std::vector<std::vector<int32_t>>* out)
        {
            java_random rand_gen;
            const int32_t stride = between_time(work.hunger);

            for (size_t iter = begin_iter; iter < end_iter; ++iter)
            {
                // iter * stride <= max_tick by construction; cast is safe.
                const int32_t tick = static_cast<int32_t>(iter) * stride;
                auto produced = reproduce_pattern(tick, work.low_health, &rand_gen);

                for (size_t k = 0; k < work.tokens->size(); ++k)
                {
                    if (pattern_matches((*work.tokens)[k], produced))
                    {
                        (*out)[k].push_back(tick);
                    }
                }
            }
        }
    }

    video_time hud_ticks_to_vid_time(int32_t hud_ticks)         
    {             
        video_time vt;
        int32_t total_seconds = hud_ticks / 20; // Convert 20 Hz ticks to total seconds

        vt.hours = total_seconds / 3600;             
        total_seconds %= 3600;             
        vt.minutes = total_seconds / 60;             
        vt.seconds = total_seconds % 60;             

        return vt;         
    }

    std::vector<std::vector<int32_t>> batch_solve(std::vector<query> inputs, int32_t max_tick)
    {
        std::vector<std::vector<int32_t>> results(inputs.size());

        if (max_tick < 0)
        {
            return results;
        }

        // validate everything up front before doing any work
        for (const auto& in : inputs)
        {
            if (in.hunger <= 0)
            {
                throw std::invalid_argument("Hunger value in an input passed to joltcr::batch_solve must always be > 0");
            }
            parse_pattern_string(in.pattern); // throws on bad pattern/length
        }

        // group inputs by (hunger, low_health) so reproduce_pattern runs once per tick per group
        struct group
        {
            int32_t hunger;
            bool low_health;
            std::vector<size_t> indices;
            std::vector<std::vector<pattern_token>> tokens;
        };

        std::vector<group> groups;
        for (size_t i = 0; i < inputs.size(); ++i)
        {
            const auto& in = inputs[i];
            auto it = std::find_if(groups.begin(), groups.end(), [&](const group& g) {
                return g.hunger == in.hunger && g.low_health == in.low_health;
            });
            if (it == groups.end())
            {
                groups.push_back({in.hunger, in.low_health, {}, {}});
                it = groups.end() - 1;
            }
            it->indices.push_back(i);
            it->tokens.push_back(parse_pattern_string(in.pattern));
        }

        // hardware_concurrency() may return 0; fall back to 4.
        unsigned hw = std::thread::hardware_concurrency();
        const size_t num_threads = (hw == 0) ? 4 : static_cast<size_t>(hw);

        for (const auto& g : groups)
        {
            const int32_t stride = between_time(g.hunger);

            // Number of ticks in the group's progression: t = 0, stride, 2*stride, ...
            // up to and including max_tick.  Computed in int64_t so (max_tick + 1)
            // can never overflow.  This is the overflow fix.
            const int64_t tick_count = static_cast<int64_t>(max_tick) / stride + 1;

            const size_t chunk = std::max<size_t>(1, static_cast<size_t>(tick_count) / num_threads);
            const size_t actual_threads = std::min<size_t>(
                num_threads,
                (static_cast<size_t>(tick_count) + chunk - 1) / chunk);

            // Per-thread partial results, indexed by thread, so merging in thread
            // order yields deterministic, tick-sorted output.
            std::vector<std::vector<std::vector<int32_t>>> partial(
                actual_threads, std::vector<std::vector<int32_t>>(g.tokens.size()));

            // Reserve: upper bound on matches per chunk is the chunk size.
            for (size_t t = 0; t < actual_threads; ++t)
            {
                const size_t begin = t * chunk;
                const size_t end = std::min<size_t>(begin + chunk, static_cast<size_t>(tick_count));
                for (auto& v : partial[t])
                {
                    v.reserve(end - begin);
                }
            }

            // Spawn workers.
            std::vector<std::thread> workers;
            workers.reserve(actual_threads);
            group_work work{g.hunger, g.low_health, &g.tokens};
            for (size_t t = 0; t < actual_threads; ++t)
            {
                const size_t begin = t * chunk;
                const size_t end = std::min<size_t>(begin + chunk, static_cast<size_t>(tick_count));
                workers.emplace_back(run_group_chunk, std::cref(work), begin, end, &partial[t]);
            }
            for (auto& w : workers)
            {
                w.join();
            }

            // Merge in thread order => sorted by tick, same order as single-threaded.
            for (size_t t = 0; t < actual_threads; ++t)
            {
                for (size_t k = 0; k < g.tokens.size(); ++k)
                {
                    auto& dst = results[g.indices[k]];
                    dst.insert(dst.end(), partial[t][k].begin(), partial[t][k].end());
                }
            }
        }

        return results;
    }

    std::vector<int32_t> solve(int32_t hunger, bool low_health, std::string pattern, int32_t max_tick)
    {
        std::vector<query> inputs;
        inputs.push_back({hunger, low_health, std::move(pattern)});
        return batch_solve(std::move(inputs), max_tick)[0];
    }
}