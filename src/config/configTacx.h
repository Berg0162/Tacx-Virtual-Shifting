#ifndef CONFIG_TACX_H
#define CONFIG_TACX_H

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
#define DEFAULTGEARRATIO 2.4  // Default gear ratio of the bike (chainring / sprocket)

//-------------------------------------------------------------------------------------------
// Uncomment to allow for haptic feedback 
// When a virtual shift is triggered (for example via Zwift Click), the trainer briefly responds 
// with a short vibration or resistance “tap”. This behavior mimics the feedback found on newer 
// Tacx trainers that received the official Virtual Shifting firmware update.
#define TACXNEO_HAPTIC_FEEDBACK

#endif // CONFIG_TACX_H