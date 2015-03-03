/***************************************************
  Original Trellis Game of Life Code:
  Written by Tony Sherwood for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

  Changes to include Midi Sequencer:
  Written by Karl Sander for Agile Hardware.
 ****************************************************/

#include <Wire.h>
#include "Adafruit_Trellis.h"
#include <SoftwareSerial.h>

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();

Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);

SoftwareSerial VS1053_MIDI(0, 2);

#define NUM_TRELLIS     4
#define NUM_COLUMNS     8
#define NUM_KEYS        (NUM_TRELLIS * 16)

#define POTI_MODE       0
#define POTI_DELAY      1
#define POTI_PITCH_BASE  2

#define MODE_SETUP_GAME 0
#define MODE_RUN_GAME   1
#define MODE_DRUM_SEQ   2

// Connect the INT wire from Trellis to pin #5 or change this constant
#define INTPIN          5

// Pins for Music Maker Shield

#define VS1053_RX  2 // This is the pin that connects to the RX pin on VS1053
#define VS1053_RESET 9 // This is the pin that connects to the RESET pin on VS1053

// MIDI Data

// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 31
#define VS1053_BANK_DEFAULT 0x00
#define VS1053_BANK_DRUMS1 0x78
#define VS1053_BANK_DRUMS2 0x7F
#define VS1053_BANK_MELODY 0x79

// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 32 for more!
#define VS1053_GM1_OCARINA 80

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0


/*
This is the arrangment of Trellis matrices that this program is configured for.
If your setup is different, you'll have to change the numbers in the chessboard array accordingly.
Hint: you can figure out your arrangment by adding a serial print function to the checkButtons function and pressing the buttons.

[ARDUINO]--[0x70]--[0x71]
                     |
           [0x72]--[0x73]

*/

int chessboard[8][8] = {
  {32, 33, 34, 35, 16, 17, 18, 19},
  {36, 37, 38, 39, 20, 21, 22, 23},
  {40, 41, 42, 43, 24, 25, 26, 27},
  {44, 45, 46, 47, 28, 29, 30, 31},
  { 0,  1,  2,  3, 48, 49, 50, 51},
  { 4,  5,  6,  7, 52, 53, 54, 55},
  { 8,  9, 10, 11, 56, 57, 58, 59},
  {12, 13, 14, 15, 60, 61, 62, 63}
};

bool buttonState[NUM_KEYS];
bool nextFrame[NUM_KEYS];

// This variable holds the time in milliseconds since boot when the next step should be run
long nextStepMillis = 0;

// if running a sequencer type program, this variable holds the current step
int step = 0;


// This function turns all LEDs on the board off
void clearBoard() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();
}

// Turn all LEDs on one by one, row by row.
// If this looks strange, adjust the chessboard matrix.
void runBootCheck() {
  for (uint8_t i=0; i<8; i++) {
    for (uint8_t j=0; j<8; j++) {
      trellis.setLED(chessboard[i][j]);
      trellis.writeDisplay();
      delay(10);
    }
  }

  clearBoard();
}

void setup() {
  // 31250 is the baud rate for the classic serial midi protocol
  // 115200 is the default for virtual serial midi devices on computers
  Serial.begin(115200);
  VS1053_MIDI.begin(31250);
  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);

  trellis.begin(0x72, 0x71, 0x70, 0x73);

  runBootCheck();

  pinMode(VS1053_RESET, OUTPUT);
  digitalWrite(VS1053_RESET, LOW);
  delay(10);
  digitalWrite(VS1053_RESET, HIGH);
  delay(10);

  midiSetChannelBank(0, VS1053_BANK_MELODY);
  midiSetInstrument(0, VS1053_GM1_OCARINA);
  midiSetChannelVolume(0, 127);
}

void toggleLED(int placeVal) {
 if (trellis.isLED(placeVal))
    trellis.clrLED(placeVal);
  else
    trellis.setLED(placeVal);
}

/* Directions are ordered clockwise starting with 0 = NORTH */
int getNeighbor(int placeVal, int neighbor) {
  int px = 0;
  int py = 0;
  int x = 0;
  int y = 0;

  getPosition(placeVal, &px, &py);
  switch (neighbor) {
    case 0:
      x = px;
      y = py - 1;
      break;
    case 1:
      x = px + 1;
      y = py - 1;
      break;
    case 2:
      x = px + 1;
      y = py;
      break;
    case 3:
      x = px + 1;
      y = py + 1;
      break;
    case 4:
      x = px;
      y = py + 1;
      break;
    case 5:
      x = px - 1;
      y = py + 1;
      break;
    case 6:
      x = px - 1;
      y = py;
      break;
    case 7:
      x = px - 1;
      y = py - 1;
      break;
    default:
      x = 0;
      y = 0;
  }
  if (x < 0) x = 7;
  if (x > 7) x = 0;
  if (y < 0) y = 7;
  if (y > 7) y = 0;

  return chessboard[x][y];
}

int getPosition(int pv, int *tx, int *ty) {
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      if (chessboard[i][j] == pv) {
        *tx = i;
        *ty = j;
        return 1;
      }
    }
  }
  return -1;
}

void liveOrDie(int placeVal) {
  // Calculate whether to live or die the next round
  int neighbors = 0;
  for (int d=0; d<=7; d++) {
    if (trellis.isLED(getNeighbor(placeVal, d))) {
      neighbors++;
    }
  }

  if (neighbors == 3 && !trellis.isLED(placeVal)) {
    buttonState[placeVal] = 1;
  } else if ((neighbors == 2 || neighbors == 3) && trellis.isLED(placeVal)) {
    buttonState[placeVal] = 1;
  } else {
    buttonState[placeVal] = 0;
  }
}

int getMode() {
  int state = analogRead(POTI_MODE);
  if(state > 768) {
    return 3;
  } else if(state > 512) {
    return MODE_DRUM_SEQ;
  } else if(state > 256) {
    return MODE_RUN_GAME;
  } else {
    return MODE_SETUP_GAME;
  }
}

int getDelay() {
  return (1024 - analogRead(POTI_DELAY));
}
int getPitchBase() {
  return 12 * map(analogRead(POTI_PITCH_BASE), 0, 1023, 3, 8);
}


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
      // if it was pressed, toggle the LED in that position
      if (trellis.justPressed(i)) {
        buttonState[i] = !buttonState[i];
      }
    }
  }
}

// runs the Game of Life
void runGame() {
  if(isNextStep()) {
    // Clear out the next frame
    for(int c=0; c<64; c++) {
      buttonState[c] = 0;
    }
    //compute the next step
    for (uint8_t i=0; i<NUM_KEYS; i++) {
      liveOrDie(i);
    }
  }
}

// This runs when the user can set up the layout for Game of Life
void setupGame() {
  checkButtons();
}

// writes the next frame to the LED Matrix
void writeFrame() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    // this displays the addition of the buttonState und nextFrame arrays
    if(buttonState[i] || nextFrame[i]) {
      trellis.setLED(i);
    } else {
      trellis.clrLED(i);
    }
  }
  trellis.writeDisplay();
}

// The Note input for MIDI functions covers one octave from 0 = C to 7 = C

// Send a MIDI signal to start the specified note
void startNote(int note) {
  Serial.write(0x90);
  switch (note) {
    case 0: Serial.write(getPitchBase() + 12); break;
    case 1: Serial.write(getPitchBase() + 11); break;
    case 2: Serial.write(getPitchBase() + 9); break;
    case 3: Serial.write(getPitchBase() + 7); break;
    case 4: Serial.write(getPitchBase() + 5); break;
    case 5: Serial.write(getPitchBase() + 4); break;
    case 6: Serial.write(getPitchBase() + 2); break;
    case 7: Serial.write(getPitchBase()); break;
  }
  Serial.write(127);
}

// send a midi signal to end the specified note
void stopNote(int note) {
  Serial.write(0x80);
  Serial.write(note);
  Serial.write(127);
}

// play the notes for the specified column in the matrix
void playColumn(int column) {
  for (uint8_t i=0; i<NUM_COLUMNS; i++) {
    if(buttonState[chessboard[i][column]]) {
      startNote(i);
    }
  }
}

void stopAllNotes() {
  for (uint8_t i=24; i<108; i++) {
    stopNote(i);
  }
}


void moveMarker(int toColumn) {
  int fromColumn;

  if(toColumn > 0) {
    fromColumn = toColumn - 1;
  } else {
    fromColumn = NUM_COLUMNS - 1;
  }

  for (uint8_t i=0; i<NUM_COLUMNS; i++) {
    nextFrame[chessboard[i][fromColumn]] = 0;
    nextFrame[chessboard[i][toColumn]] = 1;
  }
}

void runDrumSeq() {
  if(isNextStep()) {
    if(step < 15) {
      step++;
    } else {
      step = 0;
    }
    // alternatively play and stop notes
    if(step % 2 == 0) {
      moveMarker(step / 2);
      playColumn(step / 2);
    }
    else {
      stopAllNotes();
    }
  }
  checkButtons();
}

void loop() {
  switch (getMode()) {
    case MODE_SETUP_GAME:   setupGame();   break;
    case MODE_RUN_GAME:     runGame();     break;
    case MODE_DRUM_SEQ:     runDrumSeq();  break;
  }

  writeFrame();
  delay(10); // just to be on the safe side, you can experiment with lowering this
}

void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;

  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);
  VS1053_MIDI.write(inst);
}


void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;

  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}

void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;

  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((uint8_t)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}

void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;

  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}

void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;

  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}
