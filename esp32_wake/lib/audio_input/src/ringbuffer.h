#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <Arduino.h>

class Ring_Buffer
{
public:
    Ring_Buffer(size_t data_size);
    void push(int16_t *data, size_t length);
    void pop(int16_t *output, size_t length);
    int16_t operator[](size_t index) const;
    void reset_index(size_t amount);
    void update_size(size_t amount);
    size_t get_current_index() const;
    size_t get_current_size() const;
private:
    size_t front_index;
    size_t back_index;
    size_t data_size;
    size_t current_size;
    int16_t *data;
};

#endif