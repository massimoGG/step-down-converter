// Step down Converter using ADS1115 & SH1106 made by Massimo Giardina 2021-06-25 

// I2C
#include <Arduino.h>
#include <Wire.h>
// OLED
#include <U8x8lib.h>
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
// ADS1115
#include <ADS1115_WE.h> 
ADS1115_WE adc = ADS1115_WE(0x48);

// Pin Configurations
const unsigned int base = 9;  //9;
// Voltage sensors
const int inputVoltagePin  = ADS1115_COMP_0_GND;
const int outputVoltagePin = ADS1115_COMP_1_GND;
// Current sensor
const int vccVoltagePin = ADS1115_COMP_2_GND;
const int inputCurrentPin = ADS1115_COMP_3_GND;

// Voltage-divider resistors
const unsigned long r1 = 38460; // ~ohm
const unsigned long r2 = 9940;  // ~ohm

#define DEBUG 1

static float setVoltage = 0;
static float dutyref = 0;

static struct Voltage {
  float raw = 0;
  float value = 0;
  float voltage = 0;
  float ar = 0;
} inputVoltage,outputVoltage;

float readChannel(ADS1115_MUX channel) {
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy());
  return adc.getResult_mV();
}
void readInputVoltage()
{
  inputVoltage.raw = readChannel(inputVoltagePin);
  inputVoltage.voltage = inputVoltage.raw * (r1 + r2) / r2;
}
void readOutputVoltage()
{
  outputVoltage.raw = readChannel(outputVoltagePin);
  outputVoltage.voltage = outputVoltage.raw * (r1 + r2) / r2;
}

// OLED functions
void pre(void)
{
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);    
  u8x8.clear();

  u8x8.inverse();
  u8x8.drawString(0,0,"MasDev Step-Down");
  u8x8.setFont(u8x8_font_chroma48medium8_r);  
  u8x8.noInverse();
  u8x8.setCursor(0,1);
}

void setup() {
  Serial.begin(115200);

  pinMode(base, OUTPUT);

  // Inversed! 32kHz on Pin D9
  TCCR1A = 0b11110001;
  TCCR1B = TCCR1B & 0xC | 0b00001001;

  // OLED
  u8x8.begin();
  pre();
  
  // ADS1115
  Wire.begin();
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_6144);
  adc.setConvRate(ADS1115_860_SPS);
}

void loop() {
  dutyref = (setVoltage/inputVoltage.voltage);
  uint8_t dc = dutyref*0xFF;
  dc = 0;
  
  while (1) {
    readInputVoltage();
    readOutputVoltage();

    // Feedback
    // Too little -> High amperage
    if (outputVoltage.voltage < setVoltage && dc < 0xFF)
      dc++;
    
    // Too much -> Low amperage
    if (outputVoltage.voltage > setVoltage && dc > 0)
      dc--;

    OCR1A = dc;

    // OLED
    u8x8.drawString(1, 3, "Vi: ");
    u8x8.setCursor(6,3); u8x8.print(inputVoltage.voltage,1); u8x8.print("mV  ");
    u8x8.drawString(1, 5, "Vo: ");
    u8x8.setCursor(6, 5); u8x8.print(outputVoltage.voltage,1); u8x8.print("mV  ");

    // Doesn't work yet :(
    float i = readChannel(inputCurrentPin);
    float v = readChannel(vccVoltagePin);
    float deltamv = i-(0.5*v);
    u8x8.setCursor(2,6); u8x8.print(deltamv);
    u8x8.drawString(1, 7, "Ii: ");
    u8x8.setCursor(6, 7); u8x8.print(deltamv/66,1); u8x8.print("A  ");

    if (Serial.available())
      setVoltage =  Serial.parseFloat();
    
    if (DEBUG) {
      Serial.print(inputVoltage.raw); Serial.print(" -> "); Serial.print(inputVoltage.voltage,1); Serial.print("mV\t");
      Serial.print(outputVoltage.raw); Serial.print(" -> "); Serial.print(outputVoltage.voltage,1); Serial.print("mV\t");
      Serial.print(setVoltage); Serial.print(" -> "); Serial.print(dc); Serial.print("\t");
      Serial.print('\n');
    }
  }
}
