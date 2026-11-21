# OOP Dungeon Escape (C++ + WebAssembly UI)

A text-based dungeon adventure game built using **Object-Oriented Programming (OOP)** in C++, paired with a graphical browser UI via **WebAssembly (Emscripten)** and **GitHub Actions**.

## OOP Concepts Demonstrated
- **Abstraction & Inheritance:** `Item` base class with `TorchItem`, `HerbItem`, and `KeyItem` subclasses; `Entity` base class inherited by `Player`; `Obstacle` base class with `FloorTrap`, `CreviceSearch`, and `RiddleDoor`.
- **Polymorphism:** Virtual `use()` and `interact()` methods overridden by derived classes.
- **Encapsulation:** Private fields in `Inventory` and `Entity` with controlled accessors and state updates.
- **Composition / Aggregation:** `GameEngine` coordinates `Player`, `Room`s, `Item`s, and `Obstacle`s.

## How to Run via GitHub (No Local C++ Plugin Needed)
1. Upload this repository to GitHub (`main` branch).
2. GitHub Actions runs `.github/workflows/build.yml`, compiling `dungeon_engine.cpp` into `dungeon.js` and `dungeon.wasm` and publishing to the `gh-pages` branch.
3. In **Settings -> Pages**, choose **Deploy from a branch** -> `gh-pages` (`/ (root)`).
