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
byte currentByte;

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
      load();
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

  pinMode(cp1Pin, INPUT);

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
    

          int byteCount = bitCount/8;
          int currentBitPlace = bitCount % 8;

          bitWrite(currentByte, currentBitPlace, value);

//          Serial.print("Bit ");
//          Serial.print(currentBitPlace);
//          Serial.print("=");
//          Serial.print(value);
//          Serial.print(", currentByte=");
//          Serial.println(currentByte);

          if (currentBitPlace == 7) {
            data[byteCount] = currentByte;
            currentByte = 0;
//            Serial.print("Stored value: ");
//            Serial.println(data[byteCount]);
//            Serial.print("New byte ");
//            Serial.println(byteCount);
          }

          bitCount++;

//          if (bitCount % 16 == 0) {
//            Serial.println("");
//          } else if (bitCount % 8 == 0) {
//            Serial.print("\t");
//          }
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
      // TODO with the memory extension installed, it can be 4096 bits/512 bytes
//      Serial.println("Bytes received:");
//      for (int i=0; i<256; i++) {
//        Serial.print(data[i]);
//        Serial.print("\t");
//      }
      EEPROM.put(0, data);
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

void load() {

  pinMode(cp1Pin, OUTPUT);

  EEPROM.get(0, data);
  
  Serial.println("\nPlease press 'CAL' on the CP1 within the next 5 seconds.");

  // send control signal for 5 seconds (we don't need 16 seconds)
  digitalWrite(cp1Pin, HIGH);
  delay(16000);

  Serial.println("\nTransmitting data to CP1...\n");

  for (int i=0; i<256; i++) {
    currentByte = data[i];
    for (int j=0; j<8; j++) {
      int value = bitRead(currentByte, j);
      Serial.print(value);
      if (value == 1) {
        // 1 = short low, long high
        digitalWrite(cp1Pin, LOW);
        digitalWrite(ledPin, LOW);
        delay(32);
        digitalWrite(cp1Pin, HIGH);
        digitalWrite(ledPin, HIGH);
        delay(65);
      } else {
        // 0 = long low, short high
        digitalWrite(cp1Pin, LOW);
        digitalWrite(ledPin, LOW);
        delay(65);
        digitalWrite(cp1Pin, HIGH);
        digitalWrite(ledPin, HIGH);
        delay(32);
      }
    }
  }
  digitalWrite(cp1Pin, LOW);
  Serial.println("\nDone.");
  mode = mainMenuMode;
}
