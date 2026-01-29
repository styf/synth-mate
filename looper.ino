#include <Arduino.h>
#include <MIDI.h>

// --- Pin Definitions ---
#define PULLDOWN_PIN PA3
#define LED_PIN      PA15
#define BTN_RED      PB6   // REC / OVERDUB
#define BTN_GREEN    PB5   // PAUSE / RESUME
#define BTN_GREY     PB7   // STOP / CLEAR
#define BTN_ENC      PA2   // RESET TEMPO
#define ENC_A        PA1
#define ENC_B        PA0

// --- MIDI Setup ---
HardwareSerial SerialMIDI(PA10, PA9);
MIDI_CREATE_INSTANCE(HardwareSerial, SerialMIDI, MIDI);

// --- Looper Structures ---
typedef struct {
  uint32_t timestamp_ms;
  uint8_t pitch;
  uint8_t velocity;
  uint8_t channel;
} note_event_struct;

note_event_struct note_loop_buffer[512]; 
int noteMap[128]; 

// --- State Variables ---
int looper_state = 0; // 0:STOP, 1:REC, 2:PLAY, 3:HOLD, 4:OVERDUB
bool isPaused = false;
float tempo_factor = 1.0;

int note_event_count = 0;
int playback_index = 0;
uint32_t loop_length_ms = 0;
unsigned long loop_start_time = 0;
unsigned long playback_start_time = 0;
unsigned long time_paused = 0;
unsigned long pause_stamp = 0; // Fixed: Added declaration

volatile int Encoder = 0;
volatile int currentStateA;
volatile int previousStateA;

int lastBtnRed = LOW, lastBtnGreen = LOW, lastBtnGrey = LOW, lastBtnEnc = LOW;

// --- Encoder Interrupt ---
void readEnc() {
  currentStateA = digitalRead(ENC_A);
  if (currentStateA != previousStateA) {
    if (digitalRead(ENC_B) != currentStateA) {
      Encoder--;
      tempo_factor *= 0.98;
    } else {
      Encoder++;
      tempo_factor *= 1.02;
    }
  }
  previousStateA = currentStateA;
}

// --- MIDI Handlers ---
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, velocity, channel); 

  if (looper_state == 3) {
    looper_state = 1;
    loop_start_time = millis();
  }

  if ((looper_state == 1 || looper_state == 4) && note_event_count < 512) {
    uint32_t ts;
    if (looper_state == 1) ts = millis() - loop_start_time;
    else ts = (millis() - playback_start_time - time_paused) * tempo_factor;
    
    note_loop_buffer[note_event_count] = {ts, pitch, velocity, channel};
    note_event_count++;
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOff(pitch, velocity, channel); 

  if ((looper_state == 1 || looper_state == 4) && note_event_count < 512) {
    uint32_t ts;
    if (looper_state == 1) ts = millis() - loop_start_time;
    else ts = (millis() - playback_start_time - time_paused) * tempo_factor;
    
    note_loop_buffer[note_event_count] = {ts, pitch, 0, channel};
    note_event_count++;
  }
}

void hangingOff() {
  for (int i = 0; i < 128; i++) {
    if (noteMap[i] > 0) {
      MIDI.sendNoteOff(i, 0, 1);
      noteMap[i] = 0;
    }
  }
}

void playNoteLooper() {
  if ((looper_state != 2 && looper_state != 4) || isPaused) return;

  unsigned long current_time_in_loop = millis() - playback_start_time - time_paused;
  uint32_t scaled_loop_length = (uint32_t)((float)loop_length_ms / tempo_factor);

  // Restart Loop
  if (current_time_in_loop >= scaled_loop_length) {
    hangingOff();
    playback_start_time = millis();
    playback_index = 0;
    time_paused = 0;
    return;
  }

  while (playback_index < note_event_count) {
    note_event_struct e = note_loop_buffer[playback_index];
    uint32_t scaled_ts = (uint32_t)((float)e.timestamp_ms / tempo_factor);

    if (current_time_in_loop >= scaled_ts) {
      if (e.velocity > 0) {
        MIDI.sendNoteOn(e.pitch, e.velocity, e.channel);
        noteMap[e.pitch] = e.velocity;
      } else {
        MIDI.sendNoteOff(e.pitch, 0, e.channel);
        noteMap[e.pitch] = 0;
      }
      playback_index++;
    } else {
      break;
    }
  }
}

void setup() {
  // Pull down PA3 immediately
  pinMode(PULLDOWN_PIN, OUTPUT);
  digitalWrite(PULLDOWN_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_RED, INPUT);
  pinMode(BTN_GREEN, INPUT);
  pinMode(BTN_GREY, INPUT);
  pinMode(BTN_ENC, INPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  SerialMIDI.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  previousStateA = digitalRead(ENC_A);
  attachInterrupt(digitalPinToInterrupt(ENC_A), readEnc, CHANGE);
}

void loop() {
  MIDI.read();
  
  int btnRed = digitalRead(BTN_RED);
  int btnGreen = digitalRead(BTN_GREEN);
  int btnGrey = digitalRead(BTN_GREY);
  int btnEnc = digitalRead(BTN_ENC);

  // --- RED: REC / PLAY / OVERDUB ---
  if (btnRed == HIGH && lastBtnRed == LOW) {
    if (looper_state == 0) {
      looper_state = 3; // Wait for note
      note_event_count = 0;
    } else if (looper_state == 1) {
      // Transition from REC to PLAY
      loop_length_ms = millis() - loop_start_time;
      hangingOff();         
      playback_index = 0;   
      time_paused = 0;      
      playback_start_time = millis(); // Loop starts NOW
      looper_state = 2;     
    } else if (looper_state == 2) {
      looper_state = 4; // Start Overdub
    } else if (looper_state == 4) {
      looper_state = 2; // Back to Play
    }
  }
  lastBtnRed = btnRed;

  // --- GREEN: PAUSE / RESUME ---
  if (btnGreen == HIGH && lastBtnGreen == LOW) {
    if (looper_state == 2 || looper_state == 4) {
      isPaused = !isPaused;
      if (isPaused) {
        hangingOff();
        pause_stamp = millis(); 
      } else {
        time_paused += (millis() - pause_stamp);
      }
    }
  }
  lastBtnGreen = btnGreen;

  // --- GREY: CLEAR ---
  if (btnGrey == HIGH && lastBtnGrey == LOW) {
    looper_state = 0;
    hangingOff();
    note_event_count = 0;
    playback_index = 0;
    time_paused = 0;
    tempo_factor = 1.0;
  }
  lastBtnGrey = btnGrey;

  // --- ENC BTN: RESET ---
  if (btnEnc == HIGH && lastBtnEnc == LOW) {
    tempo_factor = 1.0;
    Encoder = 0;
  }
  lastBtnEnc = btnEnc;

  playNoteLooper();

  // LED Visuals
  if (looper_state == 1 || looper_state == 3) digitalWrite(LED_PIN, (millis() % 200 < 100)); // Fast Blink
  else if (looper_state == 4) digitalWrite(LED_PIN, (millis() % 600 < 300)); // Slow Blink
  else if (looper_state == 2 && !isPaused) digitalWrite(LED_PIN, HIGH); // Solid Play
  else digitalWrite(LED_PIN, LOW);
}
