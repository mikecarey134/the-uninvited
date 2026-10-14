# The Uninvited

Zombie survival RPG built on the [GameCore](https://github.com/mikecarey134/GameCore) engine.

## Build

The GameCore engine is a git submodule in `engine/`. It needs Irrlicht 1.8 (apt `libirrlicht-dev`, brew `irrlicht`, or vcpkg `irrlicht`).

```
git clone --recurse-submodules https://github.com/mikecarey134/the-uninvited.git
cd the-uninvited
cmake -S . -B build
cmake --build build -j
./build/game
```

## Status

Early development.
