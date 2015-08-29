#include <msp430.h>
#include "fifo.h"
#include "console.h"

#define FIFO_BUFF_SIZE  (32)

fifo_t rxFifo;

static uint8_t inBuff[FIFO_BUFF_SIZE];

void setupUART() {

	// If calibration constant erased
	if (CALBC1_1MHZ==0xFF)
	{
		// do not load, trap CPU!!	
		while(1);
	}

	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	DCOCTL = CALDCO_1MHZ;
	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

void putStr(char* str) {
	while(*str != 0) {
		while (!(IFG2&UCA0TXIFG));
		UCA0TXBUF = *str++;
	}
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	P1DIR |= 0x01;					// Set P1.0 to output direction

	fifoInit(&rxFifo, FIFO_BUFF_SIZE, inBuff);

	setupUART();

	__enable_interrupt();

	for(;;) {
		volatile unsigned int i;	// volatile to prevent optimization

		P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR

		i = 10000;					// SW Delay
		do i--;
		while(i != 0);

		consoleProcess();
	}
	
	return 0;
}

void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void) {
	char rx = UCA0RXBUF;
	fifoPush(&rxFifo, rx);
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = rx;
}
