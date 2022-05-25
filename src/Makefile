SRC=main.c

CC 				:=
ifeq ($(OS),Windows_NT)
	CC = C:\Program Files\SDCC\bin\sdcc.exe
	ASM = "C:\Program Files (x86)\gputils\bin\gpasm.exe"
	LD = "C:\Program Files (x86)\gputils\bin\gplink.exe"
	LDPARAMS = -I"C:\Program Files\SDCC\bin\..\lib\pic16" -I"C:\Program Files\SDCC\bin\..\non-free\lib\pic16"   -w -r -m -s linker/18f45k50.lkr -o build/firmware.hex "main.o"  "crt0iz.o" "libdev18f45k50.lib" "libsdcc.lib" "libc18f.lib"
	DEL = del
	DELPARAMS =
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
		CC = sdcc
		LD = gplink
	endif
		UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		CC = sdcc
		LD = gplink
		LDPARAMS = -I/opt/homebrew/bin/../share/sdcc/lib/pic16 -I/opt/homebrew/Cellar/sdcc/4.2.0/share/sdcc/lib/pic16 -I/opt/homebrew/bin/../share/sdcc/non-free/lib/pic16 -I/opt/homebrew/Cellar/sdcc/4.2.0/share/sdcc/non-free/lib/pic16 -w -r -m -s linker/18f45k50.lkr -o build/firmware.hex main.o crt0iz.o libdev18f45k50.lib libsdcc.lib libc18f.lib
		DEL = rm
		DELPARAMS = -f
	endif
endif

FAMILY=pic16
PROC=18f45k50

all: compile assamble link clean

compile: $(SRC)
	$(CC) --use-non-free -m$(FAMILY) -p$(PROC) $^ -V -c -S

assamble:
	$(ASM) -D__STACK_MODEL_SMALL -o "main.o" -c "main".asm

link:
	$(LD) $(LDPARAMS)

clean:
	${DEL} ${DELPARAMS} $(SRC:.c=.o) && ${DEL} ${DELPARAMS} $(SRC:.c=.lst) && ${DEL} ${DELPARAMS} $(SRC:.c=.asm) && cd build && ${DEL} ${DELPARAMS} firmware.cod && ${DEL} ${DELPARAMS} firmware.map && ${DEL} ${DELPARAMS} firmware.lst

.PHONY: all clean