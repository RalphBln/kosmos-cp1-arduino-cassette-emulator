/*
  Arduino Kosmos CP1 Cassette Module Emulator.
  Allows saving and loading Kosmos CP1 memory contents
  Tested with Arduino Nano ATmega328.
  
  Ralph Schäfermeier <ralph.schafermeier@gmail.com>
  https://github.com
 */

#include <EEPROM.h>

const int ledPin = 13;
const int cp1Pin = 2;

void setup() {                
  pinMode(ledPin, OUTPUT);
  pinMode(cp1Pin, INPUT);
  Serial.begin(9600);
  Serial.println("Starting.");
  Serial.print("Current value on the line: ");
  Serial.println(digitalRead(cp1Pin));
}

unsigned long lastRisingEdgeTimestamp  = 0;
unsigned long lastFallingEdgeTimestamp = 0;
int lastLevel;

int bitCount = 0;

enum receivingState { idle, receivingControlSignal, inTransmission };

receivingState state = idle;

void loop() {

  int level = digitalRead(cp1Pin);  
  digitalWrite(ledPin, level);
  
  unsigned long timestamp = millis();

  if (level != lastLevel) {

//    Serial.print("State: ");
//    Serial.println(state);
    
    if (level == HIGH) {
      
 //     Serial.println("LOW -> HIGH");

      switch (state) {
        
        case idle:
          // first rising edge, this must be the beginning of the control signal
          Serial.println("Receiving control signal. About to start transmission.");
          state = receivingControlSignal;
          break;

        case receivingControlSignal:
          // second rising edge (first rising edge of after the control signal): This is the beginning of the first actual transmission cycle
          Serial.println("Should not happen: End of control signal. Starting to receive.");
          state = inTransmission;
          break;

        case inTransmission: 
          // rising edge - middle
          lastRisingEdgeTimestamp = timestamp;
          break;

        default:
          Serial.println("Oops, this should not have happened (rising)...");
          break;
      }

      
    } else {
      
      // falling edge
//      Serial.println("HIGH -> LOW");

      switch (state) {

        case receivingControlSignal:
          // first falling edge - end of control signal
          // start of first cycle
          Serial.println("End of control signal. Transmission starting...");
          state = inTransmission;
          break;

        case inTransmission:
          unsigned long lowSignalLength = lastRisingEdgeTimestamp - lastFallingEdgeTimestamp;
          unsigned long highSignalLength   = timestamp - lastRisingEdgeTimestamp;
          int value = highSignalLength < lowSignalLength ? 0 : 1;
    
//            Serial.print(highSingnalLength);
//            Serial.print("\t");
//            Serial.print(lowSignalLength);
//            Serial.print("\t");
          Serial.print(value);
          bitCount++;

          if (bitCount % 16 == 0) {
            Serial.println("");
          } else if (bitCount % 8 == 0) {
            Serial.print("\t");
          }
          break;

        default:
          Serial.println("Oops, this should not have happened (falling)...");
          break;
      }

      lastFallingEdgeTimestamp = timestamp;      
    }

  }

  if (state == inTransmission && timestamp - lastFallingEdgeTimestamp > 1000) {
    // no rising edge since more than a second - we assume the transmission is over
    Serial.println("\n");
    Serial.println("Transmittion ended.");
    Serial.print("Bits read: ");
    Serial.println(bitCount);
    state = idle;
    bitCount = 0;
  }
  
  delay(4);
  lastLevel = level;
}
