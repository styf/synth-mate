//#undef min
//#undef max
//#include <Arduino.h>
#include <MIDI.h>  // Add Midi Library
#include <stdlib.h>
#include <stdbool.h>


#define LED         13
#define orangeLED   30
#define greenLED    28

#define NUMBER_OF_SLOTS         16
#define NUMBER_OF_MODCONTROL     7



int period = 300;
unsigned long time_now = 0;
bool state;

bool EDIT=0;
bool RECORD=0;

int notesPressed;
int slots[32];
int nrpnbuffer[4];

uint8_t ccbuffer[5000];


typedef struct {
  uint8_t  pitch;
  uint8_t  velocity;
  uint8_t  channel;
} note_struct;
note_struct notein;

typedef struct {
  uint8_t  value;
  uint8_t  number;
  uint8_t  channel;
  bool     number_change;
  bool     value_change;
}ctl_struct;
ctl_struct ctlin;

typedef struct {
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
}automation_struct;
automation_struct automation;    //[NUMBER_OF_SLOTS];

automation_struct rates[NUMBER_OF_SLOTS];


typedef struct {
  int number;
  int value;
}nrpn_struct;
nrpn_struct nrpnin;

enum
{
  empty,
  modwheel,
  pitchbend_up,
  pitchbend_down,
  velocity,
  aftertouch,
  _random,

} modcontrol;

typedef struct  {
  int source;
  int destination;
  int min;
  int max;
  bool polarity;
}mod_struct;
mod_struct mod[4];


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
    
    
    for (int i = 0; i < NUMBER_OF_SLOTS; ++i)
    {
        rates[i].rate = 100;
    }

}






void loop() {


    if (0){
      ctlin.value_change = 0;
      MIDI.read();
      if (ctlin.value_change){
        static bool jou = 0;
        jou = !jou;
        digitalWrite(greenLED, jou);
        ctlin.value_change = 0;

        }

      if (modcontrol){
        static bool jou = 0;
        jou = !jou;
        digitalWrite(orangeLED, jou);
        modcontrol = empty;
        }
      }
    
      MODCONTROL(EDIT * modcontrol);

      if (EDIT){
        ctlin.value_change = 0;
        MIDI.read();
        }
//
      PLAY(!EDIT);
//
      AUTOMATION(EDIT * ctlin.value_change);
    
}

//_________________FUNCTIONS____________________________________________


void AUTOMATION(int flag){
    if (flag)
    {
      MIDI.read();

        automation.cc = ctlin.number;
        automation.slot = find_slot(automation.cc);
        automation.length = 3;
        if (automation.slot + 1) {
            ccbuffer[automation.slot * 1000] = 0;
        }

      
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

void MODCONTROL(int flag){
    if (flag) {
        static int loc_modcontrol = 0;
        
        if ((modcontrol != pitchbend_up) && (modcontrol != pitchbend_down)) {
                loc_modcontrol = modcontrol;
                mod[modcontrol].destination = 0;
        }
        else{
            if (mod[modcontrol].source > 43) {
                loc_modcontrol = modcontrol;
                mod[modcontrol].destination = 0;
            }
            if (mod[modcontrol].source < -43) {
                loc_modcontrol = modcontrol;
                mod[modcontrol].destination = 0;
            }
        }

        ctlin.value_change = 0;
        MIDI.read();

        while(EDIT * loc_modcontrol * ctlin.value_change){
          MIDI.read();
          find_mod_minmax(loc_modcontrol);
          if(timer(0, 150)){
            static bool onoff = 0;
            onoff = !onoff;
            digitalWrite(orangeLED, EDIT*onoff);
            }
        }
    }
}

void PLAY(int flag){
    if (flag)
    {
        static int slot;
        int rate = 100;

        MIDI.read();


    switch (modcontrol)
    {
        case empty:
            break;
        case modwheel:
            send_modcontrol();
            modcontrol = empty;
            break;
        case pitchbend_up:
            send_modcontrol();
            modcontrol = empty;
            break;
        case pitchbend_down:
            send_modcontrol();
            modcontrol = empty;
            break;
        case velocity:
            send_modcontrol();
            modcontrol = empty;
            break;
        case aftertouch:
            send_modcontrol();
            modcontrol = empty;
            break;
        case _random:
            send_modcontrol();
            modcontrol = empty;
            break;

    }

            MIDI.read();
            slot = 0;
            if (timer(slot, rates[slot].rate) && notesPressed)
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
}


void send_modcontrol(){
    if (mod[modcontrol].destination){
        if (modcontrol == pitchbend_up) {
            if (mod[modcontrol].polarity)
            {
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, 0, 127, mod[modcontrol].min, mod[modcontrol].max), 1);
            }else{
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, 127, 0, mod[modcontrol].min, mod[modcontrol].max), 1);
            }
        }
        if (modcontrol == pitchbend_down) {
            if (mod[modcontrol].polarity)
            {
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, 0, -127, mod[modcontrol].min, mod[modcontrol].max), 1);
            }else{
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, -127, 0, mod[modcontrol].min, mod[modcontrol].max), 1);
            }
        }else{
            if (mod[modcontrol].polarity)
            {
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, 0, 127, mod[modcontrol].min, mod[modcontrol].max), 1);
            }else{
              MIDI.sendControlChange(mod[modcontrol].destination, map(mod[modcontrol].source, 127, 0, mod[modcontrol].min, mod[modcontrol].max), 1);
            }
        }
    }
}

void find_mod_minmax(int modcontrol){
    static int previous_value;
    if (ctlin.number == mod[modcontrol].destination) {
        if (ctlin.value > mod[modcontrol].max) {
            mod[modcontrol].max = ctlin.value;
            mod[modcontrol].polarity = 1;
        }
        if (ctlin.value < mod[modcontrol].min) {
            mod[modcontrol].min = ctlin.value;
            mod[modcontrol].polarity = 0;
        }
        if (ctlin.value > previous_value) {
            mod[modcontrol].polarity = 1;
            previous_value = ctlin.value;
        }
        if (ctlin.value < previous_value) {
            mod[modcontrol].polarity = 0;
            previous_value = ctlin.value;
        }
    }else{
        mod[modcontrol].destination = ctlin.number;
        mod[modcontrol].min = 127;
        mod[modcontrol].max = 0;
        mod[modcontrol].destination = ctlin.number;
        mod[modcontrol].polarity = 1;
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


int find_automation_destination(int cc){
    for (int i = 0; i < sizeof(slots)/sizeof(slots[0]); ++i)
    {
        if (cc == slots[i])
        {
        printf("cc je ve slotu %d\n", i);
        return i;
        break;
        }
    }
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
  //  y = 1.981824 - (-0.744925/-0.0215325)*(1 - e^(+0.0215325*x))
  //time = 1.981824 - (-0.744925/-0.0215325)*(1 - exp(+0.0215325*time));
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
    if (bend >= 0) {
        modcontrol = pitchbend_up;
        mod[modcontrol].source = bend/64;
        if (mod[pitchbend_down].destination) {
            MIDI.sendControlChange(mod[pitchbend_down].destination, map(127 * mod[pitchbend_down].polarity, 0, 127, mod[pitchbend_down].min, mod[pitchbend_down].max), 1);
        }
    }else{
        modcontrol = pitchbend_down;
        mod[modcontrol].source = bend/64 + 1;
        if (mod[pitchbend_up].destination) {
            MIDI.sendControlChange(mod[pitchbend_up].destination, map(127 * mod[pitchbend_up].polarity, 0, 127, mod[pitchbend_up].min, mod[pitchbend_up].max), 1);
        }
    }
}

  

void handleCC(byte channel, byte number, byte value){

static int old_number;
      
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
    case 1:
      modcontrol = modwheel;
      mod[modcontrol].source = value;
      break;
    default:

 // _______________________ POTREBA UPRAVIT, R3 MA KNOBY PORAD SPOJENE
        if (!EDIT){
          if (find_automation_destination(number) + 1) {
              rates[find_automation_destination(number)].rate = value;
              break;
          }else{
          ctlin.channel = channel;
          ctlin.number = number;
          ctlin.value = value;
          ctlin.value_change = 1;
            
              if (ctlin.number != old_number) {
                  ctlin.number_change = 1;
                  old_number = ctlin.number;
              }else{
                  ctlin.number_change = 0;
              } 
            }
        }
        
        if (EDIT){
      ctlin.channel = channel;
      ctlin.number = number;
      ctlin.value = value;
      ctlin.value_change = 1;
        
        if (ctlin.number != old_number) {
            ctlin.number_change = 1;
            old_number = ctlin.number;
        }else{
            ctlin.number_change = 0;
        }
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
