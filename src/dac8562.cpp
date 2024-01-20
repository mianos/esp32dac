#include "DAC8562.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

DAC8562::DAC8562(int sck, int miso, int mosi, int cs, uint32_t freq)
    : sckPin(sck), misoPin(miso), mosiPin(mosi), csPin(cs), frequency(freq) {
}

void DAC8562::begin() {
    initSPI();
    reset();
}

void DAC8562::reset() {
    write24(DAC_RESET_ALL);
    write24(DAC_REFERENCE_ENABLE_G2);
    write24(DAC_GAIN_B2A2);
}

void DAC8562::writeData(uint32_t data) {
    write24(data);
}

void DAC8562::write24(uint32_t data) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 24;
    t.tx_buffer = &data;
    spi_device_transmit(spi, &t);
}

void DAC8562::initSPI() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosiPin,
        .miso_io_num = misoPin,
        .sclk_io_num = sckPin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };


    spi_device_interface_config_t spi_devcfg;
    memset(&spi_devcfg, 0, sizeof(spi_device_interface_config_t));
    spi_devcfg.mode = 0;
    spi_devcfg.clock_speed_hz = static_cast<int>(frequency);
    spi_devcfg.input_delay_ns = 20; //?
    spi_devcfg.spics_io_num = csPin;
    spi_devcfg.queue_size = 7;

    // Initialize the SPI bus and add the device
    spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    spi_bus_add_device(HSPI_HOST, &spi_devcfg, &spi);
}
