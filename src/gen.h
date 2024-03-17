#pragma once

enum CurrentState_t {
	STOPPED, IDLE, COUNTING, START
};

extern void signal_gen(uint32_t freq_hz=1000000, int gpio=17);
extern void initializePCNT();
extern enum CurrentState_t checkPCNTOverflow();
extern void initialize_hardware_timer();
extern void	SetState(CurrentState_t newState);
extern void runTest(int seconds);
extern int get_LastTestCount();

struct TestConfig {
  int period = 10;
  int test_id = 0;
  int repeat = 1;
};

extern QueueHandle_t testQueue;
