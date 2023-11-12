//***********************************************************************************
//Universidad del Valle de Guatemala
//Dulce Nicole Monney Paiz, 21549
//BE3029 - Electrónica Digital 2
//Proyecto 2 – SPI, TFT & SD
//***************************************************************************************************************************************

//Librerías
#include <SPI.h>
#include <SD.h>

#include "pitches.h"

#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include <SPI.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"
//*********************************************************************************** 

//Definición de pines
/*SPIO en 0
MOSI a PA_5
MISO a PA_4
SCK a PA_2
CS a PA_3 
SD_CS a PB_3*/

#define LCD_RST PD_0
#define LCD_DC PD_1
#define LCD_CS PA_3 

#define BSENSE PF_4 //Botón para preguntar el valor del sensor al ESP32 - SW1
#define BSD PF_0 //Botón para guardar el valor de la medición  del sensor en el SD - SW2
#define RXp2 PD6
#define TXp2 PD7
#define CS_PIN PB_3 //aka pin 12 para el chip select del SD

#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_E4  330
#define NOTE_C5  329
#define NOTE_G5  784
#define buzzerpin 40
//***************************************************************************************************************************************

//Variables globales 
float receivedvaluesensor; //Variable determinada para recibir el valor del sensor LM35 y que se enviará a la ILI9341 & a la SD
int clave=0; //Aviso al ESP32 de presión del botón BSENSE
unsigned long lastDebounceTime=0;
unsigned long debounceDelay=50;

int melody1[] = { //Melodía medición
  NOTE_A4, NOTE_C5, NOTE_B4, NOTE_E4};
int noteDurations1[] = {
1, 1, 1, 2,
};

int melody2[] = { //Melodía SD
  NOTE_C5, NOTE_C5, NOTE_C5, NOTE_G5};
int noteDurations2[] = {
4, 4, 4, 2,
};
//***********************************************************************************

//Prototipos de funciones
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);

void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset);

void melodysensor(void);
void melodySD(void);
//***************************************************************************************************************************************

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
  pinMode(RED_LED, OUTPUT);
  pinMode(buzzerpin, OUTPUT);
  
  Serial.println("Inicio");
  LCD_Init();
  LCD_Clear(0x00);

  FillRect(0, 0, 319, 210, 0x421b);
  String text1 = "Temperatura:";
  LCD_Print(text1, 65, 100, 2, 0xffff, 0x421b);
  LCD_Bitmap(20, 15, 64, 64, temperature);
  delay(5000);
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
    Serial.print("LM35:");
    Serial.println(receivedvaluesensor);
    melodysensor(); //Agregar melodía indicativa de que se realizó una medición con el sensor
    //Impresión de temperatura 
    int tempInt = receivedvaluesensor*100;
    //Cálculos mediante módulo
    int tempunidad = (tempInt/1)%10; //Cálculo del decimal del valor de temperatura, lo multiplica por 10 para convertirlo en una fracción de 10 grados y almacena el resultado como un número entero en la variable tempDecimal.
    int tempdecena = (tempInt/10)%10;
    int tempdecimal = (tempInt/100)%10;
    int tempcentena = (tempInt/1000)%10;

    String UNI=String(tempunidad);
    String DEK=String(tempdecena);
    String DES=String(tempdecimal);
    String CEN=String(tempcentena);

    String tmp=CEN+DES+"."+DEK+UNI;
    LCD_Print(tmp, 120, 120, 2, 0x1105, 0xF7BD);}
        
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
        //Agregar melodía indicativa que se almacenó un dato en la memoria SD
        }
        //Si no fue posible abrir el archivo→ERROR
        else{
          Serial.println("Error al abrir el archivo datalog.txt y no se ha guardado la medida de temperatura");
        }
        melodySD();
        String text2 = "Almacenada en SD:D";
        LCD_Print(text2, 20, 150, 2, 0xffff, 0x421b);
        delay(1000);
        FillRect(20, 150, 2000, 40, 0x421b);
        }
        delay(20);
}

//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER) 
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40|0x80|0x20|0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
//  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on 
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_DC, LOW);
  SPI.transfer(cmd);
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_DC, HIGH);
  SPI.transfer(data);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);   
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);   
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);   
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);   
  LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c){  
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);   
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
    }
  digitalWrite(LCD_CS, HIGH);
} 
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i,j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8); 
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);  
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y+h, w, c);
  V_line(x  , y  , h, c);
  V_line(x+w, y  , h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y+i, w, c);
  }
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background) 
//***************************************************************************************************************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;
  
  if(fontSize == 1){
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if(fontSize == 2){
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }
  
  char charInput ;
  int cLength = text.length();
  Serial.println(cLength,DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength+1];
  text.toCharArray(char_array, cLength+1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1){
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2){
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]){  
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+width;
  y2 = y+height;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      //LCD_DATA(bitmap[k]);    
      k = k + 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset){
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW); 

  unsigned int x2, y2;
  x2 =   x+width;
  y2=    y+height;
  SetWindows(x, y, x2-1, y2-1);
  int k = 0;
  int ancho = ((width*columns));
  if(flip){
  for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width -1 - offset)*2;
      k = k+width*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k - 2;
     } 
  }
  }else{
     for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width + 1 + offset)*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k + 2;
     } 
  }
    }
  digitalWrite(LCD_CS, HIGH);
}

//Funciones musicales 
void melodysensor(void){
  for (int thisNote = 0; thisNote < 4; thisNote++) {
    int noteDuration = 100/noteDurations1[thisNote];
    tone(buzzerpin, melody1[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration + 50;     //delay between pulse
    delay(pauseBetweenNotes);
    noTone(buzzerpin);         // stop the tone playing
  }
  }

void melodySD(void){
  for (int thisNote = 0; thisNote < 4; thisNote++) {
    int noteDuration = 100/noteDurations2[thisNote];
    tone(buzzerpin, melody2[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration + 50;     //delay between pulse
    delay(pauseBetweenNotes);
    noTone(buzzerpin);         // stop the tone playing
  }
  }
