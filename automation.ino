// RECORD KNOB


#include <Arduino.h>
#include <MIDI.h>

#define PULLDOWN_PIN PA3
#define LED_PIN      PA15
#define BTN_RED      PB6   
#define BTN_GREEN    PB5   // Mode Modifier
#define BTN_GREY     PB7   
#define MAX_SLOTS    4
#define MAX_EVENTS   128  

struct AutoEvent { uint32_t ms; uint8_t val; };

// Global Data
AutoEvent slot_data[MAX_SLOTS][MAX_EVENTS];
int       slot_count[MAX_SLOTS] = {0,0,0,0};
uint8_t   slot_cc[MAX_SLOTS], slot_start[MAX_SLOTS], slot_min[MAX_SLOTS], slot_max[MAX_SLOTS], slot_live[MAX_SLOTS];
uint32_t  slot_dur[MAX_SLOTS], slot_last_sent[MAX_SLOTS];
int       slot_idx[MAX_SLOTS];
bool      slot_on[MAX_SLOTS] = {false, false, false, false};

// Mode & Speed Variables
float     slot_speed[MAX_SLOTS] = {1.0, 1.0, 1.0, 1.0};
bool      slot_speed_primary[MAX_SLOTS] = {false, false, false, false}; // Toggle: False = Offset is Default, True = Speed is Default

// State
int  rec_slot = 0, next_slot = 0;
bool is_rec = false;
unsigned long t_rec_start = 0, t_play_start = 0;
uint8_t last_note = 60;
int last_btn_red = LOW;

HardwareSerial SerialMIDI(PA10, PA9);
MIDI_CREATE_INSTANCE(HardwareSerial, SerialMIDI, MIDI);

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  last_note = pitch;
  MIDI.sendNoteOn(pitch, velocity, channel);
  if (!is_rec) {
    t_play_start = millis();
    for (int i = 0; i < MAX_SLOTS; i++) {
      if (slot_count[i] > 0) { slot_idx[i] = 0; slot_on[i] = true; }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) { MIDI.sendNoteOff(pitch, 0, channel); }

void updateSpeed(int i, uint8_t val) {
  if (val < 64) slot_speed[i] = map(val, 0, 64, 10, 100) / 100.0f; 
  else slot_speed[i] = map(val, 64, 127, 100, 1000) / 100.0f;
}

void handleControlChange(byte channel, byte num, byte val) {
  bool greenActive = (digitalRead(BTN_GREEN) == HIGH);

  for (int i = 0; i < MAX_SLOTS; i++) {
    if (slot_count[i] > 0 && slot_cc[i] == num) {
      
      // LOGIC SWITCH: Handle Mode Toggling
      if (greenActive) {
        // Toggle the default behavior for this slot if Green is held during movement
        slot_speed_primary[i] = !slot_speed_primary[i];
        
        // If Green is held, perform the NON-default action
        if (!slot_speed_primary[i]) updateSpeed(i, val); // Offset is default, so Green+Knob = Speed
        else slot_live[i] = val;                          // Speed is default, so Green+Knob = Offset
      } else {
        // Perform the DEFAULT action
        if (!slot_speed_primary[i]) slot_live[i] = val;   // Default: Offset
        else updateSpeed(i, val);                         // Default: Speed
      }
    }
  }

  // Handle Recording logic
  if (is_rec) {
    if (slot_count[rec_slot] == 0) {
      for (int i = 0; i < MAX_SLOTS; i++) { if (slot_count[i] > 0 && slot_cc[i] == num) { rec_slot = i; break; } }
      slot_cc[rec_slot] = num; slot_start[rec_slot] = val; slot_min[rec_slot] = val; slot_max[rec_slot] = val;
      slot_live[rec_slot] = val; slot_data[rec_slot][0] = {0, val}; slot_count[rec_slot] = 1;
    } 
    else if (slot_count[rec_slot] < MAX_EVENTS) {
      int c = slot_count[rec_slot];
      slot_data[rec_slot][c] = {(uint32_t)(millis() - t_rec_start), val};
      if (val < slot_min[rec_slot]) slot_min[rec_slot] = val;
      if (val > slot_max[rec_slot]) slot_max[rec_slot] = val;
      slot_count[rec_slot]++;
    }
  }
  MIDI.sendControlChange(num, val, channel);
}

void setup() {
  pinMode(PULLDOWN_PIN, OUTPUT); digitalWrite(PULLDOWN_PIN, LOW);
  pinMode(LED_PIN, OUTPUT); pinMode(BTN_RED, INPUT); pinMode(BTN_GREEN, INPUT); pinMode(BTN_GREY, INPUT);
  SerialMIDI.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn); MIDI.setHandleNoteOff(handleNoteOff); MIDI.setHandleControlChange(handleControlChange);
}

void loop() {
  MIDI.read();
  unsigned long now = millis();

  int btn = digitalRead(BTN_RED);
  if (btn == HIGH && last_btn_red == LOW) {
    is_rec = true; for(int i=0; i<MAX_SLOTS; i++) slot_on[i] = false;
    rec_slot = next_slot; slot_count[rec_slot] = 0; t_rec_start = now;
    MIDI.sendNoteOn(last_note, 127, 1); digitalWrite(LED_PIN, HIGH);
  } else if (btn == LOW && last_btn_red == HIGH) {
    is_rec = false;
    if (slot_count[rec_slot] > 0) {
      slot_dur[rec_slot] = now - t_rec_start;
      if (rec_slot == next_slot && next_slot < 3) next_slot++;
    }
    MIDI.sendNoteOff(last_note, 0, 1); digitalWrite(LED_PIN, LOW);
  }
  last_btn_red = btn;

  if (digitalRead(BTN_GREY) == HIGH) {
    for (int i = 0; i < MAX_SLOTS; i++) { slot_count[i] = 0; slot_on[i] = false; slot_speed[i] = 1.0; slot_speed_primary[i] = false; }
    next_slot = 0;
  }

  if (!is_rec) {
    for (int i = 0; i < MAX_SLOTS; i++) {
      if (slot_on[i]) {
        uint32_t v_elapsed = (uint32_t)((now - t_play_start) * slot_speed[i]);
        if (v_elapsed >= slot_dur[i]) { slot_on[i] = false; continue; }

        float offset = 0, start = (float)slot_start[i], l_val = (float)slot_live[i];
        if (l_val > start) {
          float k_range = 127.0f - start;
          if (k_range > 0) offset = (l_val - start) * ((127.0f - (float)slot_max[i]) / k_range);
        } else if (l_val < start) {
          if (start > 0) offset = (l_val - start) * ((float)slot_min[i] / start);
        }

        int cur_idx = slot_idx[i];
        while (cur_idx < slot_count[i]) {
          if (v_elapsed >= slot_data[i][cur_idx].ms) {
            if (now - slot_last_sent[i] >= 5 || cur_idx == slot_count[i] - 1) {
              int f = (int)((float)slot_data[i][cur_idx].val + offset);
              MIDI.sendControlChange(slot_cc[i], (uint8_t)constrain(f, 0, 127), 1);
              slot_last_sent[i] = now;
            }
            cur_idx++;
          } else break;
        }
        slot_idx[i] = cur_idx;
      }
    }
  }
}
