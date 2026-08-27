# Programming this board in something other than C/C++

Researched 2026-08-25. Nothing here has been run on the hardware — this is a
survey of what is available and what it would cost, recorded so the next person
does not repeat the search. **No toolchain for any of it is installed on this
machine**: `rustc`, `rustup` and `espup` are all absent and there is no
`~/.espressif`.

The existing C++ under `src/` is unaffected by any of this. These would be
additional `[env:]` blocks, not a replacement.

## The one fact that decides every answer

This board is an **ESP32-D0WDQ6 — an Xtensa LX6 core**.

The newer ESP32-C3/C6/H2 parts are **RISC-V**, which upstream LLVM and upstream
Rust support directly. Xtensa is the older architecture with far weaker
toolchain support. In 2026 a project that advertises "ESP32 support" usually
means the RISC-V parts.

**Check the architecture before believing any claim of ESP32 support.** That
single question resolves most of the table below.

## uLisp — the most interesting option for this board

A Common Lisp subset by David Johnson-Davies, distributed as an Arduino sketch,
so it builds under the framework this project already uses.

| | |
|---|---|
| Repository | <https://github.com/technoblogy/ulisp-esp> |
| Author | David Johnson-Davies, www.technoblogy.com |
| Licence | **MIT** — stated in the source header |
| Version seen | **4.9a, dated 2 March 2026** |
| Project size | 79 commits, 10 open issues, **0 pull requests** |
| Documentation | <http://www.ulisp.com/> and <http://forum.ulisp.com/> |

Header of `ulisp-esp.ino`, verbatim:

```c
/* uLisp ESP Release 4.9a - www.ulisp.com
   David Johnson-Davies - www.technoblogy.com - 2nd March 2026

   Licensed under the MIT license: https://opensource.org/licenses/MIT
*/
```

### There are two repositories and one of them is dead

| Repo | Use it? |
|---|---|
| `technoblogy/ulisp-esp` | **Yes.** 79 commits, current, ESP8266 + ESP32 |
| `technoblogy/ulisp-esp32` | **No.** 8 commits; its own README says it is superseded by the above |

The dead one is the more obvious name and ranks in search results. Same trap
shape as `ESP-1ch-Gateway` versus `ESP-1ch-Gateway-v5.0--OLD` elsewhere in this
project: the plausible name is the wrong one.

### What the language actually is

Present: `defun`, `lambda`, `let`/`let*`, closures, `mapcar`, backquote,
`defmacro`, 32-bit integers, floats, strings, arrays, streams, a serial REPL,
and `save-image` to persist a workspace across reboots.

Absent: CLOS, packages, conditions and restarts, `loop`, bignums, rationals,
full `format`. It is a **subset**, not a reduced Common Lisp — code written for
SBCL will not port.

Workspace is a fixed cell array sized by a `#define`. Garbage collection is
stop-the-world mark-sweep.

Hardware primitives are built in: `pinmode`, `digitalwrite`, `analogread`,
`with-i2c`, `with-spi`, plus ESP32 WiFi words (`wifi-connect`, `wifi-server`,
`with-client`).

### Extend it without forking — this is the important part

The source ships with the switch already present, commented out near line 20:

```c
// #define extensions
```

Enable it and add an `Extensions.ino` beside the main sketch. In it you write
ordinary C++ functions returning `object*` and register them in a second lookup
table that uLisp appends to its own at startup:

```c
const tbl_entry_t lookup_table2[] PROGMEM = {
  { stringnow, fn_now, 0203, docnow },
};
```

Four fields: the Lisp symbol name, the C function pointer, a min/max
argument-count code in octal, and a docstring or `NULL`. Documented at
<http://www.ulisp.com/show?19Q4=>.

**So new primitives cost a file you own, not a fork.** You stay on 4.9a and take
4.10 by dropping in the new sketch.

### MIT means what you would hope

Fork it, change it, redistribute it, sell it, keep your changes closed. The only
obligation is preserving the copyright notice and licence text in the source. No
copyleft, no share-alike, no attribution requirement in documentation. Fine for
this repository being public.

### The right shape for this board

**Do not drive the radio from Lisp. Expose `lib/sx1276` to Lisp.**

Write `Extensions.ino` with a few primitives — `(radio-init)`,
`(radio-read reg)`, `(radio-write reg val)`, `(radio-poll)` — each a thin
wrapper over the existing C++ class.

The payoff is real for this project specifically. Almost every finding recorded
in `CLAUDE.md` came from register archaeology: probing `RegVersion` to settle
the pin map, finding `CrcOnPayload` in `RegHopChannel`, walking `RegLna` for lab
mode, reading the `RegModemStat` ladder. Each was an edit-build-flash-read
cycle, and `src/tools/probe` and `src/tools/sweep` exist only because there was
no way to ask the chip a question interactively. A REPL collapses that loop.

### The trap, which is this project's usual one

**uLisp's `with-spi` uses Arduino's default `SPI` object, which on a generic
`esp32dev` board is VSPI — GPIO 18/19/23.** Those pins go to the headers, not
the radio. Reads would return `0x00` forever with no error, exactly the silent
failure `platformio.ini` warns about in its header comment.

This project avoids it in one line, `lib/sx1276/sx1276.cpp:168`:

```c
spi_.begin(board::kPinSck, board::kPinMiso, board::kPinMosi, board::kPinNss);
```

Wrapping the existing driver as an extension sidesteps the problem entirely,
because uLisp's own `with-spi` is then never used for the radio. Driving the
radio from Lisp directly would require patching uLisp's SPI setup instead.

Also check what rate uLisp's `Serial.begin` uses. Everything else in this
project assumes 115200.

### Caveats

- **Zero pull requests across 79 commits.** One person's project, distributed as
  a single large `.ino`. Nothing stops you forking — MIT is MIT — but expect a
  hard fork with no path back upstream. The extensions file exists so that is
  not necessary.
- **It is a file, not a library.** Forking means owning several thousand lines of
  someone else's interpreter and rebasing it against every release.
- **If vendored here**, it belongs in `vendor/` fetched by
  `scripts/fetch-libraries.sh`, matching how the ten gateway libraries are
  handled: script and pinned ref committed, payload not.

## Rust — viable, with real toolchain friction

Two variants:

| | `no_std` (esp-hal) | `std` (esp-idf-hal) |
|---|---|---|
| Target triple | `xtensa-esp32-none-elf` | `xtensa-esp32-espidf` |
| Underneath | bare metal | ESP-IDF (C) with Rust bindings |
| Binary size | ~100–250 KB | ~700 KB – 1 MB |
| WiFi | `esp-wifi` crate | full IDF stack |
| Fits `app0` (1280 KB)? | easily | yes, without much room |

**The friction is Xtensa.** It needs Espressif's LLVM fork, installed with
`espup`, which adds an `esp` rustup channel and about 2 GB. It is not
`rustup target add`. Xtensa has been landing in upstream LLVM as an experimental
backend, so **check whether the fork is still required** rather than assuming.

The radio side is good news: the `lora-phy` crate (embassy ecosystem) supports
SX1276/77/78/79 and takes its SPI and DIO pins as constructor arguments, so the
pin map is four values you pass in rather than a firmware target to match. One
wrinkle — it expects a reset pin, and this board has none (`kPinRst = -1`).

## MicroPython — easy, but the radio driver is hobby code

Officially supported generic ESP32 port; 4 MB is the standard build target.
Flash `ESP32_GENERIC-*.bin` at `0x1000` and you get a REPL over the same CH340.

Budget roughly 1.7 MB of firmware, ~2 MB left as filesystem, ~100 KB free heap.

**No LoRa driver in the standard build.** You would use a pure-Python SX127x
register driver; several exist, all unmaintained. For SF7 receive that is
survivable — the packet sits in the radio's FIFO once DIO0 asserts, so the read
is not timing-critical and MicroPython's poor interrupt latency does not lose
it. What you would be doing is re-deriving work already done in `lib/sx1276`.

Good for interactive poking. A poor base for the link work.

## The rest

| Option | Verdict on this board |
|---|---|
| **Common Lisp proper** (SBCL, CCL, ECL) | **No.** Needs an MMU, megabytes of RAM, a filesystem |
| **Zig** | **No.** Zig has no Xtensa target — RISC-V ESP32s only |
| **TinyGo** | Technically has an ESP32 target; GPIO and little else, no WiFi. Not usable |
| **CircuitPython** | Builds exist, but Adafruit targets S2/S3; plain ESP32 is second-class |
| **s7 Scheme** | Wants more RAM than 520 KB with no PSRAM allows |
| **Chibi-Scheme, TinyScheme** | Embeddable C, portable in principle, no ready-made ESP32 build |
| **fe** | A Lisp in ~800 lines of C. Ports in an afternoon, too minimal to be useful |
| **LispBM** | Built for microcontrollers, concurrent, used in VESC firmware. More Scheme-flavoured. Real, but its ESP32 deployments are mostly ESP32-C3 (RISC-V) |
| **Lua (NodeMCU)** | ESP32 branch exists, much less active than the ESP8266 original |
| **JavaScript (Espruino)** | Unofficial ESP32 builds, dated |
| **Toit** | ESP32-first by design, open source, small ecosystem |
| **esp32forth** | Not a Lisp, but the same interactive-on-target appeal, and capable. Brad Nelson's version is actively developed |

If the attraction is a REPL on the target rather than s-expressions
specifically, esp32forth is the stronger option than uLisp.

## Two facts no language changes

**GPIO12.** The flash-voltage strap is sampled by the ROM bootloader before any
of your code exists. Rust, Lisp, MicroPython — a failed flash still means
power-cycling the board. See `CLAUDE.md`.

**There is no reset pin.** `lib/board/board.h` has `kPinRst = -1`, established by
probe. Any driver or crate that expects a reset GPIO has to be told it does not
exist. `lora-phy` is the specific case that would need thought rather than
transcription.

## Sources

- <https://github.com/technoblogy/ulisp-esp> — current uLisp for ESP32
- <https://raw.githubusercontent.com/technoblogy/ulisp-esp/master/ulisp-esp.ino>
  — MIT licence and release 4.9a read from the source header
- <https://github.com/technoblogy/ulisp-esp32> — superseded, do not use
- <http://www.ulisp.com/show?19Q4=> — adding your own functions
- <http://forum.ulisp.com/> — active; the author answers there
