#include "Utilities.h"
#include <cmath>

// -------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate
//#define DEBUG
// Include these debug utility macros in all cases!
#include "config/configDebug.h"
// --------------------------------------------------------------------------------------------

double const UTILS::pi = 3.14159;
double const UTILS::gravity = 9.81;
double UTILS::rollingResistanceCoefficient = 0.00415; // defaul: Asphalt Road
double UTILS::windResistanceCoefficient = 0.51; // Road bike + touring position
double const UTILS::wheelDiameter = WHEELDIAMETER;
uint16_t const UTILS::difficulty = DIFFICULTY;
double const UTILS::defaultGearRatio = DEFAULTGEARRATIO;
const uint16_t UTILS::FEC_ZERO = 0x4E20; // 0x4E20 = 20000 = 0%
const int16_t UTILS::MAX_FEASIBLE_GRADE = 2000; // +20.00%
const int16_t UTILS::MIN_FEASIBLE_GRADE = -200; //  –2.00% Downhill handling because:
// 1)Avoid excessive trainer acceleration, 2)Prevent totally "free" coasting and 3)Keep workouts productive

// Known gear ratios for gears 1 to 24
const std::vector<float> UTILS::knownRatios = {0.75, 0.87, 0.99, 1.11, 1.23, 1.38, 1.53, 1.68, 1.86, 2.04, 2.22, 2.40, 2.61, 2.82, \
                                              3.03, 3.24, 3.49, 3.74, 3.99, 4.24, 4.54, 4.84, 5.14, 5.49};

#ifdef TACXNEO_FIRSTGENERATION
// Median Filter Over Small Window
#define MEDIAN_WINDOW 5 // Sample Window of 5 can crush 2 subsequent spikes
uint8_t UTILS::medianFilter(uint8_t newVal) {
	static uint8_t cadenceBuffer[MEDIAN_WINDOW] = {0};
	static uint8_t cadenceIndex = 0;
    cadenceBuffer[cadenceIndex] = newVal;
    cadenceIndex = (cadenceIndex + 1) % MEDIAN_WINDOW;

    uint8_t sorted[MEDIAN_WINDOW];
    memcpy(sorted, cadenceBuffer, MEDIAN_WINDOW);
    std::sort(sorted, sorted + MEDIAN_WINDOW);
    return sorted[MEDIAN_WINDOW / 2];
}

// Adaptive Threshold Determination
#define MAX_JUMP_PERCENT 40     // default for steady power and cadence
#define LOW_POWER_THRESHOLD 120 // watts, tune based on your rides
uint8_t UTILS::getAllowedJumpPercent(uint8_t lastCad, uint16_t lastPower) {
    // Startup zone — allow anything
    if(lastCad < 30) return 200;    // allow huge jump (startup)
    if(lastCad < 60) return 100;    // allow 2x increase (still spinning up)

    // Known "spike zone": low power in combination with low/moderate 65–85 rpm 
    if(lastPower < LOW_POWER_THRESHOLD) {
            return 15;  // VERY strict when low power
        }

    // Steady power and cadence
    return MAX_JUMP_PERCENT;
}

/*
How this filter behaves:
	- Eliminates nearly all false cadence spikes.
	- Handles startup acceleration naturally, without false suppression.
	- Preserves real cadence changes, even during sprints.
	- Removes the disruptive resistance jumps in Zwift Virtual Shifting.
	- Adds minimal complexity and no extra latency.
*/
uint8_t UTILS::getFilteredCadence(uint8_t rawCadence, uint16_t instantPower){
  static uint8_t lastValidCadence = 0;
  
  if(instantPower == 0) {
        rawCadence = 0;
        lastValidCadence = 0;
  } else {

        // Median filter → cleans short subsequent cadence spikes
        rawCadence = UTILS::medianFilter(rawCadence);
		
        // Reject dynamically, sudden cadence jumps vs last valid cadence
        if(lastValidCadence > 0) {
			uint8_t dynamicJump = UTILS::getAllowedJumpPercent(lastValidCadence, instantPower);
            uint8_t allowedMax = lastValidCadence + (lastValidCadence * dynamicJump / 100);
            uint8_t allowedMin = (lastValidCadence > (lastValidCadence * dynamicJump / 100)) \
                                     ? lastValidCadence - (lastValidCadence * dynamicJump / 100) : 0;

            if(rawCadence > allowedMax || rawCadence < allowedMin) {
                rawCadence = lastValidCadence;  // clamp
            }
        }
		
        // Save for next iteration
        lastValidCadence = rawCadence;
    }
  return rawCadence;
}
#endif

void UTILS::setTrainerCrr(double trainerCrr) {
  UTILS::rollingResistanceCoefficient = trainerCrr;
}

void UTILS::setTrainerCw(double trainerCw){
  UTILS::windResistanceCoefficient = trainerCw;
}

void UTILS::logF(const char* format, ...) {
    static char buffer[256];  // Static buffer to prevent stack overflows
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer) - 2, format, args);  // Leave space for \n and \0
    va_end(args);

    if (len > 0) {
        buffer[len] = '\n';   // Append newline
        buffer[len + 1] = '\0'; // Null-terminate
    }

    Serial.print(buffer);  // Send buffer in one atomic operation
//    Serial.flush();        // Ensure output is fully written before continuing
    delay(2);              // Small delay to stabilize serial output
}

std::string UTILS::getHexString(const uint8_t* data, size_t length) {
  static char hexNumber[3];
  std::string hexString = "[";
  for (size_t index = 0; index < length; index++) {
    sprintf(hexNumber, "%02X", data[index]);
    hexString.append(hexNumber);
    if (index < (length - 1)) {
      hexString.append(" ");
    }
  }
  hexString.append("]");
  return hexString;
}

std::string UTILS::getHexString(const std::string& str) {
      return getHexString( reinterpret_cast<const uint8_t*>(str.data()), static_cast<uint8_t>(str.length()) );
}

std::string UTILS::getHexString(std::vector<uint8_t> data) {
  return getHexString(data.data(), data.size());
}

std::string UTILS::getHexString(std::vector<uint8_t>* data) {
  return getHexString(data->data(), data->size());
}

void UTILS::ConvertMacAddress(char *fullAddress, uint8_t addr[6], bool NativeFormat)
{ // Display byte by byte in HEX 
  if(NativeFormat) { // Unaltered: representation
    sprintf(fullAddress, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2], \
      addr[3], addr[4], addr[5], HEX);   
  } else { // Altered: in reversed order
    sprintf(fullAddress, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], \
      addr[2], addr[1], addr[0], HEX);       
  }
}

int UTILS::getGearNumberFromRatio(float gearRatio) {
    const float epsilon = 0.01; // tolerance for floating-point comparison
    for (size_t i = 0; i < knownRatios.size(); ++i) {
        if (std::fabs(gearRatio - knownRatios[i]) < epsilon) {
            return static_cast<int>(i + 1); // gears are 1-based
        }
    }
    return 0; // Not found
}

/**
 * Calculates the gravitational resistance based on the total weight and the grade
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @return Gravitational resistance in N
 */
double UTILS::calculateGravitationalResistance(double totalWeight, double grade) {
  double gravitationalResistance = gravity * sin(atan(grade/100)) * totalWeight;
  LOG("Gravitational resistance: %f", gravitationalResistance);
  return gravitationalResistance;
}

/**
 * Calculates the rolling resistance based on the total weight
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @return Rolling resistance in N
 */
double UTILS::calculateRollingResistance(double totalWeight) {
  double rollingResistance = gravity * totalWeight * rollingResistanceCoefficient;
  LOG("Rolling resistance: %f", rollingResistance);
  return rollingResistance;
}

/**
 * Calculates the wind resistance based on the bicycle and wind speed
 * 
 * @param bicycleSpeed Speed of bicycle in m/s
 * @param windSpeed Speed of wind in m/s (positive for headwind, negative for tailwind)
 * @return Wind resistance in N
 */
double UTILS::calculateWindResistance(double bicycleSpeed, double windSpeed) {
  double windResistance = 0.5 * windResistanceCoefficient * pow(bicycleSpeed + windSpeed, 2);
  LOG("Wind resistance: %f", windResistance);
  return windResistance;
}

/**
 * Calculates the total resistance
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param speed Speed of bicycle in m/s
 * @param gearRatio Requested gear ratio
 * @return Total resistance in N
 */
double UTILS::calculateGearedResistance(double totalWeight, double grade, double speed, double gearRatio) {
  // calculate the relative gear ratio
  double relativeGearRatio = calculateRelativeGearRatio(gearRatio);

  // calculate the wind resistance based on speed and gear ratio
  double windResistance = calculateWindResistance(speed, 0);

  // calculate the gravitational and rolling resistance 
  double gravitationalResistance = calculateGravitationalResistance(totalWeight, grade);
  double rollingResistance = calculateRollingResistance(totalWeight);

  // apply the difficulty to the gravitational and wind resistance
  gravitationalResistance = gravitationalResistance * (difficulty / 100.0);
  windResistance = windResistance * (difficulty / 100.0);

  // calculate the geared total resistance (that would apply with the specified gear ratio)
  double gearedTotalResistance = calculateGearedValue(gravitationalResistance + rollingResistance, relativeGearRatio) + windResistance;
  LOG("Geared resistance: %fN (gravitational: %fN, rolling: %fN, wind: %fN, speed: %fm/s = %fkm/h, difficulty: %d)", gearedTotalResistance, \
                                                  gravitationalResistance, rollingResistance, windResistance, speed, speed * 3.6, difficulty);
  return gearedTotalResistance;
}


/**
 * Calculates the FE-C track resistance grade for the simulation
 * 
 * @param totalWeight Total weight of the bike and the rider in kg
 * @param grade Grade in percentage
 * @param measuredSpeed Measured speed from trainer in m/s
 * @param cadence Cadence in RPM
 * @param gearRatio Requested gear ratio
 * @return FE-C resistance grade in 0.01% (0x4E20 = 0%)
 */
uint16_t UTILS::calculateFECTrackResistanceGrade(double totalWeight, double grade, double measuredSpeed, uint8_t cadence, double gearRatio) {

  // calculate the virtual Speed based on cadence, wheel diameter and gear ratio
  double virtualSpeed = calculateSpeed(cadence, gearRatio); // Gear-based motion

  // calculate the wind resistance (including internal difficulty) based on actual flywheel Speed
  double windResistance = calculateWindResistance(measuredSpeed, 0); // Based on true movement
  windResistance = windResistance * (difficulty / 100.0);

  // calculate the rolling resistance
  double rollingResistance = calculateRollingResistance(totalWeight);

  // calculate the geared total resistance
  double gearedTotalResistance = calculateGearedResistance(totalWeight, grade, virtualSpeed, gearRatio);

  // calculate the geared grade based on the theoretical gravitational resistance
  double theoreticalGravitationalResistance = gearedTotalResistance - windResistance - rollingResistance;
  double gravityForce = totalWeight * gravity; 
  double slopeRatio = std::clamp((theoreticalGravitationalResistance / gravityForce), -1.0, 1.0);
  double calculatedGrade = tan(asin(slopeRatio)) * 100;

  int16_t fecGrade = static_cast<int16_t>(calculatedGrade * 100.0);  // In 0.01% units
  uint16_t trackResistanceGrade = FEC_ZERO + fecGrade;
  // Clamp to reasonable ANT+ FE-C limits
  trackResistanceGrade = std::clamp(trackResistanceGrade, static_cast<uint16_t>(FEC_ZERO + MIN_FEASIBLE_GRADE), \
                                                              static_cast<uint16_t>(FEC_ZERO + MAX_FEASIBLE_GRADE));
  LOG("FE-C Track resistance grade: %d (%f%)", trackResistanceGrade, calculatedGrade);
  return trackResistanceGrade;
}

/**
 * Calculates the speed based on the cadence, wheel diameter and gear ratio
 * 
 * @param cadence Cadence in RPM
 * @param gearRatio Gear ratio
 * @return Speed in m/s
 */
double UTILS::calculateSpeed(uint8_t cadence, double gearRatio) {
  return ((cadence * gearRatio) * (wheelDiameter * pi) / 60);
}

/**
 * Calculates the geared value based on the relative gear ratio
 * 
 * @param originalValue Original value (unit doesn't matter)
 * @param gearRatio Gear ratio
 * @return Geared value (unit as original value)
 */
double UTILS::calculateGearedValue(double originalValue, double gearRatio) {
  double gearedValue = originalValue * gearRatio;
  if (originalValue < 0) {
    gearedValue = originalValue * (1 / gearRatio);
  }
  LOG("Original / Ratio / Geared: %f / %f / %f", originalValue, gearRatio, gearedValue);
  return gearedValue;
}

/**
 * Calculates the relative gear ratio based on the software requested and hardware installed gear ratio
 * 
 * @param gearRatio Requested gear ratio
 * @return Relative gear ratio
 */
double UTILS::calculateRelativeGearRatio(double gearRatio) {
  double relativeGearRatio = gearRatio / defaultGearRatio;
  LOG("Relative gear ratio: %f (%f / %f)", relativeGearRatio, gearRatio, defaultGearRatio);
  return relativeGearRatio;
}