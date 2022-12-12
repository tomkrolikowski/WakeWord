#ifndef HANNING_H
#define HANNING_H
#include <Arduino.h>

class HanningWindow
{
public:
    HanningWindow(size_t size);
    ~HanningWindow();
    void apply_window(double *data) const;
private:
    float *coef;
    size_t window_size;
};

#endif