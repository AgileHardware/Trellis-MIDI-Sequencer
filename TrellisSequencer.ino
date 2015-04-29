/***************************************************
  Written by Karl Sander for Agile Hardware.

  Originally adapted from Trellis Game of Life Example:
  Written by Tony Sherwood for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

  Functions to send MIDI signals taken from Adafruit VS1053 Library Examples > player_miditest:
  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#include "Adafruit_Trellis.h"

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();

Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);

#define NUM_TRELLIS     4
#define NUM_COLUMNS     8
#define NUM_KEYS        (NUM_TRELLIS * 16)

#define POTI_MODE       0
#define POTI_DELAY      1
#define POTI_PITCH_BASE 2
#define POTI_INSTRUMENT 3

#define CHANNEL_DRUM    0
#define CHANNEL_BASS    1
#define CHANNEL_PAD     2
#define CHANNEL_LEAD    3

// Connect the INT wire from Trellis to pin #5 or change this constant
#define INTPIN          5

/*
This is the arrangment of Trellis matrices that this program is configured for.
If your setup is different, you'll have to change the numbers in the chessboard array accordingly.
Hint: you can figure out your arrangment by adding a serial print function to the checkButtons function and pressing all the buttons.

[ARDUINO]--[0x70]--[0x71]
                     |
           [0x72]--[0x73]

*/

const uint8_t chessboard[8][8] = {
  {32, 33, 34, 35, 16, 17, 18, 19},
  {36, 37, 38, 39, 20, 21, 22, 23},
  {40, 41, 42, 43, 24, 25, 26, 27},
  {44, 45, 46, 47, 28, 29, 30, 31},
  { 0,  1,  2,  3, 48, 49, 50, 51},
  { 4,  5,  6,  7, 52, 53, 54, 55},
  { 8,  9, 10, 11, 56, 57, 58, 59},
  {12, 13, 14, 15, 60, 61, 62, 63}
};

// global variables

// holds the patterns for all 4 channels
bool pattern[4][NUM_KEYS];
// holds the playhead
bool nextFrame[NUM_KEYS];
// holds the time in milliseconds since boot when the next step should be run
long nextStepMillis = 0;
// holds the current step in the sequencer
uint8_t step = 0;


// This function turns all LEDs off
void clearBoard() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();
}

void runBootCheck() {
  // Turn all LEDs on one by one, row by row.
  // play one octave of notes
  // If this looks strange, adjust the chessboard matrix.
  for (uint8_t i=0; i<8; i++) {
    for (uint8_t j=0; j<8; j++) {
      trellis.setLED(chessboard[i][j]);
      trellis.writeDisplay();
      delay(5);
    }
  }
  clearBoard();
}


void setupTrellis() {
  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);

  trellis.begin(0x72, 0x71, 0x70, 0x73);
}


void toggleLED(uint8_t placeVal) {
 if (trellis.isLED(placeVal))
    trellis.clrLED(placeVal);
  else
    trellis.setLED(placeVal);
}

// functions to get user settings from the potentionmeters

uint8_t getChannel() {
  int state = analogRead(POTI_MODE);
  if(state > 768) {
    return CHANNEL_LEAD;
  } else if(state > 512) {
    return CHANNEL_PAD;
  } else if(state > 256) {
    return CHANNEL_BASS;
  } else {
    return CHANNEL_DRUM;
  }
}

int getDelay() {
  return map((1024 - analogRead(POTI_DELAY)), 1024, 0, 512, 0);
}
int getPitchBase() {
  return 12 * map(analogRead(POTI_PITCH_BASE), 0, 1023, 3, 8);
}
int getInstrument() {
  return map(analogRead(POTI_INSTRUMENT), 0, 1023, 0, 7);
}

// check if enough time has passed to run the next step
bool isNextStep() {
  if(nextStepMillis < millis()) {
    nextStepMillis = millis() + getDelay();
    return true;
  } else {
    return false;
  }
}

void checkButtons() {
  if (trellis.readSwitches()) {
    // go through every button
    for (uint8_t i=0; i<NUM_KEYS; i++) {
      if (trellis.justPressed(i)) {
        // toggle the pressed position in the pattern
        pattern[getChannel()][i] = !pattern[getChannel()][i];

        // uncomment the following to figure out chessboard assignment
        // Serial.writeln(i);
      }
    }
  }
}

// writes the next frame to the LED Matrix
void writeFrame() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    // this displays the sum of the pattern and nextFrame arrays
    if(pattern[getChannel()][i] || nextFrame[i]) {
      trellis.setLED(i);
    } else {
      trellis.clrLED(i);
    }
  }
  trellis.writeDisplay();
}

// The note input for MIDI functions covers one octave from 0 = C to 7 = C
uint8_t getNote(uint8_t note, uint8_t chan) {
  if(chan == 0) {
    return getDrumNote(note);
  } else {
    switch (note) {
      case 0: return getPitchBase() + 12;
      case 1: return getPitchBase() + 11;
      case 2: return getPitchBase() + 9;
      case 3: return getPitchBase() + 7;
      case 4: return getPitchBase() + 5;
      case 5: return getPitchBase() + 4;
      case 6: return getPitchBase() + 2;
      case 7: return getPitchBase();
    }
  }
}


// for the drum channel, different note values are appropriate because it is not melodic. It should also not be affected by pitch changes
uint8_t getDrumNote(uint8_t note) {
  if(getInstrument() == 3) {
    switch (note) {
      case 0: return 84;
      case 1: return 80;
      case 2: return 76;
      case 3: return 72;
      case 4: return 68;
      case 5: return 64;
      case 6: return 60;
      case 7: return 56;
    }
  } else {
    switch (note) {
      case 0: return 60;
      case 1: return 59;
      case 2: return 57;
      case 3: return 50;
      case 4: return 48;
      case 5: return 40;
      case 6: return 36;
      case 7: return 38;
    }
  }
}

// move the playhead to the specified column
void movePlayhead(uint8_t toColumn) {
  uint8_t fromColumn;

  if(toColumn > 0) {
    fromColumn = toColumn - 1;
  } else {
    fromColumn = NUM_COLUMNS - 1;
  }

  for (uint8_t i=0; i<8; i++) {
    nextFrame[chessboard[i][fromColumn]] = false;
    nextFrame[chessboard[i][toColumn]] = true;
  }
}

// arduino default functions

void setup() {
  // uncomment the following for debugging, eg figuring out your chessboard assignment
  // Serial.begin(115200);

  setupTrellis();

  delay(10);
  runBootCheck();
}

void loop() {
  checkButtons();
  writeFrame();
  delay(10);
}