#ifndef TDSREADINGS_H
#define TDSREADINGS_H

#include <RTClib.h>
#include <MPU6050.h>

extern MPU6050 mpu;
extern RTC_DS3231 rtc;

void readAndLogTDS();

#endif
