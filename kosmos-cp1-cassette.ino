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
  while(!Serial) {
    delay(2);
  }
  Serial.println("Starting.");
  Serial.print("Current value on the line: ");
  Serial.println(digitalRead(cp1Pin));
}

unsigned long lastRisingEdgeTimestamp  = 0;
unsigned long lastFallingEdgeTimestamp = 0;
int lastLevel;

enum programMode { mainMenuMode, saveMode, loadMode };
programMode mode = mainMenuMode;

int bitCount = 0;

byte data[256];

enum receivingState { idle, receivingControlSignal, inTransmission };
receivingState state = idle;

void loop() {
  switch (mode) {
    case mainMenuMode:
      mainMenu();
      break;
    case saveMode:
      save();
      break;
    case loadMode:
      break;
    default: break;
  }
}

void mainMenu() {
  Serial.println("Kosmos CP1 Cassette Emulator");
  Serial.println("----------------------------");
  Serial.println("");
  Serial.println("Please select an option:");
  Serial.println("");
  Serial.println("s: Save");
  Serial.println("l: Load");
  Serial.println("");
  char input;
  while (true) {
    input = Serial.read();
    if (input == 's') {
      Serial.println("\nSave");
      Serial.println("\nPlease press 'CAS' on your CP1.");
      mode = saveMode;
      break;
    } else if (input == 'l') {
      Serial.println("\nLoad");
      Serial.println("\nPlease press 'CAL' on your CP1.");
      mode = loadMode;
      break;
    }
  }
}

void save() {

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

          int byteCount = bitCount/8;
          int bit

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

  lastLevel = level;

  if (state == inTransmission && timestamp - lastFallingEdgeTimestamp > 1000) {
    // no rising edge since more than a second - we assume the transmission is over
    Serial.println("\n");
    Serial.println("Transmission ended.");
    if (bitCount == 2048) {
      // TODO with the memory extension installed, it can be 4096 bits
    } else {
      Serial.println("\nUnexpected amount of data read. Either you have pressed 'STP' on the CP1 or there was a transmission error. Please try again.");
    }
    state = idle;
    bitCount = 0;
    mode = mainMenuMode;
    return;
  }
  
  delay(4);
}
