# Central configuration of Tacx-Virtual-Shifting

# configNimBLE.h
Make configNimBLE.h settings to comply with your <b>Legacy Tacx FE-C</b> smart trainer.

+ Open file, edit and save(!): `/documents/arduino/libraries/Tacx-Virtual-Shifting/src/config/configNimBLE.h`

Tacx-Virtual-Shifting <b>NimBLE-Arduino</b> configuration options:
```C++
// ---------------------------------------------------------------------------------------
// Training Apps set MTU after connection from 23 to 255, this allows for max data throughput!
const uint8_t MAX_PAYLOAD = 20;  // Max 20 (23-3) byte size for array DATA and strings
// Preferred Connection Parameters for max data throughput: Client-side (!) AND Server-side !
const uint16_t ConnectionMinInterval = 6; 	// Min interval (7.5ms)  
const uint16_t ConnectionMaxInterval = 12;	// Max interval (15ms)  
const uint16_t ConnectionLatency = 0;   	// No slave latency (immediate response)
const uint16_t ConnectionTimeout = 200; 	// Supervision timeout (2 seconds before disconnect)
const uint16_t ScanInterval = 160;		    // Scan every 100ms (160 * 0.625ms)
const uint16_t ScanWindow = 160;		    // Active scanning for 100ms (160 * 0.625ms)
const uint16_t ConnectionMTU = 23;     	    // ATT (Attribute Protocol) Maximum Transmission Unit
const uint16_t AdvertiseMinInterval = 160;	// 100ms (160 * 0.625ms)
const uint16_t AdvertiseMaxInterval = 240;	// 150ms (240 * 0.625ms)
```

# configDebug.h
Tacx-Virtual-Shifting has a fine-grained scheme for allowing debug-messages during operation. Every `/src/<file name>.cpp` has its own <b>#define DEBUG</b> at the beginning of the code that can be (un)commented (switched on or off) discretionary. However, when one reaches the state that debugging has no longer a purpose, you want to get rid of all the overhead (processor load) and optimise for speed. This `configDebug.h` allows you to switch OFF in one place <b>all Tacx-Virtual-Shifting debugging messages</b> with one <b>master switch</b>.<br>

Make configDEBUG.h settings to comply with the status of your <b>Tacx-Virtual-Shifting</b> setup.

+ Open file, edit and save(!): `/documents/arduino/libraries/Tacx-Virtual-Shifting/src/config/configDEBUG.h`

Tacx-Virtual-Shifting <b>Debug</b> configuration options:

```C++
// ------------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress ALL DEBUG messages that help to debug code sections
// Uncomment general "#define GLOBAL_DEBUG" to activate the Tacx-Virtual-Shifting debug master switch...
#define GLOBAL_DEBUG

```

# configTacx.h
Make configTacx.h settings to comply with your <b>Legacy Tacx FE-C</b> smart trainer.

+ Open file, edit and save(!): `/documents/arduino/libraries/Tacx-Virtual-Shifting/src/config/configTacx.h`

+ Set your <b>Tacx</b>-configuration for use with <b>Tacx-Virtual-Shifting</b>:

```C++
//-----------------------------------------------------------------------------------------
  // TACX Neo 1 has a specific "flaw": trainer firmware detection of Cadence is UNRELIABLE! 
  // see for example: https://www.trainerroad.com/forum/t/tacx-neo-1-cadence-issue/14795
  // Uncomment to allow a specific cadence filter for Tacx Neo 1:  
  // - Eliminates nearly all false cadence spikes.  
  // - Handles startup acceleration naturally, without false suppression.  
  // - Preserves real cadence changes, even during sprints.  
  // - Removes the disruptive resistance jumps in Zwift Virtual Shifting.  
  // - Adds minimal complexity and no extra latency.  
#define TACXNEO_FIRSTGENERATION

// To calculate the speed we assume a default wheel diameter of 0.7m and take the current 
// cadence from the trainer
#define WHEELDIAMETER 0.7  // Wheel diameter in m

// Internal "difficulty" setting used to modify some calculations
#define DIFFICULTY 100  // Difficulty in percentage (0-200%)

// As the Zwift Cog has 14 teeth and a standard chainring 34 teeth the default ratio is 
// defined at 2.4 which roughly matches Zwift Gear 12
// Calculated as Chainring/Sprocket --> 2.4 equals gear 12    
// My setup: 34/17 -> 2.0 -> gear 10
#define DEFAULTGEARRATIO 2.4  // Default gear ratio of the bike (chainring / sprocket)
```

<b>Warning</b>: When a new version of **Tacx-Virtual-Shifting** is installed in Arduino IDE 2 it will override <b>ALL</b> files of a previous version! If you have made modifications in a file that is part of `Tacx-Virtual-Shifting` --> <b>Make</b> a <b>copy</b> of the file(s) in question <b>BEFORE</b> you <b>install</b> a new library version!
