//Send.ino

#include<SPI.h>
#include<RF24.h>
#include <avr/sleep.h>

const int switchPin = 3;
const int ledPin = 13;
int currentStatus;
int spi_save;
char charBuf[10];
String text;

// ce, csn pins
RF24 radio(9, 10);

void setup(){
  Serial.begin(9600);
  Serial.println("starting RF24 radio...");
  ADCSRA = 0;
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(0x76);
  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.enableDynamicPayloads();
  radio.powerUp();
  digitalWrite(ledPin, HIGH);
  attachInterrupt(1, wakeUpNow, CHANGE);
  //pinMode(ledPin, OUTPUT);
  digitalWrite(switchPin, HIGH);
  spi_save = SPCR;
}

void sleepNow()         // here we put the arduino to sleep
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we 
     * choose the according 
     * sleep mode: SLEEP_MODE_PWR_DOWN
     * 
     */
    Serial.println("sleeping...");
    delay(1000);  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

    sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin 

    /* Now it is time to enable an interrupt. We do it here so an 
     * accidentally pushed interrupt button doesn't interrupt 
     * our running program. if you want to be able to run 
     * interrupt code besides the sleep function, place it in 
     * setup() for example.
     * 
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.   
     * 
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */

    attachInterrupt(1,wakeUpNow, CHANGE); // use interrupt 0 (pin 2) and run function
    radio.powerDown();
    SPCR = 0;
    // power down                                       // wakeUpNow when pin 2 gets LOW 
    digitalWrite(ledPin, LOW);
    sleep_mode();            // here the device is actually put to sleep!!
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP

    sleep_disable();         // first thing after waking from sleep:
                             // disable sleep...
    detachInterrupt(1);      // disables interrupt 0 on pin 2 so the 
                             // wakeUpNow code will not be executed 
                             // during normal running time.
    digitalWrite(ledPin, HIGH);
    SPCR = spi_save;
    radio.powerUp();
    Serial.println("waking up...");

}

void wakeUpNow() {
  Serial.println("awake!");
  if (digitalRead(switchPin) == LOW) {
    currentStatus = 1;
  } else {
    currentStatus = 0;
  }
  //digitalWrite(ledPin, HIGH);
}

void loop(){
  Serial.println("loop start");
  delay(100);
  if (currentStatus > 0) {
    text = "Closed";
    text.toCharArray(charBuf, 10);
    Serial.print("Closed - ");
  } else {
    text = "Open!!";
    text.toCharArray(charBuf, 10);
    Serial.print("Open - ");
  }
  Serial.println(currentStatus);
  radio.write(&charBuf, sizeof(text));
  Serial.println("Preparing to sleep...");
  sleepNow();
  Serial.println("loop end");  
}
