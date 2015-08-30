#include <stdint.h>
#include <msp430.h>
// #include "fifo.h"
// #define FIFO_BUFF_SIZE  (32)
// static uint8_t inBuff[FIFO_BUFF_SIZE];
// fifo_t rxFifo;

#define VREF (250000)		// 2.5V * 100000
#define ADC_DIV (1024)		// 10-bit adc
#define V0 (40000)			// .400 V * 100000
#define TC (195)			// (19.5 mV/C) / 10 to get degC * 10

static volatile uint32_t msTime;
static uint32_t nextBlink;
static uint32_t nextTempCheck;


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

    return rval;
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

static const char hexDigits[] = "0123456789ABCDEF";

void uint16ToHex(uint16_t val) {
	char buff[5];
	buff[0] = hexDigits[(val >> 12) & 0xF];
	buff[1] = hexDigits[(val >> 8) & 0xF];
	buff[2] = hexDigits[(val >> 4) & 0xF];
	buff[3] = hexDigits[(val >> 0) & 0xF];
	buff[4] = 0;
	putStr(buff);
}

void uint16ToDec(uint16_t val) {
	char buff[6];
	char *chrPtr = &buff[5];
	buff[5] = 0;

	do {
		*--chrPtr = val % 10 + '0';
		val /= 10;
	} while(val);

	putStr(chrPtr);
}

uint16_t getADCAvg(uint8_t ch, uint16_t samples) {
	uint32_t adcVal = 0;

	enableADCWithCh(ch);

	for(uint16_t count = 0; count < samples; count++){
		adcVal += readADC();
	}

	adcVal /= samples;

	return adcVal;
}

void printTemp(uint16_t adcVal) {
	uint32_t temp;
	temp = ((uint32_t)adcVal * VREF / ADC_DIV); // Get adc voltage * 100000
	temp = (temp - V0)/TC; // Temp in degrees * 10
	uint16ToDec(temp/10);
	putchar('.');
	uint16ToDec(temp - (temp/10) * 10);
}

void checkTemps() {
	uint16_t t1, t2, tRef;
	t1 = getADCAvg(3, 512);
	t2 = getADCAvg(4, 512);
	tRef = getADCAvg(5, 512);

	putStr("t1=");
	printTemp(t1);
	putStr(" t2=");
	printTemp(t2);
	putStr(" tRef=");
	printTemp(tRef);

	if(t1 > tRef) {
		P1OUT |= (1 << 6);
		putStr(" Enable Fan!!");
	} else {
		P1OUT &= ~(1 << 6);
	}

	putchar('\n');
}

#define BLINK_PERIOD_MS (500)
#define TEMP_CHECK_PERIOD_MS (1000)
#define TIMER_PERIOD_US (1000)

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	P1DIR |= (1 << 0) | (1 << 6);	// Set P1.0 and P1.6 as outputs

	nextBlink = 0;

	// Setup timer
	CCTL0 = CCIE;				// CCR0 interrupt enabled
	CCR0 = TIMER_PERIOD_US;				// 1ms timer			
	TACTL = TASSEL_2 + MC_2;	// SMCLK, contmode

	// fifoInit(&rxFifo, FIFO_BUFF_SIZE, inBuff);

	setupUART();

	__enable_interrupt();

	for(;;) {
		if(msTime > nextBlink) {
			nextBlink += BLINK_PERIOD_MS;
			P1OUT ^= 0x01; // Toggle P1.0
		}

		if(msTime > nextTempCheck) {
			nextTempCheck += TEMP_CHECK_PERIOD_MS;
			checkTemps();
		}

		// consoleProcess();

		LPM0; // Enter LPM0, interrupts enabled
	}
	
	return 0;
}

void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void) {
	__bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)
}

void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void) {
	char rx = UCA0RXBUF;
	// fifoPush(&rxFifo, rx);
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = rx;
	
	LPM0_EXIT; // Enter LPM0, interrupts enabled
}

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A (void) {

	CCR0 += TIMER_PERIOD_US; // Add Offset to CCR0
	msTime++;

	LPM0_EXIT; // Enter LPM0, interrupts enabled
}
