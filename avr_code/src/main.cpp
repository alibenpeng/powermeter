/*
 * Alexanders Stromzaehler - Zaehlt S0-Impulse von bis zu 5 Zaehlern und gibt alle 5 Sekunden
 * gezaehlten Impulse und die Dauer zwischen den letzten 2 Impulsen auf einem Kanal in ms aus
*/

#include <RF12.h>
#include <Ports.h>
#include <avr/interrupt.h>   
#include <util/delay.h>
#include <util/atomic.h>
#include <WProgram.h>

#define LED1 8
#define LED2 9

#define NODE_ID 5
#define NETGROUP 42

#define NUM_COUNTERS 5

char state[NUM_COUNTERS];
char last_state[NUM_COUNTERS];
uint32_t last_time_high[NUM_COUNTERS];

struct {
	char counter[NUM_COUNTERS];
	uint32_t duration[NUM_COUNTERS];
} payload;


MilliTimer sendTimer;


ISR( PCINT2_vect ) {
	state[4] = (PIND & (1 << PIND0));
	state[3] = (PIND & (1 << PIND1));
	state[2] = (PIND & (1 << PIND3));
	state[1] = (PIND & (1 << PIND4));
	state[0] = (PIND & (1 << PIND5));

	for (int i = 0 ; i < NUM_COUNTERS ; i++) {
		if (state[i] == 0) {                          // Input Low = Impuls
			if (last_state[i] == 1) {                    // fallende Flanke = neuer Impuls
				payload.counter[i]++;
				payload.duration[i] = millis() - last_time_high[i];
				last_time_high[i] = millis();
				last_state[i] = 0;
			}
		} else {                                      // Input High = kein Impuls
			last_state[i] = 1;
		} 
	}
}

void setup() {
	pinMode(0, INPUT);
	pinMode(1, INPUT);
	pinMode(3, INPUT);
	pinMode(4, INPUT);
	pinMode(5, INPUT);

	// enable internal pullups
	digitalWrite(0, HIGH);
	digitalWrite(1, HIGH);
	digitalWrite(3, HIGH);
	digitalWrite(4, HIGH);
	digitalWrite(5, HIGH);

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
 
 //make digital pins 0-1 and 3-5 into pin change interrupts  
 PCICR |= (1 << PCIE2);

 PCMSK2 |= (1 << PCINT16);
 PCMSK2 |= (1 << PCINT17);  
 PCMSK2 |= (1 << PCINT19);  
 PCMSK2 |= (1 << PCINT20);  
 PCMSK2 |= (1 << PCINT21);  
 
	rf12_initialize(NODE_ID, RF12_868MHZ, NETGROUP);
	rf12_encrypt(RF12_EEPROM_EKEY);
	rf12_easyInit(0);
}

void loop() {
	if (sendTimer.poll(5000)) {
		cli();
		rf12_easyPoll();
		rf12_easySend(&payload, sizeof(payload));
		for (int i = 0; i < NUM_COUNTERS; i++) {
			payload.counter[i] = 0;
			payload.duration[i] = 0;
		}
		sei();
		digitalWrite(LED2, HIGH);
		_delay_ms(50);
		digitalWrite(LED2, LOW);
	}
}
