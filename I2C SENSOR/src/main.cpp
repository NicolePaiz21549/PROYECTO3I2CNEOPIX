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
int clavedos = 0; //Comando para activar la bandera de envio para el neopixel

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
         Serial.print("Time: ");
         Serial.println(millis()); // Se puede usar esto para determinar el tiempo que lleva recolectar 256 muestras (frecuencia de muestreo).
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

         Serial.print("BPM: ");
         Serial.println(beatsPerMinute);
         if (ir < 50000)
         Serial.print(" No finger?");
         Serial.print("ir:");
         Serial.print(ir);
         Serial.println();
      }
  }
#endif

if(Serial2.available()){
      //request = Serial2.readStringUntil('\n');
      //Serial.print("BPM: ");
      //Serial.println(request);
      //if(request==1){
      //Serial.print("BPM: ");
      //Serial.println(request);
      //if (ir < 50000)
      //Serial2.print(" No finger?");
      //delay(100); // Delay para que la Tiva C tenga tiempo para leer la respuesta
      //clavedos = Serial.parseInt();
      //colorWipe(strip.Color(0, 255, 0), 10); // Green      
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


   //Configuración
void setup() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
  Serial.begin(115200); //Inicialización de la comunicación serial con el monitor serial
  Serial2.begin(115200); //Comunicación UART con ESP32
  SPI.setModule(0); //Implementación del módulo SPIO por la pantalla ili9341
  
  Serial.print("Inicializando la tarjeta SD...");
  if(!SD.begin(CS_PIN)){
    Serial.println("¡Falló la inicialización de la tarjeta!D:");
  }
  Serial.println("Inicialización realizada:D");
  
  //Configuración de los SW  
  pinMode(BSENSE, INPUT_PULLUP); //Configuración de SW1
  pinMode(BSD, INPUT_PULLUP); //Configuración de SW1
  pinMode(RED_LED, OUTPUT); //LED de verificación del funcionamiento del BSENSE

  //Interfaz gráfica de la pantalla TFT ili9341
  Serial.println("Inicio"); 
  LCD_Init();
  LCD_Clear(0x00);

  FillRect(0, 0, 320, 240, 0x421b);
  String text1 = "BPM:";
  LCD_Print(text1, 128, 116, 2, 0xffff, 0x421b);
  delay(1000);
}

//Loop Infinito
void loop() {
        for(int x =0; x<320-107; x++){
    delay(15);   
    int anim2=(x/10)%3;
    LCD_Sprite(107, 20, 107, 80, heart, 3, anim2, 0, 0);
    }

  //Verificación de BSENSE para realizar la medición del sensor
  if(digitalRead(BSENSE) == LOW && (millis()-lastDebounceTime)>debounceDelay){
    digitalWrite(RED_LED, HIGH); //Control LED
    lastDebounceTime = millis();
    clave=1;
    Serial2.print(clave); //Activación neopixel via UART2
    
  //}
    if (Serial2.available()){ //Utilizar Serial2 para comunicarse con el ESP32
    receivedvaluesensor=Serial2.parseInt();
    Serial.println("BPM:");
    Serial.println(receivedvaluesensor);
    
    if(receivedvaluesensor<35){
      receivedvaluesensor=35;
      } else if(receivedvaluesensor>200){
        receivedvaluesensor=200;
      }

    //FillRect(120, 140, 2, 0x1105, 0x421b);
    
    //Impresión del bpm
    String BPM=String(receivedvaluesensor); //Impresión en la pantalla TFT ili9341

    //Serial2.print("BPM:");
    //Serial2.println(BPM);
    
    if (receivedvaluesensor >= 35 && receivedvaluesensor <= 99) {
      // Si el valor está entre 35 y 99, se agrega un espacio en el tercer dígito
      BPM = String(String(receivedvaluesensor).substring(0, 2) + " " + String(receivedvaluesensor).substring(2));
      } else if (receivedvaluesensor >= 100 && receivedvaluesensor <= 200) {
        // Si el valor está entre 100 y 200, se muestran los tres dígitos
        BPM = String(receivedvaluesensor);
        }
    LCD_Print(BPM, 120, 140, 2, 0x1105, 0xF7BD);
    FillRect(120, 140, 2, 0x1105, 0x421b);
    Serial2.print("BPM:");
    Serial2.println(BPM);
  }
  }
    
    /*for(int x =0; x<320-107; x++){
    delay(15);   
    int anim2=(x/10)%3;
    LCD_Sprite(107, 20, 107, 80, heart, 3, anim2, 0, 0);
    }
    }*/
        
   //Verificación de BSD para guardar en la SD
   if(digitalRead(BSD)==LOW && (millis()-lastDebounceTime)>debounceDelay){
    lastDebounceTime=millis();
    //if(dataFile){
      File dataFile=SD.open("datalog.txt", FILE_WRITE); //Apertura del archivo en modo escritura
      if(dataFile){
        dataFile.println(receivedvaluesensor);
        dataFile.close(); //Se cierra el archivo
        Serial.println("Se ha guardado exitosamente la medición de la temperatura en la tarjeta SD.");
        Serial.println(receivedvaluesensor);
        }
        //Si no fue posible abrir el archivo→ERROR
        else{
          Serial.println("Error al abrir el archivo datalog.txt y no se ha guardado la medida de temperatura");
        }
        String text2 = "Almacenada en SD";
        LCD_Print(text2, 20, 190, 2, 0xffff, 0x421b);
        delay(1000);
        FillRect(20, 190, 2000, 40, 0x421b);
        }
        //delay(20);
}