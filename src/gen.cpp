#include <Arduino.h>
#include <cstring>
#include <ESPDateTime.h>
#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "driver/timer.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


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

volatile int pcnt_overflow_counter = 0;

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

volatile enum {
	IDLE, COUNTING, IDLE_WAIT
} currentState = IDLE;

bool checkPCNTOverflow() {
	int countValue;

    // Check if there are any new counts and print them
    if (xQueueReceive(countQueue, &countValue, portMAX_DELAY) == pdPASS) {
        Serial.printf("New count: %d\n", countValue);
    }
	if (currentState == IDLE_WAIT) {
		return true;
	}
	return false;
}


#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_IDX TIMER_0



const int period = 10; // Desired counting period in seconds
const int idleWaitPeriod = 5; // Configurable idle wait period in seconds
volatile int secondsElapsed = 0; // Tracks elapsed seconds during COUNTING state


extern "C" void IRAM_ATTR timer_group0_isr(void* para) {
  static BaseType_t xHigherPriorityTaskWoken;
  // FSM logic for controlling the PCNT based on timer interrupts
  switch (currentState) {
    case IDLE:
      // Transition from IDLE to COUNTING
      currentState = COUNTING;
      secondsElapsed = 0; // Reset elapsed time
      pcnt_overflow_counter = 0;
      pcnt_counter_clear(PCNT_UNIT); // Reset the PCNT counter
      pcnt_counter_resume(PCNT_UNIT); // Start counting
      break;
    case COUNTING:
      secondsElapsed++; // Increment the seconds counter
      if (secondsElapsed >= period) {
        // Period has elapsed, stop counting, process the count, then go to IDLE_WAIT
        pcnt_counter_pause(PCNT_UNIT); // Stop counting
        // Process the counted pulses here
        int16_t count = 0;
        pcnt_get_counter_value(PCNT_UNIT, &count); // Get the pulse count
        int finalCount = PCNT_H_LIM_VAL * pcnt_overflow_counter + count + 11;// TODO: this handler 11 uS
        // Reset for the next period
        currentState = IDLE_WAIT;
        secondsElapsed = 0;
        // Send the final count to the queue from ISR
        xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(countQueue, &finalCount, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
          portYIELD_FROM_ISR(); // Ensure the higher priority task is executed immediately
        }
      }
      break;
    case IDLE_WAIT:
      secondsElapsed++;
      if (secondsElapsed >= idleWaitPeriod) {
        // Idle wait period has elapsed, go back to IDLE
        currentState = IDLE;
      }
      break;
  }
  if (TIMER_IDX == TIMER_0) {
    TIMERG0.int_clr_timers.t0 = 1;
  } else if (TIMER_IDX == TIMER_1) {
    TIMERG0.int_clr_timers.t1 = 1;
  }
  TIMERG0.hw_timer[TIMER_IDX].config.alarm_en = TIMER_ALARM_EN;
  timer_count++;
}

void initialize_hardware_timer() {
    // Correctly configure the timer
    timer_config_t config = {};
    config.divider = 80;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TIMER_AUTORELOAD_EN; // Use enum value instead of true

    timer_init(TIMER_GROUP, TIMER_IDX, &config);

    // Set counter and alarm values
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0x00000000ULL);
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, 1000000ULL); // 1 second

    // Enable interrupt and register ISR
    timer_enable_intr(TIMER_GROUP, TIMER_IDX);

	int timer_intr_alloc_flags = ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED;

	timer_isr_register(TIMER_GROUP, TIMER_IDX, timer_group0_isr, 
					   reinterpret_cast<void*>(static_cast<intptr_t>(TIMER_IDX)), timer_intr_alloc_flags, NULL);


    // Start the timer
    timer_start(TIMER_GROUP, TIMER_IDX);
}

