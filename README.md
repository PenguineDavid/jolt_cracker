# joltcr

A C++ library for cracking Minecraft HUD jolt patterns back to HUD ticks.

Given a hunger value, whether the player is at low health, and the 10-symbol
jolt pattern Minecraft's HUD produced, `joltcr` brute-forces every candidate
HUD tick in a bounded range that could have produced that pattern.

The library reproduces Mojang's `java.util.Random` LCG exactly, so results are
deterministic and match the game's own HUD jitter logic.

## Features

- Exact `java.util.Random` reimplementation (48-bit LCG, `next_bits` / `next_int`)
- Single-query (`solve`) and batched (`batch_solve`) search
- Wildcard support: `?` matches any jolt direction
- Multithreaded across all available cores
- Deterministic, tick-sorted output regardless of thread scheduling
- CMake package with `find_package(joltcr)` support

## Requirements

- C++17 or later
- CMake 3.21 or later (for building)
- A compiler with `std::thread` support (GCC, Clang, MSVC)

## Building

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build
```

To install system-wide:

```bash
sudo cmake --install build --prefix /usr/local
```

## Using it from another CMake project

After installing:

```cmake
find_package(joltcr REQUIRED)
target_link_libraries(my_app PRIVATE joltcr::joltcr)
```

## Vendoring

If you'd rather not install the library, copy `include/joltcr.h` and
`src/joltcr.cpp` into your project and compile `src/joltcr.cpp` alongside your
own sources. No other files are required at runtime.

## API

```cpp
#include <joltcr.h>

// Single query: returns every HUD tick <= max_tick that matches.
std::vector<int32_t> ticks = joltcr::solve(
    /* hunger     */ 18,
    /* low_health */ false,
    /* pattern    */ "0++000++--",
    /* max_tick   */ 8 * 60 * 60 * 20   // 8 hours at 20 ticks/sec
);

// Batched query: one result vector per input query.
std::vector<joltcr::query> queries = {
    {18, false, "0++000++--"},
    {18, false, "0++0?0++--"},   // ? matches any direction
    {10, false, "0++000++--"},
};
std::vector<std::vector<int32_t>> results = joltcr::batch_solve(queries, max_tick);

// Convert a HUD tick to hours:minutes:seconds of in-game time.
joltcr::video_time vt = joltcr::hud_ticks_to_vid_time(ticks[0]);
```

### Pattern format

Patterns are exactly 10 characters, one per hunger shank, read from the
in-game HUD:

| Character | Meaning           |
|-----------|-------------------|
| `+`       | jolt shifted up   |
| `-`       | jolt shifted down |
| `0`       | no shift          |
| `?`       | any of the above  |

### Result units

`solve` and `batch_solve` return **HUD ticks** (the same unit Minecraft uses
internally). Use `hud_ticks_to_vid_time` if you want hours/minutes/seconds of
in-game time. HUD ticks are the more precise unit and are what you should store
or compare against.

## Credits

The jolt-cracking technique was independently developed by multiple groups. This
library is an independent C++ implementation and does not contain code from any
existing tool.

Credit for popularising the technique and releasing the first public tools goes
to **Dark Spart** (web-based Jolt Cracker and Python version) and **Fanda**
(Tick Studio). They ask for credit for popularising the method and providing
tools, while explicitly not claiming exclusive ownership of the technique itself.

## License

LGPL-3.0-or-later. See `LICENSE`.