//Librerías usadas
#include <LiquidCrystal_I2C.h> //LCD 2004
#include <TM1637Display.h> //TM1637
#include <NewPing.h> //Lectura HC-SR04
#include <Servo.h> //Servomotores

// Definición de pines
const int buzzer = 0; //pin del buzzer
const int relay_pin = 2;      // Pin del relé (electroimán)
const int laser_servoPin = 5; //Pin del servomotor del laser      
const int laser_pin = 4;      // Pin del láser
const int hc_head_servoPin = 3; //Pin del servomotor del laser  
const int trigger_pin_1 = 7; // Pin del Arduino conectado al trigger del sensor ultrasónico #1
const int echo_pin_1 = 6; // Pin del Arduino conectado al echo del sensor ultrasónico #1
const int clk = 8; // Pin del Arduino conectado al CLK del TM1637 principal
const int dio = 9; // Pin del Arduino conectado al DIO del TM1637 principal
const int trigger_pin_2 = 11; // Pin del Arduino conectado al trigger del sensor ultrasónico de la bandera
const int echo_pin_2 = 10; // Pin del Arduino conectado al echo del sensor ultrasónico de la bandera
const int trigger_pin_3 = 12; // Pin del Arduino conectado al trigger del sensor ultrasónico del suelo
const int echo_pin_3 = 13; // Pin del Arduino conectado al echo del sensor ultrasónico del suelo
const int next_button_pin = A0; //Botón para pasar al siguiente paso
const int on_button_pin = A1; // Botón para encender el electroiman
const int off_button_pin = A2; // Botón para apagar el electroiman
const int one_two_button_pin = A3; // Botón para encender seleccionar "1" o "2"

// Variables de estado
int relayState = 0;   // Estado inicial del relé
int number_step = 1;
int initial_step = 0;
int anvil = 1;
const int max_distance = 300; /// Distancia máxima que queremos medir en el ultrasonico #1 (en centímetros).

#define ALTURA_ACTIVACION 80 // Altura para activar el buzzer y brazo del hc sr04 del headform
#define ALTURA_MAXIMA 2 //Altura máxima entre velocimetro y bandera

bool buzzer_activated = false; // Variable para evitar que el buzzer se repita
int initial_pos_laser = 0; //posición inicial del brazo laser
int initial_pos_hc_head = 90; //posición inicial del brazo del HC-SR04 del headform
int final_pos_laser = 45; //posición final del brazo laser, alineado al centro del anvil
int final_pos_hc_head = 45; //posición final del brazo del HC-SR04 del headform, alineado al centro del anvil

const int hc_flat = 2; //Distancia en cm del sensor ultrasonico del headform con la superficie del yunque plano
const int hc_sphere = 3; //Distancia en cm del sensor ultrasonico del headform con la superficie del yunque semiesferico
const int hc_2_ground = 0; //MODIFICAAAAAAAAAAAAAR  Distancia(cm) fija del ultrasonico de la bandera (medido desde su parte frontal) con el suelo

//Configuraciones iniciales de modulos
LiquidCrystal_I2C lcd(0x27, 20, 4); // Dirección I2C 0x27, 20 columnas y 4 filas
NewPing main_1(trigger_pin_1, echo_pin_1, max_distance); // Configuración del sensor ultrasónico #1
NewPing flag_2(trigger_pin_2, echo_pin_2, max_distance); // Configuración del sensor ultrasónico BANDERA
NewPing ground_3(trigger_pin_3, echo_pin_3, max_distance); // Configuración del sensor ultrasónico SUELO
TM1637Display display(clk, dio);
Servo laser, hc_head;

// Variables para el debounce
unsigned long lastButtonPress = 0;  // Para controlar el tiempo de la última pulsación boton next
unsigned long lastButtonPress_2 = 0; // Para controlar el tiempo de la última pulsación boton 1_2
unsigned long debounceDelay = 250;   // Tiempo para evitar el rebote (en milisegundos)

// Definir el número de lecturas a promediar del ultrasónico
#define NUM_READINGS 5 // Número de lecturas para promediar

// Variable para almacenar el último estado del botón
int lastButtonState = LOW;  // El botón está inicialmente en bajo (no presionado)
int lastButtonState_2 = LOW;  // El botón 1_2 está inicialmente en bajo (no presionado)

// Variable de control para saber si estamos mostrando el paso
bool stepActive = true;  // Puede ser 'true' cuando se quiera mostrar el paso, 'false' cuando no

bool countdown_active = false; // Estado de la cuenta regresiva

void setup() {
  //Asignación tipo de pin (lectura/escritura)
  pinMode(buzzer, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(laser_pin, OUTPUT);
  pinMode(on_button_pin, INPUT); 
  pinMode(off_button_pin, INPUT);
  pinMode(next_button_pin, INPUT);
  pinMode(one_two_button_pin, INPUT);

  //Inicialización modulos
  lcd.init(); // Initialize the LCD I2C display
  lcd.backlight(); //abre la luz de fondo

  laser.attach(laser_servoPin);
  hc_head.attach(hc_head_servoPin);
  laser.write(initial_pos_laser);
  hc_head.write(initial_pos_hc_head);

  display.setBrightness(7); // Configurar el brillo de la pantalla del display TM1637

  //Inicialización del serial para seguir a tiempo real los datos a visualizar de interés
  //Serial.begin(9600);

  //escritura de variables al iniciar
  digitalWrite(relay_pin, relayState); // Asegurar que inicia en estado apagado
  digitalWrite(laser_pin, LOW);

  //digitalWrite(buzzer, buzzer_activated);
}



void loop() {

  button_step();

  serial();

  if(number_step == 1){
    lcd_setup();
    }
  else if(number_step == 2){
    step_2();
    }
  else if(number_step == 3){
    step_3();
  }
  else if(number_step == 4){
    //stepActive = false;  // Ahora el display no mostrará el paso
    step_4();
  }
  else if(number_step == 5){
    step_5();
  }
  else if(number_step == 6){
    step_6();
  }
  else if(number_step == 7){
    step_7();
  }
  else if(number_step == 8){
    step_8();
  }
  else if(number_step == 9){
    step_9();
  }
  else if(number_step == 10){
    step_10();
  }


  digitalWrite(relay_pin, relayState);   // Aplicar el estado al relé

  // Muestra el número de paso con el formato "P:1", "P:2", etc.
  //showStep(number_step);

}


void button_step(){
  unsigned long currentMillis = millis();  // Obtiene el tiempo actual en milisegundos
  int buttonState = digitalRead(next_button_pin);  // Lee el estado del botón

  // Si el estado del botón ha cambiado (de LOW a HIGH)
  if (buttonState == HIGH && lastButtonState == LOW && currentMillis - lastButtonPress >= debounceDelay) {
    lastButtonPress = currentMillis;  // Actualiza el tiempo de la última pulsación
    number_step += 1;  // Incrementa el contador
    lcd.clear();       // Limpiar la pantalla
    if (number_step > 10) {  // Si el número de paso excede 11, reinicia a 2
      number_step = 2;
  }}
  // Actualiza el último estado del botón
  lastButtonState = buttonState;
}

void type_anvil(){
  unsigned long currentMillis = millis();  // Obtiene el tiempo actual en milisegundos
  int buttonState_2 = digitalRead(one_two_button_pin);  // Lee el estado del botón 1_2

  // Si el estado del botón ha cambiado (de LOW a HIGH)
  if (buttonState_2 == HIGH && lastButtonState_2 == LOW && currentMillis - lastButtonPress_2 >= debounceDelay) {
    lastButtonPress_2 = currentMillis;  // Actualiza el tiempo de la última pulsación
    anvil += 1;  // Incrementa el contador
    lcd.clear();       // Limpiar la pantalla
    if (anvil > 2) {  // Si el número de paso excede 11, reinicia a 2
      anvil = 1;
  }}
  // Actualiza el último estado del botón
  lastButtonState_2 = buttonState_2;

}

void serial(){
  Serial.print("Number_step: ");
  Serial.print(number_step);
  Serial.print(" | Relay state: ");
  Serial.print(relayState);
  Serial.print(" | Buzzer state: ");
  Serial.println(buzzer_activated);
}

void update_electromagnet(){
  int on_button = digitalRead(on_button_pin);
  int off_button = digitalRead(off_button_pin);

   // Si el botón de encendido se presiona
  if (on_button == 1 && off_button == 0) {
    delay(150); // Pequeño debounce
    relayState = 1;
  }

  // Si el botón de apagado se presiona
  /*if (on_button == 0 && off_button == 1) {
    delay(150); // Pequeño debounce
    relayState = 0;
  }*/
  digitalWrite(relay_pin, relayState);   // Aplicar el estado al relé
}

void lcd_setup(){
  lcd.setCursor(0, 0);            // move cursor the 1st row
  lcd.print("BIENVENIDO ESTIMADO"); 
  lcd.setCursor(3, 1);            // move cursor to the 2nd row
  lcd.print("USUARIO TITON A"); // print message at the 2nd row
  lcd.setCursor(3, 2);            // move cursor to the 3rd row
  lcd.print("LA MAQUINA DE"); // print message at the 3rd row
  lcd.setCursor(1, 3);            // move cursor to the 4th row
  lcd.print("IMPACTOS VERTICAL.");   // print message the 4th row
}

void step_2(){
  lcd.setCursor(1, 0);            // move cursor the 1st row
  lcd.print("DESCIENDA EL CARRO"); 
  lcd.setCursor(1, 1);            // move cursor to the 2nd row
  lcd.print("DE SUJECION CON EL"); // print message at the 2nd row
  lcd.setCursor(1, 2);            // move cursor to the 3rd row
  lcd.print("WINCH HASTA ACOPLAR"); // print message at the 3rd row
  lcd.setCursor(1, 3);            // move cursor to the 4th row
  lcd.print("EL CARRO DE CAIDA");   // print message the 4th row
  update_electromagnet();
}

void step_3(){
  lcd.setCursor(0, 0);            // move cursor the 1st row
  lcd.print("'SIGUIENTE' SI ESTA"); 
  lcd.setCursor(1, 1);            // move cursor to the 2nd row
  lcd.print("SUJETO EL CARRITO."); // print message at the 2nd row
  lcd.setCursor(2, 2);            // move cursor to the 3rd row
  lcd.print("DE LO CONTRARIO,"); // print message at the 3rd row
  lcd.setCursor(1, 3);            // move cursor to the 4th row
  lcd.print("PRESIONAR 'AGARRE'");   // print message the 4th row
  update_electromagnet();
}

void step_4(){
  lcd.setCursor(0, 0);            // move cursor the 1st row
  lcd.print("SUBA EL CARRITO UNA"); 
  lcd.setCursor(1, 1);            // move cursor to the 2nd row
  lcd.print("ALTURA DE 1M. MIN."); // print message at the 2nd row
  lcd.setCursor(0, 2);            // move cursor to the 3rd row
  lcd.print("ESCOJA ANILLO,ANVIL,"); // print message at the 3rd row
  lcd.setCursor(2, 3);            // move cursor to the 4th row
  lcd.print("HEADFORM Y CASCO");   // print message the 4th row
  flag_hcsr04();
}

void step_5(){
  type_anvil();
  lcd.setCursor(1, 0);            // move cursor the 1st row
  lcd.print("SELECCIONAR ANVIL"); 
  lcd.setCursor(1, 1);            // move cursor to the 2nd row
  lcd.print("CON BOTON '1 | 2':"); // print message at the 2nd row
  lcd.setCursor(0, 2);            // move cursor to the 3rd row
  lcd.print("'1': 'SEMIESFERICO'."); // print message at the 3rd row
  lcd.setCursor(0, 3);            // move cursor to the 4th row
  lcd.print("'2': 'PLANO'----> ");   // print message the 4th row
  lcd.print(anvil);   // print message the 4th row
}

void step_6(){
  lcd.setCursor(0, 0);            // move cursor the 1st row
  lcd.print("CON APOYO DEL LASER"); 
  lcd.setCursor(0, 1);            // move cursor to the 2nd row
  lcd.print("SELECCIONAR PUNTO DE"); // print message at the 2nd row
  lcd.setCursor(0, 2);            // move cursor to the 3rd row
  lcd.print("IMPACTO EN EL CASCO"); // print message at the 3rd row
  lcd.setCursor(4, 3);            // move cursor to the 4th row
  lcd.print("PERTINENTE.");   // print message the 4th row
  laser.write(final_pos_laser);
  digitalWrite(laser_pin, HIGH); 
}

void step_7(){
  digitalWrite(laser_pin, LOW);
  laser.write(initial_pos_laser);
  lcd.setCursor(0, 0);            // move cursor the 1st row
  lcd.print("DESCIENDA EL CARRITO"); 
  lcd.setCursor(0, 1);            // move cursor to the 2nd row
  lcd.print("HASTA TOCAR EL ANVIL"); // print message at the 2nd row
  lcd.setCursor(0, 2);            // move cursor to the 3rd row
  lcd.print("Y CONFIRME PUNTO DE"); // print message at the 3rd row
  lcd.setCursor(2, 3);            // move cursor to the 4th row
  lcd.print("IMPACTO EN CASCO");   // print message the 4th row
}

void step_8(){
  lcd.setCursor(0, 0);            // move cursor to the 1st row
  lcd.print("COMPRUEBE DISTANCIA"); // print message at the 1st row
  lcd.setCursor(2, 1);            // move cursor to the 2nd row
  lcd.print("ENTRE BANDERA Y");   // print message at the 2nd row
  lcd.setCursor(1, 2);            // move cursor to the 3rd row
  lcd.print("VELOCIMETRO MENOR A"); // print message at the 3rd row
  lcd.setCursor(0, 3);            // move cursor to the 4th row
  lcd.print("2CM, SINO MODIFIQUE.");  // print message the 4th row
  flag_velocimeter();
}

void step_9(){
  lcd.setCursor(0, 0);            // move cursor to the 1st row
  lcd.print("SUBA EL CARRITO A LA"); // print message at the 1st row
  lcd.setCursor(7, 1);            // move cursor to the 2nd row
  lcd.print("ALTURA");   // print message at the 2nd row
  lcd.setCursor(2, 2);            // move cursor to the 3rd row
  lcd.print("DE CAIDA DESEADA"); // print message at the 3rd row
  lcd.setCursor(0, 3);            // move cursor to the 4th row
  lcd.print("(ALTURA MAXIMA: 3M)");  // print message the 4th row
  main_hcsr04();
}

void step_10(){
  lcd.setCursor(1, 0);            // move cursor to the 1st row
  lcd.print("PRESIONE EL BOTON"); // print message at the 1st row
  lcd.setCursor(1, 1);            // move cursor to the 2nd row
  lcd.print("DE CONFIRMACION DE");   // print message at the 2nd row
  lcd.setCursor(0, 2);            // move cursor to the 3rd row
  lcd.print("SOLTADO, ALEJESE POR"); // print message at the 3rd row
  lcd.setCursor(1, 3);            // move cursor to the 4th row
  lcd.print("SEGURIDAD, GRACIAS");  // print message the 4th row
  int off_button = digitalRead(off_button_pin);

  // Si se presiona el botón y la cuenta no ha comenzado
  if (off_button == 1 && !countdown_active) {
    delay(150); // Pequeño debounce

    hc_head.write(initial_pos_hc_head);

    countdown_active = true; // Activa la cuenta regresiva

    // Cuenta regresiva de 10 a 0
    for (int i = 10; i >= 0; i--) {
    display.showNumberDec(i, true); // Muestra el número en el TM1637

    // Hacer sonar el buzzer una sola vez
    digitalWrite(buzzer, HIGH);
    delay(100); // Bip corto
    digitalWrite(buzzer, LOW);

    delay(900); // Espera el resto del segundo antes del siguiente número
  }
  
    countdown_active = false; // Resetea la variable para poder reiniciar
    relayState = 0;
    lcd.clear();
    number_step = 2;
}}


void main_hcsr04(){
  long total = 0;
  // Tomar varias lecturas y sumarlas
  for (int i = 0; i < NUM_READINGS; i++) {
    total += main_1.ping_cm(); // Obtener la lectura actual
    delay(50); // Pequeño retraso para la siguiente medición
  }

  // Calcular el promedio de las lecturas
  long average = total / NUM_READINGS;
  
  if(anvil == 1){
    // Mostrar el promedio en el display y en el monitor serial
    display.showNumberDec(average+hc_sphere); // Mostrar en el display TM1637
  }else if(anvil == 2){
    display.showNumberDec(average+hc_flat); // Mostrar en el display TM1637
  }


  if(average > ALTURA_ACTIVACION){
    hc_head.write(initial_pos_hc_head);
  }
  else{
    hc_head.write(final_pos_hc_head);
  }

}

void flag_hcsr04(){
  long total_2 = 0;
  // Tomar varias lecturas y sumarlas
  for (int i = 0; i < NUM_READINGS; i++) {
    total_2 += flag_2.ping_cm(); // Obtener la lectura actual
    delay(50); // Pequeño retraso para la siguiente medición
  }

  // Calcular el promedio de las lecturas
  long average_2 = (total_2 / NUM_READINGS) + hc_2_ground;
  
  // Mostrar el promedio en el display y en el monitor serial
  display.showNumberDec(average_2); // Mostrar en el display TM1637

  // Si la distancia es 100 cm y el buzzer no ha sonado antes, lo activamos
  if (average_2 >= ALTURA_ACTIVACION && average_2 <(ALTURA_ACTIVACION+10) && !buzzer_activated) {
    digitalWrite(buzzer, HIGH);
    delay(3000);  // Suena por 3 segundos
    digitalWrite(buzzer, LOW);
    buzzer_activated = true; // Marcamos que ya se activó
  }

  // Si la distancia cambia a otro valor diferente de 100 cm, reiniciamos la variable
  if (average_2 < ALTURA_ACTIVACION || average_2 > (ALTURA_ACTIVACION+10)) {
    buzzer_activated = false;
  }
}

void flag_velocimeter(){
  long total_3 = 0;
  // Tomar varias lecturas y sumarlas
  for (int i = 0; i < NUM_READINGS; i++) {
    total_3 += flag_2.ping_cm(); // Obtener la lectura actual
    delay(50); // Pequeño retraso para la siguiente medición
  }

  long total_4 = 0;
  // Tomar varias lecturas y sumarlas
  for (int i = 0; i < NUM_READINGS; i++) {
    total_4 += ground_3.ping_cm(); // Obtener la lectura actual
    delay(50); // Pequeño retraso para la siguiente medición
  }

  // Calcular el promedio de las lecturas
  long average_3 = abs(((total_3 / NUM_READINGS) + hc_2_ground)-(total_4/NUM_READINGS));
  
  // Mostrar el promedio en el display y en el monitor serial
  display.showNumberDec(average_3); // Mostrar en el display TM1637

  // Si la distancia es 100 cm y el buzzer no ha sonado antes, lo activamos
  if (average_3 <= ALTURA_MAXIMA && average_3 >= 0 && !buzzer_activated) {
    digitalWrite(buzzer, HIGH);
    delay(3000);  // Suena por 3 segundos
    digitalWrite(buzzer, LOW);
    buzzer_activated = true; // Marcamos que ya se activó
  }

  // Si la distancia cambia a otro valor diferente de 100 cm, reiniciamos la variable
  if (average_3 > ALTURA_MAXIMA || average_3 < 0) {
    buzzer_activated = false;
  }
}

/* void showStep(int number_step) {
  // Función para mostrar el paso en el formato "P:1", "P:2", etc.

  uint8_t data[] = {0x00, 0x00, 0x00, 0x00};  // Inicializa el array de datos para los segmentos

  if (stepActive) {  // Solo muestra "P1", "P2", etc. cuando stepActive es verdadero
    switch(number_step) {
      case 1:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_B | SEG_C;  // '1'
        break;
      case 2:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_B | SEG_D | SEG_E | SEG_G;  // '2'
        break;
      case 3:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_B | SEG_G | SEG_D | SEG_C;  // '3'
        break;
      case 5:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_F | SEG_G | SEG_D | SEG_C;  // '5'
        break;
      case 6:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_F | SEG_G | SEG_D | SEG_C | SEG_E;  // '6'
        break;
      case 7:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_B | SEG_C ;  // '7'
        break;
      case 8:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G ;  // '8'
        break;
      case 11:
        data[1] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;  // 'P'
        data[2] = SEG_B | SEG_C ;  // '1'
        data[3] = SEG_B | SEG_C ;  // '1'
        break;

    }
  } else {
    data[1] = 0x00;  // Apagar la posición 1
    data[2] = 0x00;  // Apagar la posición 2
  }

  // Muestra los segmentos en el display
  display.setSegments(data);
}
*/

