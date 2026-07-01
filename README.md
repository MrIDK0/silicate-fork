# slc replay format
# Silicate (Custom Fork)

## V3 Documentation
> **IMPORTANT NOTE:** This is an **unofficial custom fork** of the original Silicate bot. This repository is maintained independently and is not affiliated with the official Peony's Silicate project. Please do not report bugs found here to the official support channels.

A tiny, fast and extensible replay format, primarly used for Geometry Dash replays.
**Silicate** is an advanced, feature-rich macro bot and utility mod for Geometry Dash, built on top of the Geode framework. 

### Note on universality
---

Unlike slc version 2, slc version 3 is designed to be incredibly modular and extensible. Assuming you (or the developer of the tool you're using) use(s) builtin atoms for what they're designed to do, this format can prove to be useful in a cross-bot setting as well.
## Fork Exclusive Features

## Example
Unlike the original bot, this fork includes a powerful custom implementation for dual-player macro management inside the **Record** tab!
New themes!
And many more features or fixes!

```cpp
#include <slc/slc.hpp>
---

// Notice how we don't define the replay yet; at this point in time we're just defining the actions in the replay
// This is assuming the tickrate is stored somewhere and the slc3 replay is not the single source of truth for metadata (it shouldn't be)
## Versioning

// It's safe to use an atom as a single source of truth though, which is what we do for actions
slc::ActionAtom actions;
Currently, the bot is in **alpha**, utilizing `1.0.0-alpha.XX` builds.
* Beta releases will use `1.0.0-beta.XX`.
* Upon full release, the project will migrate to a calendar-based versioning system (e.g., `2026.01-01` for the first build in January 2026).

using ActionType = slc::Action::ActionType;
---

actions.addAction(400, ActionType::Jump, false, true);
actions.addAction(500, ActionType::Jump, false, false);
## Repository Structure

slc::Replay<> replay;

replay.m_meta.m_tps = 480.0; // Set TPS for all action streams (atoms)

replay.m_atoms.add(std::move(actions)); // Add atom to replay

// Save the replay
std::ostream file("out.slc"); // Recommended to use .slc as the extension
replay.write(file);
```

## V2 Documentation

A tiny and incredibly fast replay format for storing Geometry Dash replays.

### Warning!

This format is not meant to be universal; an `slc` replay from one bot may not work with another bot. If you find yourself constantly switching between bots, you may want to consider using bots with a more universal format, like `.gdr`.

## Example

```cpp
#define SLC_NO_DEFAULT
#include <slc/slc.hpp>

namespace sv2 = slc::v2;
using InputType = sv2::Input::InputType;

// Declare the meta by using a struct
struct ReplayMeta {
  uint64_t seed;
};

sv2::Replay<ReplayMeta> replay;

// OR

sv2::Replay<void> replay; // For no meta

// Set tps by directly changing the member
replay.m_tps = 480.0;

// Add inputs to the replay
replay.addInput(400, InputType::Jump, false, true);
replay.addInput(500, InputType::Jump, false, false);

// Change the TPS mid-replay
replay.addTPSInput(1000, 720.0);

// Save the replay
std::ostream file("out.slc");
replay.write(file);
```

## Features

- **Tiny**: The format is incredibly small, allowing huge savings in storage space.
- **Fast**: The format is incredibly fast to parse and write.
- **Extensible**: The format allows you to add your own metadata to the file. Adding metadata to inputs isn't possible due to it being unnecessary.
- **Comprehensive**: The format allows you to account for every single edge case, or input type you can think of.
- **Safe**: No such thing as a corrupted replay file.

## Motivation

Currently used replay formats are either incredibly slow or incredibly big, or both. Inefficient storage of replay data leads to lack of shareability
for very large files. Notoriously tiny formats (like `.ybot`) are incredibly slow to parse and write, due to them using variable length integers. The most popular replay format (`.gdr`) uses msgpack or json for serialization, which is incredibly slow and wasteful.
```text
src/
    assist/       - Assist features (autoclicker, hitboxes, etc.)
    bot/          - Core bot components
    checkpoint/   - Practice mode fix implementation
    hooks/        - Core game logic hooks
    label/        - Overlays and text label rendering system
    physics/      - Geometry Dash physics decompilation for accurate trajectory
    render/       - Video renderer and DSP audio recorder
    replay/       - Replay system and macro parsing (.slc3 format)
    settings/     - Bot's internal settings module
    shared/       - Shared utilities (e.g., keybind logic)
    trajectory/   - Simulation and physics trajectory logic
    ui/           - Mod interface and menu management
    util/         - Generic utilities (midhooking tools, etc.)
lib/
    tabby/        - Custom UI framework based on ImGui
