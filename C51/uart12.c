// Interrupt driven serial functions for DS89C440 MCU with RTS/CTS handshaking..
// Both serial0 (for the console) and serial1 (for the Wheelwriter) use 
// receive buffers in internal MOVX SRAM. Serial0 in mode 1 uses timer 1 
// for baud rate generation. Serial1 in mode 2 uses the system clock for 
// baud rate generation. 'init_serial0()' and 'init_serial1()' must be called 
// before using UARTs. No syntax error handling. No handshaking.

// For use with 12 MHZ crystal

// for the Keil C51 compiler

#include <reg420.h>

#define FALSE 0
#define TRUE  1

//////////////////////////////////////// Serial 0 /////////////////////////////////////
#define BUFFERSIZE 128
#if BUFFERSIZE < 4
    #error BUFFERSIZE may not be less than 4.
#elif BUFFERSIZE > 256
    #error BUFFERSIZE may not be greater than 256.
#elif ((BUFFERSIZE & (BUFFERSIZE-1)) != 0)
    #error BUFFERSIZE must be a power of 2.
#endif
		
#define PAUSELEVEL BUFFERSIZE/4                             // pause communications (RTS = 1) when buffer space < 32 bytes
#define RESUMELEVEL BUFFERSIZE/2                            // resume communications (RTS = 0) when buffer space > 64 bytes

sbit CTS = P3^6;                                            // CTS input for serial 0  (pin 16)
sbit RTS = P3^7;                                            // RTS output for serial 0 (pin 17)
volatile unsigned char rx_head;                             // receive write index for serial 0
volatile unsigned char rx_tail;                             // receive read index for serial 0
volatile unsigned char rx_remaining;                        // Receive buffer space remaining for serial 0 
volatile unsigned char xdata rx_buf[BUFFERSIZE];            // receive buffer for serial 0 in internal MOVX RAM
volatile bit tx_ready;

// ---------------------------------------------------------------------------
// Serial 0 interrupt service routine
// ---------------------------------------------------------------------------
void uart0_isr(void) interrupt 4 using 3 {
    // serial 0 transmit interrupt
    if (TI) {                                                // transmit interrupt?
        TI = FALSE;                                          // clear transmit interrupt flag
        tx_ready = TRUE;                                     // transmit buffer is ready for a new character
    }

    // serial 0 receive interrupt
    if(RI) {                                                 // receive character?
        RI = 0;                                              // clear serial receive interrupt flag
        rx_buf[rx_head] = SBUF0;                             // Get character from serial port and put into serial 0 fifo.
        rx_head = ++rx_head &(BUFFERSIZE-1);
        
        --rx_remaining;                                      // space remaining in serial 0 buffer decreases
        if (!RTS){                                           // if communications is not now paused...
            if (rx_remaining < PAUSELEVEL) {
               RTS = 1;                                      // pause communications when space in serial buffer decreases to less than 32 bytes
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  Initialize serial 0 for mode 1, 9600 bps, standard full-duplex asynchronous
//  communications using timer 1 clocked at OSC/1 instead of the default OSC/12
//  for baud rate generation. Note: baudrate values are for use with a 12MHz crystal.
// ---------------------------------------------------------------------------
void uart_init(void) {
    rx_head = 0;                   		           // initialize head/tail pointers.
    rx_tail = 0;
    rx_remaining = BUFFERSIZE;                    // 128 characters

    TMOD = (TMOD & 0x0F) | 0x20;   			        // Timer 1, mode 2, 8-bit reload.
    CKMOD |= 0x10;				   			        			// Make timer 1 clocked by OSC/1 instead of the default OSC/12
    TH1 = 0xD9;                             		  // 9600 bps
    TR1 = TRUE;                    			        // Run timer 1.

    SCON0 = 0x50;                  			        // Serial 0 for mode 1.
    REN = TRUE;                    			        // Enable receive characters.
    TI = TRUE;                     			        // Set TI of SCON to Get Ready to Send
    RI  = FALSE;                   			        // Clear RI of SCON to Get Ready to Receive
    ES0 = TRUE;                    			        // enable serial interrupt.
    RTS = 0;                                      // clear RTS to allow transmissions from remote console
}

// ---------------------------------------------------------------------------
// returns 1 if there are character waiting in the serial 0 receive buffer
// ---------------------------------------------------------------------------
bit uart_char_avail(void) {
    return (rx_head != rx_tail);
}

//-----------------------------------------------------------
// waits until a character is available in the serial 0 receive
// buffer. returns the character. does not echo the character.
//-----------------------------------------------------------
char uart_getchar(void) {
    unsigned char buf;

    while (rx_head == rx_tail);                              // wait until a character is available
    buf = rx_buf[rx_tail];
    rx_tail = ++rx_tail &(BUFFERSIZE-1);    
    
    ++rx_remaining;                                          // space remaining in buffer increases
    if (RTS) {                                               // if communication is now paused...
       if (rx_remaining > RESUMELEVEL) {         
          RTS = 0;                                         // clear RTS to resume communications when space remaining in buffer increases above 64 bytes
       }  
    }
    return(buf);
}

// ---------------------------------------------------------------------------
// sends one character out to serial 0.
// ---------------------------------------------------------------------------
char uart_putchar(char c)  {
    while (!tx_ready);                                       // wait here for transmit ready
    //while (CTS);                                           // wait here for clear to send
    SBUF0 = c;
    tx_ready = 0;
    return (c);
}
