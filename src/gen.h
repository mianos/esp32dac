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
