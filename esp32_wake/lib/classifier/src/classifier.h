#ifndef CLASSIFIER
#define CLASSIFIER
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
namespace tflite
{
    template <unsigned int tOpCount>
    class MicroMutableOpResolver;
    class ErrorReporter;
    class Model;
    class MicroInterpreter;
}

class Classifier
{
public:
    Classifier();
    ~ Classifier();
    float *getInputBuffer();
    float predict();
private:
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    tflite::ErrorReporter* errorReporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;

    // approximate working size of our model
    const int kTensorArenaSize = 25000;
    uint8_t *tensorArena = nullptr;
};

#endif