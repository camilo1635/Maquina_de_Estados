#include "StateMachineLib.h"
#include "DHT.h"
#define DHTPIN 3     // Digital pin connected to the DHT sensor
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
int sensorPin = A0;    // select the input pin for the potentiometer
#include "AsyncTaskLib.h"
#include <LiquidCrystal.h>
#include <Keypad.h>

/* Display */
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

/* Keypad setup */
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {5, 4, 3, 2}; //R1 = 5, R2 = 4, R3 = 3. R4 = 2
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

char correctPassword[] = "1234"; // Clave correcta
char enteredPassword[5]; // Se reservan 5 caracteres para almacenar la clave ingresada (incluyendo el carácter nulo de terminación)
byte passwordAttempts = 0; // Contador de intentos de clave incorrecta
boolean passwordReady = false; // Bandera para indicar que la contraseña está lista para ser validada

// State Alias
enum State
{
  Inicio = 0,
  Monitoreo = 1,
  Alarma = 2,
  Bloqueado = 3
};

// Input Alias
enum Input
{
  Reset = 0,
  Forward = 1,
  Backward = 2,
  Unknown = 3,
};

// Create new StateMachine
StateMachine stateMachine(4, 7);

// Stores last user input
Input input;

// Setup the State Machine
void setupStateMachine()
{
  // Add transitions
  stateMachine.AddTransition(Inicio, Monitoreo, []() { return input == Forward; });
  stateMachine.AddTransition(Inicio, Bloqueado, []() { return input == Forward; });
  stateMachine.AddTransition(Inicio, Bloqueado, []() { return input == Backward; });

  stateMachine.AddTransition(Monitoreo, Alarma, []() { return input == Forward; });
  stateMachine.AddTransition(Monitoreo, Alarma, []() { return input == Backward; });
  stateMachine.AddTransition(Monitoreo, Bloqueado, []() { return input == Forward; });
  stateMachine.AddTransition(Monitoreo, Bloqueado, []() { return input == Backward; });
  
  stateMachine.AddTransition(Alarma, Monitoreo, []() { return input == Backward; });
  stateMachine.AddTransition(Alarma, Monitoreo, []() { return input == Forward; });

  stateMachine.AddTransition(Bloqueado, Monitoreo, []() { return input == Forward; });
  stateMachine.AddTransition(Bloqueado, Monitoreo, []() { return input == Backward; });
  stateMachine.AddTransition(Bloqueado, Inicio, []() { return input == Backward; });
  stateMachine.AddTransition(Bloqueado, Inicio, []() { return input == Reset; });
  
  // Add actions
  stateMachine.SetOnEntering(Inicio, Contrasena);//llamar función del estado
  stateMachine.SetOnEntering(Monitoreo, initTasks);
  stateMachine.SetOnEntering(Monitoreo, Sensores);
  stateMachine.SetOnEntering(Monitoreo, readPhoto);
  stateMachine.SetOnEntering(Alarma, alarma);
  stateMachine.SetOnEntering(Bloqueado, Bloq);

  stateMachine.SetOnLeaving(Inicio, []() {Serial.println("Leaving A"); });
  stateMachine.SetOnLeaving(Monitoreo, []() {Serial.println("Leaving B"); });
  stateMachine.SetOnLeaving(Alarma, []() {Serial.println("Leaving C"); });
  stateMachine.SetOnLeaving(Bloqueado, []() {Serial.println("Leaving D"); });
}

//Tareas
void Sensores(void);
AsyncTask Task1(2300, true, Sensores);
void readPhoto(void);
AsyncTask Task2(1000, true, readPhoto);

const byte greenLed = A6;
const byte blueLed = A5;
const byte redLed = A4;


void setup() 
{
  lcd.begin(16, 2);

  lcd.setCursor(1, 0);
  lcd.print("INGRESA CLAVE:");
  lcd.setCursor(6, 1);

  Serial.begin(9600);

  Serial.println("Starting State Machine...");
  setupStateMachine();  
  Serial.println("Start Machine Started");

  // Initial state
  stateMachine.SetState(Inicio, false, true);
  
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  
  dht.begin();
}

void loop() 
{
  // Read user input
  input = static_cast<Input>(readInput());

  // Update State Machine
  stateMachine.Update();
  //asignar tareas asincronicas
  Task1.Update();
  Task2.Update();
}

// Auxiliar function that reads the user input
int readInput()
{
  Input currentInput = Input::Unknown;
  if (Serial.available())
  {
    char incomingChar = Serial.read();

    switch (incomingChar)
    {
      case 'R': currentInput = Input::Reset;  break;
      case 'A': currentInput = Input::Backward; break;
      case 'D': currentInput = Input::Forward; break;
      default: break;
    }
  }

  return currentInput;
}

// Auxiliar output functions that show the state debug
void Contrasena() {
  char key = keypad.getKey();

  if (key != NO_KEY) {
    if (key == 'D') { // Si se presiona la tecla 'D'
      if (strlen(enteredPassword) == 4) { // Verificar que la longitud de la clave sea exactamente 4 dígitos
        passwordReady = true;
      } else { // Si la longitud de la clave no es 4 dígitos, se descarta
        memset(enteredPassword, 0, sizeof(enteredPassword)); // Limpiar la clave ingresada
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("INGRESA CLAVE:");
        lcd.setCursor(6, 1);
      }
    } else {
      enteredPassword[strlen(enteredPassword)] = key;
      lcd.print("*");
    }
  }

  if (passwordReady) {
    if (strcmp(enteredPassword, correctPassword) == 0) { // Si la clave ingresada es correcta
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("CLAVE");
      lcd.setCursor(4, 1);
      lcd.print("CORRECTA");
      digitalWrite(greenLed, HIGH);
      delay(5000);
      digitalWrite(greenLed, LOW); // Apagar el LED verde
      stateMachine.SetOnEntering(Monitoreo, initTasks);
      stateMachine.SetOnEntering(Monitoreo, Sensores);
      stateMachine.SetOnEntering(Monitoreo, readPhoto);
    } else { // Si la clave ingresada es incorrecta
      passwordAttempts++;
      if (passwordAttempts >= 3) { // Si se superaron los 3 intentos
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("SISTEMA");
        lcd.setCursor(3, 1);
        lcd.print("BLOQUEADO");
        stateMachine.SetOnEntering(Bloqueado, Bloq);
      } else {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("CLAVE");
        lcd.setCursor(3, 1);
        lcd.print("INCORRECTA");
        digitalWrite(blueLed, HIGH);
        delay(2000);
        digitalWrite(blueLed, LOW); // Apagar el LED azul
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

void initTasks(){
   Task1.Start(); 
   Task2.Start(); 
}

void readPhoto(){
  Serial.println("hello read Photo: ");
  int sensorValue = analogRead(sensorPin);
  Serial.println(sensorValue);

  float t = dht.readTemperature();

  if (sensorValue < 40 && t > 32) { 
    stateMachine.SetOnEntering(Bloqueado, Bloq); // Cambiar al estado Bloqueado
  }
}


void Sensores(){ 
  //Temperatura
  Serial.println("hello read temp");
  // Wait a few seconds between measurements.g

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  if(t > 30){
    stateMachine.SetOnEntering(Alarma, alarma);
  }
}

void alarma(){
  
  Task1.Stop();
  Task2.Stop();

  unsigned long previousMillis = 0;

  for(int i=0; i<3; i++) {

    // Enciende led
    digitalWrite(blueLed, HIGH);  

    // Espera medio segundo
    unsigned long currentMillis = millis();
    while(currentMillis - previousMillis <= 500) {
      currentMillis = millis(); 
    }
    previousMillis = currentMillis;

    // Apaga led    
    digitalWrite(blueLed, LOW);

  }
  stateMachine.SetOnEntering(Monitoreo, initTasks);
  stateMachine.SetOnEntering(Monitoreo, Sensores);
  stateMachine.SetOnEntering(Monitoreo, readPhoto);
}

void Bloq(){
  unsigned long previousMillis = 0;

  for(int i=0; i<6; i++) {

    // Enciende led
    digitalWrite(redLed, HIGH);  

    // Espera medio segundo
    unsigned long currentMillis = millis();
    while(currentMillis - previousMillis <= 800) {
      currentMillis = millis(); 
    }
    previousMillis = currentMillis;

    // Apaga led    
    digitalWrite(redLed, LOW);

  }

  
}
