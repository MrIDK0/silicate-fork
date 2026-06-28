# Silicate (Custom Fork)

> **IMPORTANT NOTE:** This is an **unofficial custom fork** of the original Silicate bot. This repository is maintained independently and is not affiliated with the official Peony's Silicate project. Please do not report bugs found here to the official support channels.

**Silicate** is an advanced, feature-rich macro bot and utility mod for Geometry Dash, built on top of the Geode framework. 

---

## Fork Exclusive Features

Unlike the original bot, this fork includes a powerful custom implementation for dual-player macro management inside the **Record** tab!
New themes!
And many more features or fixes!

---

## Versioning

Currently, the bot is in **alpha**, utilizing `1.0.0-alpha.XX` builds.
* Beta releases will use `1.0.0-beta.XX`.
* Upon full release, the project will migrate to a calendar-based versioning system (e.g., `2026.01-01` for the first build in January 2026).

---

## Repository Structure

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
