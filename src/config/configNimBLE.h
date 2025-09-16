#ifndef CONFIG_NIMBLE_H_
#define CONFIG_NIMBLE_H_

// ---------------------------------------------------------------------------------------
// NimBLE Client, Server related settings -------------------------------------------
#define BLE_APPEARANCE_GENERIC_CYCLING   1152
#define THISDEVICENAME "TACXS" // Shortname 


// ---------------------------------------------------------------------------------------
// Training Apps set MTU after connection from 23 to 255, this allows for max data throughput!
const uint8_t MAX_PAYLOAD = 20;  // Max 20 (23-3) byte size for array DATA and strings
// Preferred Connection Parameters for max data throughput: Client-side (!) AND Server-side !
const uint16_t ConnectionMinInterval = 6; 	// Min interval (7.5ms)  
const uint16_t ConnectionMaxInterval = 12;	// Max interval (15ms)  
const uint16_t ConnectionLatency = 0;   	// No slave latency (immediate response)
const uint16_t ConnectionTimeout = 200; 	// Supervision timeout (2 seconds before disconnect)
const uint16_t ScanInterval = 160;		// Scan every 100ms (160 * 0.625ms)
const uint16_t ScanWindow = 160;		// Active scanning for 100ms (160 * 0.625ms)
const uint16_t ConnectionMTU = 23;     	// ATT (Attribute Protocol) Maximum Transmission Unit
const uint16_t AdvertiseMinInterval = 160;	// 100ms (160 * 0.625ms)
const uint16_t AdvertiseMaxInterval = 240;	// 150ms (240 * 0.625ms)

// ---------------------------------------------------------------------------------------
const bool indications = false;  //false as first argument to subscribe (!) to indications
const bool notifications = true; //true as first argument to subscribe to notifications

// ---------------------------------------------------------------------------------------
// Set Arduino IDE Tools Menu --> Events Run On: "Core 1"
// Set Arduino IDE Tools Menu --> Arduino Runs On: "Core 1"
// xTask constants to set xTasks to run on "Core 0" or "Core 1"
const BaseType_t xTaskCoreID0 = 0;
const BaseType_t xTaskCoreID1 = 1;

#endif // CONFIG_NIMBLE_H_
