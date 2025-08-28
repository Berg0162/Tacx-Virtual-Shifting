#ifndef TACXVIRTUALSHIFTING_H
#define TACXVIRTUALSHIFTING_H

static const char* CODE_VERSION = "1.0.0";

#include <NimBLEDevice.h>
#include <nimble/nimble/host/services/gap/include/services/gap/ble_svc_gap.h>

// Include the debug utility macros in all cases!
#include "config/configDebug.h"

// ------------------------------------------------------------------------------------------------
// Include all NimBLE related constants
#include "config/configNimBLE.h"

// ------------------------------------------------------------------------------------------------
// Include Tacx trainer specific settings
#include "config/configTacx.h"

// ------------------------------------------------------------------------------------------------
// Include some general component
#include "Utilities.h"

// ------------------------------------------------------------------------------------------------
// Include NimBLE Manager class
#include "NimBLEManager.h"
NimBLEManager* BLEmanager = NimBLEManager::getInstance();

#endif // TACXVIRTUALSHIFTING_H