# MIDI Magic

Arduino sketch for an a 8 x 8 MIDI sequencer based on 4 Adafruit Trellis Keypads and the Adafruit Music Maker Shield / VLSI VS1053b.

The project consists of 4 Trellis Keypads that form an 8x8 matrix for an 8 note, 8 step pattern, and 4 Potentiometers to control the various functions. [More information on the hardware setup can be found here (in german).](http://www.agile-hardware.de/midi-magic/)

[Check out this video of the sequencer in action](https://vimeo.com/125694294)

## Channels

The Sequencer has 4 separate channels that play at the same time, some of them have specific behaviours:

Channel 1 - Drums: Not effected by pitch changes. Does not use CDEFGAH Note scale, but specific notes defined in the playDrumNote function
Channel 2 - Bass
Channel 3 - Pads: The notes are not stopped immediately in the following step, but only when another note is encountered or all notes have been removed
Channel 4 - Lead

The Channel to be shown on the Matrix is selected with the first potentiometer.

## Speed

The second potentiometer sets the play speed via a delay. At its max setting there is no delay. At its minimal setting there is a 512 ms delay.

## Pitch

The third potentiometer sets the octave to use / the pitch. This does not affect the Drum channel. It goes from C2 (midi notes 36 - 48) to C6 (midi notes 108 - 120).

## Instruments

Instruments are selected with the fourth potentiometer. All available instruments and their corresponding values can be found on page 32 of [this document](http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf). Instruments are selected in Groups. Default Assignment is as follows:

Index |Name           | Drums             | Bass                  | Pad               | Lead
----- |---------------|-------------------|-----------------------|-------------------|---------------------------
0     |Glockenspiel   |Bank 2: Percussion |38: Slap Bass 2        |42: Viola          |10: Glockenspiel
1     |Drums          |Bank 2: Percussion |118: Melodic Tom       |114: Agogo         |116: Woodblock
2     |Rock           |Bank 2: Percussion |38: Slap Bass 2        |19: Rock Organ     |29: Electric Guitar (muted)
3     |Choir          |116: Woodblock     |54: Voice Oohs         |53: Choir Aahs     |86: Voice Lead
4     |Church         |15: Tubular Bells  |53: Choir Aahs         |20: Church Organ   |20: Church Organ
5     |Glitchy        |120: Reverse Cymbal|121: Guitar Fret Noise |77: Blown Bottle   |114: Agogo
6     |Electronic     |119: Synth Drum    |87: Fifths Lead        |83: Calliope Lead  |7: Harpsichord
7     |Electronic     |119: Synth Drum    |87: Fifths Lead        |83: Calliope Lead  |7: Harpsichord
