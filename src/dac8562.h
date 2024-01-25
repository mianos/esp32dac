#pragma once

#include <Arduino.h>
#include "driver/spi_master.h"

class DAC8562 {
public:
    DAC8562(int sckPin = 25, int misoPin = 27, int mosiPin = 26, int csPin = 33, uint32_t frequency = 1000000);
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
    static constexpr uint32_t DAC_RESET = 0x280000;
    static constexpr uint32_t DAC_RESET_ALL = DAC_RESET | 0x1;
    static constexpr uint32_t DAC_RESET_INPUT = DAC_RESET;
    static constexpr uint32_t DAC_REFERENCE = 0x380000;
    static constexpr uint32_t DAC_REFERENCE_ENABLE_G2 = DAC_REFERENCE | 0x1;
    static constexpr uint32_t DAC_GAIN = 0x20000;
    static constexpr uint32_t DAC_GAIN_B2A2 = 0x0;
    static constexpr uint32_t DAC_GAIN_B1A1 = DAC_GAIN | 0x3;
    static constexpr uint32_t DAC_WRITE_REGISTER_A = 0x00000;
    static constexpr uint32_t DAC_WRITE_REGISTER_B = DAC_WRITE_REGISTER_A | 0x10000;
    static constexpr uint32_t DAC_UPDATE_ALL = 0xf0000;
    static constexpr uint32_t DAC_WRITE_UPDATE_ALL = 0x100000;
    static constexpr uint32_t DAC_WRITE_UPDATE_ALL_A = DAC_WRITE_UPDATE_ALL;
    static constexpr uint32_t DAC_WRITE_UPDATE_ALL_B = DAC_WRITE_UPDATE_ALL | 0x10000;
    static constexpr uint32_t DAC_WRITE_UPDATE_ALL_AB = DAC_WRITE_UPDATE_ALL | 0x70000;
    static constexpr uint32_t DAC_WRITE_A_UPDATE_A = 0x180000;
    static constexpr uint32_t DAC_WRITE_B_UPDATE_B = DAC_WRITE_A_UPDATE_A | 0x10000;
    static constexpr uint32_t DAC_WRITE_AB_UPDATE_AB = DAC_WRITE_A_UPDATE_A | 0x70000;
    static constexpr uint32_t DAC_POWER_UP = 0x200000;
    static constexpr uint32_t DAC_POWER_UP_A = DAC_POWER_UP | 0x1;
    static constexpr uint32_t DAC_POWER_UP_B = DAC_POWER_UP | 0x2;
    static constexpr uint32_t DAC_POWER_UP_AB = DAC_POWER_UP | 0x3;
    static constexpr uint32_t DAC_LDAC_MODE = 0x300000;
    static constexpr uint32_t DAC_LDAC_INACTIVE_AB = DAC_LDAC_MODE | 0x3;

    static constexpr uint16_t MAX_VALUE = 0x0FFF; // Maximum 12-bit value

    // Function to reset DAC
    void reset(bool resetAll = false) {
        write24(resetAll ? DAC_RESET_ALL : DAC_RESET_INPUT);
    }

    // Function to set reference
    void setReference(bool enableGain2 = false) {
        write24(enableGain2 ? DAC_REFERENCE_ENABLE_G2 : DAC_REFERENCE);
    }

    // Function to set gain
    void setGain(bool gainB1A1 = false) {
        write24(gainB1A1 ? DAC_GAIN_B1A1 : DAC_GAIN_B2A2);
    }

    // Function to write and update DAC channel A
    void writeA(uint16_t value) {
        write24(DAC_WRITE_A_UPDATE_A | (value & 0xFFFF));
    }

    // Function to write and update DAC channel B
    void writeB(uint16_t value) {
        write24(DAC_WRITE_B_UPDATE_B | (value & 0xFFFF));
    }

    // Function to update all channels
    void updateAll() {
        write24(DAC_UPDATE_ALL);
    }
};

