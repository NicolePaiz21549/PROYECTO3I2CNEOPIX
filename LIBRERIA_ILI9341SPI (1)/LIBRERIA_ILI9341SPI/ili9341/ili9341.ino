//***********************************************************************************
//Universidad del Valle de Guatemala
//Dulce Nicole Monney Paiz, 21549
//BE3029 - Electrónica Digital 2
//Proyecto 3 – I2C & NeoPixel
//***************************************************************************************************************************************

//Librerías
//*********************************************************************************** 

//Definición de pines
/* Colocación de pines en la pantalla TFT ili9341
SPIO en 0
MOSI a PA_5 (Tanto general como para SD)
MISO a PA_4 (Tanto general como para SD)
SCK a PA_2 (Tanto general como para SD)
CS a PA_3 
SD_CS a PB_3*/

//Para pantalla TFT ili9341 
#define BSENSE PF_4 //Botón para preguntar el valor del sensor al ESP32 - SW1
#define BSD PF_0 //Botón para guardar el valor de la medición  del sensor en el SD - SW2
#define RXp2 PD6
#define TXp2 PD7
//***************************************************************************************************************************************

//Variables globales 
float receivedvaluesensor; //Variable determinada para recibir el valor del sensor LM35 y que se enviará a la ILI9341 & a la SD
int clave=0; //Aviso al ESP32 de presión del botón BSENSE
unsigned long lastDebounceTime=0;
unsigned long debounceDelay=50;
//***********************************************************************************

//Prototipos de funciones

//***************************************************************************************************************************************

//Configuración
void setup() {
  Serial.begin(115200); //Inicialización de la comunicación serial con el monitor serial
  Serial2.begin(115200); //Comunicación UART con ESP32
  //SPI.setModule(0); //Implementación del módulo SPIO por la pantalla ili9341
  
  //Serial.print("Inicializando la tarjeta SD...");
  //if(!SD.begin(CS_PIN)){
    //Serial.println("¡Falló la inicialización de la tarjeta!D:");
  //}
  //Serial.println("Inicialización realizada:D");
  
  //Configuración de los SW  
  pinMode(BSENSE, INPUT_PULLUP); //Configuración de SW1
  pinMode(BSD, INPUT_PULLUP); //Configuración de SW1
  pinMode(RED_LED, OUTPUT);
}

//Loop Infinito
void loop() {
  //Verificación de BSENSE para realizar la medición del sensor
  if(digitalRead(BSENSE) == LOW && (millis()-lastDebounceTime)>debounceDelay){
    digitalWrite(RED_LED, HIGH); 
    lastDebounceTime = millis();
    clave=1; 
    Serial2.print(clave); //Pedir al ESP32 el valor del sensor
    Serial2.print("∖n");
  }
  
  if (Serial2.available()){ //Utilizar Serial2 para comunicarse con el ESP32
    receivedvaluesensor=Serial2.parseFloat();
    Serial.print("BPM:"); //Hay un delay de entre 10 a 15 segundos para recibir el valor del sensor actual por el FFT
    Serial.println(receivedvaluesensor);
    }
        
   //Verificación de BSD para guardar en la SD
   /*if(digitalRead(BSD)==LOW && (millis()-lastDebounceTime)>debounceDelay){
    lastDebounceTime=millis();
    //if(dataFile){
      File dataFile=SD.open("datalog.txt", FILE_WRITE); //Apertura del archivo en modo escritura
      if(dataFile){
        dataFile.println(receivedvaluesensor);
        dataFile.close(); //Se cierra el archivo
        Serial.println("Se ha guardado exitosamente la medición de la temperatura en la tarjeta SD.");
        Serial.println(receivedvaluesensor);
        //Agregar melodía indicativa que se almacenó un dato en la memoria SD
        }
        //Si no fue posible abrir el archivo→ERROR
        else{
          Serial.println("Error al abrir el archivo datalog.txt y no se ha guardado la medida de temperatura");
        }
        String text2 = "Almacenada en SD:D";
        LCD_Print(text2, 20, 150, 2, 0xffff, 0x421b);
        delay(1000);
        FillRect(20, 150, 2000, 40, 0x421b);
        }
        delay(20);*/
}
//***************************************************************************************************************************************

//FUNCIONES PARA EL FUNCIONAMIENTO DE LA PANTALLLA TFT ILI9341
//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
