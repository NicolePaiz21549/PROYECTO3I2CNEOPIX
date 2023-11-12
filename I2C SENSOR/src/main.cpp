//***********************************************************************************
//Universidad del Valle de Guatemala
//Dulce Nicole Monney Paiz, 21549
//BE3029 - Electrónica Digital 2
//Proyecto 3 – I2C & NeoPixel
//***********************************************************************************

//Librerías
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"    
#include "arduinoFFT.h"
//***********************************************************************************

//Definiciones de pines
#define RXp2 16 
#define TXp2 17
//***********************************************************************************

//Variables globales
MAX30105 particleSensor;
arduinoFFT FFT;

int    i            = 0;
int    Num          = 100;  // calculate SpO2 by this sampling interval

#define TIMETOBOOT    3000  // wait for this time(msec) to output SpO2
#define SCALE         88.0  // adjust to display heart beat and SpO2 in the same scale
#define SAMPLING      100   //25 //5     // if you want to see heart beat more precisely, set SAMPLING to 1
#define FINGER_ON     30000 // if red signal is lower than this, it indicates your finger is not on the sensor
#define USEFIFO
#define PULSE_SAMPLES 256
#define SAMPLE_FREQ   50

// Variables para la frecuencia cardíaca --- For Heart Rate ---
byte   rateSpot         = 0;
long   lastBeat         = 0;  // Time at which the last beat occurred
int    beatAvg          = 0;
bool   detect_high      = 0;

double redArray[PULSE_SAMPLES]; // array to store samples from the sensor
double vReal[PULSE_SAMPLES];
double vImag[PULSE_SAMPLES];
double beatsPerMinute = 0;

int request=0; //Comando para activar bandera de envio del valor medido por el sensor
//***********************************************************************************

//Configuración
void setup()
{
   Serial.begin(115200); //Comunicación con el monitor serial/PC
   Serial2.begin(115200); //Comunicación UART 2 con la Tiva C
   Serial.setDebugOutput(true);
   Serial.println();

   Serial.println("Running...");
   delay(3000);

   // Initialize sensor
   while (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
   {
      Serial.println("MAX30102 was not found. Please check wiring/power/solder jumper at MH-ET LIVE MAX30102 board. ");
      //while (1);
   }

   //Setup to sense a nice looking saw tooth on the plotter
   byte ledBrightness = 0x7F;  // Options: 0=Off to 255=50mA
   byte sampleAverage = 4;     // Options: 1, 2, 4, 8, 16, 32
   byte ledMode       = 2;     // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
   //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
   int sampleRate     = 200;   // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
   int pulseWidth     = 411;   // Options: 69, 118, 215, 411
   int adcRange       = 16384; // Options: 2048, 4096, 8192, 16384
  
   // Set up the wanted parameters
   particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
   particleSensor.enableDIETEMPRDY();
}
//***********************************************************************************

//Loop principal
void loop()
{
   uint32_t ir, red, green;
   double fred, fir;
   
#ifdef USEFIFO
   particleSensor.check();               // Check the sensor, read up to 3 samples

   while (particleSensor.available()) 
   {  // Do we have new data
#ifdef MAX30105
      red = particleSensor.getFIFORed(); // Sparkfun's MAX30105
      ir  = particleSensor.getFIFOIR();  // Sparkfun's MAX30105
#else
      red = particleSensor.getFIFOIR();  // why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
      ir  = particleSensor.getFIFORed(); // why getFIFORed output IR data by MAX30102 on MH-ET LIVE breakout board
#endif

      i++;
      i = i % PULSE_SAMPLES; // wrap around every 256 samples
      fred = (double)red;

      redArray[i] = fred; // populate the array

      particleSensor.nextSample(); // We're finished with this sample so move to next sample

      if (i == 0) // execute every PULSE_SAMPLES
      {
         Serial.print("Time: ");
         Serial.println(millis()); // can use this to determine time it takes to collect 256 samples (sample rate)
         for (int idx=0; idx < PULSE_SAMPLES; idx++)
         {
            vReal[idx] = redArray[idx];
            vImag[idx] = 0.0;
         }

         FFT = arduinoFFT(vReal, vImag, PULSE_SAMPLES, SAMPLE_FREQ); /* Create FFT object */
         FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
         FFT.Compute(FFT_FORWARD); /* Compute FFT */
         FFT.ComplexToMagnitude(); /* Compute magnitudes */

         double peak = FFT.MajorPeak();
         beatsPerMinute = peak * 60;
         Serial.print("BPM: ");
         Serial.println(beatsPerMinute);

      }
  }
#endif

if(Serial2.available()){
      request = Serial2.parseInt();
      if(request==1){
         Serial2.print(beatsPerMinute);
         delay(100); //Delay para que la Tiva C tenga tiempo para leer la respuesta
      }
}
}
//***********************************************************************************