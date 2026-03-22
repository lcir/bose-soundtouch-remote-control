# CLion + PlatformIO Debug Workflow Design

## Context

Project uses `PlatformIO` with the `Arduino` framework and currently targets a generic `esp32dev` environment in [platformio.ini](/Users/peny/Development/Projects/boser-remote-control/platformio.ini). The actual hardware is an `ESP32-WROOM-32` board connected over `CP2102` serial. This setup supports flashing and serial monitoring, but it does not provide built-in `USB-JTAG`, so true breakpoint debugging in `CLion` is not available without extra hardware such as `ESP-Prog`.

The goal is not to replace `PlatformIO` or migrate to `ESP-IDF`. The goal is to make day-to-day development in `CLion` practical now, with a strong serial-debug workflow, while keeping a clean upgrade path to real `JTAG` debugging later.

## Goals

- Keep `PlatformIO` as the only build system.
- Add a clean `debug` workflow in `CLion` for build, upload, and serial monitoring.
- Improve observability through targeted firmware logs.
- Avoid invasive refactors or speculative simulator work.
- Prepare a future `JTAG` environment without requiring another redesign.

## Non-Goals

- No migration to `ESP-IDF` or plain `CMake`.
- No full software simulator of `ESP32`, OLED, encoder, and Wi-Fi stack.
- No host-side unit test framework in this phase.
- No attempt to provide breakpoint debugging on the current `CP2102`-only board.

## Recommended Approach

Use multiple `PlatformIO` environments to separate production and diagnostic use cases:

- `env:esp32dev-release`
  - default production-oriented build
  - lower log verbosity
  - normal optimization
- `env:esp32dev-debug`
  - serial-debug oriented build
  - higher log verbosity
  - monitor filters and explicit serial port setup
  - compiler flags chosen for easier diagnosis
- `env:esp32dev-jtag` or equivalent placeholder
  - reserved for future external debugger setup
  - not expected to work until extra hardware is added

`CLion` should call `PlatformIO` tasks instead of introducing a second build path. The IDE becomes a control surface for the existing firmware workflow, not a replacement for it.

## Configuration Design

### PlatformIO Environments

The current single environment should be split into named environments with shared defaults. Shared settings should include:

- `platform = espressif32`
- target board and framework
- common dependencies
- common monitor speed

Per-environment differences should then be isolated to:

- `build_flags`
- `build_type`
- `monitor_filters`
- optional `upload_port` and `monitor_port`
- future `debug_tool`, `debug_init_break`, and related debug settings

The `debug` environment should enable more firmware output, for example through:

- a higher `CORE_DEBUG_LEVEL`
- a dedicated compile-time flag such as `-DAPP_DEBUG=1`

This combination is more useful than `CORE_DEBUG_LEVEL` alone, because the current codebase does not emit enough diagnostic information on its own.

### CLion Integration

The intended `CLion` workflow is:

1. `Build Debug`
2. `Upload Debug`
3. `Monitor Debug`

This can be implemented either through the PlatformIO plugin or shell-backed run configurations that call:

```bash
pio run -e esp32dev-debug
pio run -e esp32dev-debug -t upload
pio device monitor -e esp32dev-debug
```

The same pattern applies to `release`. The important part is that `CLion` always runs the named environment explicitly, so there is no ambiguity about which flags or log level are active.

## Firmware Diagnostics Design

The firmware needs a lightweight logging layer that is easy to leave in place long term. A minimal wrapper header is enough, for example macros like:

- `LOG_INFO(...)`
- `LOG_WARN(...)`
- `LOG_ERROR(...)`
- `LOG_DEBUG(...)`

These macros should compile to serial output only when `APP_DEBUG` is enabled, or reduce verbosity in `release`. This avoids scattering raw `Serial.print` calls everywhere and keeps diagnostic behavior centrally controlled.

### Critical Log Points

Most valuable log points:

- [src/main.cpp](/Users/peny/Development/Projects/boser-remote-control/src/main.cpp)
  - boot start
  - setup mode reason
  - Wi-Fi connect attempt
  - Wi-Fi connected/disconnected transitions
  - Bose reconnect attempts
  - volume commit events
  - button actions and overlay-triggering events
- [src/BoseClient.cpp](/Users/peny/Development/Projects/boser-remote-control/src/BoseClient.cpp)
  - HTTP request path and result status
  - empty response / timeout / invalid response conditions
  - WebSocket connect, disconnect, reconnect
  - switch to polling fallback
  - scheduled refresh causes
- [src/InputController.cpp](/Users/peny/Development/Projects/boser-remote-control/src/InputController.cpp)
  - button press detection in debug mode
  - encoder delta readings when troubleshooting direction or noise

The design intent is not full tracing of every loop iteration. Logs should describe transitions and actions, not flood the serial line continuously.

## Data Flow During Debug Sessions

The operational flow in a debug session should be:

1. `CLion` builds the `esp32dev-debug` environment.
2. Firmware is uploaded to the board through serial.
3. `CLion` opens a serial monitor against the same environment.
4. Developer reproduces a scenario such as boot, setup mode, Wi-Fi reconnect, Bose connection failure, or encoder input.
5. Logs reveal the exact stage and failure mode.
6. OLED remains a secondary coarse status indicator for field use without a terminal.

This gives two layers of observability:

- detailed serial diagnostics in `CLion`
- short user-facing status on device display

## Error Handling Expectations

The design should make failure classes distinguishable. At minimum, logs should clearly separate:

- missing or invalid saved config
- forced setup mode by held power button
- Wi-Fi association failure or retry
- Bose host unreachable
- HTTP transport failure
- HTTP error status
- WebSocket disconnected and polling fallback active

This matters because the current device behavior already includes retries and fallback logic. Without explicit logs, many different failure states look identical from the outside.

## Testing and Simulation Position

This phase does not add a full simulator. For this project, a full emulator for `ESP32 + Arduino + Wi-Fi + encoder + OLED + Bose device` would be disproportionately expensive relative to the immediate benefit.

If future testing is needed before acquiring `JTAG`, the recommended next step is a mock Bose service, not a board simulator. That would let the real firmware run on real hardware while Bose endpoints such as `/info`, `/sources`, `/volume`, and `/now_playing` are simulated on the network. This is much more useful for diagnosing integration behavior than attempting to emulate the whole device stack.

## Future JTAG Upgrade Path

When an external debugger is added later, the design should extend rather than change:

- keep existing `release` and `debug` environments
- add a hardware-specific debug environment for `JTAG`
- keep the logging layer enabled in debug builds

Even after `JTAG` is available, serial logs remain useful for networking and timing-related investigation. The project should not treat logging as a temporary substitute to be removed later.

## Implementation Outline

When implementation starts, the work should proceed in this order:

1. Refactor [platformio.ini](/Users/peny/Development/Projects/boser-remote-control/platformio.ini) into shared defaults plus `release` and `debug` environments.
2. Add a small debug logging header and wire it into the firmware.
3. Insert targeted logs into boot, connectivity, Bose transport, and input handling paths.
4. Document `CLion` run configurations and serial workflow in project docs.
5. Optionally prepare a placeholder `JTAG` environment with comments explaining its future use.

## Acceptance Criteria

The design is successful when:

- firmware can be built, uploaded, and monitored from `CLion` using explicit `PlatformIO` environments
- a `debug` build emits actionable logs for boot, connectivity, Bose I/O, and input events
- `release` remains quieter and suitable for normal use
- the project structure stays on `PlatformIO`
- future `JTAG` support can be added without redesigning the workflow
