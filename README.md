# Astral Dungeon (C++ OOP + WebAssembly UI)

A 4-room text and graphical dungeon adventure built in C++ using OOP principles and compiled to WebAssembly via GitHub Actions.

## Features & OOP Structure
- **Items (`Item` -> `TorchItem`, `BladeItem`, `HerbItem`, `KeyItem`):** Gear that boosts attack damage, heals HP, or unlocks the final gate.
- **Entities & Enemies (`Entity` -> `Player`, `Enemy` -> `CryptStalker`, `ShadowWarden`):** Turn-based combat and health tracking.
- **Obstacles & Puzzles (`Obstacle` -> `FloorTrap`, `CreviceSearch`, `BrazierPuzzle`, `RiddleDoor`):** Interactive hazards and riddles with guided UI choices.
- **4 Rooms (`Room` graph in `GameEngine`):** Damp Dungeon Cell -> Trapped Gallery -> Ember Sanctum -> Astral Exit Gate.
