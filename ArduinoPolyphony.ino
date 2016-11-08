#include <avr/pgmspace.h>   // This package is for PROGMEM which stores variables in static memory instead of dynamic memory

// semiNote[] is an array that stores half the period of each note, in microseconds. The array index corresponds to the midi number of the note.

const unsigned long semiNote [] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18180, 17160, 16200, 15288, 14432, 13620, 12856, 12136, 11456, 10808, 10204, 9632, 9088, 8580, 8100, 7644, 7144, 6808, 6432, 6064, 5724, 5404, 5100, 4816, 4544, 4292, 4048, 3824, 3608, 3404, 3216, 3032, 2864, 2704, 2552, 2408, 2272, 2144, 2024, 1912, 1804, 1704, 1608, 1516, 1432, 1352, 1276, 1204, 1136, 1072, 1012, 956, 904, 852, 804, 760, 716, 676, 640, 600, 568, 536, 504, 476, 452, 424, 400, 380, 360, 336, 320, 300, 284, 268, 252, 240, 224, 212, 200, 188, 180, 168, 160, 152, 144, 136, 128, 120};

//--------------------------------------------------------
//--------------------------------------------------------
// CONTROL PANEL
// Modify these to suit your setup
// BUZZER_NUMBER is the number of (passive) buzzers you intend to use
// MAX_NOTES is the maximum number of notes for each buzzer. You do not need the same number of notes for each buzzer: the rest of the empty array will simply be skipped.
// MAX_PROPORTION you can choose how you want to write in the proportion for each note. For example, MAX_PROPORTION = 100 means that setting a note's proportion to 50 will cause the note to play for half the alloted length. Must be an integer between 1 and 255.
// buzzerPin[] lets you specify which digital output pins you have connected your buzzers to.

#define BUZZER_NUMBER 3
#define MAX_NOTES 100
#define MAX_PROPORTION 100
const int buzzerPin [] = {10, 11, 12};

//--------------------------------------------------------
//--------------------------------------------------------


void setup() {
    for (int bdx = 0; bdx < BUZZER_NUMBER; bdx++)
    {
        pinMode(buzzerPin[bdx],OUTPUT);
    }
}

void sing(const uint8_t notes [BUZZER_NUMBER][MAX_NOTES][4] PROGMEM, float tempo){
    // Declare the time runner
    unsigned long currentTime = micros();

    // Declare the per-buzzer specific arrays
    bool buzzerToggle[BUZZER_NUMBER];               // Store the current state of each buzzer (HIGH or LOW)
    bool newNoteToggle[BUZZER_NUMBER];              // Whether a new note needs to be loaded for this buzzer(true = yes)
    bool buzzerDone[BUZZER_NUMBER];                 // Whether a buzzer has finished all the notes on its list
    int buzzerSum = 0;                              // Number of buzzers that are done
    int currentNoteIndex[BUZZER_NUMBER];            // Store the index of the current note for each buzzer as stored in notes[]
    int currentNoteMidi[BUZZER_NUMBER];             // Store the midi value of the current note (this is the index for semiNote[])
    float currentNoteType[BUZZER_NUMBER];           // Store the type of the current note e.g. 1/4 note = 1 beat
    unsigned long currentNoteSemiPeriod[BUZZER_NUMBER];     // Store the semi-period of the note in microseconds
    unsigned long currentNotePeriod[BUZZER_NUMBER];         // Store the period of the note in microseconds
    unsigned long noteStartTime[BUZZER_NUMBER];     // Aabsolute starting time of the current note according to micros()
    unsigned long noteEndLength[BUZZER_NUMBER];     // Relative length of the current note
    unsigned long notePlayLength[BUZZER_NUMBER];    // Relative length that a sound should be made for the current note
    unsigned long noteRunningTime[BUZZER_NUMBER];   // Relative running time for each note

    // Populate in the buzzer specific arrays
    for (int bdx = 0; bdx < BUZZER_NUMBER; bdx++){
        buzzerToggle[bdx] = false;                  // Set buzzer output initially to off
        newNoteToggle[bdx] = true;                  // Schedule the new notes
        buzzerDone[bdx] = false;                    // Set all the buzzers to not done
        currentNoteIndex[bdx] = -1;                 // Set to -1 as this will get incremented on the first iteration of the while loop
    }

    while (true) {
        currentTime = micros();                         // Get the current time

        for (int bdx = 0; bdx < BUZZER_NUMBER; bdx++){  // For each buzzer...

            if (newNoteToggle[bdx] == true){        // If we moved on to a new note...
                currentNoteIndex[bdx]++;            // Increase the note index counter for that buzzer
                newNoteToggle[bdx] = false;         // Reset the toggle
                if (currentNoteIndex[bdx] == MAX_NOTES) {   // If the new index equals MAX_NOTES...
                    buzzerDone[bdx] == true;                // This buzzer is done
                    buzzerSum++;                            // Increment the number of buzzers that are done
                    if (buzzerSum == BUZZER_NUMBER){        // If all the buzzers are done...
                        return;                             // We're finished here.
                    }
                }

                if (currentNoteIndex[bdx] < MAX_NOTES) {
                    currentNoteMidi[bdx] = notes[bdx][currentNoteIndex[bdx]][0];
                    if (notes[bdx][currentNoteIndex[bdx]][3] != 0)
                        currentNoteType[bdx] = float(notes[bdx][currentNoteIndex[bdx]][2])/float(notes[bdx][currentNoteIndex[bdx]][3]);     // Calculate the currentNoteType (Watch out for division by 0)
                    else
                        currentNoteType[bdx] = 0;
                    currentNoteSemiPeriod[bdx] = semiNote[currentNoteMidi[bdx]];
                    currentNotePeriod[bdx] = currentNoteSemiPeriod[bdx]*2;
                    noteEndLength[bdx] = 240.0/tempo*currentNoteType[bdx]*1000000.0;
                    notePlayLength[bdx] = noteEndLength[bdx] * float(float(notes[bdx][currentNoteIndex[bdx]][1])/float(MAX_PROPORTION));
                    noteStartTime[bdx] = micros();
                    noteRunningTime[bdx] = 0;
                } else {
                    currentNoteMidi[bdx] = 0;
                }
                
            } else {        // Otherwise we're still on the same note...
                noteRunningTime[bdx] = currentTime - noteStartTime[bdx];    // Update the running time for this note
            }

            if (noteRunningTime[bdx] < noteEndLength[bdx]){             // If the note is not over...
                if (currentNoteMidi[bdx] != 0 && noteRunningTime[bdx] < notePlayLength[bdx]){       // If the note is not a rest and is scheduled to make a sound...

                    // Then create a square wave with a 50% duty cycle with the correct period
                    if ((currentTime % currentNotePeriod[bdx]) < currentNoteSemiPeriod[bdx]) {
                        if (buzzerToggle[bdx] == false){
                            digitalWrite(buzzerPin[bdx], HIGH);
                            buzzerToggle[bdx] = true;
                        }
                    } else {
                        if (buzzerToggle[bdx] == true){
                            digitalWrite(buzzerPin[bdx], LOW);
                            buzzerToggle[bdx] = false;
                        }
                    }
                }      
            } else {                                                    // Otherwise the note is over...
                newNoteToggle[bdx] = true;                              // Schedule the new note
            }
        }
    }
}

void loop() {
    const uint8_t music [BUZZER_NUMBER][MAX_NOTES][4] PROGMEM = {
        // BUZZER 1
        {
            // bars 1 - 3
            0, 0, 7, 4,         // This is a rest since midi = 0
            62, 90, 1, 12,      // Here, midi = 62 means that it is the D above middle-C. The note type is 1 / 12 also known as a triplet-quaver. Proportion = 90 means that the note will play for 90% of the note length. Decreasing the proportion make the note more staccato.
            62, 90, 1, 12,
            62, 90, 1, 12,
            65, 90, 1, 2,
            0, 0, 1, 2,
            // bars 4 - 7
            0, 0, 3, 4,
            62, 90, 1, 12,
            62, 90, 1, 12,
            62, 90, 1, 12,
            65, 90, 1, 4,
            65, 90, 1, 12,
            65, 90, 1, 12,
            65, 90, 1, 12,
            69, 90, 1, 4,
            69, 90, 1, 12,
            69, 90, 1, 12,
            69, 90, 1, 12,
            74, 90, 1, 4,
            74, 90, 1, 12,
            74, 90, 1, 12,
            74, 90, 1, 12,
            77, 90, 1, 4,
            79, 90, 1, 4,
            81, 90, 1, 2,
            0, 0, 2, 4,
            // bars 8 - 10
            0, 0, 3, 4,
            77, 90, 1, 12,
            77, 90, 1, 12,
            77, 90, 1, 12,
            74, 90, 1, 4,
            74, 90, 1, 12,
            74, 90, 1, 12,
            74, 90, 1, 12,
            72, 90, 1, 4,
            72, 90, 1, 12,
            72, 90, 1, 12,
            72, 90, 1, 12,
            69, 90, 1, 4,
            69, 90, 1, 12,
            69, 90, 1, 12,
            69, 90, 1, 12,
            67, 90, 1, 4,
            64, 90, 1, 4
        },
        // BUZZER 2
        {
            // bars 1 - 6
            50, 50, 1, 2,
            45, 50, 1, 2,
            50, 50, 1, 2,
            45, 50, 1, 2,
            50, 50, 1, 2,
            45, 50, 1, 2,
            50, 50, 1, 2,
            45, 50, 1, 2,
            50, 50, 1, 2,
            45, 50, 1, 2,
            50, 50, 1, 2,
            45, 50, 1, 2,
            // bars 7 - 10
            53, 50, 1, 2,
            48, 50, 1, 2,
            53, 50, 1, 2,
            48, 50, 1, 2,
            53, 50, 1, 2,
            48, 50, 1, 2,
            53, 50, 1, 2,
            48, 50, 1, 2
        },
        // BUZZER 3
        {
            // bars 1 - 10
            0, 0, 10, 1,
        }
    };
    
    // syntax: sing(array of notes, tempo);
    sing(music, 105.0);
}
