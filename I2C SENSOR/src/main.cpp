//Universidad del Valle de Guatemala
//Dulce Nicole Monney Paiz, 21549
//BE3029 - Electrónica Digital 2
//Proyecto 3 – I2C & NeoPixel
//*****************************

//Librerías
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"    
#include "arduinoFFT.h"
//Librerías para el neopixel
#include <Adafruit_NeoPixel.h>
#ifdef AVR
#include <avr/power.h> // Requisito para AVR chips
#endif
//*****************************

//Definiciones de pines
#define PIN 5
#define RXp2 16 
#define TXp2 17
//*****************************

//Variables globales
MAX30105 particleSensor;
arduinoFFT FFT;

int    i            = 0;
int    Num          = 100;  // cálculo de SpO2 según este intervalo de muestreo

#define TIMETOBOOT    3000  // Tiempo de espera para mostrar SpO2
#define SCALE         88.0  // Ajuste para mostrar frecuencia cardíaca y SpO2 en la misma escala
#define SAMPLING      100   //Intervalo de muestreo (si se quiere ver la frecuencia cardíaca con más precisión, ajustar a 1)
#define FINGER_ON     50000 // Si la señal roja es menor que esto, indica que el dedo no está en el sensor
#define USEFIFO
#define PULSE_SAMPLES 256 //Originalmente eran 256 (Se pueden reducir a 4 o 64 o incluso 128 para comprobar si UART está en tiempo)
#define SAMPLE_FREQ   50 //Frecuencia de muestreo

// Variables para la frecuencia cardíaca
byte   rateSpot         = 0;
long   lastBeat         = 0;  // Tiempo en el que ocurrió el último latido
int    beatAvg          = 0;
bool   detect_high      = 0;

double redArray[PULSE_SAMPLES]; // array para almacenar muestras del sensor
double vReal[PULSE_SAMPLES];
double vImag[PULSE_SAMPLES];
double beatsPerMinute = 0;

String request; //Variable que recibe de regreso el valor de la medición de sensor

Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);
//*****************************

//Prototipos de funciones
void colorWipe(uint32_t c, uint8_t wait);
//*****************************

//Configuración
void setup()
{
   //NEOPIXEL
   #if defined (AVR_ATtiny85)
   if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
   #endif
   
   strip.begin();
   strip.setBrightness(50);
   strip.show(); // Inicializa todos los píxeles en "apagado"
   //*****************************

   Serial.begin(115200); //Comunicación con el monitor serial/PC
   Serial2.begin(115200); //Comunicación UART 2 con la Tiva C
   //Serial.setDebugOutput(true);
   Serial.println();

   Serial.println("Cargando...");
   delay(3000);

   // Inicialización del sensor
   while (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Usando el puerto I2C predeterminado, velocidad de 400 kHz
   {
      Serial.println("No se encontró MAX30102. Verificar el puente de cableado/alimentación/soldadura en la placa MH-ET LIVE MAX30102. ");
      //while (1);
   }

   //Configuración para detectar un bonito diente de sierra en el trazador
   byte ledBrightness = 0x7F;  // Options: 0=Off to 255=50mA
   byte sampleAverage = 4;     // Options: 1, 2, 4, 8, 16, 32
   byte ledMode       = 2;     // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
   //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
   int sampleRate     = 200;   // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
   int pulseWidth     = 411;   // Options: 69, 118, 215, 411
   int adcRange       = 16384; // Options: 2048, 4096, 8192, 16384
  
   // Configuración de los parámetros deseados
   particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
   particleSensor.enableDIETEMPRDY();
}
//*****************************

//Loop principal
void loop()
{

   uint32_t ir, red, green;
   double fred, fir;
   
#ifdef USEFIFO
   particleSensor.check();               // Verificación del sensor, lea hasta 3 muestras

   while (particleSensor.available()) 
   {  //¿Hay nuevos datos?
#ifdef MAX30105
      red = particleSensor.getFIFORed(); // Sparkfun's MAX30105
      ir  = particleSensor.getFIFOIR();  // Sparkfun's MAX30105
#else
      red = particleSensor.getFIFOIR();  // ¿Por qué getFOFOIR genera datos rojos mediante MAX30102 en la placa de conexión MH-ET LIVE?
      ir  = particleSensor.getFIFORed(); // ¿Por qué getFOFOIRed genera datos del infrarrojo mediante MAX30102 en la placa de conexión MH-ET LIVE? 
#endif

      i++;
      i = i % PULSE_SAMPLES; // realizar 256 muestras
      fred = (double)red;

      redArray[i] = fred; // llenado del array

      particleSensor.nextSample(); // //Se terminó con esta muestra, así que pase a la siguiente. 

      if (i == 0) // Ejecutar cada PULSE_SAMPLES
      {
         //Serial.print("Time: ");
         //Serial.println(millis()); // Se puede usar esto para determinar el tiempo que lleva recolectar 256 muestras (frecuencia de muestreo).
         for (int idx=0; idx < PULSE_SAMPLES; idx++)
         {
            vReal[idx] = redArray[idx];
            vImag[idx] = 0.0;
         }

         FFT = arduinoFFT(vReal, vImag, PULSE_SAMPLES, SAMPLE_FREQ); /* Creación de un objeto FFT */
         FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Peso de la data */
         FFT.Compute(FFT_FORWARD); /* Computar FFT */
         FFT.ComplexToMagnitude(); /* Computar magnitudes */

         double peak = FFT.MajorPeak();
         beatsPerMinute = peak * 60;
         int bpm = round(beatsPerMinute);
         Serial2.println(bpm);  
         delay(50); // Tiempo para que la Tiva C lea el valor del bpm 
         colorWipe(strip.Color(0, 0, 255), 5); //AZUL probar con 5 o 10

         //Serial.print("BPM: ");
         //Serial.println(beatsPerMinute);
         //if (ir < 50000)
         //Serial.print(" No finger?");
         //Serial.print("ir:");
         //Serial.print(ir);
         //Serial.println();
      }
  }
#endif

if(Serial2.available()){
      request = Serial2.readStringUntil('\n');
      //Serial.print("BPM: ");
      Serial.println(request);
      //if(request==1){
      //Serial.print("BPM: ");
      //Serial.println(request);
      //if (ir < 50000)
      //Serial2.print(" No finger?");
      //delay(100); // Delay para que la Tiva C tenga tiempo para leer la respuesta
      }
}
//*****************************

//FUNCIONES DEL NEOPIXEL
   //Llenado de los neopixeles uno después del otro
   void colorWipe(uint32_t c, uint8_t wait){
      for (uint16_t i = 0; i < strip.numPixels(); i++){
         strip.setPixelColor(i, c);
         strip.show();
         delay(wait);
      }
   }
   //*****************************