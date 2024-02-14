#include "dac1220.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

DAC1220::DAC1220(int sck, int miso, int mosi, int cs, uint32_t freq)
    : sckPin(sck), misoPin(miso), mosiPin(mosi), csPin(cs), frequency(freq) {
}

void DAC1220::begin() {
  initSPI();
  
  // Ideally apply the reset pattern, which enters the Normal mode when complete.
  // but this does all sorts of wacky bit banging so see how we go without it
  // reset_all();

  // Set configuration to 20-bit, straight binary mode.
  uint32_t cmr = read_command_register();
  cmr |= (1 << CMR_RES) | (1 << CMR_DF);
  set_command_register(cmr);

  // Calibrate with the output connected.
  calibrate(true);
}

void DAC1220::reset_all() {
#if 0
  spi_end();

  // Temporarily switch CLK and CE0 pins to be outputs.
  bcm2835_gpio_fsel(CLK_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(CE0_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_write(CLK_PIN, LOW);
  bcm2835_gpio_write(CE0_PIN, LOW);
  bcm2835_delayMicroseconds(t10/1000);

  // Apply the reset pattern.
  bcm2835_gpio_write(CLK_PIN, HIGH);
  bcm2835_delayMicroseconds(t16/1000);
  bcm2835_gpio_write(CLK_PIN, LOW);
  bcm2835_delayMicroseconds(t17/1000);
  bcm2835_gpio_write(CLK_PIN, HIGH);
  bcm2835_delayMicroseconds(t18/1000);
  bcm2835_gpio_write(CLK_PIN, LOW);
  bcm2835_delayMicroseconds(t17/1000);
  bcm2835_gpio_write(CLK_PIN, HIGH);
  bcm2835_delayMicroseconds(t19/1000);
  bcm2835_gpio_write(CLK_PIN, LOW); // DAC resets here.

  // Get ready to switch back to SPI mode.
  bcm2835_delayMicroseconds(t14/1000);
  bcm2835_gpio_write(CE0_PIN, HIGH);
  bcm2835_delayMicroseconds(t15/1000);
#endif
}


void DAC1220::write24(uint32_t data) {
    uint8_t tx_data[3];
    
    tx_data[0] = (data >> 12) & 0xFF; // Extracts the most significant 8 bits of the 20-bit value.
    tx_data[1] = (data >> 4) & 0xFF;  // Extracts the middle 8 bits.
    tx_data[2] = (data << 4) & 0xF0;  // Extracts the least significant 4 bits and places them in the MSBs of the last byte.


    spi_transaction_t t;
    memset(&t, 0, sizeof(t));  // Zero out the transaction
    t.length = 24;             // Length in bits
    t.tx_buffer = &tx_data;    // Transmit buffer
    spi_device_transmit(spi, &t);  // Perform the SPI transaction
}

void DAC1220::initSPI() {
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




void DAC1220::write_register(uint8_t cmd, uint32_t reg) {
    uint8_t buffer[4] = {cmd, 0, 0, 0}; // Ensure buffer is large enough for command + data
    spi_transaction_t t;

    memset(&t, 0, sizeof(t)); // Zero out the transaction structure

    // Set up the transaction based on the command byte's indication of register byte length
    switch (cmd & ((uint8_t)0b11 << CB_MB)) {
        case (CB_MB_1 << CB_MB): // 1 byte
            buffer[1] = (char)(reg & 0xFF);
            t.length = 16; // Command byte + 1 data byte = 16 bits
            break;
        case (CB_MB_2 << CB_MB): // 2 bytes
            buffer[1] = (char)((reg >> 8) & 0xFF);
            buffer[2] = (char)(reg & 0xFF);
            t.length = 24; // Command byte + 2 data bytes = 24 bits
            break;
        case (CB_MB_3 << CB_MB): // 3 bytes
            buffer[1] = (char)((reg >> 16) & 0xFF);
            buffer[2] = (char)((reg >> 8) & 0xFF);
            buffer[3] = (char)(reg & 0xFF);
            t.length = 32; // Command byte + 3 data bytes = 32 bits
            break;
    }

    t.tx_buffer = buffer; // Set the transmit buffer
    spi_device_transmit(spi, &t); // Transmit the transaction
}



uint32_t DAC1220::read_register(uint8_t cmd) {
    uint8_t tx_buffer[4] = {cmd, 0, 0, 0}; // Buffer for command and padding
    uint8_t rx_buffer[4] = {0}; // Buffer to hold received data
    spi_transaction_t t;

    memset(&t, 0, sizeof(t)); // Zero out the transaction
    t.length = 8; // Command byte length in bits
    t.tx_buffer = tx_buffer; // Transmit buffer
    t.rxlength = 0; // Set receive length to 0 initially

    // Determine the number of bytes to read based on the command
    int num_bytes = 0;
    switch (cmd & ((uint8_t)0b11 << CB_MB)) {
        case (CB_MB_1 << CB_MB): // 1 byte
            num_bytes = 1;
            break;
        case (CB_MB_2 << CB_MB): // 2 bytes
            num_bytes = 2;
            break;
        case (CB_MB_3 << CB_MB): // 3 bytes
            num_bytes = 3;
            break;
    }

    if (num_bytes > 0) {
        t.rxlength = num_bytes * 8; // Set receive length in bits
        t.rx_buffer = rx_buffer; // Set receive buffer
        // Send the command byte and read the response
        spi_device_transmit(spi, &t); // Transmit the transaction
    }

    // Construct the value from the received bytes
    uint32_t value = 0;
    for (int i = 0; i < num_bytes; i++) {
        value |= ((uint32_t)rx_buffer[i] << (8 * (num_bytes - 1 - i)));
    }

    return value;
}


/**
 * Generates output voltages specified as left-justified 20-bit numbers
 * within a right-aligned 24-bit (xxFFFFFxh) integer value.
 */
void DAC1220::set_value(uint32_t value) {
  // Assumes 20-bit, straight-binary code.
  set_data_input_register(value);
}

void DAC1220::set_command_register(uint32_t cmr) {
  // Register value set as a right-aligned 16-bit number (xxxxFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_2 << CB_MB) | (CMR_ADR << CB_ADR);
  write_register(cmd, cmr);
}


void DAC1220::set_data_input_register(uint32_t dir) {
  // Register value set as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_3 << CB_MB) | (DIR_ADR << CB_ADR);
  write_register(cmd, dir);
}


void DAC1220::set_offset_calibration_register(uint32_t ocr) {
  // Register value set as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_3 << CB_MB) | (OCR_ADR << CB_ADR);
  write_register(cmd, ocr);
}


void DAC1220::set_full_scale_calibration_register(uint32_t fcr) {
  // Register value set as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_W << CB_RW) | (CB_MB_3 << CB_MB) | (FCR_ADR << CB_ADR);
  write_register(cmd, fcr);
}


uint32_t DAC1220::read_command_register() {
  // Value returned as a right-aligned 16-bit number (xxxxFFFFh).
  uint8_t cmd = (CB_RW_R << CB_RW) | (CB_MB_2 << CB_MB) | (CMR_ADR << CB_ADR);
  return read_register(cmd);
}


uint32_t DAC1220::read_data_input_register() {
  // Value returned as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_R << CB_RW) | (CB_MB_3 << CB_MB) | (DIR_ADR << CB_ADR);
  return read_register(cmd);
}


uint32_t DAC1220::read_offset_calibration_register() {
  // Value returned as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_R << CB_RW) | (CB_MB_3 << CB_MB) | (OCR_ADR << CB_ADR);
  return read_register(cmd);
}


uint32_t DAC1220::read_full_scale_calibration_register() {
  // Value returned as a right-aligned 24-bit number (xxFFFFFFh).
  uint8_t cmd = (CB_RW_R << CB_RW) | (CB_MB_3 << CB_MB) | (FCR_ADR << CB_ADR);
  return read_register(cmd);
}


