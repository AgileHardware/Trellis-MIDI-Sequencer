/***************************************************
  Original Trellis Game of Life Code:
  Written by Tony Sherwood for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

  Changes to include Midi Sequencer:
  Written by Karl Sander
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
#define POTI_INTENSITY  2

#define MODE_SETUP_GAME 0
#define MODE_RUN_GAME   1
#define MODE_DRUM_SEQ   2

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

bool nextFrame[64];

// This variable holds the time in milliseconds since boot when the next step should be run
long nextStepMillis = 0;

// Connect the INT wire from Trellis to pin #5 or change this constant
#define INTPIN 5

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

  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);

  trellis.begin(0x72, 0x71, 0x70, 0x73);

  runBootCheck();
}

void toggle(int placeVal) {
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
    nextFrame[placeVal] = 1;
  } else if ((neighbors == 2 || neighbors == 3) && trellis.isLED(placeVal)) {
    nextFrame[placeVal] = 1;
  } else {
    nextFrame[placeVal] = 0;
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
int getIntensity() {
  return analogRead(POTI_INTENSITY);
}

void checkSwitches() {
  if (trellis.readSwitches()) {
    // go through every button
    for (uint8_t i=0; i<NUM_KEYS; i++) {
      // if it was pressed, toggle the LED in that position
      if (trellis.justPressed(i)) {
        nextFrame[i] = !nextFrame[i];
      }
    }
  }
}

// runs the Game of Life
void runGame() {
  if(nextStepMillis < millis()) {
    nextStepMillis = millis() + getDelay();
    // Clear out the next frame
    for(int c=0; c<64; c++) {
      nextFrame[c] = 0;
    }
    //compute the next step
    for (uint8_t i=0; i<NUM_KEYS; i++) {
      liveOrDie(i);
    }
  }
}

// This runs when the user can set up the layout for Game of Life
void setupGame() {
  checkSwitches();
}

// writes the next frame to the LED Matrix
void writeFrame() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    if(nextFrame[i] == 1) {
      trellis.setLED(i);
    } else {
      trellis.clrLED(i);
    }
  }
  trellis.writeDisplay();
}

// The Note input for MIDI functions covers one octave from 0 = C to 6 = B

// Send a MIDI signal to start the specified note
void startNote(int note) {
  Serial.write(0x90);
  switch (note) {
    case 0: Serial.write(48); break;
    case 1: Serial.write(50); break;
    case 2: Serial.write(52); break;
    case 3: Serial.write(53); break;
    case 4: Serial.write(55); break;
    case 5: Serial.write(57); break;
    case 6: Serial.write(59); break;
  }
  Serial.write(127);
}

// send a midi signal to end the specified note
void stopNote(int note) {
  Serial.write(0x80);
  switch (note) {
    case 0: Serial.write(48); break;
    case 1: Serial.write(50); break;
    case 2: Serial.write(52); break;
    case 3: Serial.write(53); break;
    case 4: Serial.write(55); break;
    case 5: Serial.write(57); break;
    case 6: Serial.write(59); break;
  }
  Serial.write(127);
}

// play the notes for the specified column in the matrix
void playColumn(int column) {
  for (uint8_t i=0; i<8; i++) {
    if(nextFrame[chessboard[i][column]]) {
      startNote(i);
    }
  }
  delay(50);
  for (uint8_t i=0; i<8; i++) {
    if(nextFrame[chessboard[i][column]]) {
      stopNote(i);
    }
  }
}

void flashColumn(int column) {
  for (uint8_t i=0; i<8; i++) {
    trellis.setLED(chessboard[i][column]);
  }
  trellis.writeDisplay();
  delay(100);
  for (uint8_t i=0; i<8; i++) {
    if(nextFrame[chessboard[i][column]] == 0) {
      trellis.clrLED(chessboard[i][column]);
    }
  }
  trellis.writeDisplay();
}

int step = 0;
void runDrumSeq() {
  flashColumn(step);
  playColumn(step);
  checkSwitches();
  if(step < 7) {
    step++;
  } else {
    step = 0;
  }
}

void loop() {
  int mode = getMode();

  switch (mode) {
    case MODE_SETUP_GAME:   setupGame();   break;
    case MODE_RUN_GAME:     runGame();     break;
    case MODE_DRUM_SEQ:     runDrumSeq();  break;
  }

  writeFrame();
  delay(10); // just to be on the safe side, you experiment with lowering this
}

