#include "hanning.h"
#include <cmath>

HanningWindow::HanningWindow(size_t size){
    window_size = size;
    coef = new float[size];
    float maximum_val = 0;
    for (size_t i = 0; i <= (size / 2); ++i){
        float temp = 0.5 * (1 - cos((M_PI * 2 * i) / window_size));
        coef[i] = temp;
        coef[size - 1 - i] = temp;
        maximum_val = max(temp, maximum_val);
    }
    for(size_t i = 0; i < size; ++i){
        coef[i] /= maximum_val;
    }
}

HanningWindow::~HanningWindow(){
    delete coef;
}

void HanningWindow::apply_window(double *data) const{
    for (size_t i = 0; i < window_size; ++i){
        data[i] *= coef[i];
    }
}