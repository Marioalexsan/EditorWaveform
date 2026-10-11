# Editor Waveform

Geode mod that draws the main song's waveform on the Geometry Dash editor grid.
Hooks/settings live in `src/main.cpp`, audio in `src/waveform.*`, and rendering in
`src/timeline.*`. `references/` contains the original mod for context.
Keep player-facing docs short; development instructions belong here.

## Build

Requires Geode CLI, an SDK matching `mod.json`, CMake 3.25+, and C++23.
Set `GEODE_SDK` to the installed SDK; use its supplied bindings, cocos, and FMOD.

```sh
geode build --config RelWithDebInfo
```

Output: `build/asdalion.editor-waveform.geode`. The CLI installs to its active
profile; add `-- -DGEODE_DONT_INSTALL_MODS=ON` for packaging only.
CI builds universal macOS and Windows x64 packages.

## Tests

Core tests run without Geode or a game installation:

```sh
cmake -S . -B build/tests -DEDITOR_WAVEFORM_TESTS_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests --config Debug --parallel
ctest --test-dir build/tests -C Debug --output-on-failure
```

Optional macOS audio tests also need `GEODE_SDK`, `EDITOR_WAVEFORM_FMOD_TESTS=ON`,
and `EDITOR_WAVEFORM_FMOD_LIBRARY` pointing to the game's original FMOD dylib.
For hook/UI changes, check playback, playtesting, song/offset changes, settings
persistence, and loading with and without Eclipse in-game. Windows gameplay
still needs verification. Run `git diff --check` before finishing.
