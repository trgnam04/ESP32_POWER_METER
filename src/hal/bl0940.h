#ifndef __BL0940_H_
#define __BL0940_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/
struct STR_BL0940_DATA
{
  float  voltage      = 0.0;
  float  current      = 0.0;
  float  activePower  = 0.0;
  float  activeEnergy = 0.0;
  float  powerFactor  = 0.0;
  float  temperature  = 0.0;
  String dt           = "";
};

/* Variables -----------------------------------------------------------------*/
class BL0940
{
public:
  BL0940();
  ~BL0940();

  bool getVoltage(float* voltage);            //[V]
  bool getCurrent(float* current);            //[A]
  bool getActivePower(float* activePower);    //[W]
  bool getActiveEnergy(float* activeEnergy);  //[kWh]            //[kWh]
  bool getPowerFactor(float* powerFactor);    //[%]
  bool getTemperature(float* temperature);    //[deg C]
  bool getApparentPower(float* apparentPower);
  bool getPhaseAngle(float* phraseAngle);
  bool getReactivePower(float* reactivePower);

  bool setFrequency(uint32_t Hz = 50);                          // 50 or 60  [Hz]
  bool setUpdateRate(uint32_t rate = 400);                      // 400 or 800  [ms]
  bool setOverCurrentDetection(float detectionCurrent = 15.0);  //[A] CF pin is high if current is larger than detectionCurrent
  bool setCFOutputMode();                                       // Enegy pulse output CF pin

  bool reset();

  float voltage       = 0.0;
  float current       = 0.0;
  float activePower   = 0.0;
  float activeEnergy  = 0.0;
  float powerFactor   = 0.0;
  float temperature   = 0.0;
  float energyCF      = 0.0;
  float reactivePower = 0.0;
  float apparentPower = 0.0;
  float energyDelta   = 0.0;
  float phaseAngle    = 0.0;

private:
  const float Vref     = 1.218;  //[V] 1.218
  const float Vref_V   = 1.218;
  const float Vref_I   = 1.218;
  const float Vref_kWh = 1.218 * 1.218;

  const uint16_t timeout    = 1000;  // Serial timeout[ms]
  uint16_t       Hz         = 50;    //[Hz]
  uint16_t       updateRate = 400;   //[ms]


  const float R1 = 0.024;         //[KOhm]
  const float R2 = (20.0 * 5.0);  //[KOhm]

  const float R_DIVIDE_CALIB = 1.0;
  const float R_DIVIDE       = (R2 / R1) * R_DIVIDE_CALIB;

  const float RL_CALIB = 1;
  const float RL       = 1.0 * RL_CALIB;  //[mili Ohm]

  const float FREQUENCY_SAMPLING = 1000000;

  uint8_t _culcCheckSum(uint8_t* txData, int txLenght, uint8_t* rxData, int rxLenght);
  bool    _writeRegister(uint8_t address, uint32_t data);
  bool    _readRegister(uint8_t address, uint32_t* data);

  // My Source

public:
  std::vector<STR_BL0940_DATA> electricity_list;

  uint8_t previous_device_status = 0;
  uint8_t current_device_status  = 0;

  float previous_current      = 0.0;
  float previous_activeEnergy = 0.0;

  // test

  // test

  bool getData(void);
  void restorekWh(void);

private:
  float kWh_restore = 0.0;
  float kWh_pre     = 0.0;

  unsigned long current_millis  = 0;
  unsigned long previous_millis = 0;

  bool setOffset(void);
};

#endif