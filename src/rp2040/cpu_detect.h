
#ifndef CPUDETECT_H
#define CPUDETECT_H CPUDETECT_H

typedef enum {
   CPU_UNDEF, CPU_6502, CPU_65C02, CPU_65816
} cputype_t;

cputype_t cpu_detect();
const char *cputype_name( cputype_t cputype );

#endif

