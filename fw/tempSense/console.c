#include <string.h>
#include "console.h"
#include "fifo.h"

typedef struct {
	char *commandStr;
	void (*fn)(uint8_t argc, char *argv[]);
	char *helpStr;
} command_t;

extern void putStr(char* str);

extern fifo_t rxFifo;

static char cmdBuff[64];
static uint8_t argc;
static char* argv[8];

static void helpFn(uint8_t argc, char *argv[]);
static void command1(uint8_t argc, char *argv[]);
static void command2(uint8_t argc, char *argv[]);

static const command_t commands[] = {
	{"command1", command1, "This is command 1, a test command."},
	{"command2", command2, "This is command 2, a different, better, test command."},
	// Add new commands here!
	{"help", helpFn, "Print this!"},
	{NULL, NULL, NULL}
};

//
// Print the help menu
//
static void helpFn(uint8_t argc, char *argv[]) {
	const command_t *command = commands;

	if(argc < 2) {
		while(command->commandStr != NULL) {
			putStr(command->commandStr);
			putStr(" - ");
			putStr(command->helpStr);
			putStr("\n");
			command++;
		}
	} else {
		while(command->commandStr != NULL) {
			if(strcmp(command->commandStr, argv[1]) == 0) {
				putStr(command->commandStr);
				putStr(" - ");
				putStr(command->helpStr);
				putStr("\n");
				break;
			}
			command++;
		}
	}
}

//
// Example Commands
//
static void command1(uint8_t argc, char *argv[]) {
	char num[] = {argc - 1 + '0', 0};
	putStr("Command 1 called with ");
	putStr(num);
	putStr(" arguments!\n");
}

//
// Example commands
//
static void command2(uint8_t argc, char *argv[]) {
	char num[] = {argc - 1 + '0', 0};
	putStr("Command 2 called with ");
	putStr(num);
	putStr(" arguments!\n");
}

void consoleProcess() {
	uint32_t inBytes = fifoSize(&rxFifo);
	if(inBytes > 0) {
		uint32_t newLine = 0;
		for(int32_t index = 0; index < inBytes; index++){
			if((fifoPeek(&rxFifo, index) == '\n') || (fifoPeek(&rxFifo, index) == '\r')) {
				newLine = index + 1;
				break;
			}
		}

		if(newLine > sizeof(cmdBuff)) {
			newLine = sizeof(cmdBuff) - 1;
		}

		if(newLine) {
			uint8_t *pBuf = (uint8_t *)cmdBuff;
			while(newLine--){
				*pBuf++ = fifoPop(&rxFifo);
			}

			// If it's an \r\n combination, discard the second one
			if((fifoPeek(&rxFifo, 0) == '\n') || (fifoPeek(&rxFifo, 0) == '\r')) {
				fifoPop(&rxFifo);
			}

			*(pBuf - 1) = 0; // String terminator

			argc = 0;

			// Get command
			argv[argc] = strtok(cmdBuff, " ");

			// Get arguments (if any)
			while ((argv[argc] != NULL) && (argc < sizeof(argv)/sizeof(char *))){
				argc++;
				argv[argc] = strtok(NULL, " ");
			}

			if(argc > 0) {
				const command_t *command = commands;
				while(command->commandStr != NULL) {
					if(strcmp(command->commandStr, argv[0]) == 0) {
						command->fn(argc, argv);
						break;
					}
					command++;
				}

				if(command->commandStr == NULL) {
					putStr("Unknown command\n");
					helpFn(1, NULL);
				}
			}
		}
	}
}
