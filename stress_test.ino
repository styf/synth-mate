// MIDI STRESS TEST ON 4 MHZ CLOCK

#include <Arduino.h>
#include <MIDI.h>

HardwareSerial SerialMIDI(PA10, PA9);
MIDI_CREATE_INSTANCE(HardwareSerial, SerialMIDI, MIDI);

extern "C" void SystemClock_Config(void);

#define USB_PIN PA3
#define INTERVAL_MS 0   // 0 = Full speed stress test, >0 = Precise interval

unsigned long lastMessageTime = 0;
int ccValue = 0;

void setup() {
  SystemClock_Config();

  // Initialize PA3 pull-down
  pinMode(USB_PIN, OUTPUT);
  digitalWrite(USB_PIN, LOW);

  // Initialize MIDI on Channel 1
  MIDI.begin(1); 
}

void loop() {
  unsigned long currentTime = millis();

  // Non-blocking timer for stress testing
  if (currentTime - lastMessageTime >= INTERVAL_MS) {
    
    // Flood the bus with CC74 messages
    MIDI.sendControlChange(74, ccValue, 1);
    
    // Increment and wrap CC value (0-127)
    ccValue++;
    if (ccValue > 127) ccValue = 0;

    lastMessageTime = currentTime;
  }
  
  // Keep the internal MIDI buffers clear if any data comes back
  MIDI.read();
}

/* -------- Clock configuration (Currently 4 MHz) -------- */
extern "C" void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // MSI Range 6 = 4 MHz. Change to RCC_MSIRANGE_1 for 100 kHz (Stress Test)
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; 
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    while (1); 
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    while (1);
  }
}
