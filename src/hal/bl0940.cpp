/* Includes ------------------------------------------------------------------*/
#include "bl0940.h"


/* Define --------------------------------------------------------------------*/
#define BL0940_DEBUG 1
#if BL0940_DEBUG
#define DBG(...)                 \
  {                              \
    \
  }
#define ERR(...)                 \
  {                              \
    \
  }
#else
#define DBG(...)
#define ERR(...)
#endif /* BL0940_DBG */

#define PIN_CS   15  // Chip Select (CS) Pin
#define PIN_CLK  14  // Clock (CLK) Pin
#define PIN_MOSI 13  // Master Out Slave In (MOSI) Pin
#define PIN_MISO 12  // Master In Slave Out (MISO) Pin

/* Variables -----------------------------------------------------------------*/
uint32_t  bl0940_RMSOS                            = 0x00000000;
uint32_t  bl0940_WATTOS                           = 0x00000000;
uint32_t  bl0940_WA_CREEP                         = 0x00000000;
uint16_t  reset_count                             = 0;
float     bl0940_kWh                              = 0.0;

float     bl0940_R_DIVIDE_CALIB                   = 1.0;
float     bl0940_RL_CALIB                         = 1.0;


/* Functions -----------------------------------------------------------------*/
BL0940::BL0940()
{
  // Set up SPI bus
  SPI.begin(PIN_CLK, PIN_MISO, PIN_MOSI, PIN_CS);  
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);

  delay(500);

  this->reset();
  this->setFrequency(50);
  this->setUpdateRate(800);
  this->setOffset();

  Serial.println("bl: \t [init] ");
}

BL0940::~BL0940()
{
  SPI.end();
  Serial.println("bl: \t [end]");
}

uint8_t BL0940::_culcCheckSum(uint8_t* txData, int txLenght, uint8_t* rxData, int rxLenght)
{
  uint8_t checksum = 0;
  for (int i = 0; i < txLenght; i++)
  {
    checksum += txData[i];
  }
  for (int i = 0; i < rxLenght; i++)
  {
    checksum += rxData[i];
  }
  checksum = ~checksum;
  return checksum;
}

bool BL0940::_writeRegister(uint8_t address, uint32_t data)
{
  // Register Unlock
  uint8_t unlockTxData[6] = {0xA8, 0x1A, 0, 0, 0x55, 0};
  unlockTxData[5]         = ~((0xA8 + 0x1A + 0 + 0 + 0x55) & 0xFF);

  SPI.transfer(unlockTxData, sizeof(unlockTxData));
  SPI.endTransaction();

  // Write Register
  uint8_t txData[6] = {0xA8, address, (uint8_t)data, (uint8_t)(data >> 8), (uint8_t)(data >> 16)};
  txData[5]         = ~((0xA8 + txData[1] + txData[2] + txData[3] + txData[4]) & 0xFF);

  SPI.transfer(txData, sizeof(txData));
  SPI.endTransaction();

  return true;
}

bool BL0940::_readRegister(uint8_t address, uint32_t* data)
{
  uint8_t txData[]  = {0x58, address};
  uint8_t rxData[4] = {0, 0, 0, 0};

  SPI.transfer(txData, sizeof(txData));
  // delayMicroseconds(10);
  SPI.transfer(rxData, sizeof(rxData));

  uint8_t checksum = ~((0x58 + address + rxData[0] + rxData[1] + rxData[2]) & 0xFF);

  if (rxData[3] != checksum)
  {
    char message[128];
    sprintf(message, "bl: \t [read] checksum error (expected: %x received: %x)", checksum, rxData[3]);
    ERR(message);
    return false;
  }

  {
    Serial.print("bl: \t [read] ");
    Serial.print(txData[0], HEX);
    Serial.print(" ");
    Serial.print(txData[1], HEX);
    Serial.print(" ");
    Serial.print(rxData[0], HEX);
    Serial.print(" ");
    Serial.print(rxData[1], HEX);
    Serial.print(" ");
    Serial.print(rxData[2], HEX);
    Serial.print(" ");
    Serial.print(rxData[3], HEX);
    Serial.print(" (");
    Serial.print(checksum, HEX);
    Serial.println(")");
  }

  *data = ((uint32_t)rxData[0] << 16) | ((uint32_t)rxData[1] << 8) | (uint32_t)rxData[2];
  return true;
}

/*!
 * bl0940::getVoltage
 * Get measured voltage
 *
 * @return Last measured voltage in Volts
 *
 * Calculations form BL0940 Application Note
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/guide/BL0940_APPNote_TSSOP14_V1.04_EN.pdf
 * Voltage(V) = V_RMS_Reg∗Vref∗(R2+R1) / 79931∗R1∗1000
 * Vref=1.218V
 * (R2+R1)/R1*1000 = BL0940_VOLTAGE_DIVIDER. Keep in mind *both R1 and R2 are in kiloohms*
 * Vref/79931 = BL0940_V_RMS_COEFFICIENT
 */
bool BL0940::getVoltage(float* voltage)
{
  uint32_t data;
  if (false == _readRegister(0x06, &data))
  {
    // ERR("bl: \t [get V] Can not read V_RMS register.");
    return false;
  }

  // *voltage = (float)data * Vref * (R1 + R2) / (79931.0 * R1 * 1000);
  *voltage = (float)data * Vref_V * (R_DIVIDE) / (79931.0 * 1000);
  return true;
}

/*!
 * bl0940::getCurrent
 * Get measured current in Amperes
 *
 * @return Last measured current
 *
 * Calculations form BL0940 Application Note
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/guide/BL0940_APPNote_TSSOP14_V1.04_EN.pdf
 * Current(A) = I_RMS_Reg∗Vref / 324004∗RL
 * Vref = 1.218V
 * Vref/324004 = BL0940_I_RMS_COEFFICIENT
 * RL = BL0940_SHUNT_RESISTANCE
 */
bool BL0940::getCurrent(float* current)
{
  uint32_t data;
  if (false == _readRegister(0x04, &data))
  {
    // ERR("bl: \t [get I] Can not read I_RMS register.");
    return false;
  }
  *current = ((float)data * Vref_I) / (324004.0 * RL);
  return true;
}

/*!
 * bl0940::getActivePower
 * Get measured Active Power in Watts
 *
 * @return Last measured active power
 *
 * Calculations form BL0940 Application Note
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/guide/BL0940_APPNote_TSSOP14_V1.04_EN.pdf
 * Active Power(W) = WATT_Reg∗VrefVref∗(R2+R1) / 4046∗RL∗R1∗1000
 * Vref = 1.218V
 * Vref*Vref/4046 = BL0940_WATT_COEFFICIENT
 * (R2+R1)/R1*1000 = BL0940_VOLTAGE_DIVIDER
 * RL = BL0940_SHUNT_RESISTANCE
 */
bool BL0940::getActivePower(float* activePower)
{
  uint32_t data;
  if (false == _readRegister(0x08, &data))
  {
    // ERR("bl: \t [get kW] Can not read WATT register.");
    return false;
  }

  int32_t rowActivePower = (int32_t)(data << 8) / 256;
  if (rowActivePower < 0)
    rowActivePower = -rowActivePower;
  // *activePower = ((float)rowActivePower * Vref * Vref * (R1 + R2)) / (4046.0 * RL * R1 * 1000);
  *activePower = ((float)rowActivePower * Vref_V * Vref_I * (R_DIVIDE)) / (4046.0 * RL * 1000);
  // *activePower = (this->current * this->voltage * (this->powerFactor / 100));

  return true;
}

/*!
 * bl0940::getReactivePower
 * Calculate Reactive Power in VAr
 *
 * @return Calculated Reactive power
 *
 * Calculations from Power Triangle
 */
bool BL0940::getReactivePower(float* reactivePower)
{
  float activePower = 0.0;
  float phraseAngle = 0.0;

  if (!getActivePower(&activePower) || !getPhaseAngle(&phraseAngle))
  {
    // ERR("bl: \t [get Reactive Power] Can not read W and Phrase Angle register.");
    return false;
  }

  *reactivePower = ((float)activePower) * tan((float)phraseAngle);
  return true;
}

/*!
 * bl0940::getApparentPower
 * Calculate Apparent Power in VA
 *
 * @return Calculated Apparent power
 *
 * Calculations from Power Triangle
 */
bool BL0940::getApparentPower(float* apparentPower)
{
  float activePower = 0.0;
  float powerFactor = 0.0;

  if (!getActivePower(&activePower) || !getPowerFactor(&powerFactor))
  {
    // ERR("bl: \t [get Reactive Power] Can not read W and Power Factor register.");
    return false;
  }

  *apparentPower = (float)activePower / (float)powerFactor;

  return true;
}

/*!
 * bl0940::getEnergy
 * Get measured Energy in kWh
 *
 * @return Total measured Energy
 */

/*!
 * bl0940::getEnergy
 * Calculate Energy in kWh for number of CF pulses
 *
 * @param Counted Energy pulses
 * @return Total measured Energy
 *
 * Calculations form BL0940 Application Note
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/guide/BL0940_APPNote_TSSOP14_V1.04_EN.pdf
 * tcf = 1638.4*256 / WATT
 * Active Energy (kWh ) = CF * 1638.4*256*Vref*Vref*(R2+R1) / 3600000*4046*RL*R1*1000
 * Vref = 1.218V
 * Vref*Vref/4046 = BL0940_WATT_COEFFICIENT
 * (R2+R1)/R1*1000 = BL0940_VOLTAGE_DIVIDER
 * RL = BL0940_SHUNT_RESISTANCE
 * 1638.4*256 = BL0940_ACTIVE_ENERGY_COEFFICIENT
 *                             BL0940_ACTIVE_ENERGY_COEFFICIENT * BL0940_WATT_COEFFICIENT * BL0940_VOLTAGE_DIVIDER
 * Active Energy (kWh ) = CF * -----------------------------------------------------------------------------------
 *                                                    3600000 * BL0940_SHUNT_RESISTANCE
 */
bool BL0940::getActiveEnergy(float* activeEnergy)
{
  uint32_t data;
  if (false == _readRegister(0x0A, &data))
  {
    // ERR("bl: \t [get kWh] Can not read CF_CNT register.");
    return false;
  }

  int32_t rowCF_CNT = (int32_t)(data << 8) / 256;
  if (rowCF_CNT < 0)
    rowCF_CNT = -rowCF_CNT;
  *activeEnergy = ((float)rowCF_CNT * 1638.4 * 256.0 * Vref_V * Vref_I * (R1 + R2)) / (3600000.0 * 4046.0 * R1 * RL * 1000);

  return true;
}

/*!
 * bl0940::getPhaseAngle
 * Get Phase Angle in Radians
 *
 * @return Phase Angle
 *
 * Calculations form BL0940 Calibration-free Metering IC Datasheet
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/datasheet/BL0940_V1.1_en.pdf
 * PA = 2*pi*CORNER*Fc/Fo
 * Fc = AC Frequency
 * Fo = BL0940_SAMPLING_FREQUENCY
 */
bool BL0940::getPhaseAngle(float* phraseAngle)
{
  uint32_t data;
  if (false == _readRegister(0x0C, &data))
  {
    ERR("bl: \t [get Angle] Can not read Phrase Angle register.");
    return false;
  }

  *phraseAngle = 2 * M_PI * (float)data * (float)Hz / (float)FREQUENCY_SAMPLING;

  return true;
}

/*!
 * bl0940::getPowerFactor
 * Get Power factor
 *
 * @param Result in percentage ( default true )
 * @return Power factor
 */
bool BL0940::getPowerFactor(float* powerFactor)
{
  uint32_t data;
  if (false == _readRegister(0x0C, &data))
  {
    ERR("bl: \t [get PF] Can not read CORNER register.");
    return false;
  }

  float rowPowerFactor = cos(2.0 * 3.1415926535 * (float)data * (float)Hz / 1000000.0) * 100.0;
  if (rowPowerFactor < 0)
    rowPowerFactor = -rowPowerFactor;
  *powerFactor = rowPowerFactor;

  return true;
}

/*!
 * bl0940::getTemperature
 * Get Intrenal temerature in °C
 *
 * @return Intrenal temperature
 *
 * Calculations form BL0940 Calibration-free Metering IC Datasheet
 * http://www.belling.com.cn/media/file_object/bel_product/BL0940/datasheet/BL0940_V1.1_en.pdf
 * Tx=(170/448)(TPS1/2-32)-45
 */
bool BL0940::getTemperature(float* temperature)
{
  uint32_t data;
  if (false == _readRegister(0x0E, &data))
  {
    ERR("bl: \t [get Temp] Can not read TPS1 register.");
    return false;
  }

  int16_t rowTemperature = (int16_t)(data << 6) / 64;
  *temperature           = (170.0 / 448.0) * (rowTemperature / 2.0 - 32.0) - 45;
  return true;
}

bool BL0940::setFrequency(uint32_t Hz)
{
  uint32_t data;
  if (false == _readRegister(0x18, &data))
  {
    ERR("bl: \t [set F] Can not read MODE register.");
    return false;
  }

  uint16_t mask = 0b0000001000000000;  // 9bit
  if (Hz == 50)
    data &= ~mask;
  else
    data |= mask;

  if (false == _writeRegister(0x18, data))
  {
    ERR("bl: \t [set F] Can not write MODE register.");
    return false;
  }

  if (false == _readRegister(0x18, &data))
  {
    ERR("bl: \t [set F] Can not read MODE register.");
    return false;
  }

  if ((data & mask) == 0)
  {
    Hz = 50;
    DBG("bl: \t [set F] Set frequency: 50Hz");
  }
  else
  {
    Hz = 60;
    DBG("bl: \t [set F] Set frequency: 60Hz");
  }
  return true;
}

bool BL0940::setUpdateRate(uint32_t rate)
{
  uint32_t data;
  if (false == _readRegister(0x18, &data))
  {
    ERR("bl: \t [update F] Can not read MODE register.");
    return false;
  }

  uint16_t mask = 0b0000000100000000;  // 8bit
  if (rate == 400)
    data &= ~mask;
  else
    data |= mask;

  if (false == _writeRegister(0x18, data))
  {
    ERR("bl: \t [update F] Can not write MODE register.");
    return false;
  }

  if (false == _readRegister(0x18, &data))
  {
    ERR("bl: \t [update F] Can not read MODE register.");
    return false;
  }

  if ((data & mask) == 0)
  {
    updateRate = 400;
    DBG("bl: \t [update F] Set update rate: 400ms.");
  }
  else
  {
    updateRate = 800;
    DBG("bl: \t [update F] Set update rate: 800ms.");
  }
  return true;
}

bool BL0940::setOverCurrentDetection(float detectionCurrent)
{
  // const float magicNumber = 0.72; // I_FAST_RMS = 0.72 * I_RMS (Values obtained by experiments in the case of resistance load)

  // // MODE[12] CF_UNABLE set 1 : alarm, enable by TPS_CTRL[14] configured
  // uint32_t data;
  // if (false == _readRegister(0x18, &data))
  // {
  //     ERR("bl: \t [set CURR DETECTION] Can not read MODE register.");
  //     return false;
  // }
  // data |= 0b0001000000000000; // 12bit
  // if (false == _writeRegister(0x18, data))
  // {
  //     ERR("bl: \t [set CURR DETECTION] Can not read write register.");
  //     return false;
  // }

  // // TPS_CTRL[14] Alarm switch set 1 : Over-current and leakage alarm on
  // if (false == _readRegister(0x1B, &data))
  // {
  //     ERR("bl: \t [set CURR DETECTION] Can not read TPS_CTRL register.");
  //     return false;
  // }
  // data |= 0b0100000000000000; // 14bit  0b0100000000000000
  // if (false == _writeRegister(0x1B, data))
  // {
  //     ERR("bl: \t [set CURR DETECTION] Can not write TPS_CTRL register.");
  //     return false;
  // }

  // // Set detectionCurrent I_FAST_RMS_CTRL
  // data = (uint32_t)(detectionCurrent * magicNumber / Vref * ((324004.0 * R5 * 1000.0) / Rt));
  // data >>= 9;
  // data &= 0x007FFF;
  // float actualDetectionCurrent = (float)(data << 9) * Vref / ((324004.0 * R5 * 1000.0) / Rt);
  // data |= 0b1000000000000000; // 15bit=1 Fast RMS refresh time is every cycle
  // data &= 0x00000000FFFFFFFF;
  // if (false == _writeRegister(0x10, data))
  // {
  //     ERR("bl: \t [set CURR DETECTION] Can not write I_FAST_RMS_CTRL register.");
  //     return false;
  // }
  // char massage[128];
  // sprintf(massage, "bl: \t [set CURR DETECTION] Set Current Detection:%.1fA.", actualDetectionCurrent);
  // DBG(massage);

  return true;
}

bool BL0940::setCFOutputMode()
{
  // MODE[12] CF_UNABLE set 0 : alarm, enable by TPS_CTRL[14] configured
  uint32_t data;
  if (false == _readRegister(0x18, &data))
  {
    ERR("bl: \t [set CF OUTPUT MODE] Can not read MODE register.");
    return false;
  }
  data &= ~0b0001000000000000;  // 12bit
  if (false == _writeRegister(0x18, data))
  {
    ERR("bl: \t [set CF OUTPUT MODE] Can not read write register.");
    return false;
  }
  return true;
}

bool BL0940::reset()
{
  if (false == _writeRegister(0x19, 0x5A5A5A))
  {
    ERR("bl: \t [reset] Can not write SOFT_RESET register.");
    return false;
  }

  delay(500);
  return true;
}

// My Source
bool BL0940::setOffset(void)
{
  Serial.println("bl: \t [offset]");

  bl0940_WATTOS   = 0xFFFF1D;  // 0x1D == 1W
  bl0940_WA_CREEP = 0xFFFF1D;  // 0x1D == 1W

  _writeRegister(0x15, (uint32_t)0xFFFF1D);
  _writeRegister(0x17, (uint32_t)0xFFFF1D);

  return true;
}

void BL0940::restorekWh(void)
{
  memory_save_bl0940_energy_kwh();
  this->kWh_restore = bl0940_kWh;
}

bool BL0940::getData(void)
{
  static STR_BL0940_DATA electricity_data = {};

  static uint8_t test_current_counter = 0;
  static uint8_t test_kWh_counter     = 0;


  // Save previous data
  previous_current      = this->current;
  previous_activeEnergy = this->activeEnergy;

  // Get data
  if (this->getVoltage(&this->voltage) == false)
    this->voltage = -1;
  if (this->getCurrent(&this->current) == false)
    this->current = -1;
  if (this->getPowerFactor(&this->powerFactor) == false)
    this->powerFactor = -1;
  if (this->getActivePower(&this->activePower) == false)
    this->activePower = -1;
  if (this->getActiveEnergy(&this->activeEnergy) == false)
    this->activeEnergy = -1;
  if (this->getTemperature(&this->temperature) == false)
    this->temperature = -1;

  if (this->temperature == -1 || this->current == -1
  || this->powerFactor == -1 || this->activePower == -1
  || this->activeEnergy == -1 || this->voltage == -1)
  {
    return false;
  }
  else return true;
  // // Save kWh
  // this->activeEnergy = this->activeEnergy + this->kWh_restore;

  // if (this->activeEnergy - this->kWh_pre == 1)
  // {
  //   bl0940_kWh = this->activeEnergy;
  //   save_bl0940_kWh();
  // }

  // this->kWh_pre = this->activeEnergy;

  // // Validate data
  // if (this->current == 0)
  //   this->powerFactor = 100;

  // if (this->temperature < 0)
  //   this->temperature = 0;

  /* Testing ----------------------------------------------------------------------------- */
  // this->previous_current       = this->current;
  // this->previous_activeEnergy  = this->activeEnergy;
  // this->previous_device_status = this->current_device_status;

  // if (relay_status == 0)
  // {
  //   if (this->current > 0)
  //     this->current_device_status = 1;
  //   else
  //     this->current_device_status = 0;

  //   if (test_current_counter == 0)
  //     this->current = ((int)this->current + 1) % 5;

  //   if (test_kWh_counter == 0)
  //     this->activeEnergy = ((int)this->activeEnergy + 1) % 100;
  // }
  // else
  // {
  //   this->current_device_status = 0;
  //   this->current               = 0;
  //   this->activeEnergy          = 0;
  // }

  // test_current_counter = (test_current_counter + 1) % 1;
  // test_kWh_counter     = (test_kWh_counter + 1) % 5;

  // if (this->current_device_status != this->previous_device_status)
  // {
  //   // Serial.printf("mqtt: \t [publish] device changed status\n");
  //   mqtt_pub_status("device changed status", relay_status, this->current_device_status);
  // }

  // if (this->current != this->previous_current)
  // {
  //   // Serial.printf("mqtt: \t [publish] current changed\n");
  //   mqtt_pub_electricity();
  // }

  // if (this->activeEnergy != this->previous_activeEnergy)
  // {
  //   // Serial.printf("mqtt: \t [publish] kWh changed\n");
  //   mqtt_pub_electricity();
  // }

  /* Testing ----------------------------------------------------------------------------- */

  // // Check if the current difference is greater than 0.1 Ampe
  // if ((abs(this->current - previous_current) >= 0.1 || (abs(this->activeEnergy - previous_current) >= 0.1)) && rtc_state == true)
  // {
  //   // Add data to node
  //   electricity_data.voltage      = this->voltage;
  //   electricity_data.current      = this->current;
  //   electricity_data.activePower  = this->activePower;
  //   electricity_data.activeEnergy = this->activeEnergy;
  //   electricity_data.powerFactor  = this->powerFactor;
  //   electricity_data.temperature  = this->temperature;
  //   electricity_data.dt           = rtc_string;em 

  //   // Add node to list
  //   electricity_list.push_back(electricity_data);

  //   // Update previous current value
  //   previous_current = this->current;
  // }

  /*Serial.printf("bl: \t [get] %0.2f (V); \t %0.2f (A); \t %0.2f (W); \t %0.2f (Wh); \t %0.2f (%%); \t %0.2f (°C)\n", this->voltage, this->current,
                this->activePower, this->activeEnergy, this->powerFactor, this->temperature);*/

}
