// A Simple STOPWATCH project
// Author: MCermak
// 6_12_2016

// Created using MBED, target is a NUCLEO ATmega controller, 7 segment display
// Additional UART control 

#include "mbed.h"

//#define UART_SPEED 9600
#define UART_SPEED 115200

Serial uart(USBTX, USBRX);  //define serial named uart

SPI spi(D11, D12, D3);  // mosi, miso, sclk
DigitalOut cs(D10);     // 7-segment pin
DigitalOut led(LED1);   // led pin

InterruptIn preruseni(PC_13); // interrupt(USER_BUTTON)

Ticker flipper;
Ticker counter;
Timer t;

volatile char rxZnak;
volatile bool ticker = 0;
bool overflow = 0;
bool run = 0;
bool reset = 0;
bool restart;
        
enum SEG {seg1 = 0b00000011, seg2 = 0b000000101, seg3 = 0b00001001, seg4 = 0b00010000};

enum T {DECIMAL, TIME};

int ARR [] = {0b10000001, 0b11110011,0b01001001, 0b01100001, 0b00110011, 0b00100101, 0b00000101, 0b11110001, 0b00000001, 0b00100001};

class NUM {
private:
    NUM* NUM_next;
    int NUM_current;
    int NUM_segment;
    bool SegmentIsOpened;
    T NUM_system;

public:
    NUM(SEG s, NUM* num_next = NULL, T sys = DECIMAL) {
        NUM_system = sys;
        NUM_current = 0;
        NUM_segment = s;
        NUM_next = num_next;
        SegmentIsOpened = true; // sets all segments to be visible - meaning 00.00 at the beginning - 
                                // set this to false in order for the segments to show up after carry
    }

    // Opens the segment for print function
    // Carrys +1 to the upper numerical order
    void NUM_carry (NUM* num_ptr) {
        num_ptr->SegmentIsOpened = true;
        num_ptr->NUM_current++;

        if (!num_ptr->NUM_next) {
            if (num_ptr->NUM_current == 6) overflow = true;
            return;
        }

        // Call carry() recurently on the next num.
        // DECIMAL system for 0 - 10, TIME system for 0 - 6 carries
        switch ( NUM_system ) {
            case ( DECIMAL):
                if (num_ptr->NUM_current == 10) {
                    num_ptr->NUM_current = 0;
                    num_ptr->NUM_carry(num_ptr->NUM_next);
                }
                break;
            case ( TIME ):
                if (num_ptr->NUM_current == 6) {
                    num_ptr->NUM_current = 0;
                    num_ptr->NUM_carry(num_ptr->NUM_next);
                }
                break;
        }
    }

    // Print numbers from 0 to 9
    // When reached 10, set to 0 and carry 1
    void NUM_base_count() {

        if (NUM_current == 10) {
            NUM_next->SegmentIsOpened =  true;
            NUM_carry(NUM_next);
            NUM_current = 0;
        }
        
        cs = 1;
        spi.write(ARR[NUM_current]);
        spi.write(NUM_segment);
        cs = 0;
        
        if (ticker) {   
            NUM_current++;
            ticker = 0;
        }
    }
    
    void NUM_print() {
        if (SegmentIsOpened) {
            cs = 1;
            spi.write(ARR[NUM_current]);
            spi.write(NUM_segment);
            cs = 0;
        }
    }
};


void tick () {
    if (run) ticker = 1;
}

void flip() {
    if ( restart ) { led = 0; return; }
    run? led = !led : led = 1;
}

/*
*   /Starts the timer when button is pressed
*/
void pressed() {
    run = !run;
    restart = false;
    t.start();
}

/*
*   /Stops the timer, checks if the button
*   was pressed longer then 2 secs - if so, reset
*/
void released() {
    t.stop();
    if ( t.read() >= 2) reset = 1;
    t.reset();
}


// the function is called while RX interrupt
void callback()
{
    // Note: you need to actually read from the serial to clear the RX interrupt
    rxZnak=uart.getc(); // get the char from serial
    
    switch ( rxZnak ) {
    	case '1': 
    		uart.printf("Received 1 \n");
    		run = !run;
    		restart = false;
    		break;
    	case '0':
    		uart.printf("Received 0 \n");
    		run ? uart.printf("I am running.. stop before reset\n") : reset = 1;
    		break;
	}
}

int main()
{
      	
	uart.baud(UART_SPEED);

	uart.attach(&callback);//preruseni na RX
	
    cs = 1;
    spi.format(8,1);
    spi.frequency(1000000);
    cs = 0;
    
    preruseni.fall(&pressed);
    preruseni.rise(&released);
    flipper.attach(&flip, 0.1);
	counter.attach(&tick, 0.01);
    
    while (1) {
    	
    	uart.printf("Stopwatch ready\n");
 
		restart = true;
		
        led = 0;
        run = 0;
        
        NUM a(seg4, NULL, TIME), b(seg3, &a), c(seg2, &b, TIME), d(seg1, &c);
        
        while (! (overflow | reset) ){
            
            
            a.NUM_print();
            b.NUM_print();
            c.NUM_print();
            d.NUM_base_count(); 
            
            if(overflow | reset) {
                overflow = false;
                reset  = false;
                break;
                }
        }
    } // infinit while
    
    return 0;
} //main
