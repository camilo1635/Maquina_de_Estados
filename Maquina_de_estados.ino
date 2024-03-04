#include "StateMachineLib.h"
#include "DHT.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include "AsyncTaskLib.h"

const int potenciometerPin = A6;
const int sensorPin = A5;

#define DHTPIN 6
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Display
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// Keypad setup
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {5, 4, 3, 2};
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

char correctPassword[] = "1234"; // Clave correcta
char enteredPassword[4]; // Se reservan 5 caracteres para almacenar la clave ingresada (incluyendo el carácter nulo de terminación)
byte passwordAttempts = 0; // Contador de intentos de clave incorrecta
boolean passwordReady = false; // Bandera para indicar que la contraseña está lista para ser validada

// State Alias
enum State {
  Inicio = 0,
  Monitoreo = 1,
  Alarma = 2,
  Bloqueado = 3
};

// Input Alias
enum Input {
  Reset = 0,
  Forward = 1,
  Backward = 2,
  Unknown = 3,
};

// Create new StateMachine
StateMachine stateMachine(4, 7);

Input input;

// Setup the State Machine
void setupStateMachine() {
  // Add transitions
  stateMachine.AddTransition(Inicio, Monitoreo, []() { return input == Forward; });
  stateMachine.AddTransition(Inicio, Bloqueado, []() { return input == Backward; });
  stateMachine.AddTransition(Monitoreo, Alarma, []() { return input == Forward; });
  stateMachine.AddTransition(Monitoreo, Bloqueado, []() { return input == Backward; });
  stateMachine.AddTransition(Alarma, Monitoreo, []() { return input == Backward; });
  stateMachine.AddTransition(Bloqueado, Monitoreo, []() { return input == Forward; });
  stateMachine.AddTransition(Bloqueado, Inicio, []() { return input == Reset; });
  
  // Add actions
  stateMachine.SetOnEntering(Inicio, funcionClave); //llamar función del estado
  stateMachine.SetOnEntering(Monitoreo, funcionMonitoreo);
  stateMachine.SetOnEntering(Alarma, funcionAlarma);
  stateMachine.SetOnEntering(Bloqueado, funcionBloqueo);

  stateMachine.SetOnLeaving(Inicio, funcionFinClave);
  stateMachine.SetOnLeaving(Monitoreo, funcionFinMonitoreo);
  stateMachine.SetOnLeaving(Alarma, funcionFinAlarma);
  stateMachine.SetOnLeaving(Bloqueado, funcionFinBloqueo);
}

//Tareas
void Sensores(void);
AsyncTask Task1(2300, true, Sensores);
void readPhoto(void);
AsyncTask Task2(1000, true, readPhoto);
void Potenciometer(void);
AsyncTask Task3(1500, true, Potenciometer);
void alarma(void);
AsyncTask Task4(3000, true, alarma);
void Bloq(void);
AsyncTask Task5(5000, true, Bloq);
void Contrasena(void);
AsyncTask Task6(800, false, Contrasena);


void funcionClave(){
  Serial.println("estado inicio");
  Serial.println(input);
  Task6.Start();
}

void funcionFinClave(){
  Task6.Stop();
}  


void funcionMonitoreo(){
  Serial.println("estado monitoreo");
  Serial.println(input);
  Task1.Start();
  Task2.Start();
  Task3.Start();
}

void funcionFinMonitoreo(){
  Task1.Stop();
  Task2.Stop();
  Task3.Stop();
}  

void funcionAlarma(){
  Serial.println("estado alarma");
  Serial.println(input);
  Task4.Start();
}  

void funcionFinAlarma(){
  Serial.println("fin estado alarma");
  Serial.println(input);
  Task4.Stop();
}

void funcionBloqueo(){
  Serial.println("estado bloqueo");
  Serial.println(input);
  Task5.Start();
}  

void funcionFinBloqueo(){
  Task5.Stop();
}

//leds
const byte greenLed = A7;
const byte blueLed = A8;
const byte redLed = A9;



void setup() 
{
  Serial.begin(9600);
  
  lcd.begin(16, 2); // Inicializar la pantalla LCD
  dht.begin();
  
  Serial.println("Starting State Machine...");
  setupStateMachine();  
  Serial.println("Start Machine Started");

  
  
  // Initial state
  stateMachine.SetState(Inicio, true, true);
  funcionClave(); // Llama a la función clave después de inicializar la pantalla LCD
  
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(redLed, OUTPUT);
}

void loop() 
{ 
  // Read user input
  Task6.Update();
  Task1.Update();
  Task2.Update();
  Task3.Update();
  Task4.Update();
  Task5.Update();
  
  // Update State Machine
  stateMachine.Update();
  //asignar tareas asincronicas
  
  input = Input::Unknown;
  
}



// Función para ingresar la contraseña
void Contrasena() {
  passwordAttempts = 0; // Reinicia el contador de intentos
  while (true) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      if (strlen(enteredPassword) == 4) { // Verificar que la longitud de la clave sea exactamente 4 dígitos
        passwordReady = true;
        //break; // Sale del ciclo while
      } else {
        enteredPassword[strlen(enteredPassword)] = key;
        lcd.print("*");
      }
    }
  

    if (passwordReady) {
      if (strcmp(enteredPassword, correctPassword) == 0) { // Si la clave ingresada es correcta
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Procesando...");
        lcd.clear();
        input = Input::Forward; // Cambia el estado a Monitoreo
        break;// Sale del ciclo while
      } else { // Si la clave ingresada es incorrecta
        passwordAttempts++;
        if (passwordAttempts >= 3) { // Si se superaron los 3 intentos
          lcd.clear();
          lcd.setCursor(4, 0);
          lcd.print("SISTEMA");
          lcd.setCursor(3, 1);
          lcd.print("BLOQUEADO");
          input = Input::Backward; // Cambia el estado a Bloqueado
          break; // Sale del ciclo while
        } else {
          lcd.clear();
          lcd.setCursor(5, 0);
          lcd.print("CLAVE");
          lcd.setCursor(3, 1);
          lcd.print("INCORRECTA");
          
        }
      }
      memset(enteredPassword, 0, sizeof(enteredPassword)); // Limpiar la clave ingresada
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("INGRESA CLAVE:");
      lcd.setCursor(6, 1);
      passwordReady = false; // Reiniciar la bandera
    }
  }
}

void Potenciometer() {
  Serial.println("Reading potentiometer:");
  int potenciometerValue = analogRead(potenciometerPin); 
  Serial.println(potenciometerValue);
  lcd.clear();
  lcd.setCursor(2, 7);
  lcd.print(potenciometerValue);
}

int sensorValue = analogRead(sensorPin);

void readPhoto(){
    int sensorValue = analogRead(sensorPin);
    Serial.println("hello read Photo: ");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print(sensorValue);
    Serial.println(sensorValue);
  
}

void Sensores() {
  int sensorValue = analogRead(sensorPin);
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);
  float t = dht.readTemperature();

    if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    
  }
    float hif = dht.computeHeatIndex(f, h);
    float hic = dht.computeHeatIndex(t, h, false);
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println("°C");
    lcd.clear();
    lcd.setCursor(1, 2);
    lcd.print(t);

    // Verificar si la temperatura es menor a 30 y la luz menor a 40
    if (sensorValue < 40 && t < 30) { 
      input = Input::Backward;
    }

    if (t > 27) {
      input = Input::Forward;
    }
}

void alarma() {
  digitalWrite(blueLed, HIGH);
  delay(500);
  digitalWrite(blueLed, LOW);
  input = Input::Backward;
  }



void Bloq(){
  digitalWrite(redLed, HIGH);
  delay(800);
  digitalWrite(redLed, LOW);
  input = Input::Reset;
  }
