#pragma once

#include <Arduino.h>
#include "driver/spi_master.h"

class DAC1220 {
public:
    DAC1220(int sckPin = 25, int misoPin = 27, int mosiPin = 26, int csPin = 33, uint32_t frequency = 1000000);
    void begin();
    void reset_all();

private:
    int sckPin, misoPin, mosiPin, csPin;  // Pin assignments
    uint32_t frequency;                   // SPI frequency
    spi_device_handle_t spi;              // SPI device handle

    void write24(uint32_t data);
    void initSPI();


public:
    // Constants
    static constexpr uint32_t MAX_VALUE = 0xFFFFF;

    // Function to reset DAC
    void reset(bool resetAll = false) {
//        write24(resetAll ? DAC_RESET_ALL : DAC_RESET_INPUT);
    }

    // Function to set reference
    void setReference(bool enableGain2 = false) {
//        write24(enableGain2 ? DAC_REFERENCE_ENABLE_G2 : DAC_REFERENCE);
    }

    // Function to set gain
    void setGain(bool gainB1A1 = false) {
//        write24(gainB1A1 ? DAC_GAIN_B1A1 : DAC_GAIN_B2A2);
    }

    // Function to write and update DAC channel A
    void writeA(uint16_t value) {
//        write24(DAC_WRITE_A_UPDATE_A | (value & 0xFFFF));
    }

    // Function to update all channels
    void updateAll() {
//        write24(DAC_UPDATE_ALL);
    }
};

