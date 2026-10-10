# Editor Waveform

Shows audio waveforms inside the Geometry Dash level editor. This port uses
Geode's GD bindings, cocos drawing, FMOD headers/libraries, and hooks.

## Using the mod

- The full built-in or downloaded main song is decoded when the editor opens.
  Its waveform appears directly on the editor grid, including during music
  playback and playtesting, and follows GD's native timing and the song offset.
- There is no separate song panel or source selector. Pan and zoom using the
  editor's normal controls.
- Enable, camera lock, vertical offset/scale, line width, and opacity are Geode
  settings. If Eclipse 1.9.4 or newer is installed, the **Editor Waveform** tab
  exposes the same settings. Eclipse is optional. Inline numeric editing targets
  its **ImGui** menu: type signed/decimal offsets and scales directly; line width
  and opacity use whole numbers. Changes synchronize with Geode settings, which
  save the values across restarts. Cocos menu text-edit behavior is not verified.

The preview represents the main song file; Song/SFX triggers and gameplay audio
processing are not included. Platformer levels do not show a grid waveform.
The decoded peak envelope has a 24 MiB memory allowance. Decode errors, including
memory limits and missing songs, are reported once per load in the Geode log.
Song-file changes are detected automatically, including newly downloaded songs.

## Build on macOS

Requires Geometry Dash **2.2081**, a matching **Geode 5.11.x** SDK/runtime,
Geode CLI, CMake 3.25+, and a compiler with C++23 support.
Development uses the SDK selected by `GEODE_SDK`; it does not clone or modify
the installed SDK. CMake discovers the compiler from the development environment.
Use a matching loader: a 5.10.x runtime cannot load this 5.11.x package.

```sh
geode build --config RelWithDebInfo
```

Run this from the project directory with `GEODE_SDK` already set. It configures
and builds in `build/` and installs through the active Geode CLI profile.
Restart Geometry Dash to load the new package, including when updating from
1.0.0: the old song-state hook stays installed until the game exits.

The default build contains both arm64 and x86_64. Geode supplies the macOS
frameworks and FMOD link library. Dependencies and Eclipse's public API are
resolved by the SDK/CLI; no submodule initialization is needed. An SDK version
change may require updating `mod.json` and using the matching loader.
The Eclipse bridge uses its header-only event API; the mod does not link to
Eclipse's binary, so Eclipse can remain absent at runtime.

For development with an already installed Eclipse, pass its local headers:

```sh
geode build --config RelWithDebInfo -- \
  -DEDITOR_WAVEFORM_ECLIPSE_API="/path/to/Geometry Dash.app/Contents/geode/unzipped/eclipse.eclipse-menu/include"
```

This uses the local API headers rather than downloading the Eclipse dependency.
You can also use `GEODE_BINDINGS_REPO_PATH` and CPM's `CPM_<name>_SOURCE` cache
variables to reuse local dependency sources. Keep their versions aligned with
the SDK. These are SDK facilities, not extra libraries in this repository.

The output is `build/asdalion.editor-waveform.geode`. Without an active CLI
profile, install that package manually. For a package-only build with a profile,
pass `-- -DGEODE_DONT_INSTALL_MODS=ON`; restore automatic installation with
`-- -DGEODE_DONT_INSTALL_MODS=OFF` because CMake remembers the option.

## CI packages

GitHub Actions runs standalone tests and builds separate Geode packages for
macOS (arm64 and x86_64) and Windows (x86_64). Builds run on pushes and pull
requests to `main`, or through a manual workflow run. Download the matching
`EditorWaveform-macOS` or `EditorWaveform-Windows` artifact from the workflow run;
each contains `asdalion.editor-waveform.geode` for that platform.

## Tests

Tests are opt-in for normal mod builds. Standalone test builds enable them by
default. Core tests run without Geode, FMOD, or a game installation:

```sh
cmake -S . -B build/tests -DEDITOR_WAVEFORM_TESTS_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure
```

For integration tests against the local macOS FMOD runtime:

```sh
cmake -S . -B build/audio-tests \
  -DEDITOR_WAVEFORM_TESTS_ONLY=ON -DEDITOR_WAVEFORM_FMOD_TESTS=ON \
  -DEDITOR_WAVEFORM_FMOD_LIBRARY="/path/to/Geometry Dash.app/Contents/Resources/fmod/lib/libfmod.dylib" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/audio-tests --parallel
ctest --test-dir build/audio-tests --output-on-failure
```

The SDK's FMOD link shim requires Geode and cannot be used for standalone tests.
The integration suite uses the original local library in `Resources/fmod/lib`
with a non-realtime silent output. It tests real decoding, sample rates, mono
and stereo peaks, PCM bit depths, missing files, and cancellation. The core suite
covers PCM conversion, envelope ranges, and memory limits.

The 1.0.1 playback fix removes an unsafe ARM64 song-state hook that overlaps
GD's adjacent delegate thunk. Song-file polling still refreshes the preview,
and offset polling updates its grid alignment. The Eclipse bridge also corrects
1.9.4's reversed numeric-input arguments without changing Eclipse's headers or library.

The in-game regression checks require a game session; standalone audio tests
cannot verify native hook dispatch or menu interaction:

- At least 30 editor music start/stop cycles, including rapid toggling; verify
  the full-song waveform remains available and no song panel appears.
- Playtest pause/resume/stop, start positions, and exit while a song is decoding.
- Change the song/offset, download or replace a song, and edit speed portals;
  verify the waveform refreshes and aligns with native editor timing.
- Disable/re-enable the waveform and check camera movement, zoom, and normal
  mouse/touch interaction in the area previously occupied by the song panel.
- In Eclipse ImGui, edit signed offsets, decimals, line width 1–10, and opacity
  0–255. Check readable labels, Geode settings synchronization, and persistence
  after restarting GD. Also check loading without Eclipse installed.

Broader in-game checks cover Retina scaling and the absence of a grid waveform
in platformer levels. CI builds packages for both macOS and Windows; Windows
in-game behavior is not verified.

## Structure

`src/main.cpp` contains hooks and the Eclipse settings bridge. `src/config.hpp`
reads Geode settings. The waveform pair handles song decoding and peaks;
the timeline pair owns the editor session and renders the grid waveform. All cocos
and FMOD objects use the libraries supplied by Geode, without replacement engine
code or bundled third-party libraries.

Original mod by ConfiG / cgytrus. [Original demonstration](https://youtu.be/sE_xLYuiZm8).
Licensed under the original MIT license.
