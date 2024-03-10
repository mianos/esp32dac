#include <Arduino.h>
#include <cstring>
#include <ESPDateTime.h>
#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "gen.h"

void signal_gen(uint32_t freq_hz, int gpio) {
    // Configure the timer
    ledc_timer_config_t ledc_timer;
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_1_BIT;
    ledc_timer.timer_num = LEDC_TIMER_0;
    ledc_timer.freq_hz = freq_hz; // Frequency of PWM signal
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel;
    memset(&ledc_channel, 0, sizeof(ledc_channel_config_t));
    ledc_channel.speed_mode     = LEDC_HIGH_SPEED_MODE,
    ledc_channel.channel        = LEDC_CHANNEL_0,
    ledc_channel.intr_type      = LEDC_INTR_DISABLE,
    ledc_channel.timer_sel      = LEDC_TIMER_0,
    ledc_channel.gpio_num       = gpio,
    ledc_channel.duty           = 1, // Set duty to 0%
    ledc_channel_config(&ledc_channel);
}


QueueHandle_t countQueue;


#define PCNT_INPUT_SIG_IO   22 // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  -1 // Control GPIO (not used, hence -1)
#define PCNT_CHANNEL        PCNT_CHANNEL_0
#define PCNT_UNIT           PCNT_UNIT_0
#define PCNT_H_LIM_VAL      25000
#define PCNT_L_LIM_VAL      -1

volatile static int pcnt_overflow_counter = 0;

static void IRAM_ATTR pcnt_intr_handler(void *arg) {
    pcnt_overflow_counter++;
//    pcnt_counter_clear(PCNT_UNIT); // Reset counter
}

void initializePCNT() {

	countQueue = xQueueCreate(10, sizeof(int));
    if (countQueue == NULL) {
        Serial.println("Queue create failed");
    	return;
    }
    pcnt_config_t pcnt_config;
    pcnt_config.pulse_gpio_num = PCNT_INPUT_SIG_IO;
    pcnt_config.ctrl_gpio_num = PCNT_INPUT_CTRL_IO;
    pcnt_config.channel = PCNT_CHANNEL;
    pcnt_config.unit = PCNT_UNIT;
    pcnt_config.pos_mode = PCNT_COUNT_INC;   // Count up on the positive edge
    pcnt_config.neg_mode = PCNT_COUNT_DIS;   // Keep the counter value on the negative edge
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP; // Keep the primary counter mode if low
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP; // Keep the primary counter mode if high
    pcnt_config.counter_h_lim = PCNT_H_LIM_VAL;
    pcnt_config.counter_l_lim = PCNT_L_LIM_VAL;

    pcnt_unit_config(&pcnt_config);

//    pcnt_set_filter_value(PCNT_UNIT, 1023); // Optionally enable the noise filter
//    pcnt_filter_enable(PCNT_UNIT);

    // Install PCNT driver
    pcnt_isr_service_install(ESP_INTR_FLAG_IRAM); // 0 - default intr_alloc_flags
    pcnt_isr_handler_add(PCNT_UNIT, pcnt_intr_handler, NULL); // Add ISR handler

	pcnt_event_enable(PCNT_UNIT, PCNT_EVT_H_LIM);

    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);
    // pcnt_counter_resume(PCNT_UNIT);
	Serial.printf("PCNT init on %d\n", PCNT_INPUT_SIG_IO);
}

volatile int timer_count = 0;

static int period = 10; // Desired counting period in seconds
volatile static enum CurrentState_t currentState = IDLE;

void	SetState(CurrentState_t newState) {
	currentState = newState;
}

void runTest(int seconds) {
	if (currentState != IDLE) {
		Serial.printf("State is not idle\n");
		return;
	}
	period = seconds;
	currentState = START;
	Serial.printf("started test\n");
}

enum CurrentState_t checkPCNTOverflow() {
  CurrentState_t newState;

  while (xQueueReceive(countQueue, &newState, 0) == pdPASS) { 
    Serial.printf("State changed to: %d\n", newState);

    if (newState == IDLE) { // Assuming count retrieval makes sense only after IDLE_WAIT
      int16_t count = 0;
      pcnt_get_counter_value(PCNT_UNIT, &count);
      int totalCounts = PCNT_H_LIM_VAL * pcnt_overflow_counter + count + 11; // Replace '11' with actual adjustment
      Serial.printf("New count: %d scaled %7.2f\n", totalCounts, (double)totalCounts / (double)period);
    } 
  }

  return currentState; 
}


int secondsElapsed = 0;


extern "C" void IRAM_ATTR one_pps_handler(void* para) {
  static BaseType_t xHigherPriorityTaskWoken;

  switch (currentState) {
    case STOPPED:
      currentState = IDLE;
      xQueueSendFromISR(countQueue, (const void*) &currentState, &xHigherPriorityTaskWoken);  // still send IDLE, STOP never seen
      break;

	case IDLE:
	  break;
	
	case START:
      secondsElapsed = 0;
      pcnt_overflow_counter = 0;
      pcnt_counter_clear(PCNT_UNIT);
      pcnt_counter_resume(PCNT_UNIT);
	  currentState = COUNTING;
      xQueueSendFromISR(countQueue, (const void*) &currentState, &xHigherPriorityTaskWoken); 
	  break;

    case COUNTING:
      secondsElapsed++;
      if (secondsElapsed >= period) {
        pcnt_counter_pause(PCNT_UNIT);
        currentState = STOPPED;
		xQueueSendFromISR(countQueue, (const void*) &currentState, &xHigherPriorityTaskWoken); 
      }
      break;

  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR(); 
  }
}

#define GPIO_INPUT_PIN 15
#define GPIO_INPUT_PIN_MASK (1ULL<<GPIO_INPUT_PIN)
#define ESP_INTR_FLAG_DEFAULT 0

void initialize_hardware_timer() {
    // Configure GPIO 15 as input
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Set interrupt on rising edge
    io_conf.mode = GPIO_MODE_INPUT; // Set as input mode
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_MASK; // Bitmask for GPIO 15
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Enable pull-up
    gpio_config(&io_conf);

    // Install GPIO ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT); // Install ISR service with default flags

    // Attach the interrupt service routine
	gpio_isr_handler_add(static_cast<gpio_num_t>(GPIO_INPUT_PIN), one_pps_handler, NULL); // Cast GPIO number here if necessary

}

