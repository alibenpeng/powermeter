/*

Smartmeter:

Poll Input pins for state change and transmit a packet containing the pin ID if one is detected
See if this works better than the interrupt-centered solution

Never use any kind of delay function in this code!!!

*/

#include <RF12.h>
#include <Ports.h>
#include <avr/wdt.h>   
#include <WProgram.h>

//#define DEBUG_ENABLED

#define LED1 8
#define LED2 9

#define NODE_ID 5
#define NETGROUP 42

#define COUNTER_ID 0x0001
#define NUM_COUNTERS 5

#define COUNTER1 PIND4
#define COUNTER2 PIND3
#define COUNTER3 PIND5
#define COUNTER4 PIND1
#define COUNTER5 PIND0


// this constant won't change:

#ifdef DEBUG_ENABLED
const int  counterPin[NUM_COUNTERS] = {5, 6}; // testpins
#else
const int  counterPin[NUM_COUNTERS] = {
  COUNTER1,
  COUNTER2,
  COUNTER3,
  COUNTER4,
  COUNTER5,
};    // the pins that the counters are connected to
#endif


struct {
	uint32_t counter_millis;
	uint32_t active_counter;
} payload;


MilliTimer led1Timer;


// Variables will change:
int counterState[NUM_COUNTERS];         // current state of the button
int lastCounterState[NUM_COUNTERS];     // previous state of the button
uint32_t counterMillis[NUM_COUNTERS];
uint32_t lastCounterMillis[NUM_COUNTERS];

void setup() {

  for (int i = 0; i < NUM_COUNTERS; i++) {
    counterState[i] = 0;
    lastCounterState[i] = 0;
    lastCounterMillis[i] = 0;
    // initialize the button pin as a input:
    pinMode(counterPin[i], INPUT);
    digitalWrite(counterPin[i], HIGH);
  }
  // initialize the LED as an output:
  pinMode(LED1, OUTPUT);

	rf12_initialize(NODE_ID, RF12_868MHZ, NETGROUP);
	rf12_encrypt(RF12_EEPROM_EKEY);
 

#ifdef DEBUG_ENABLED
  // initialize serial communication:
  Serial.begin(57600);
  Serial.println("Warum geht das nicht?");
  wdt_enable(WDTO_1S);
#else
  wdt_enable(WDTO_60MS); // sending a packet of 64bit length takes about ??ms
#endif
}


void loop() {
  for (int i = 0; i < NUM_COUNTERS; i++) { // loop through the counters
    // read the counter input pin:
    counterState[i] = digitalRead(counterPin[i]);

    // compare the counterState to its previous state
    if (counterState[i] != lastCounterState[i]) {
      if (counterState[i] == LOW) {
        digitalWrite(LED1, HIGH);
        // if the current state is HIGH then the button
        // wend from off to on:
#ifdef DEBUG_ENABLED
        Serial.print(i);
        Serial.println(" LOW");
#endif

				payload.active_counter = i;
				counterMillis[i] = millis();
				payload.counter_millis = counterMillis[i];
 
        if (counterMillis[i] - lastCounterMillis[i] > 90) {
				// send a packet
				while (!rf12_canSend()) 
					rf12_recvDone();

				// send as broadcast, payload will be encrypted
				rf12_sendStart(0, &payload, sizeof(payload));
				rf12_sendWait(1);
        }
        lastCounterMillis[i] = counterMillis[i];
#ifdef DEBUG_ENABLED
      } else {
        // transision from LOW to HIGH means the pulse is over
        // we're not interested in that
        Serial.print(i);
        Serial.println(" HIGH"); 
#endif
      }
      Serial.println();
    }
    // save the current state as the last state, 
    //for next time through the loop
    lastCounterState[i] = counterState[i];

    if (led1Timer.poll(90)) {
      digitalWrite(LED1, LOW);
    }
  }
  wdt_reset();
  
}










// vim: expandtab sw=2 ts=2
