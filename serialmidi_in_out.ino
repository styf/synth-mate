
//#include <Arduino.h>
#include <MIDI.h>  // Add Midi Library

#define LED   13   
#define orangeLED 30   
#define greenLED  28   

int period = 300;
unsigned long time_now = 0;
        bool state;

bool EDIT=0;

int notesPressed;


struct notein {
  uint8_t  pitch;
  uint8_t  velocity;
  uint8_t  channel;
};
notein notein;

struct ctlin {
  uint8_t  value;
  uint8_t  number;
  uint8_t  channel;
};
ctlin ctlin;

struct automation {
  uint8_t  slot;
  uint8_t  cc;
  uint8_t  length;
  uint8_t  min;
  uint8_t  max;
  uint8_t  mode;
  uint8_t  rate;
  uint8_t  offset;
};
automation automation;

struct modcontrol {
  uint8_t  cc;
  uint8_t  min;
  uint8_t  max;
  uint8_t  offset;
};
modcontrol modcontrol;

uint8_t ccbuffer[5000];

MIDI_CREATE_DEFAULT_INSTANCE();



void setup() {
  pinMode (LED, OUTPUT);
  pinMode (greenLED, OUTPUT); 
  pinMode (orangeLED, OUTPUT);
  MIDI.begin(MIDI_CHANNEL_OMNI); // Initialize the Midi Library.
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn); // This is important!! This command
  MIDI.setHandleNoteOff(handleNoteOff); // This command tells the Midi Library 
  MIDI.setHandleControlChange(handleCC);
//  MIDI.setHandleProgramChange(handlePGM);
//  MIDI.setHandlePitchBend(handlePB);

//  Serial.begin(31250);
//  while (!Serial)
//  {}

}

//int PLAY{
//  ;
//  return !EDIT;
//}


void loop() { // Main loop
  
while (!EDIT)
{
    MIDI.read();
    digitalWrite(orangeLED, EDIT*state);
    digitalWrite(greenLED, EDIT*state);
    
    if (timer(0, 100) && notesPressed)
    {
    static int j = 2;
    MIDI.sendControlChange(ccbuffer[0 * 1000], ccbuffer[0 * 1000 + j], 1);  
      if (j < ccbuffer[0 * 1000 + 1])
      {
        j++;
      }else{
        j=2;
      }      
    }
    
    if (timer(1, 100) && notesPressed)
    {
    static int j = 1 * 1000 + 2;
    MIDI.sendControlChange(ccbuffer[1 * 1000], ccbuffer[j], 1);  
      if (j < ccbuffer[1 * 1000 + 1])
      {
        j++;
      }else{
        j=1 * 1000 + 2;
      }      
    }
    
//    if (timer(2, 100) && notesPressed)
//    {
//    static int j = 2 * 1000 + 2;
//    MIDI.sendControlChange(ccbuffer[2 * 1000], ccbuffer[2 * 1000 + j], 1);  
//      if (j < ccbuffer[2 * 1000 + 1])
//      {
//        j++;
//      }else{
//        j=2 * 1000 + 2;
//      }      
//    }
//
//    if (timer(3, 100) && notesPressed)
//    {
//    static int j = 3 * 1000 + 2;
//    MIDI.sendControlChange(ccbuffer[3 * 1000], ccbuffer[3 * 1000 + j], 1);  
//      if (j < ccbuffer[3 * 1000 + 1])
//      {
//        j++;
//      }else{
//        j=3 * 1000 + 2;
//      }      
//    }
}  


if (EDIT)
  {
    MIDI.read();

      automation.cc = ctlin.number;
      automation.slot = find_slot(automation.cc);
      automation.length = automation.slot * 1000 + 2;
    
    while (EDIT)
    {
      MIDI.read();
      if(timer(0, 300)){
        static bool onoff = 0;
        onoff = !onoff;
        digitalWrite(greenLED, EDIT*onoff);
        }
      if(timer(1, 100)){
        ccbuffer[automation.length] = ctlin.value;
        ccbuffer[automation.slot * 1000] = automation.cc;
        ccbuffer[automation.slot * 1000 + 1] = automation.length - automation.slot * 1000;
        automation.length++;

        state = !state;
        digitalWrite(orangeLED, EDIT*state);
        } 
    }
  }




  
}



int play_automation(int i){
    if (timer(i, 100) && notesPressed)
    {
    static int j = 2;
    MIDI.sendControlChange(ccbuffer[i * 1000], ccbuffer[i * 1000 + j], 1);  
      if (j < ccbuffer[i * 1000 + 1])
      {
        j++;
      }else{
        j=2;
      }      
    }
}

int slots[32];

int find_slot(uint8_t cc){
    for (int i = 0; i < sizeof(slots)/sizeof(slots[0]); ++i)
    {
        if (cc == slots[i])
        {
        printf("cc je ve slotu %d\n", i);
        return i;
        break;
        }
    }
    for (int i = 0; i < sizeof(slots)/sizeof(slots[0]); ++i)
    {
        if (slots[i] == 0)
        {
        printf("prazdny slot %d\n", i);
        slots[i] = cc;
        return i;
        break;
        }
    }
    printf("malo mista\n");
    return -1;
}


struct timer_variables {
  unsigned long previousmillis;
  unsigned long currentmillis;
};

timer_variables timer_var[32];


boolean timer(int index, unsigned long time) {
  //static unsigned long previousmillis = 0;
  timer_var[index].currentmillis = millis();
  if (timer_var[index].currentmillis - timer_var[index].previousmillis >= time) {
    timer_var[index].previousmillis = timer_var[index].currentmillis;
    return true;
  }
  return false;
}


boolean __delay(unsigned long time) {
  static unsigned long previousmillis = 0;
  unsigned long currentmillis = millis();
  if (currentmillis - previousmillis >= time) {
    previousmillis = currentmillis;
    return true;
  }
  return false;
}

void handleCC(byte channel, byte number, byte value){
  ctlin.channel = channel;
  ctlin.number = number;
  ctlin.value = value;
}


void handleNoteOn(byte channel, byte pitch, byte velocity) { 
  notein.channel = channel;
  notein.pitch = pitch;
  notein.velocity = velocity;

  notesPressed++;

  
  digitalWrite(LED,HIGH);  //Turn LED on
  if (pitch == 48){
    state = 0;
    EDIT=1;
  }
    
  MIDI.sendNoteOn(pitch,velocity,channel);
  MIDI.sendNoteOn(pitch + 4,velocity,channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity) { 
  notein.channel = channel;
  notein.pitch = pitch;
  notein.velocity = 0;

  notesPressed--;
  if (notesPressed<0)
  {
    notesPressed=0;
  }

    if (pitch == 48)
    EDIT=0;
  
  digitalWrite(LED,LOW);  //Turn LED off
  MIDI.sendNoteOff(pitch,velocity,channel);
  MIDI.sendNoteOff(pitch + 4,velocity,channel);
}
