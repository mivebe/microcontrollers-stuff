// Over-the-air updates
//
// The new firmware goes into the spare app slot. After the reboot it is on probation: it only
// becomes permanent once it reaches the broker (markFirmwareGood). If it crashes, hangs or
// reboots before that (including the 10 min offline reboot), the bootloader goes back to the
// previous firmware.
#pragma once
#include <Arduino.h>

void requestOta(const String &url);  // from the "update" command; the download runs in otaLoop()
void otaLoop();
void markFirmwareGood();     // call once connected to the broker
bool firmwareOnProbation();  // true after an update until markFirmwareGood()
void reportRollback();       // at boot: log if an earlier update was rolled back
