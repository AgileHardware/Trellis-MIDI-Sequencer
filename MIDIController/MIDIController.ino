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
#include <MIDI.h>

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();

Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiUSB);

#define NUM_TRELLIS     4
#define NUM_COLUMNS     8
#define NUM_KEYS        (NUM_TRELLIS * 16)

#define POTI_PITCH_BASE 2

// Connect the INT wire from Trellis to pin #5 or change this constant
#define INTPIN          5


#define MIDI_NOTE_ON        0x90
#define MIDI_NOTE_OFF       0x80
#define MIDI_CHAN_MSG       0xB0
#define MIDI_CHAN_BANK      0x00
#define MIDI_CHAN_VOLUME    0x07
#define MIDI_CHAN_PROGRAM   0xC0
#define MIDI_START          0xfa
#define MIDI_STOP           0xfc
#define MIDI_CLOCK          0xf8
#define MIDI_CONTINUE       0xfb

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
bool pattern[NUM_KEYS];
// holds the playhead
bool nextFrame[NUM_KEYS];
// holds all notes that are currently on
bool currentlyPlaying[128];
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

// functions to get user settings from the potentionmeters

int getPitchBase() {
  return 12 * map(analogRead(POTI_PITCH_BASE), 0, 1023, 3, 8);
}

void checkButtons() {
  if (trellis.readSwitches()) {
    // go through every button
    for (uint8_t i=0; i<NUM_KEYS; i++) {
      if (trellis.justPressed(i)) {
        // toggle the pressed position in the pattern
        pattern[i] = !pattern[i];
      }
    }
  }
}

// writes the next frame to the LED Matrix
void writeFrame() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    // this displays the sum of the pattern and nextFrame arrays
    if(pattern[i] || nextFrame[i]) {
      trellis.setLED(i);
    } else {
      trellis.clrLED(i);
    }
  }
  trellis.writeDisplay();
}

// The note input for MIDI functions covers one octave from 0 = C to 7 = C
uint8_t getNote(uint8_t note) {
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

// play the notes for the specified column in the matrix
void playColumn(uint8_t column) {
  for (uint8_t i=0; i<NUM_COLUMNS; i++) {
    if(pattern[chessboard[i][column]]) {
      midiUSB.sendNoteOn(getNote(i), 127, 1);
      currentlyPlaying[getNote(i)] = true;
    }
  }
}

void stopNotes() {
  for (uint8_t note = 0; note < 128; note++) {
    if(currentlyPlaying[note]) {
      midiUSB.sendNoteOff(note, 127, 1);
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


void playNoteStep() {
  step = (step + 1) % 16;
  // alternatively play and stop the previous column
  if(step % 2 == 0) {
    movePlayhead(step / 2);
    playColumn(step / 2);
  } else {
    stopNotes();
  }
}

uint8_t pulse = 0;

void handleClock() {
  if(pulse == 0) {
    playNoteStep();
  }
  pulse = (pulse + 1) % 3;
}

// arduino default functions

void setup() {
  midiUSB.begin(MIDI_CHANNEL_OMNI);
  midiUSB.turnThruOff();
  midiUSB.setHandleClock(handleClock);
  setupTrellis();

  delay(10);
  runBootCheck();
}

void loop() {
  checkButtons();
  writeFrame();
  midiUSB.read();
  writeFrame();
}