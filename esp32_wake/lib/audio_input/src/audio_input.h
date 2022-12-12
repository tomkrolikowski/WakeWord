#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <mutex>
#include "hanning.h"
#include "ringbuffer.h"
#include "NeuralNetwork.h"

#define FRAME_SIZE 128
#define FRAME_STEP 128
#define AUDIO_LENGTH 8000
class Audio_Input
{
public:
    Audio_Input(i2s_config_t i2s_config, i2s_pin_config_t pin_config, TaskHandle_t processor_task_handle);
    void start_audio();
    QueueHandle_t receive_queue;
    TaskHandle_t processor_task_handle;
    TaskHandle_t blink_task_handle;
    TaskHandle_t remove_noise_task_handle;
    Ring_Buffer *process_buffer;
    void get_spectrogram(float *output);
    int16_t *data_buffer;
    float *spectrogram;
    float *noise_spec;
    std::mutex nn_mutex;
    NeuralNetwork *nn;
    size_t row_size;
    size_t col_size;
private:
    // void spectrogram_segment(float *input, size_t start_index);
    HanningWindow *hanning_window;
    TaskHandle_t receiver_task_handle;
    i2s_config_t i2s_config;
    i2s_pin_config_t i2s_pin_config;
    arduinoFFT FFT;
    uint8_t exponent;
    uint16_t samples;
    size_t fft_window_size;
    size_t energy_size;
};

#endif