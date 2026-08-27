# CLI build/test/deploy for the SparkFun LoRa Gateway 1-Channel (ESP32).
#
# PlatformIO does the work; this is a thin, discoverable front door so the
# common operations are one word and nobody has to remember pio's flags. It is
# also what CLion, cron, CI or another agent should call -- keeping one entry
# point means the IDE and the terminal cannot drift apart.
#
#   make              what you can do
#   make build        build every application
#   make listen       build one, by name
#   make flash APP=gateway
#   make monitor      serial monitor (quit Ctrl+C, menu Ctrl+])
#   make run APP=rx   flash, then monitor
#   make test         unit tests on the host
#   make check        shell script syntax + shellcheck if present
#   make libs         fetch third-party libraries into vendor/
#   make restore      put the original 2019 gateway firmware back

PIO      ?= $(HOME)/.local/bin/pio
APP      ?= listen
# Autodetected by vendor/model, NOT hardcoded. /dev/ttyUSBn is assigned in
# enumeration order, so a power cycle can move the board -- it went ttyUSB1 ->
# ttyUSB2 mid-session on 2026-08-21 and every hardcoded path broke at once.
# Override for a second board: make flash PORT=/dev/ttyUSB5
PORT     ?= $(shell $(dir $(abspath $(lastword $(MAKEFILE_LIST))))scripts/find-port.sh 2>/dev/null)
# miniterm's menu key. Default is Ctrl+T (20), which terminal emulators, tmux
# and IDE terminals commonly grab, so the banner advertises a key that never
# arrives. 29 is Ctrl+]. Not 28 -- that is SIGQUIT. CLI-only: there is no
# monitor_menu_char project option, despite platformio.ini accepting one.
MENU_CHAR ?= 29
APPS     := listen gateway rx tx tx-lab probe scan sweep

# Everything runs from the makefile's own directory, so `make -C` works.
ROOT     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

.DEFAULT_GOAL := help
.PHONY: help build $(APPS) flash monitor run test check libs clean distclean restore board ide

help:
	@echo 'SparkFun LoRa Gateway 1-Channel (ESP32)'
	@echo
	@echo '  applications : $(APPS)'
	@echo '  default APP  : $(APP)          override with APP=<name>'
	@echo '  port         : $(PORT)     override with PORT=<path>'
	@echo
	@echo '  make build              build every application'
	@echo '  make <app>              build one (e.g. make scan)'
	@echo '  make flash APP=<app>    build and upload'
	@echo '  make monitor            serial monitor: quit Ctrl+C, menu Ctrl+]'
	@echo '  make run APP=<app>      flash then monitor'
	@echo '  make board              identify the attached board'
	@echo '  make test               host-side unit tests'
	@echo '  make check              lint the shell scripts'
	@echo '  make libs               fetch third-party libs into vendor/'
	@echo '  make ide                regenerate CLion CMake files'
	@echo '  make restore            reflash the original 2019 firmware'
	@echo '  make clean / distclean'

build:
	$(PIO) run

# One target per application, so `make scan` works and tab-completion helps.
$(APPS):
	$(PIO) run -e $@

flash:
	$(PIO) run -e $(APP) -t upload --upload-port $(PORT)

monitor:
	$(PIO) device monitor --port $(PORT) --menu-char $(MENU_CHAR)

run: flash
	$(PIO) device monitor --port $(PORT) --menu-char $(MENU_CHAR)

# Host-side tests only. Anything needing the radio is an integration test and
# belongs in scripts/, because a unit test that silently needs hardware is a
# test that fails for the wrong reason on someone else's machine.
test:
	$(PIO) test -e native

check:
	@set -e; for f in $(ROOT)scripts/*.sh; do \
	    bash -n "$$f" && echo "  syntax ok  $$f"; \
	done
	@# Test for the tool separately from running it. `shellcheck ... || echo
	@# "not installed"` conflates a missing tool with findings, because
	@# shellcheck exits non-zero when it has something to say.
	@if command -v shellcheck >/dev/null 2>&1; then \
	    shellcheck -x $(ROOT)scripts/*.sh && echo '  shellcheck clean'; \
	else \
	    echo '  shellcheck not installed -- syntax check only'; \
	fi

libs:
	$(ROOT)scripts/fetch-libraries.sh

board:
	$(ROOT)scripts/read-banner.sh --no-reset --seconds 6 || true
	@esptool --port $(PORT) --no-stub chip-id 2>/dev/null | grep -E 'Chip type|MAC' || true

restore:
	$(ROOT)scripts/restore-firmware.sh

# CLion indexes a PlatformIO project through generated CMake files. They are
# derived, not authored: regenerate rather than edit, and never commit them.
ide:
	$(PIO) project init --ide clion
	@echo 'CMakeLists.txt regenerated. Builds still go through PlatformIO.'

clean:
	$(PIO) run -t clean

distclean:
	rm -rf $(ROOT).pio $(ROOT)vendor
