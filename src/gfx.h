#pragma once
#include <LovyanGFX.hpp>

// LGFX for TTGO T-Display
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

public:

  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();

      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 14000000;
      cfg.spi_3wire  = true;
      cfg.use_lock   = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = 18;
      cfg.pin_mosi = 19;
      cfg.pin_miso = -1;
      cfg.pin_dc   = 16;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _light_instance.config();

      cfg.pin_bl = 4;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs           =     5;
      cfg.pin_rst          =    23;
      cfg.pin_busy         =    -1;

      cfg.panel_width      =   135;
      cfg.panel_height     =   240;
      cfg.offset_x         =    52;
      cfg.offset_y         =    40;
      cfg.offset_rotation  =     0;
      cfg.dummy_read_pixel =    16;
      cfg.dummy_read_bits  =     1;
      cfg.readable         =  true;
      cfg.invert           =  true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       =  true;

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};



class GFX {
  LGFX tft;

public:
  GFX() {
    tft.init();
    tft.setRotation(1); // Adjust rotation according to your display's orientation
    tft.fillScreen(TFT_BLACK); // Clear the screen
  }

  void displayVoltage(float voltage) {
    // Choose a built-in font
    tft.setFont(&fonts::Font6); // Example to set a built-in font, adjust based on available fonts

    // Calculate font size multiplier to make text larger
    int fontSize = 1; // Adjust this value to make the font larger or smaller

    // Format the voltage to a string
    char voltageStr[10];
    sprintf(voltageStr, "%+6.3f", voltage); // Format the voltage value as a string

    // Set text attributes
    tft.setTextDatum(MC_DATUM); // Set datum to middle center for easy positioning
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set text color
    tft.setTextSize(fontSize); // Set the calculated font size

    // Clear the screen to avoid overwriting
    tft.fillScreen(TFT_BLACK);

    // Determine the display center
    int centerX = tft.width() / 2;
    int centerY = tft.height() / 2;

    // Display the voltage
    tft.drawString(voltageStr, centerX, centerY);
  }
};

