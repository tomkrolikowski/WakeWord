#include <cmath>
#include <utility.h>
#include <arduinoFFT.h>
#include <mutex>
#include "audio_input.h"
#include <freertos/task.h>

Audio_Input::Audio_Input(i2s_config_t i2s_config, i2s_pin_config_t pin_config, TaskHandle_t processor_task_handle){
    printf("Initializing Audio input...\n");
    nn = new NeuralNetwork();
    processor_task_handle = processor_task_handle;
    process_buffer = new Ring_Buffer(20000);
    i2s_config = i2s_config;
    i2s_pin_config = i2s_pin_config;
    i2s_driver_install(I2S_NUM_0, &i2s_config, i2s_config.dma_buf_count, &receive_queue);
    // i2s_set_pdm_rx_down_sample(I2S_NUM_0, i2s_pdm_dsr_t::I2S_PDM_DSR_16S);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    fft_window_size = 1;
    while (fft_window_size < FRAME_SIZE) {
        fft_window_size <<= 1;
    }
    energy_size = fft_window_size / 2 + 1;
    // noise_spec = new float[row_size * col_size];
    // for(size_t i = 0; i < row_size * col_size; ++i){
    //     noise_spec[i] = 0;
    // }
    printf("FFT Size: %u\n", fft_window_size);
    printf("FFT Size: %u\n", FRAME_SIZE);
    printf("STFT Frame Step: %u\n", FRAME_STEP);
    printf("STFT Row Size %u\n", (AUDIO_LENGTH - FRAME_SIZE) / FRAME_STEP);
    printf("STFT Col Size: %u\n", energy_size);
    hanning_window = new HanningWindow((size_t)FRAME_SIZE);
    arduinoFFT FFT = arduinoFFT();
    exponent = FFT.Exponent(fft_window_size); 
    printf("Finished Initializing Audio...\n");
    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
}

void receive_queue_task(void *parameters){
    Audio_Input *audio_input = (Audio_Input *)parameters;
    const gpio_config_t receive_config = {
        .pin_bit_mask = 1 << 15, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&receive_config);
    while(1){
        i2s_event_t event;
        if (xQueueReceive(audio_input->receive_queue, &event, portMAX_DELAY) == pdTRUE && event.type == I2S_EVENT_RX_DONE){
            gpio_set_level(GPIO_NUM_15, HIGH);
            uint8_t data[512];
            size_t bytes_read;
            i2s_read(I2S_NUM_0, data, 512, &bytes_read, portMAX_DELAY);
            int16_t *b = (int16_t*)data;
            audio_input->process_buffer->push(b, bytes_read / 2);
            // printf("Received %u bytes from DMA\n", bytes_read);
            gpio_set_level(GPIO_NUM_15, LOW);
        }
        if (audio_input->process_buffer->get_current_size() >= (AUDIO_LENGTH / 2)){
            printf("Audio Length: %u\n", audio_input->process_buffer->get_current_size());
            audio_input->process_buffer->update_size(AUDIO_LENGTH / 2);
            xTaskNotifyGive(audio_input->processor_task_handle);
        }
    }
}

void process_noise_spectrogram(void *parameters){
    Audio_Input *audio_input = (Audio_Input *)parameters;
    while(1) {
        int notification_val = 0;
        notification_val = ulTaskNotifyTake(pdTRUE, (TickType_t)(portMAX_DELAY)); 
        if (notification_val > 0) {
            printf("Collecting Noise Sample\n");
            // std::lock_guard<std::mutex> lock(audio_input->nn_mutex);
            float *buf = audio_input->nn->getInputBuffer();
            for(size_t i = 0; i < 62 * 65; ++i){
                audio_input->noise_spec[i] += buf[i];
            }
            // printf("Collected Noise Sample and closed task\n");
            vTaskDelay(20 * 1000);
            // vTaskDelete(audio_input->remove_noise_task_handle);
        }
    }
}

std::pair<float, float> calc_stats(const Ring_Buffer &buffer){
    std::pair<float, float> stats = {0.0, 0.0};
    for (size_t i = 0; i < AUDIO_LENGTH; ++i){
        float val = ((float)buffer[i]) / (float)INT16_MAX;
        stats.first += val;
    }
    stats.first /= (float)AUDIO_LENGTH;
    for (size_t i = 0; i < AUDIO_LENGTH; ++i){
        float val = ((float)buffer[i]) / (float)INT16_MAX;
        stats.second += (val - stats.first) * (val - stats.first);
    }
    stats.second = sqrt(stats.second / (float)AUDIO_LENGTH);
    return stats;
}

void process_audio_task(void *parameters){
    Audio_Input *audio_input = (Audio_Input *)parameters;
    printf("Started Processor Task\n");
    const gpio_config_t blue_config = {
        .pin_bit_mask = 1 << 2, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&blue_config);

    const gpio_config_t nn_config = {
        .pin_bit_mask = 1 << 17, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&nn_config);

    const gpio_config_t spec_config = {
        .pin_bit_mask = 1 << 16, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&spec_config);
    while(1){
        printf("Waiting...\n");
        int notification_val = 0;
        notification_val = ulTaskNotifyTake(pdTRUE, (TickType_t)(portMAX_DELAY));
        if (notification_val > 0){
            gpio_set_level(GPIO_NUM_2, HIGH);
            printf("Started Queue Processs:\n");
            // audio_input->nn_mutex.lock();
            gpio_set_level(GPIO_NUM_16, HIGH);
            float *input_buffer = audio_input->nn->getInputBuffer();
            audio_input->get_spectrogram(input_buffer);
            gpio_set_level(GPIO_NUM_16, LOW);
            gpio_set_level(GPIO_NUM_17, HIGH);
            float *result = audio_input->nn->predict();
            gpio_set_level(GPIO_NUM_17, LOW);
            // audio_input->nn_mutex.unlock();
            float max = 0;
            size_t argmax = 0;
            printf("[ ");
            for(size_t i = 0; i < 4; ++i) {
                printf(" %f, ", result[i]);
                if(result[i] > max) {
                    max = result[i];
                    argmax = i;
                }
            }
            printf(" ]\n");
            printf("Predicting: %u\n", argmax);
            // if((argmax == 2 && max > 0.85 ) || (argmax == 3 && max > 0.85)){
            //     xTaskNotify(audio_input->blink_task_handle, argmax, eSetValueWithOverwrite);
            // }
            // if(argmax == 0 && max > 0.95){
            //     xTaskNotifyGive(aud io_input->remove_noise_task_handle);
            // }
            gpio_set_level(GPIO_NUM_2, LOW);
        } 
    } 
}

void Audio_Input::get_spectrogram(float *output){
    std::pair<float,float> stats = calc_stats(*process_buffer);
    double real[fft_window_size];
    double imag[fft_window_size] = {0};
    for(size_t i = 0, index = 0; i < AUDIO_LENGTH - FRAME_SIZE; i += FRAME_STEP, ++index){
        for(size_t j = 0; j < fft_window_size; ++j){
            if (j < FRAME_SIZE){
                real[j] = (((float)(*process_buffer)[i + j] / (float)INT16_MAX) - stats.first) / stats.second;
                imag[j] = 0;
            }
            else{
                real[j] = 0;
                imag[j] = 0;
            }
        }
        hanning_window->apply_window(real);
        FFT.Compute(real, imag, fft_window_size, exponent, FFT_FORWARD);
        for(size_t j = 0; j < energy_size; ++j){
            float real_v = real[j];
            float imag_v = imag[j];
            output[j + index * energy_size] = sqrt(real_v * real_v + imag_v * imag_v);
        }
    }
    process_buffer->reset_index(AUDIO_LENGTH / 2);
}

void blink_result_task(void *parameters){
    const gpio_config_t green_config = {
        .pin_bit_mask = 1 << 0, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    const gpio_config_t red_config = {
        .pin_bit_mask = 1 << 4, // pin 0
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t status = gpio_config(&green_config);
    if(status != ESP_OK){
        printf("GPIO 0 Configuration Error\n");
    }
    status = gpio_config(&red_config);
    if(status != ESP_OK){
        printf("GPIO 4 Configuration Error\n");
    }

    while(1){
        int notification_val = 0;
        notification_val = ulTaskNotifyTake(pdTRUE, (TickType_t)(portMAX_DELAY));
        if( notification_val > 0) {
            if (notification_val == 2){ 
                //sheila
                gpio_set_level(GPIO_NUM_0, HIGH);
                vTaskDelay(1000);
                gpio_set_level(GPIO_NUM_0, LOW);
            }
            if (notification_val == 3){
                //marvin
                gpio_set_level(GPIO_NUM_4, HIGH);
                vTaskDelay(1000);
                gpio_set_level(GPIO_NUM_4, LOW);
            }
        }
    }
}

void Audio_Input::start_audio(){
    printf("Starting Audio input\n");
    xTaskCreatePinnedToCore(receive_queue_task, "i2s Reader Task", 8192, this, 1, &receiver_task_handle, APP_CPU_NUM);
    xTaskCreatePinnedToCore(process_audio_task, "Process Audio", 8192, this, 1, &processor_task_handle, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(blink_result_task, "Blink Result Task", 1024, NULL, 2, &blink_task_handle, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(process_noise_spectrogram, "Noise Spectrogram", 8192, this, 4, &remove_noise_task_handle, PRO_CPU_NUM);
}