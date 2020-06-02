
//#include <Arduino.h>
#include <MIDI.h>  // Add Midi Library

#define LED   13   
#define orangeLED 30   
#define greenLED  28   

int period = 300;
unsigned long time_now = 0;
        bool state;

bool EDIT=0;
bool RECORD=0;

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
  int      length;
  uint8_t  length_msb;
  uint8_t  length_lsb;
  uint8_t  min;
  uint8_t  max;
  uint8_t  mode;
  uint8_t  rate;
  uint8_t  offset;
};
automation automation;

struct nrpn {
  int number;
  int value;
};
nrpn nrpnin;


uint8_t ccbuffer[5000];

int nrpnbuffer[4];

enum 
{
  modwheel, 
  pitchbend,  
  velocity,  
  aftertouch, 
  _random,  
  empty,
} modcontrol;

struct MOD {
  int source;
  int destination;
  int min;
  int max;

};
MOD mod[4];



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
  MIDI.setHandlePitchBend(handlePB);
//  MIDI.setHandleProgramChange(handlePGM);

//  Serial.begin(31250);
//  while (!Serial)
//  {}

}

//int PLAY{
//  ;
//  return !EDIT;
//}


void loop() { // Main loop


  while(1){
    MIDI.sendNoteOff(60,64,1);
    delay(2000);
    MIDI.sendNoteOn(60,64,1);
    delay(2000);


    }
  
while (!EDIT)
{
    static int slot;
    int rate =100;

    MIDI.read();


switch (modcontrol)
{
    case modwheel:
      MIDI.sendControlChange(mod[modwheel].destination, map(mod[modwheel].source, 0, 127, mod[modwheel].min, mod[modwheel].max), 1);  
      modcontrol = empty;
      break;
    case pitchbend:
      modcontrol = empty;
      break;
    case velocity:
      modcontrol = empty;
      break;
    case aftertouch:
      modcontrol = empty;
      break;
    case _random:
      modcontrol = empty;
      break;
    case empty:
      break;
}


       MIDI.read();
       slot = 0;
       if (timer(slot, rate) && notesPressed)
       {
       static int eachkey = 0;
       static int j = 3;
         if (eachkey != notesPressed)
         {
           j = 3;
           eachkey = notesPressed;
         }
       int length = (ccbuffer[slot * 1000 + 1] << 8) + ccbuffer[slot * 1000 + 2];
       MIDI.sendControlChange(ccbuffer[slot * 1000], ccbuffer[slot * 1000 + j], 1);  
         if (j < length)
         {
           j++;
         }else{
           j=3;
         }      
       }

       MIDI.read();
       slot = 1;
       if (timer(slot, rate) && notesPressed)
       {
       static int eachkey = 0;
       static int j = 3;
         if (eachkey != notesPressed)
         {
           j = 3;
           eachkey = notesPressed;
         }
       int length = (ccbuffer[slot * 1000 + 1] << 8) + ccbuffer[slot * 1000 + 2];
       MIDI.sendControlChange(ccbuffer[slot * 1000], ccbuffer[slot * 1000 + j], 1);  
         if (j < length)
         {
           j++;
         }else{
           j=3;
         }      
       }

      MIDI.read();
      slot = 2;
      if (timer(slot, rate) && notesPressed)
      {
      static int eachkey = 0;
      static int j = 3;
        if (eachkey != notesPressed)
        {
          j = 3;
          eachkey = notesPressed;
        }
      int length = (ccbuffer[slot * 1000 + 1] << 8) + ccbuffer[slot * 1000 + 2];
      MIDI.sendControlChange(ccbuffer[slot * 1000], ccbuffer[slot * 1000 + j], 1);  
        if (j < length)
        {
          j++;
        }else{
          j=3;
        }      
      }

      MIDI.read();
      slot = 3;
      if (timer(slot, rate) && notesPressed)
      {
      static int eachkey = 0;
      static int j = 3;
        if (eachkey != notesPressed)
        {
          j = 3;
          eachkey = notesPressed;
        }
      int length = (ccbuffer[slot * 1000 + 1] << 8) + ccbuffer[slot * 1000 + 2];
      MIDI.sendControlChange(ccbuffer[slot * 1000], ccbuffer[slot * 1000 + j], 1);  
        if (j < length)
        {
          j++;
        }else{
          j=3;
        }      
      }
}  


if (EDIT)
  {
    MIDI.read();

      automation.cc = ctlin.number;
      automation.slot = find_slot(automation.cc);
      automation.length = 3;
    
    while (RECORD * EDIT)
    {
      MIDI.read();
      if(timer(1, 100)){
        ccbuffer[automation.slot * 1000 + automation.length] = ctlin.value;
        ccbuffer[automation.slot * 1000] = automation.cc;
        ccbuffer[automation.slot * 1000 + 1] = automation.length >> 8;
        ccbuffer[automation.slot * 1000 + 2] = automation.length;
        automation.length++;

        state = !state;
        digitalWrite(orangeLED, EDIT*state);
        } 

      if(timer(0, 300)){
        static bool onoff = 0;
        onoff = !onoff;
        digitalWrite(greenLED, EDIT*onoff);
        }
    }
  }




  
}



void play_automation(int slot, int rate){
    if (timer(slot, rate) && notesPressed)
    {
      static int eachkey = 0;
      static int j = 3;
        if (eachkey != notesPressed)
        {
          j = 3;
          eachkey = notesPressed;
        }
      int length = (ccbuffer[slot * 1000 + 1] << 8) + ccbuffer[slot * 1000 + 2];
      MIDI.sendControlChange(ccbuffer[slot * 1000], ccbuffer[slot * 1000 + j], 1);  
      if (j < length)
      {
        j++;
      }else{
        j=3;
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

void handlePB(byte channel, int bend){
  MIDI.sendPitchBend(channel, bend);
  mod[pitchbend].source = bend;
  modcontrol = pitchbend;
}

  

void handleCC(byte channel, byte number, byte value){

switch (number)
{
    case 99:
      nrpnbuffer[0] = value;
      break;

    case 98:
      nrpnbuffer[1] = value;
      break;
    case 38:
      nrpnbuffer[2] = value;
      break;
    case  6:
      nrpnbuffer[3] = value;
      nrpnin.number = 128*nrpnbuffer[0] + nrpnbuffer[1];
      nrpnin.value  = 128*nrpnbuffer[2] + nrpnbuffer[3];
      break;
    default:
      ctlin.channel = channel;
      ctlin.number = number;
      ctlin.value = value;
      if (ctlin.number == 1)
      {
        mod[modwheel].source = ctlin.value;
        modcontrol = modwheel;
      }
}



if (nrpnin.number == 4)
{
  if (nrpnin.value > 0)
  {
  digitalWrite(greenLED, 1);
  EDIT = 1;
  }else{
  digitalWrite(greenLED, 0);
  EDIT = 0;
  }
}

}

void handleNoteOn(byte channel, byte pitch, byte velocity) { 
  notein.channel = channel;
  notein.pitch = pitch;
  notein.velocity = velocity;

  notesPressed++;

  
  digitalWrite(LED,HIGH);  //Turn LED on
  if (pitch == 48){
    state = 0;
    RECORD=1;
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
    RECORD=0;
  
  digitalWrite(LED,LOW);  //Turn LED off
  MIDI.sendNoteOff(pitch,velocity,channel);
  MIDI.sendNoteOff(pitch + 4,velocity,channel);
}
