#include "classifier.h"
#include "model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

Classifier::Classifier(){
    tensorArena = (uint8_t *)malloc(kTensorArenaSize);
    if (!tensorArena)
    {
        TF_LITE_REPORT_ERROR(errorReporter, "Could not allocate arena");
        return;
    }   
    TF_LITE_REPORT_ERROR(errorReporter, "Loading model");

    static tflite::MicroErrorReporter micro_error_reporter;
    errorReporter = &micro_error_reporter;

    model = tflite::GetModel(converted_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(errorReporter,
                            "Model provided is schema version %d not equal "
                            "to supported version %d.",
                            model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::MicroMutableOpResolver<9> microOpResolver(errorReporter);
    if (microOpResolver.AddConv2D() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddMaxPool2D() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddFullyConnected() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddMul() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddAdd() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddLogistic() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddReshape() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddQuantize() != kTfLiteOk) {
        return;
    }
    if (microOpResolver.AddDequantize() != kTfLiteOk) {
        return;
    }

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model, microOpResolver, tensorArena, kTensorArenaSize, errorReporter);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(errorReporter, "AllocateTensors() failed");
        return;
    }

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);
    output = interpreter->output(0);

}

Classifier::~Classifier(){
    free(tensorArena);
}

float *Classifier::getInputBuffer(){
    return input->data.f;
}

float Classifier::predict(){
    interpreter->Invoke();
    return output->data.f[0];
}