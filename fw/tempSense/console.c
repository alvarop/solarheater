#include <string.h>
#include "console.h"
#include "fifo.h"

typedef struct {
	char *commandStr;
	void (*fn)(uint8_t argc, char *argv[]);
	char *helpStr;
} command_t;

extern int putchar(int chr);
extern void putStr(char* str);
extern uint16_t readADC();
extern void enableADCWithCh(uint8_t ch);
extern void printTemp(uint16_t adcVal);

extern fifo_t rxFifo;

static char cmdBuff[64];
static uint8_t argc;
static char* argv[8];

static void helpFn(uint8_t argc, char *argv[]);
static void read1(uint8_t argc, char *argv[]);
static void read2(uint8_t argc, char *argv[]);
static void read3(uint8_t argc, char *argv[]);

static const command_t commands[] = {
	{"r1", read1, "Print temp1"},
	{"r2", read2, "Print temp2"},
	{"r3", read3, "Print reference temp"},
	// Add new commands here!
	{"help", helpFn, "Print this!"},
	{NULL, NULL, NULL}
};

//
// Print the help menu
//
static void helpFn(uint8_t argc, char *argv[]) {
	const command_t *command = commands;

	while(command->commandStr != NULL) {
		putStr(command->commandStr);
		putStr(" - ");
		putStr(command->helpStr);
		putStr("\n");
		command++;
	}
	
}

#define ADC_COUNT (1024)	// Take this many samples and average them out

void printTempWithCh(uint8_t ch) {
	uint32_t adcVal = 0;

	enableADCWithCh(ch);

	for(uint16_t count = 0; count < ADC_COUNT; count++){
		adcVal += readADC();
	}

	adcVal /= ADC_COUNT;

	putStr("T = ");
	printTemp(adcVal);
	putStr(" C\n");
}

static void read1(uint8_t argc, char *argv[]) {
	printTempWithCh(3);
}

static void read2(uint8_t argc, char *argv[]) {
	printTempWithCh(4);
}

static void read3(uint8_t argc, char *argv[]) {
	printTempWithCh(5);
}

void consoleProcess() {
	uint32_t inBytes = fifoSize(&rxFifo);
	if(inBytes > 0) {
		uint32_t newLine = 0;
		for(int32_t index = 0; index < inBytes; index++){
			if(fifoPeek(&rxFifo, index) == '\n') {
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

			*(pBuf - 1) = 0; // String terminator

			const command_t *command = commands;
			while(command->commandStr != NULL) {
				if(strcmp(command->commandStr, cmdBuff) == 0) {
					command->fn(0, NULL);
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
