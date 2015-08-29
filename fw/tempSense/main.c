#include <msp430.h>
#include "fifo.h"
#include "console.h"

#define FIFO_BUFF_SIZE  (32)

fifo_t rxFifo;

static uint8_t inBuff[FIFO_BUFF_SIZE];

void enableADCWithCh(uint8_t ch) {
	// Enable ADC, interrupts, 2.5V reference, 64 cycles
	ADC10CTL0 = ADC10SHT_3 + ADC10ON + REFON + SREF0 + REF2_5V + ADC10IE; 

	ADC10CTL1 = (ch << 12) & 0xF000; 
	ADC10AE0 |= (1 << ch); // ADC option select
}

uint16_t readADC() {
	uint16_t rval = 0;
	 ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
    rval = ADC10MEM;
}

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

int putchar(int chr) {
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = chr;

	return 1;
}

void putStr(char* str) {
	while(*str != 0) {
		putchar(*str++);
	}
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	P1DIR |= 0x01;					// Set P1.0 to output direction

	fifoInit(&rxFifo, FIFO_BUFF_SIZE, inBuff);

	setupUART();

	enableADCWithCh(3);

	__enable_interrupt();

	for(;;) {
		P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR

		consoleProcess();

		LPM0; // Enter LPM0, interrupts enabled
	}
	
	return 0;
}

void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void) {
  __bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)
}

void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void) {
	char rx = UCA0RXBUF;
	fifoPush(&rxFifo, rx);
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = rx;
	LPM0_EXIT; // Enter LPM0, interrupts enabled
}
