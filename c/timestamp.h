#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdlib.h>
#include <xstatus.h>
#include <xil_printf.h>

void timestamps_initialize(u32 **arrayPointer);

void timestamps_start(u32 **arrayPointer);

void timestamps_stop();

#endif
