#include <cstdint>
#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <string>
// Include specific settings of the Tacx trainer
#include "config/configTacx.h"

class UTILS {
 public:
  static double calculateGravitationalResistance(double totalWeight, double grade);
  static double calculateRollingResistance(double totalWeight);
  static double calculateWindResistance(double bicycleSpeed, double windSpeed);
  static double calculateGearedResistance(double totalWeight, double grade, double speed, double gearRatio);
  static uint16_t calculateFECTrackResistanceGrade(double totalWeight, double grade, double measuredSpeed, \
                                                                          uint8_t cadence, double gearRatio);
  static double calculateSpeed(uint8_t cadence, double gearRatio);
  static double calculateGearedValue(double originalValue, double gearRatio);
  static double calculateRelativeGearRatio(double gearRatio);
  static int getGearNumberFromRatio(float gearRatio);

  static void logF(const char* format, ...);
  static std::string getHexString(const uint8_t* data, size_t length);
  static std::string getHexString(const std::string& str);
  static std::string getHexString(std::vector<uint8_t> data);
  static std::string getHexString(std::vector<uint8_t>* data);
  static void ConvertMacAddress(char *fullAddress, uint8_t addr[6], bool NativeFormat);

#ifdef TACXNEO_FIRSTGENERATION
  static uint8_t medianFilter(uint8_t newVal);
  static uint8_t getAllowedJumpPercent(uint8_t lastCad, uint16_t lastPower);
  static uint8_t getFilteredCadence(uint8_t rawCadence, uint16_t instantPower);
#endif

  static double const pi;
  static double const gravity;
  static double rollingResistanceCoefficient;
  static void setTrainerCrr(double trainerCrr);
  static double windResistanceCoefficient;
  static void setTrainerCw(double trainerCw);
  static double const wheelDiameter;    // Wheel diameter in m
  static uint16_t const difficulty;     // INTERNAL Difficulty in percentage (0-200%)
  static double const defaultGearRatio; // Default gear ratio of the bike (chainring / sprocket)
  static const std::vector<float> knownRatios;
  enum class TrainerMode : uint8_t {
        ERG_MODE = 0,
        SIM_MODE = 1,
        SIM_MODE_VIRTUAL_SHIFTING = 2
    };

  static const uint16_t FEC_ZERO;
  static const int16_t MAX_FEASIBLE_GRADE;
  static const int16_t MIN_FEASIBLE_GRADE;  

 private:
};

#endif