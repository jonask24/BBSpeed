#define MCU attiny84
#define F_CPU 8000000UL;

//Libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <millis.h>
#include <SoftwareSerial.h>
#include <util/delay.h>

//Pins
#define PIN_IR1 ADC0
#define PIN_IR2 ADC2
#define PIN_RX PB1
#define PIN_TX PB0

//Constants
const int sensorDistance = 116; //millimeters
const int regularBreakValue1 = 277; //Normal 100-200, Totally blocked 1000.
const int regularBreakValue2 = 220; 
const int reverseBreakValue1 = 230; 
const int reverseBreakValue2 = 2; 
const int calcTime = 120; //microseconds it takes for calculations

//Variables
int mode = 1; //1-RegularChrono, 2-RevDirChrono, 3-RateOfFire

int main (void){
  DDRB |= (1 << PIN_TX);  
  DDRB &= ~(1 << PIN_RX);
  
  ADCSRA = B10000011; //ADC enable, div clock by 8, single conv mode
  
  SoftwareSerial BTSerial(PIN_RX, PIN_TX);
  BTSerial.begin(9600);  // HC-05 default speed in AT command more

  init_millis(8000000UL);
  sei();

  while (1){
    readBT();
    _delay_ms(500);
    switch (mode){
      case 1: regularVelocity();
        break;
      case 2: reverseVelocity();
        break;
      case 3: rof();
        break;
    }  
  }
}

void regularVelocity(){ //mode 1
  BTSerial.println("Regular Velocity");
  _delay_ms(500);
  ADMUX = B11100000; //1.1v IRV, left adjust, ADC0 as input channel
  do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir1pinVal = ADCH;
  }while (ir1pinVal < regularBreakValue1){}
  int time1 = millis(); // 1M per second

  ADMUX = B11100010; //1.1v IRV, left adjust, ADC2 as input channel
  do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir2pinVal = ADCH;
    if ((millis()-time1) > 500){
      BTSerial.println("E1"); 
      readBT();
      return;
    }
  }while (ir2pinVal < regularBreakValue2){}
  int time2 = millis();
  
  int shotTime = ((time2 - time1) - calcTime);
  int velocity = sensorDistance/shotTime;
  BTSerial.println(velocity);
}

void reverseVelocity(){ //mode 2
  BTSerial.println("Reverse Velocity");
  _delay_ms(500);
  ADMUX = B11100010; //1.1v IRV, left adjust, ADC2 as input channel
  do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir2pinVal = ADCH;
  }while (ir2pinVal < reverseBreakValue1){}
  int time1 = millis();

  ADMUX = B11100000; //1.1v IRV, left adjust, ADC0 as input channel
  do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir1pinVal = ADCH;
    if ((millis()-time1) > 500){
      BTSerial.println("E2"); 
      return;
    }
  }while (ir1pinVal < reverseBreakValue2){}
  int time2 = millis();
  
  int shotTime = ((time2 - time1) - calcTime);
  int velocity = sensorDistance/shotTime;
  BTSerial.println(velocity);
}

void rof(){  //mode 3
  BTSerial.println("ROF");
  _delay_ms(500);
  ADMUX = B11100000; //1.1v IRV, left adjust, ADC0 as input channel
  do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir1pinVal = ADCH;
  }while (ir1pinVal < regularBreakValue1){}
  int time1 = millis();
  int i = 1;

  while (i<=3){
    do{
    ADCSRA |= (1 << ADSC);  //start conversion
    int ir1pinVal = ADCH;
    }while (ir1pinVal < regularBreakValue1){}
    i++;
    _delay_ms(20);
  }
  time2 = millis();
  int timeDiff = (time2 - time1)/1000;
  int timePerShot = (timeDiff/i);
  int rof = 1/timePerShot;
  BTSerial.println(rof); 
}

void readBT(){
  while (BTSerial.available()){
    String incomingData = BTSerial.readString();
    char function = incomingData.charAt(0);
    int value = incomingData.substring(1).toInt();

    switch(function){
      case 'F':
        if (value >= 1 && value <= 3){
        mode = value;}
        break;
      case 'S':
        regularBreakValue1 = value;
        BTSerial.println(regularBreakValue1);
        break;
      case 's':
        regularBreakValue2 = value;
        BTSerial.println(regularBreakValue2);
        break;
      case 'R':
        reverseBreakValue1 = value;
        BTSerial.println(reverseBreakValue1);
        break;
      case 'r':
        reverseBreakValue2 = value;
        BTSerial.println(reverseBreakValue2);
        break;
      case 'G':
        BTSerial.print("F");
        BTSerial.println(mode);
        _delay_ms(300);
        BTSerial.print("S");
        BTSerial.println(regularBreakValue1);
        _delay_ms(300);
        BTSerial.print("s");
        BTSerial.println(regularBreakValue2);
        _delay_ms(300);
        BTSerial.print("R");
        BTSerial.println(reverseBreakValue1);
        _delay_ms(300);
        BTSerial.print("r");
        BTSerial.println(reverseBreakValue2);
        break;      
    }
  _delay_ms(500);  
  }
}