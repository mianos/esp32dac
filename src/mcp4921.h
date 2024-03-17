#pragma once
#if DAC == 8562

#include <Arduino.h>
#include "driver/spi_master.h"

class MCP4921 {
public:
    MCP4921(int sckPin = 25, int misoPin = 27, int mosiPin = 26, int csPin = 33, uint32_t frequency = 1000000);
    void begin();

private:
    int sckPin, misoPin, mosiPin, csPin;  // Pin assignments
    uint32_t frequency;                   // SPI frequency
    spi_device_handle_t spi;              // SPI device handle

    void write16(uint16_t data);
    void initSPI();

public:
public:
    static constexpr uint16_t DAC_SELECT_B           = 0x8000;
    static constexpr uint16_t DAC_SELECT_A           = 0x0000;
    static constexpr uint16_t BUFFERED_VREF          = 0x4000;
    static constexpr uint16_t UNBUFFERED_VREF        = 0x0000;
    static constexpr uint16_t OUTPUT_GAIN_1X         = 0x2000;
    static constexpr uint16_t OUTPUT_GAIN_2X         = 0x0000;
    static constexpr uint16_t OUTPUT_ACTIVE          = 0x1000;
    static constexpr uint16_t OUTPUT_INACTIVE        = 0x0000;

    static constexpr uint16_t MAX_VALUE = 0x0FFF; // Maximum 12-bit value

private:
    uint16_t currentGain = OUTPUT_GAIN_1X; // Default gain

public:
    void reset() {
        write16(DAC_SELECT_A | OUTPUT_INACTIVE);
    }

    void setGain(bool gain2x) {
        uint16_t gain = gain2x ? OUTPUT_GAIN_2X : OUTPUT_GAIN_1X;
        // Store the gain setting for later use in write operations
        currentGain = gain;
    }

    void writeA(uint16_t value) {
        value &= MAX_VALUE; // Ensure the value is in the range of 0-4095
        write16(DAC_SELECT_A | BUFFERED_VREF | currentGain | OUTPUT_ACTIVE | value);
    }
};
#endif
