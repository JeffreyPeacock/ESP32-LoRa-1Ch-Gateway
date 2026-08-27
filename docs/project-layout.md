# Project layout

Built for many small programs plus a few real applications, driven from the CLI
and indexed by CLion. Written down because the constraints are PlatformIO's, not
obvious, and violating them fails in confusing ways.

```
Makefile                  the CLI front door -- build, flash, monitor, test
platformio.ini            one [env:...] per application
lib/                      OUR shared libraries            COMMITTED
  board/board.h             the pin map, one definition
  sx1276/                   radio driver
  loramath/                 pure arithmetic, host-testable
  netconfig/                WiFi credentials in NVS
src/
  apps/                   full applications, one directory each
    listen/                 receive and echo to serial -- nothing else
    gateway/                WiFi + HTTP + LoRa receive
    link/                   two-radio range test (rx and tx roles)
  tools/                  small single-purpose utilities
    probe/                  does the radio answer, and on which SPI bus?
    scan/                   sweep known LoRa settings, report what is heard
test/                     host-side unit tests, `make test`
vendor/                   third-party libraries              GITIGNORED
scripts/                  host-side shell tooling
docs/  etc/               findings, datasheets, vendor material
```

## The four rules that matter

**1. `lib/` is ours and committed; `vendor/` is theirs and is not.**
PlatformIO treats `lib/` as the project's private libraries and links whichever
ones an application `#include`s. Third-party code fetched by
`scripts/fetch-libraries.sh` goes to `vendor/`, reached through
`lib_extra_dirs`. Putting fetched code in `lib/` — which this project did
briefly — means either committing other people's libraries to a public repo or
gitignoring the directory your own shared code lives in.

**2. Every application needs its own `[env:...]` with a `build_src_filter`.**
PlatformIO compiles *everything* under `src/` by default, so two `main()`s will
not link. The filter selects exactly one:

```ini
[env:gateway]
extends = esp32
build_src_filter = +<apps/gateway/>
```

**3. Board settings live in `[esp32]`, not `[env]`.** `[env]` applies to every
environment including the host-side test one, and the native platform rejects
`board = esp32dev` with "Unknown board ID". Anything that runs on hardware
extends `[esp32]`; `[env]` holds only what is genuinely universal.

**4. Shared code that needs testing must not include `<Arduino.h>`.**
`lib/loramath` is header-only and dependency-free, which is why `make test`
runs on the workstation in a third of a second with no board attached.
Arduino-dependent code can only be tested on the device, which is slower and
needs hardware present, so keep the arithmetic separate from the register
writes.

## Adding a new program

Three steps, no build system to edit:

```bash
mkdir -p src/tools/blink && $EDITOR src/tools/blink/main.cpp
```

```ini
[env:blink]
extends = esp32
build_src_filter = +<tools/blink/>
```

```make
APPS := listen gateway rx tx tx-lab probe scan blink   # in the Makefile
```

Then `make blink` builds it and `make flash APP=blink` uploads it.

**apps/ or tools/?** If it is something you would leave running, it is an app.
If it answers one question and you read the output once, it is a tool. `probe`
answers "is the radio wired the way I think"; `gateway` is meant to sit there.

## The CLI, which is the real interface

`make` with no arguments lists everything. The common ones:

```bash
make build                 # every application
make scan                  # just one
make flash APP=gateway     # build and upload
make run APP=rx            # flash, then monitor
make monitor               # serial monitor: Ctrl+C quits, Ctrl+] is the menu
make test                  # host-side unit tests, no board needed
make check                 # shell script lint
make board                 # what is attached, and what is it running
make restore               # put the original 2019 firmware back
```

Everything routes through this file. Automation, CI and CLion should all call
`make`, so the IDE and the terminal cannot drift apart — which is the usual
failure of IDE-driven embedded projects.

### Serial monitor keys

`pio device monitor` wraps pyserial's miniterm, and two of its defaults are
unhelpful here. They are fixed in **different places**, and the reason matters:

| | default | ours | set where |
|---|---|---|---|
| local echo | off | **on** | `platformio.ini`, `monitor_echo = yes` |
| menu key | Ctrl+T (20) | **Ctrl+] (29)** | **Makefile**, `--menu-char` |

**There is no `monitor_menu_char` project option.** PlatformIO's are
`monitor_baud/dir/dtr/echo/encoding/eol/filters/parity/port/raw/rts/speed`, and
its INI parser accepts unknown keys **silently** — so an invented one is echoed
back by `pio project config` as though it took effect, and does nothing. This
project made exactly that mistake. If a `monitor_*` setting appears to be
ignored, check it against that list before assuming a bug.

The menu key is CLI-only, so `make monitor` passes it:

```make
MENU_CHAR ?= 29
monitor:
	$(PIO) device monitor --port $(PORT) --menu-char $(MENU_CHAR)
```

Quit is **Ctrl+C**. `Ctrl+] Ctrl+H` is help, and `Ctrl+] Ctrl+R` toggles RTS —
which on this board is EN, so it resets the device.

If Ctrl+] is taken in your terminal too, override it:
`make monitor MENU_CHAR=30`. Avoid 28, which is SIGQUIT.

Most of the time you will not need the menu at all: `listen` takes single-key
commands (`l`, `n`, `?`) and `gateway` takes typed lines.

## CLion

CLion does not build PlatformIO projects natively; it indexes them through
generated CMake files.

```bash
make ide        # runs: pio project init --ide clion
```

That writes `CMakeLists.txt` and `CMakeListsPrivate.txt`. **They are derived,
not authored** — regenerate rather than edit, and they are gitignored. Actual
compilation still goes through PlatformIO, so if the index and the build ever
disagree, the build is right.

Two practical notes for someone coming back to C++ after a while:

- CLion's PlatformIO plugin (JetBrains ships one) can create run configurations
  that call `pio` directly. Either that or a Makefile run configuration is
  fine; both end at the same commands.
- The generated CMake lists **one** environment. Regenerate after adding an
  app, or the new sources will not be indexed even though they build.

## Conventions this follows, and one it does not

`src/` for programs, `lib/` for reusable code, `test/` alongside, and a
`Makefile` at the root are all conventional C/C++ and will read normally to
anyone who has been away from it. The deliberate departure is `include/`:
plain C++ projects usually put public headers there, but PlatformIO expects a
library's headers next to its sources inside `lib/<name>/`, and fighting that
buys nothing.
