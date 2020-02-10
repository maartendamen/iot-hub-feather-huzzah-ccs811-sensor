#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <Wire.h>    // I2C library
#include "ccs811.h"  // CCS811 library

#if SIMULATED_DATA

// There is not support for simulated mode.

#else

CCS811 ccs811(-1); // nWAKE on D3
void initSensor()
{
  // Enable I2C
  Wire.begin(); 
  
  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok= ccs811.begin();
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Print CCS811 versions
  Serial.print("setup: hardware    version: "); Serial.println(ccs811.hardware_version(),HEX);
  Serial.print("setup: bootloader  version: "); Serial.println(ccs811.bootloader_version(),HEX);
  Serial.print("setup: application version: "); Serial.println(ccs811.application_version(),HEX);
  
  // Start measuring
  ok= ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");
}

#endif

void readMessage(int messageId, char *payload)
{

    uint16_t eco2, etvoc, errstat, raw;
    ccs811.read(&eco2,&etvoc,&errstat,&raw); 
    
    // Print measurement results based on status
    if( errstat==CCS811_ERRSTAT_OK ) { 
      Serial.print("CCS811: ");
      Serial.print("eco2=");  Serial.print(eco2);     Serial.print(" ppm  ");
      Serial.print("etvoc="); Serial.print(etvoc);    Serial.print(" ppb  ");
      //Serial.print("raw6=");  Serial.print(raw/1024); Serial.print(" uA  "); 
      //Serial.print("raw10="); Serial.print(raw%1024); Serial.print(" ADC  ");
      //Serial.print("R="); Serial.print((1650*1000L/1023)*(raw%1024)/(raw/1024)); Serial.print(" ohm");
      Serial.println();

      StaticJsonDocument<MESSAGE_MAX_LEN> jsonBuffer;
      JsonObject root = jsonBuffer.to<JsonObject>();
      root["deviceId"] = DEVICE_ID;
      root["messageId"] = messageId;
  
      // NAN is not the valid json, change it to NULL
      if (std::isnan(eco2))
      {
          root["eco2"] = NULL;
      }
      else
      {
          root["eco2"] = eco2;
      }
  
      if (std::isnan(etvoc))
      {
          root["etvoc"] = NULL;
      }
      else
      {
          root["etvoc"] = etvoc;
      }
      serializeJson(root, payload, MESSAGE_MAX_LEN);
      
    } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
      Serial.println("CCS811: waiting for (new) data");
    } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
      Serial.println("CCS811: I2C error");
    } else {
      Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
      Serial.print("="); Serial.println( ccs811.errstat_str(errstat) ); 
    }

}

void parseTwinMessage(char *message)
{
    DynamicJsonDocument root(MESSAGE_MAX_LEN);
    DeserializationError error = deserializeJson(root, message);
    if (!error)
    {
        Serial.printf("Parse %s failed.\r\n", message);
        return;
    }

    if (root["desired"]["interval"].isNull())
    {
        interval = root["desired"]["interval"];
    }
    else if (root.containsKey("interval"))
    {
        interval = root["interval"];
    }
}
