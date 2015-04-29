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

#define VS1053_RESET    9 // This is the pin that connects to the RESET

// Constants for midi signals accepted by the vs1053. See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 31
#define VS1053_BANK_DEFAULT 0x00
#define VS1053_BANK_DRUMS1  0x78
#define VS1053_BANK_DRUMS2  0x7F
#define VS1053_BANK_MELODY  0x79
#define MIDI_NOTE_ON        0x90
#define MIDI_NOTE_OFF       0x80
#define MIDI_CHAN_MSG       0xB0
#define MIDI_CHAN_BANK      0x00
#define MIDI_CHAN_VOLUME    0x07
#define MIDI_CHAN_PROGRAM   0xC0

// Volume from 0 (mute) to 10; Factors to balance instruments from 0 (off) to 12
#define VOLUME              8
#define VOLUME_FACTOR_DRUMS 11
#define VOLUME_FACTOR_BASS  10
#define VOLUME_FACTOR_PAD   9
#define VOLUME_FACTOR_LEAD  10

#define HEADPHONES_ONLY     true

// Instruments see http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 32
const uint8_t lead[8] = {10, 116, 29, 86,  20, 114, 7,   7};
const uint8_t pad[8]  = {42, 114, 19, 53,  20, 77,  83,  83};
const uint8_t bass[8] = {38, 118, 38, 54,  53, 121, 87,  87};
const uint8_t drum[8] = {3,  4,   15, 116, 15, 120, 119, 119};

/*
This is the arrangment of Trellis matrices that this program is configured for.
If your setup is different, you'll have to change the numbers in the chessboard array accordingly.
Hint: you can figure out your arrangment by adding a serial print function to the checkButtons function and pressing all the buttons.

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

// stores the patterns for all 4 channels
bool pattern[4][NUM_KEYS];

// stores the playhead
bool nextFrame[NUM_KEYS];

// This variable holds the time in milliseconds since boot when the next step should be run
long nextStepMillis = 0;

// all notes that are currently on
bool currentlyPlaying[4][128];

// current step in the sequencer
int step = 0;


// This function turns all LEDs on the board off
void clearBoard() {
  for (uint8_t i=0; i<NUM_KEYS; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();
}

// Turn all LEDs on one by one, row by row.
// play one octave of notes
// If this looks strange, adjust the chessboard matrix.
void runBootCheck() {
  for (uint8_t i=0; i<8; i++) {
    for (uint8_t j=0; j<8; j++) {
      trellis.setLED(chessboard[i][j]);
      trellis.writeDisplay();
      midiNoteOn(0, i+60, 64);
      delay(30);
      midiNoteOff(0, i+60, 64);
    }
  }
  clearBoard();
}

void setMidiDefaults() {
  midiSetChannelBank(0, VS1053_BANK_DRUMS1);
  midiSetChannelBank(1, VS1053_BANK_MELODY);
  midiSetChannelBank(2, VS1053_BANK_MELODY);
  midiSetChannelBank(3, VS1053_BANK_MELODY);
  midiSetChannelVolume(0, VOLUME * VOLUME_FACTOR_DRUMS);
  midiSetChannelVolume(1, VOLUME * VOLUME_FACTOR_BASS);
  midiSetChannelVolume(2, VOLUME * VOLUME_FACTOR_PAD);
  midiSetChannelVolume(3, VOLUME * VOLUME_FACTOR_LEAD);
  setSpeakersOn(!HEADPHONES_ONLY);
}

void setupTrellis() {
  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);

  trellis.begin(0x72, 0x71, 0x70, 0x73);
}

void setupVS1053() {
  // 31250 is the baud rate for the classic serial midi protocol
  // 115200 is the default for virtual serial midi devices on computers
  // Serial1 is specific to the Arduino Leonardo, change according to your model
  Serial1.begin(31250);

  pinMode(VS1053_RESET, OUTPUT);
  pinMode(10, OUTPUT);
  digitalWrite(VS1053_RESET, LOW);
  delay(10);
  digitalWrite(VS1053_RESET, HIGH);
  delay(10);
}

void toggleLED(int placeVal) {
 if (trellis.isLED(placeVal))
    trellis.clrLED(placeVal);
  else
    trellis.setLED(placeVal);
}

// functions to get user settings from the potentionmeters

int getChannel() {
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

// The Note input for MIDI functions covers one octave from 0 = C to 7 = C
int getNote(uint8_t note, uint8_t chan) {
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
int getDrumNote(int note) {
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

// play the notes for the specified column in the matrix
void playColumn(int column) {
  for (uint8_t chan=0; chan<4; chan++) {
    for (uint8_t i=0; i<NUM_COLUMNS; i++) {
      if(pattern[chan][chessboard[i][column]]) {
        midiNoteOn(chan, getNote(i, chan), 127);
      }
    }
  }
}

// returns true if last pad notes should be stopped
bool stopPad(uint8_t column) {
  uint8_t next;
  bool isEmpty = true;
  if(column < 7) {
    next = column + 1;
  } else {
    next = 0;
  }
  for (uint8_t i=0; i<NUM_COLUMNS; i++) {
    for (uint8_t j=0; j<NUM_COLUMNS; j++) {
      if(pattern[2][chessboard[i][j]]) {
        isEmpty = false;
        if(j == next) {
          return true;
        }
      }
    }
  }
  return isEmpty;
}

void stopChan(uint8_t chan) {
  for (uint8_t note = 0; note < 128; note++) {
    if(currentlyPlaying[chan][note]) {
      midiNoteOff(chan, note, 127);
    }
  }
}

void stopAll(uint8_t column) {
  stopChan(0);
  stopChan(1);
  stopChan(3);
  if(stopPad(column)) {
    stopChan(2);
  }
}

// move the playhead to the specified column
void movePlayhead(int toColumn) {
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

// update instrument settings changes
void updateInstrument() {
  if(getInstrument() < 3) {
    midiSetChannelBank(0, VS1053_BANK_DRUMS1);
  }
  else {
    midiSetChannelBank(0, VS1053_BANK_MELODY);
  }
  midiSetInstrument(0, drum[getInstrument()]);
  midiSetInstrument(1, bass[getInstrument()]);
  midiSetInstrument(2, pad[getInstrument()]);
  midiSetInstrument(3, lead[getInstrument()]);
}

void playNoteStep() {
  if(isNextStep()) {
    step = (step + 1) % 16;
    // alternatively play and stop the previous column
    if(step % 2 == 0) {
      movePlayhead(step / 2);
      playColumn(step / 2);
    } else {
      stopAll((step - 1) / 2);
    }
  }
}

// functions to send midi signals

void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;

  Serial1.write(MIDI_CHAN_PROGRAM | chan);
  Serial1.write(inst);
}


void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;

  Serial1.write(MIDI_CHAN_MSG | chan);
  Serial1.write(MIDI_CHAN_VOLUME);
  Serial1.write(vol);
}

void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;

  Serial1.write(MIDI_CHAN_MSG | chan);
  Serial1.write((uint8_t)MIDI_CHAN_BANK);
  Serial1.write(bank);
}

void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;

  currentlyPlaying[chan][n] = true;
  Serial1.write(MIDI_NOTE_ON | chan);
  Serial1.write(n);
  Serial1.write(vel);
}

void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;

  currentlyPlaying[chan][n] = false;
  Serial1.write(MIDI_NOTE_OFF | chan);
  Serial1.write(n);
  Serial1.write(vel);
}

void setSpeakersOn(bool on) {
  if(on) {
    digitalWrite(10, HIGH);
  } else {
    digitalWrite(10, LOW);
  }
}

// arduino default functions

void setup() {
  // uncomment the following for debugging, eg figuring out your chessboard assignment
  // Serial.begin(115200);

  setupVS1053();
  setupTrellis();

  setMidiDefaults();

  delay(10);
  runBootCheck();
}

void loop() {
  updateInstrument();
  playNoteStep();
  checkButtons();
  writeFrame();
}