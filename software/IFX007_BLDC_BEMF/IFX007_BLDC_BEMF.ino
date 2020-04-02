////This example code is for sensorless BLDC motor control
//Please be noticed it has to be modified for diffenrent motor


//Define Motor parameters
#define MotorPoles 8
#define HallPoleShift 0

//PrintInfo

//Start up - Commutation-Counts to switch over to closed-loop

#define OpenLoopToClosedLoopCount 50
uint8_t Commutation = 0;
uint8_t BEMF_phase = A1;
uint8_t ClosedLoop = 0;
uint8_t DutyCycle = 150;
uint8_t Dir = 0;
uint32_t V_neutral = 0;


// the following should be moved to a library

#include <Platform.h>

typedef struct
{
  // digital output pins
  uint16_t enU;
  uint16_t enV;
  uint16_t enW;

  // analog output pins
  uint16_t pwmU;
  uint16_t pwmV;
  uint16_t pwmW;

  // analog input channels
  uint32_t bemfU;
  uint32_t bemfV;
  uint32_t bemfW;
  uint32_t adcVs;
} MotorControllerPinConfig_t;

class MotorController
{
public:
  MotorController(const MotorControllerPinConfig_t &config, IGpio* gpio = defaultIGpio, IAdc* adc = defaultIAdc);
  
  void updateVNeutral();


  void DoCommutation();
  void UpdateHardware(uint8_t CommutationStep, uint8_t Dir);
  
private:

  const MotorControllerPinConfig_t &m_config;
  IGpio* m_gpio;
  IAdc* m_adc;

  uint32_t m_vNeutral;
};

MotorController::MotorController(const MotorControllerPinConfig_t &config, IGpio* gpio, IAdc* adc) :
  m_config {config},
  m_gpio {gpio},
  m_adc {adc}
{
  m_gpio->configureGpio(m_config.enU, GPIO_MODE_OUTPUT_PUSH_PULL);
  m_gpio->configureGpio(m_config.enV, GPIO_MODE_OUTPUT_PUSH_PULL);
  m_gpio->configureGpio(m_config.enW, GPIO_MODE_OUTPUT_PUSH_PULL);

    // analog out!
  pinMode(m_config.pwmU, OUTPUT);
  pinMode(m_config.pwmV, OUTPUT);
  pinMode(m_config.pwmW, OUTPUT);
}

void MotorController::updateVNeutral()
{
  m_vNeutral = (m_adc->getValue(m_config.adcVs) * DutyCycle) >> 8;
}

  
static const MotorControllerPinConfig_t mcConfig = {
  6, // .enU
  5, // .enV
  3, // .enW
  11, // .pwmU
  10, // .pwmV
  9, // .pwmW
  A3, // .bemfU
  A2, // .bemfV
  A1, // .bemfW
  A0, // .adcVs
};

MotorController mc(mcConfig);


void setup()
{
  // Setup the serial connection
  Serial.begin(115200);
}

void loop()
{

  uint16_t i = 5000;
  uint8_t CommStartup = 0;

  // Startup procedure: start rotating the field slowly and increase the speed
  while (i > 1000)
  {
    delayMicroseconds(i);
    Commutation = CommStartup;
    mc.UpdateHardware(CommStartup, 0);
    CommStartup++;
    if (CommStartup == 6)
      CommStartup = 0;
    i = i - 20;
  }

  // main loop:
  while (1)
  {
    mc.DoCommutation();

    while (Serial.available() > 0) {
      byte in = Serial.read();
      if (in == '+' && (DutyCycle <= 250) ) DutyCycle += 5; //DutyCycle + 5
      if (in == '-' && (DutyCycle >= 5)   ) DutyCycle -= 5; //DutyCycle - 5

      if (in == 'd') Serial.println(DutyCycle, DEC);      //Show DutyCycle
      if (in == 'm') Serial.println(millis(), DEC);       //TimeStamp
    }
  }
}

void MotorController::DoCommutation()
{
  updateVNeutral();
  // V_neutral = m_adc->getValue(m_config.BEMF_phase);

  if (Dir == 0)
  {
    switch (Commutation)
    {
      case 0:
        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfW) > V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfW) < V_neutral)
        {
          Commutation = 1;
          UpdateHardware(Commutation, 0);
        }
        break;

      case 1:

        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfV) < V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfV) > V_neutral)
        {
          Commutation = 2;
          UpdateHardware(Commutation, 0);
        }
        break;

      case 2:

        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfU) > V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfU) < V_neutral)
        {
          Commutation = 3;
          UpdateHardware(Commutation, 0);
        }
        break;

      case 3:

        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfW) < V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfW) > V_neutral)
        {
          Commutation = 4;
          UpdateHardware(Commutation, 0);
        }
        break;

      case 4:

        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfV) > V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfV) < V_neutral)
        {
          Commutation = 5;
          UpdateHardware(Commutation, 0);
        }
        break;

      case 5:

        for (int i = 0; i < 20; i++)
        {
          if (m_adc->getValue(m_config.bemfU) < V_neutral)
            i -= 1;
        }
        if (m_adc->getValue(m_config.bemfU) > V_neutral)
        {
          Commutation = 0;
          UpdateHardware(Commutation, 0);
        }
        break;

      default:
        break;
    }
  }
}

//defining commutation steps according to HALL table
void MotorController::UpdateHardware(uint8_t CommutationStep, uint8_t Dir)
{
  updateVNeutral();

  //CW direction
  if (Dir == 0)
  {

    switch (CommutationStep)
    {
      case 0:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, LOW);
        analogWrite(m_config.pwmU, DutyCycle);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, 0);
        break;

      case 1:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, LOW);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, DutyCycle);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, 0);
        break;

      case 2:
        m_gpio->setGpio(m_config.enU, LOW);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, DutyCycle);
        analogWrite(m_config.pwmW, 0);
        break;

      case 3:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, LOW);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, DutyCycle);
        analogWrite(m_config.pwmW, 0);
        break;

      case 4:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, LOW);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, DutyCycle);
        break;

      case 5:
        m_gpio->setGpio(m_config.enU, LOW);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, DutyCycle);
        break;

      default:
        break;
    }
  }
  else
  {
    //CCW direction
    switch (CommutationStep)
    {
      case 0:
        m_gpio->setGpio(m_config.enU, LOW);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, DutyCycle);
        analogWrite(m_config.pwmW, 0);
        break;

      case 1:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, LOW);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, DutyCycle);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, 0);
        break;

      case 2:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, LOW);
        analogWrite(m_config.pwmU, DutyCycle);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, 0);
        break;

      case 3:
        m_gpio->setGpio(m_config.enU, LOW);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, DutyCycle);
        break;

      case 4:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, LOW);
        m_gpio->setGpio(m_config.enW, HIGH);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, 0);
        analogWrite(m_config.pwmW, DutyCycle);
        break;

      case 5:
        m_gpio->setGpio(m_config.enU, HIGH);
        m_gpio->setGpio(m_config.enV, HIGH);
        m_gpio->setGpio(m_config.enW, LOW);
        analogWrite(m_config.pwmU, 0);
        analogWrite(m_config.pwmV, DutyCycle);
        analogWrite(m_config.pwmW, 0);
        break;

      default:
        break;
    }
  }
}
