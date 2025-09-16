/***************************************************************************************************

                    This is programming code for ESP32 Espressif boards
 
                         Tested with - Adafruit Feather ESP32 V2 
                                     - Seeed Studio XIAO ESP32-S3

                 This project builds on the work of other open-source projects.  
                 Many have invested time and resources providing open-source code!
 
                       GPL-3.0 license, check LICENSE for more information!

                      Full attribution and license details are included in:  
               https://github.com/Berg0162/Tacx-Virtual-Shifting/blob/main/NOTICE.txt

***************************************************************************************************

Select and upload the example code to your ESP32 board
 1. Start the Serial Monitor to catch debugging info
 2. Start/Power-On the Tacx Smart Trainer  
 3. Your ESP32 and Trainer will pair
 4. Start Zwift-App on your computer or tablet and wait....
 5. Search on the Zwift pairing screens for your ESP32 a.k.a. <TACXS>
 6. Pair: Power, Controllable and Cadence one after another with <TACXS>
 7. Pair: Controls, your Zwift Click device and optionally others
 8. Pair: Heartrate (optionally)
 9. Select any Zwift ride you like
10. Make Serial Monitor output window visible on top of the Zwift window 
11. Hop on the bike: do the work and feel resistance change when shifting and with road inclination
12. Inspect the info presented by Serial Monitor.....
 
This device is identified with the name <TACXS>. You will see this only when connecting to Zwift on 
the pairing screens! Notice: Zwift extends device names with additional numbers for identification!

****************************************************************************************************/

// -------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate
#define DEBUG

// --------------------------------------------------------------------------------------------
#include <TacxVirtualShifting.h>

void setup() {
#ifdef DEBUG  
    Serial.setRxBufferSize(96); // Increase RX buffer size
    Serial.begin(115200);       
    while ( !Serial ) delay(10); 
    Serial.flush();
    delay(1000); // Give Serial I/O time to settle
#endif
    LOG("ESP32 NimBLE Tacx Legacy Virtual Shifting");
    LOG("--------------- Version %s --------------", CODE_VERSION);
    delay(200);
#ifdef TACXNEO_FIRSTGENERATION
    LOG(" -> Tacx Neo First Generation modifications active!");
#endif
    
  // Init NimBLEManager 
  BLEmanager->init();
  // Start scanning for a trainer
  BLEmanager->startScanning();

  // Wait until the Peripheral/Trainer is successfuly connected or has a timeout
  const long TIMEOUT = millis() + 10000; // Within 10 seconds it should have found a Tacx trainer!
  while(!BLEmanager->clientIsConnected) {
    delay(100);
    if(millis() > TIMEOUT) {
      //LOG(">>> Scanning for Peripheral/Trainer --> Timeout!");
      break;
    }
  }

  // TacxS is connected with the Trainer, start advertising TacxS for Zwift connection!
  BLEmanager->startAdvertising();
}

void loop() { }