#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute; 
int beatAvg; //Valor BPM 

void setup()
{
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("No se encontró el sensor MAX30105. Por favor revisar cableado y/o alimentación. ");
    while (1);
  }
  Serial.println("Coloca tu dedo índice sobre el sensor con presión constante.");
  byte ledBrightness = 0x7F; //Brillo al máximo donde las opciones son: 0=OFF
  byte sampleAverage = 4;
  byte ledMode = 2; //Donde las opciones son: 1 = ROJO, 2 = ROJO + IR, 3 = ROJO + IR + VERDE
  byte sampleRate = 100; 
  int pulseWidth = 411; //Detección de latidos normales
  int adcRange = 16384; 
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with default settings
  //particleSensor.setPulseAmplitudeRed(0x1F); //Máxima amplitud para el LED rojo
  //particleSensor.setPulseAmplitudeIR(0x1F); //Máxima amplitud para el LED infrarrojo
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
    delay(50); //Para evitar saturación en todos los sentidos
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();
}