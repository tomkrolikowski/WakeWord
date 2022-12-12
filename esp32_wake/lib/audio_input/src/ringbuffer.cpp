#include "ringbuffer.h"

Ring_Buffer::Ring_Buffer(size_t data_size){
    data = new int16_t[data_size];
    this->data_size = data_size;
    current_size = 0;
    front_index = 0;
    back_index = 0;
}

void Ring_Buffer::push(int16_t *data, size_t length){
    for (size_t i = 0; i < length; ++i){
        this->data[(i + front_index) % data_size] = data[i];
    }
    front_index = (front_index + length) % data_size;
    current_size += length;
}
void Ring_Buffer::pop(int16_t *outdata, size_t length){
    for(size_t i = 0; i < length; ++i){
        outdata[i] = data[(i + back_index) % data_size];
    }
    back_index = (back_index + length) % data_size;
    current_size -= length;
}

int16_t Ring_Buffer::operator[](size_t index) const {
    return data[(index + back_index) % data_size];
}

void Ring_Buffer::reset_index(size_t amount) {
    back_index = (back_index + amount) % data_size;
}

void Ring_Buffer::update_size(size_t amount) {
    current_size -= amount;
}

size_t Ring_Buffer::get_current_index() const {
    return front_index;
}

size_t Ring_Buffer::get_current_size() const {
    return current_size;
}