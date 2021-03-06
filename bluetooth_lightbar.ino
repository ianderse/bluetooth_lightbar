#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "lightbarConfig.h"

    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"

#define PIN 6
#define NUM_LEDS 32
#define BRIGHTNESS 50


#define DEVICE_NAME "CLLightbar"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

uint8_t forward = 1;
uint8_t lightBarOn = 0;
int SPEED = 50;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

void blackout()
{
    for (int leds = 0 ; leds < NUM_LEDS; leds++)
     {
        strip.setPixelColor(leds, 0, 0, 0);
     }
    strip.show();
}

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(void)
{

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Command Mode Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);
  
  ble.info();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }

  ble.sendCommandCheckOK("AT+GAPDEVNAME=" DEVICE_NAME);
  ble.sendCommandCheckOK("ATZ");

  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop(void)
{
  // Check for incoming characters from Bluefruit
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return;
  } else if(strcmp(ble.buffer,"On") == 0) {
    strip.setBrightness(50);
    lightBarOn = 1;
  } else if (strcmp(ble.buffer,"Off") == 0) {
    blackout();
    lightBarOn = 0;
  } else {
    Serial.print(ble.buffer);
  }

  while(lightBarOn == 1) {
    ble.println("AT+BLEUARTRX");
    ble.readline();
    if(strcmp(ble.buffer,"On") == 0) {
      Serial.print("Go!");
      strip.setBrightness(50);
      lightBarOn = 1;
    } else if (strcmp(ble.buffer,"Off") == 0) {
      Serial.print("Stop it.");
      blackout();
      lightBarOn = 0;
    } else if (strncmp(ble.buffer,"SP", 2) == 0) {
      int len = strlen(ble.buffer);
      changeSpeed(atoi(&ble.buffer[len-2]));
    }
    scanner();
  }
}

/**************************************************************************/
/*!
    @brief  Checks for user input (via the Serial Monitor)
*/
/**************************************************************************/
bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while( (!Serial.available()) && !timeout.expired() ) { delay(1); }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count=0;
  do
  {
    count += Serial.readBytes(buffer+count, maxSize);
    delay(2);
  } while( (count < maxSize) && (Serial.available()) );

  return true;
}

void changeSpeed(int new_speed) {
  SPEED = (100 - new_speed);
}

void scanner() {
      if(lightBarOn == 0) {
        blackout();
        return;
      }

      if(forward == 1) {
        for(uint8_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0,0,0,255 ) );
          if (i > 1) {
            strip.setPixelColor((i-2), 0);
          }
          if (i == strip.numPixels() - 1) {
            forward = 0;
          }
          delay(SPEED);
          strip.show();
        }
      } else {
        for(uint8_t i=strip.numPixels(); i>0; i--) {
          strip.setPixelColor(i, strip.Color(0,0,0,255 ) );
          strip.setPixelColor((i+2), 0);
          if (i == 1) {
            forward = 1;
          }
          delay(SPEED);
          strip.show();
        }
      }
}
