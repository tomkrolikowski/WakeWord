#include <Arduino.h>
#include <driver/i2s.h>
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "audio_input.h"
#include "classifier.h"
#include <sdkconfig.h>

// #define configUSE_TRACE_FACILITY 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1
// #define configGENERATE_RUN_TIME_STATS 1
// #define portCONFIGURE_TIMER_FOR_TIME_STATS() 100
// #define portALT_GET_RUN_TIME_COUNTER_VALUE() 1
// #define CONFIG_FREERTOS_USE_TRACE_FACILITY 1
// #define CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS 1
// #define CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS 1
// #define CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER 1

#define PIN_CLK 5
#define PIN_DATA 35
#define DMA_BUFFERS 4
#define PDM_BYTES 2
#define DMA_BUFFER_LEN 64
#define SAMPLING_RATE 8000

// Pins
static const int i2s_num = 0; // i2s port number

i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
  .sample_rate = SAMPLING_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Interrupt level 1, default 0
  .dma_buf_count = DMA_BUFFERS,
  .dma_buf_len = DMA_BUFFER_LEN,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0 
};

i2s_pin_config_t pin_config = {
    .ws_io_num   = PIN_CLK,
    .data_in_num = PIN_DATA,
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  TaskHandle_t processor_task_handle;
  Audio_Input *audio_input = new Audio_Input(i2s_config, pin_config, processor_task_handle);
  audio_input->start_audio();
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelay(1000);
  printf("Working\n");
}