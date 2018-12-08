/*
 * 
 * Mkhanyisi Gamedze
 * C2S 342 : Embedded Systems
 * Making a mini piano synthesizer using given frequencies, through PWM & ISRs
 * Project 4 : Piano with Capacitive touch, ADC capabilities to control pitch and volume (potentiometer : volume   & joystick : pitch)
 * USART keys used for different modes to play multiple notes
 * 
 * 
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "USART.h"
#include "frequencies.h"
#include "Adafruit_MPR121.h"

//    *********** fields & global variables   ************  
#define F_CPU 16000000 // Clock Speed of CPU
/* Sound wave parameters & timer */
int TIMER1_PRESCALER=8;
int adc= 0;
uint16_t freq; // holds the current frequency value playing on the peizo device
int sustain_pitch=0; // prolong playing sound
int offset;
int val;
char mode = 'k';
int sustain = 0; // 0 means dont sustain, 1 is sustain

//int f;




// ***** Capacitive touch
Adafruit_MPR121 cap = Adafruit_MPR121();


/* For tracking MPR121 state changes (touches and releases) */
uint16_t currTouched = 0;
boolean pressed = false;

// ********************

// comprehensive list of frequencies, to be demoed
int list[50] { 239,253,268,284,301,319,338,358,379,402,426,451,478,506,536,568,602,638,676,716,758,804,851,902,956,1012,1073,1136,1204,1276,
1351,1432,1517,1607,1703,1804,1911,2025,2145,2273,2408,2551,2703,3034,3214,3405,3608,3822};

char s[60];
uint16_t sensedADC;           // most recent ADC measurement

// interrupt handlers ***********   ISR's *******************
ISR (USART_RX_vect){ // play sound
  /*
   * Capture interrupt  input and adjust frequency 
   */
    
  unsigned char receivedChar;
  receivedChar = UDR0;        // Read data from the RX buffer (input character)

  //sprintf( s, "intterupt\n");
  //printString(s);
     
  if(receivedChar== 'r'){ // flag sustain pitch | toggle with each press
      if(sustain_pitch==0){
        sustain_pitch=1;
      }
      else if(sustain_pitch==1){
        sustain_pitch=0;
      }
      else{
        // do nothing
      }
  }
    
  if(receivedChar== 'a'){
    mode = receivedChar;
  }
  else if(receivedChar == 's'){
    mode = receivedChar;
  }
  else if(receivedChar == 'd'){
    mode = receivedChar;
  }
  else if(receivedChar == 'f'){
    mode = receivedChar;
  } 
  else if(receivedChar == 'g'){
    mode = receivedChar;
  }
  else if(receivedChar == 'h'){
    mode = receivedChar;
  }
  else if(receivedChar == 'w'){
    adc=1;
  } 

  else if(receivedChar == 'q'){     
      // demo all notes within 
      for(int i=0;i<45;i++){
          TCCR1B = 0x08; // disable sound playing
          _delay_ms(100); // let 100ms pass
          
          setFreq(uint16_t(list[i])); // set another frequency
          
          TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale
          _delay_ms(100); // play note for 100ms
      } 
  }

  UDR0 = receivedChar;    // Place data into buffer, transfers the data (USART transmit data buffer register)
}

// ADC interrupt vect
ISR( ADC_vect ){

  /*   ****** disable for now *******
  // read ADC value
  sensedADC = ADCL;                 // must read low byte first
  sensedADC |= (ADCH & 0x03) << 8;  // 10-bit resolution => must grab 2 bits from ADCH
  
  
  val= int(sensedADC);

  ***************    */

  // print signal
  //sprintf( s, "interupt -> sensedADC   %d \n", int(sensedADC));
  //printString(s);
}


//   *********   Methods  *************

void setFreq(uint16_t f){ // update frequency field and output compare register    
    //freq = uint16_t(1000000/f);  //  F_CPU =16000000 , Timer_Prescale = 8
    freq = (F_CPU/(f*TIMER1_PRESCALER*2));

    OCR1AH = (freq & 0xFF00) >> 8;
    OCR1AL = (freq & 0x00FF);
}

//      **********   main method   ***************
int main( void ){

  /* Ick. I haven't yet tracked down the root of this problem, but 
     for now the MPR121 won't work without Arduino's built-in init().
     It must be configuring something that the MPR121 uses, but doesn't
     configure itself because it expects Arduino to have taken care of
     it already. Likely candidates are: timers,  */
  init();
  
  //  *************   Configure GPIO   ********************
  DDRB = (1 << PINB1);
  //DDRB = 0b00000010;   // PB[1] output for sound wave on OC1A

  /* Configure GPIO
        - I2C SCLK on pad _____ shound be an [input / output] 
              - not necessary - autoconfiguration
        - I2C SDA on pad _____ shound be an [input / output] not necessary
              - not necessary - autoconfiguration 
        - MPR121's IRQ on pad on pad _PB3_ shound be an output */
  DDRC = 0b00100000;
  PORTD = 0x04;

  //  ************   Sound generation   *****************
  /* Timer/Counter 1 Configuration :  sound wave generation on OC1A */
  TCCR1A = (1 << COM1A0); // (0b01000000) | toggle OC1A on compare match, CTC mode
  //TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale
  uint16_t f=440; // default frequency
  
  /* *********
        ADC Configuration :  collect periodic distance readings on pad A2  
        (ADC potentiometer to control sound volume)
  ***************/  

  //ADMUX  = 0b00000010;  // AReff , internal Vref off | right-adjusted ADC on PC[2] (ADC2) 
  ADMUX  = 0x42;  // right-adjusted ADC on PC[2] w/ reference to 5V (Vcc)
  ADCSRA = 0x77;  // ADC not yet enabled, but set up for automatic conversions and prescale = 128
  //ADCSRB = 0x03;  // when enabled, ADC conversions will be auto-triggered by TIMER0_COMPA events
  ADCSRB = 0x00;          // free running mode | I trigger conversions
  //DIDR0  = 0x3F;  // disable digital input on PC[2] to save power
  

  int count =2; 
  

   // *************
    /* Configure the MPR121 capacitive touch sensor, using the default
     address 0x5A. */
  printString("Adafruit MPR121 Capacitive Touch sensor test\n");
  if ( !cap.begin(0x5A) ) {         // Test that the sensor is up and running
    printString("MPR121 not found, check wiring?\n\n");
    while( true ){
    }
  }
  printString("MPR121 found!\n\n");
  currTouched = cap.touched();      // Get the currently touched pads

  /* 
    
     Configure an interrupt -- without an ISR  -- to detect changes in the 
     MPR121's state. We can't read the sensor from the ISR because ___. But 
     we can be smart about how often we query the sensor by using this flag 
     in the while loop. Just keep in mind that everything in main() will be 
     lower priority than any ISRs! 
     
     *****  To detect when an interrupt has been triggered with an ISR, check the 
     value of its flag bit (e.g. INTFn or PCIFn) in the main control loop *****
     
 */
  EICRA = 0b00000010;

  
  
  // ######    test
  
  // play A4a for 3 sec
  TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale 
  
  
  //setFreq(B5);
  //_delay_ms(3000); // 3 seconds

   
  // play C5 for 1.5 sec
  setFreq(A4);
  _delay_ms(1500);
  
  // play D6 for 3 sec
  //setFreq(D4);
  //_delay_ms(3000);  

  TCCR1B = 0x08;  // CTC mode, CS[2:0] cleared to disable counting until an interrupt is triggered

  // initiate USART 
  initUSART();   

  // enable global interrupts
  SREG |= 0x80;   // sets the global interrupt enable bit
  
  
  // **********  Parameters
  val =512;
  offset=-9999;
  sustain_pitch=0;


  
  printString("Test done:  Is sound playing ?\n Switch modes : a (5th octave) s  (4th octave) d  (3rd octave) f   (2nd octave)   or other key default \n Sustain Pitch   : r      ADC flag   : w  \n\n");

  // **************** Main Loop
  
  while (1){// Wait for interrupt

    //switch note
    //count = count % 12; // modulo 12
    /*
    setFreq(count);
    // unnnecesary
    sprintf( s, "count freq :  %d   count  %d\n", freq, count);
    printString(s);
    */

    // *********           Capacitive touch

    if(EIFR & 0x01){
      // TODO: Outside of an ISR, we have to manually clear the interrupt flag.
      EIFR |= 0b00000001; //write a 1 to the flag to clear it
      // TODO: Read in the new MPR121 state
      currTouched = cap.touched();

      // Print out some helpful information
      printString("0x");
      printHexByte( (currTouched & 0xFF00) >> 8 );
      printHexByte(  currTouched & 0x00FF );
      printString("  *\n");
      
      if( currTouched > 0 ){  // a capacitive touch pad is being touched
        sprintf( s, "->  currTouched  %d    mode :  %c  pitch sustain : %d \n", currTouched , mode, sustain_pitch );
        printString(s);

        // select frequency
        if(mode == 'a'){
            switch (currTouched) { 
               case 1: setFreq(B5); 
                       break; 
               case 2: setFreq(A5); 
                        break; 
               case 4: setFreq(G5); 
                       break; 
               case 16: setFreq(F5); 
                       break; 
               case 32: setFreq(E5); 
                       break; 
               case 64: setFreq(D5); 
                       break; 
               case 128: setFreq(C5); 
                       break; 
               case 256: setFreq(AB5); 
                       break; 
               case 8: setFreq(GA5); 
                       break; 
               case 2048: setFreq(GA5); 
                       break; 
               case 512: setFreq(FG5); 
                       break; 
               case 1024: setFreq(DE5); 
                       break;         
               default: setFreq(CD5); 
                        break;   
            }
       }
       else if (mode == 's'){
          switch (currTouched) { 
               case 1: setFreq(B4); 
                       break; 
               case 2: setFreq(A4); 
                        break; 
               case 4: setFreq(G4); 
                       break; 
               case 16: setFreq(F4); 
                       break; 
               case 32: setFreq(E4); 
                       break; 
               case 64: setFreq(D4); 
                       break; 
               case 128: setFreq(C4); 
                       break; 
               case 256: setFreq(AB4); 
                       break; 
               case 8: setFreq(GA4); 
                       break; 
               case 2048: setFreq(GA4); 
                       break; 
               case 512: setFreq(FG4); 
                       break; 
               case 1024: setFreq(DE4); 
                       break;         
               default: setFreq(CD4); 
                        break;   
            }
       }
       else if (mode == 'd'){
          switch (currTouched) { 
               case 1: setFreq(B3); 
                       break; 
               case 2: setFreq(A3); 
                        break; 
               case 4: setFreq(G3); 
                       break; 
               case 16: setFreq(F3); 
                       break; 
               case 32: setFreq(E3); 
                       break; 
               case 64: setFreq(D3); 
                       break; 
               case 128: setFreq(C3); 
                       break; 
               case 256: setFreq(AB3); 
                       break; 
               case 8: setFreq(GA3); 
                       break; 
               case 2048: setFreq(GA3); 
                       break; 
               case 512: setFreq(FG3); 
                       break; 
               case 1024: setFreq(DE3); 
                       break;         
               default: setFreq(CD3); 
                        break;   
            }
       }
       else if (mode == 'f'){
          switch (currTouched) { 
               case 1: setFreq(B2); 
                       break; 
               case 2: setFreq(A2); 
                        break; 
               case 4: setFreq(G2); 
                       break; 
               case 16: setFreq(F2); 
                       break; 
               case 32: setFreq(E2); 
                       break; 
               case 64: setFreq(D2); 
                       break; 
               case 128: setFreq(C2); 
                       break; 
               case 256: setFreq(AB2); 
                       break; 
               case 8: setFreq(GA2); 
                       break; 
               case 2048: setFreq(GA2); 
                       break; 
               case 512: setFreq(FG2); 
                       break; 
               case 1024: setFreq(DE2); 
                       break;         
               default: setFreq(CD2); 
                        break;   
            }
       }
       else{
            // default 
            switch (currTouched) { 
               case 1: setFreq(A); 
                       break; 
               case 2: setFreq(B); 
                        break; 
               case 4: setFreq(C); 
                       break; 
               case 16: setFreq(E); 
                       break; 
               case 32: setFreq(F); 
                       break; 
               case 64: setFreq(G); 
                       break; 
               case 128: setFreq(H); 
                       break; 
               case 256: setFreq(I); 
                       break; 
               case 8: setFreq(D); 
                       break; 
               case 2048: setFreq(L); 
                       break; 
               case 512: setFreq(J); 
                       break; 
               case 1024: setFreq(K); 
                       break;         
               default: setFreq(f); 
                        break;   
            }
       }
        if(sustain_pitch == 0){
            // sound generation enable 
            TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale
            _delay_ms(100); // play note for 100ms
            TCCR1B = 0x08; // disable sound playing  
        }
        else if(sustain_pitch == 1){
            // prolong sound play
            // sound generation enable 
            TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale
            //_delay_ms(400); // play note for 100ms
            //TCCR1B = 0x08; // disable sound playing  
        }   
      } 
      // no sound played
      else{                     // all capacitive touch pads have been released
        printString("\n");
      }
    } 

      // ************ ADC gets in here

      // ************ ADC 
    //printString("gets here\n"); 
    if(adc == 1){ // only in sustain pitch mode
     // *********   convert adc
    ADCSRA |= 0x88;  // enable ADC conversions and the ADC ISR  | (ISR causes error I think, not needed. Attempt was worth a try)   ******* ADC ISR Enable ******
    
    ADCSRA |= (1 << ADSC);          // manually start conversion   **  ADC Conversion **
    while ( (ADCSRA&(1<<ADIF)) == 0 );    // wait for conversion to finish, do nothing (this triggers ADC  interrupt)
    //printString("gets here\n");  // debug purpose
    // read ADC value
    sensedADC = ADCL;                 // must read low byte first
    sensedADC |= (ADCH & 0x03) << 8;  // 10-bit resolution => must grab 2 bits from ADCH

    
    // **   apply offset
     offset = ((512-int(sensedADC))/2);
     f=abs(freq+offset); // no offset
     //sprintf( s, "freq   %d     offset   %d     f    %d \n", freq, offset, f);
     //printString(s); 

     ADCSRA &= 0x77;  // clear the ADC's enable and interrupt enable bits (no longer necessary)   ********* ADC ISR disable   ***********

     // set frequency  ********  error here when rapidly changing freq, erratic values  *******
     setFreq(f);

    // play sound

    // write to comp registers
    // adjust comp time   
    OCR1AH = (freq & 0xFF00) >> 8;
    OCR1AL = (freq & 0x00FF);
      
     TCCR1B = (1 << WGM12)|(1 << CS10);  // enable sound generation by resuming counting in TC1, 8 prescale 
     sprintf( s, "frequency   %d \n", freq);
     printString(s); 
      /*  // not neessary , can be modified, or just play key again
      // sustain note playing
      if(sustain_pitch == 0){ // do not sustain pitch, just play note for 1s
          _delay_ms(100); // play note for 100ms
      }
      else{
          _delay_ms(5000); // play note for 20s
      }
      */
  
      TCCR1B = 0x08; // disable sound playing
    
    // do something with signal
    //sprintf( s, "sensedADC   %d \n", int(sensedADC));
    //printString(s); 
   
    count+=1; 
    
  }
  }
  return 0;
}
        
