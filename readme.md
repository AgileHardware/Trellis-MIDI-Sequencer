# Trellis MIDI Sequencer

Arduino sketch for an is a 8 x 8 MIDI sequencer based on 4 Adafruit Trellis Keypads. It can also play Conway's Game of Life and use that to generate sweet melodies. In addition to the 4 Trellis devices the project uses 4 potentiometers for control.

This project currently outputs raw MIDI Signals over the virtual USB Serial Port of an Arduino. To work with this MIDI signal in your DAW of choice you can use [the hairless MIDI to Serial Bridge](http://projectgus.github.io/hairless-midiserial/).

## Potentiometer Functions

In the following order from left to right:

1. Mode switch: Game of Board Life Setup -> Running Game of Life -> MIDI Sequencer
2. Speed adjustment for both Game of Life and MIDI
3. Octave for MIDI Output

## Note on MIDI output

The Sequencer includes 8 notes from C to the following C. It can be adjusted from C2 - C3 to C6 - C7