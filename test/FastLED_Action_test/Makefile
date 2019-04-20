# for ubuntu
ifneq ("$(wildcard $($HOME/arduino))","")
ARDUINO_DIR="$(wildcard $($HOME/arduino))"
else ifneq ("$(wildcard $(/Applications/arduino))","")
ARDUINO_DIR="$(wildcard $(/Applications/arduino))"
endif


ARDMK_DIR=${HOME}/elektronik/Arduino-Makefile
USER_LIB_PATH := $(realpath ../../libraries)

BOARD_TAG    = mega
BOARD_SUB    = atmega2560
ARDUINO_LIBS = FastLED DList MemoryFree FastLED_Action testmacros

USER_DEFINES += -DDEBUG_UART_ON
MONITOR_BAUDRATE = 115200

all:
	@echo "${USER_INCLUDES}"

include ${ARDMK_DIR}/Arduino.mk
