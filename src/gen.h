#pragma once

extern void signal_gen(uint32_t freq_hz=1000000, int gpio=17);
extern void initializePCNT();
extern bool checkPCNTOverflow();
extern void initialize_hardware_timer();
