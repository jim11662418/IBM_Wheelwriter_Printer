//************************************************************************//
//                                                                        //
//                       For use with 12 MHZ crystal                      //
//                                                                        //
//************************************************************************//
// Interrupt driven serial 0 functions with RTS/CTS handshaking.
// serial 0 uses receive buffers in internal MOVX SRAM. serial 0 in mode 1
// uses timer 1 for baud rate generation. uart_init() must be called
// before using UART. No syntax error checking.

// for the Small Device C Compiler (SDCC)

#include "reg420.h"

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

#define PAUSELEVEL BUFFERSIZE/4                          // pause communications (RTS = 1) when buffer space < 32 bytes
#define RESUMELEVEL BUFFERSIZE/2                         // resume communications (RTS = 0) when buffer space > 64 bytes

__sbit __at (0xb6) CTS;
__sbit __at (0xb7) RTS;

volatile unsigned char rx_head;                          // receive write index for serial 0
volatile unsigned char rx_tail;                          // receive read index for serial 0
volatile unsigned char rx_remaining;                     // Receive buffer space remaining for serial 0
volatile unsigned char __xdata rx_buf[BUFFERSIZE];       // receive buffer for serial 0 in internal MOVX RAM
volatile __bit tx_ready;

// ---------------------------------------------------------------------------
// Serial 0 interrupt service routine
// ---------------------------------------------------------------------------
void uart0_isr(void) __interrupt(4) __using(3) {
   // serial 0 transmit interrupt
   if (TI) {                                             // transmit interrupt?
      TI = FALSE;                                        // clear transmit interrupt flag
      tx_ready = TRUE;                                   // transmit buffer is ready for a new character
    }

    // serial 0 receive interrupt
    if(RI) {                                             // receive character?
        RI = 0;                                          // clear serial receive interrupt flag
        rx_buf[rx_head] = SBUF0;                         // Get character from serial port and put into serial 0 fifo.
        rx_head = ++rx_head &(BUFFERSIZE-1);

      --rx_remaining;                                    // space remaining in serial 0 buffer decreases
        if (!RTS){                                       // if communications is not now paused...
            if (rx_remaining < PAUSELEVEL) {
               RTS = 1;                                  // pause communications when space in serial buffer decreases to less than 32 bytes
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
    rx_head = 0;                                         // initialize head/tail pointers.
    rx_tail = 0;
    rx_remaining = BUFFERSIZE;                           // 256 characters
    SCON0 = 0x50;                                        // Serial 0 for mode 1.
    TMOD = (TMOD & 0x0F) | 0x20;                         // Timer 1, mode 2, 8-bit reload.
    CKMOD |= 0x10;                                       // Make timer 1 clocked by OSC/1 instead of the default OSC/12
    TH1 = 0xD9;                                          // 9600 bps
    TR1 = TRUE;                                          // Run timer 1.
    REN = TRUE;                                          // Enable receive characters.
    TI = TRUE;                                           // Set TI of SCON to Get Ready to Send
    RI  = FALSE;                                         // Clear RI of SCON to Get Ready to Receive
    ES0 = TRUE;                                          // enable serial interrupt.
    RTS = 0;                                             // clear RTS to allow transmissions from remote console
}

// ---------------------------------------------------------------------------
// returns 1 if there are character waiting in the serial 0 receive buffer
// ---------------------------------------------------------------------------
__bit uart_char_avail(void) {
   return (rx_head != rx_tail);
}

//-----------------------------------------------------------
// waits until a character is available in the serial 0 receive
// buffer. returns the character. does not echo the character.
//-----------------------------------------------------------
char uart_getchar(void) {
    unsigned char buf;

    while (rx_head == rx_tail);                          // wait until a character is available
    buf = rx_buf[rx_tail];
    rx_tail = ++rx_tail &(BUFFERSIZE-1);

    ++rx_remaining;                                      // space remaining in buffer increases
    if (RTS) {                                           // if communication is now paused...
         if (rx_remaining > RESUMELEVEL) {
            RTS = 0;                                     // clear RTS to resume communications when space remaining in buffer increases above 64 bytes
         }
    }
    return(buf);
}

// ---------------------------------------------------------------------------
// sends one character out to serial 0.
// ---------------------------------------------------------------------------
char uart_putchar(char c)  {
   while (!tx_ready);                                    // wait here for transmit ready
   //while (CTS);                                        // wait here for clear to send
   SBUF0 = c;
   tx_ready = 0;
   return (c);
}




