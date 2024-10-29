#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WebServer.h> 
#include <ArduinoJson.h> 
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHTPIN 23      // SENSOR DE TEMPERATURA JIJIJ
#define DHTTYPE DHT11 

DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Pantalla de LCD I2C

const char* ssid = "XXXXXXX";     //Nombre de la red WiFi
const char* password = "XXXXXXX";       //Contraseña de la red WiFi



WebServer server(80);  // Iniciar servidor web en el puerto 80
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -21600, 60000);


unsigned long previousMillisLed = 0;  // Almacena el último tiempo en que se actualizó el LED
const long interval = 500;  // Intervalo de parpadeo (en milisegundos)
bool ledState = LOW;  // Estado actual del LED
int pingBuzzer = 19;
int btnApagado = 4;
int pinSensorInfrarojo = 5;
int ledSensorInfrarojo = 14;
bool infrarojoEncendido = true;
bool temperaturaEncendido = true;
bool buzzerEncendido = true;
int valor = 0;

int pulsaciones = 0;
unsigned long tiempoUltimaPulsacion = 0;
const unsigned long tiempoLimite = 500; // 500 ms para considerar dos pulsaciones

/*-----------------------------------------------------------------*/
/*-------------------  RUTAS PARA LAS LLAMADAS  -------------------*/
/*-----------------------------------------------------------------*/

void handleEstados() {
  // Obtener los datos del DHT11
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();
  
  // Crear JSON para la respuesta
  StaticJsonDocument<200> jsonResponse;
  jsonResponse["temperatura"] = temperatura;
  jsonResponse["humedad"] = humedad;
  jsonResponse["estadoAlarma"] = buzzerEncendido ? "Encendida" : "Apagada";
  jsonResponse["estadoInfrarrojo"] = infrarojoEncendido ? "Encendido" : "Apagado";
  jsonResponse["estadoTemperatura"] = temperaturaEncendido ? "Encendido" : "Apagado";

  String response;
  serializeJson(jsonResponse, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleEncenderComponentes() {
    encenderComponentes();  // Llama a la función que enciende los componentes
    String json = "{\"estado\": \"Componentes encendidos\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleApagarComponentes() {
    apagarComponentes();  // Llama a la función que apaga los componentes
    String json = "{\"estado\": \"Componentes apagados\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleAlarmaToggle() {
    buzzerEncendido = !buzzerEncendido;  // Alternar el estado
    String estadoAlarma = buzzerEncendido ? "Encendido" : "Apagado";
    String json = "{\"estado\": \"" + estadoAlarma + "\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleInfrarrojoToggle() {
    infrarojoEncendido = !infrarojoEncendido;  // Alternar el estado
    String estadoInfrarrojo = infrarojoEncendido ? "Encendido" : "Apagado";
    String json = "{\"estado\": \"" + estadoInfrarrojo + "\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleTemperaturaToggle() {
    temperaturaEncendido = !temperaturaEncendido;  // Alternar el estado
    String estadoTemperatura = temperaturaEncendido ? "Encendido" : "Apagado";
    String json = "{\"estado\": \"" + estadoTemperatura + "\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

/*-----------------------------------------------------------------*/
/*-----------  FIN DE LAS LLAMADAS A LAS RUTAS  -------------------*/
/*-----------------------------------------------------------------*/

// Notas para las melodías
#define NOTE_C 543  // C5
#define NOTE_D 607  // D5
#define NOTE_E 679  // E5
#define NOTE_F 718  // F5
#define NOTE_G 804  // G5
#define NOTE_A 900  // A5
#define NOTE_B 1008 // B5

void reproducirMelodiaEstrellita() {
  int melody[] = {
    NOTE_C, NOTE_C, NOTE_G, NOTE_G, NOTE_A, NOTE_A, NOTE_G,
    NOTE_E, NOTE_E, NOTE_D, NOTE_D, NOTE_C
  };

  int noteDurations[] = {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500};

  for (int thisNote = 0; thisNote < 11; thisNote++) {
    int duration = noteDurations[thisNote];
    tone(pingBuzzer, melody[thisNote], duration);
    delay(duration);
    noTone(pingBuzzer);
    delay(50);
  }
}

void reproducirMelodiaTeQuiero() {
  int melody[] = {NOTE_G, NOTE_A, NOTE_A, NOTE_G, NOTE_E, NOTE_E};

  int noteDurations[] = {500, 500, 500, 500, 500, 500};

  for (int thisNote = 0; thisNote < 6; thisNote++) {
    int duration = noteDurations[thisNote];
    tone(pingBuzzer, melody[thisNote], duration);
    delay(duration);
    noTone(pingBuzzer);
    delay(50);
  }
}

void activarBuzzerPorTemperatura() {
  float temperatura = dht.readTemperature();  // Leer temperatura

  if (isnan(temperatura)) {
    Serial.println("Error al leer la temperatura");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");

  if (temperatura > 45 && buzzerEncendido && temperaturaEncendido) {
    reproducirMelodiaTeQuiero(); // Reproduce la melodía de "Te Quiero"
    delay(2000); 
  }
}

void controlarLed() {
     unsigned long currentMillis = millis();  // Captura el tiempo actual

    if (infrarojoEncendido && temperaturaEncendido && buzzerEncendido) {
        digitalWrite(ledSensorInfrarojo, HIGH);  // LED encendido
    } else if (!infrarojoEncendido && !temperaturaEncendido && !buzzerEncendido) {
        digitalWrite(ledSensorInfrarojo, LOW);   // LED apagado
    } else {
        // Parpadeo del LED
        if (currentMillis - previousMillisLed >= interval) {
            // Guarda el último tiempo de actualización
            previousMillisLed = currentMillis;

            // Cambia el estado del LED
            if (ledState == LOW) {
                ledState = HIGH;
            } else {
                ledState = LOW;
            }
            digitalWrite(ledSensorInfrarojo, ledState);  // Actualiza el LED
        }
    }
}

void apagarComponentes() {
  infrarojoEncendido = false;
  temperaturaEncendido = false;
  buzzerEncendido = false;
  lcd.setCursor(1, 0); 
  lcd.print("                                      ");
  lcd.print("SISTEMA APAGADO :,3");
  delay(2000);
  Serial.println("Componentes apagados");
}

void encenderComponentes() {
  infrarojoEncendido = true;
  temperaturaEncendido = true;
  buzzerEncendido = true;
  lcd.setCursor(1, 0); 
  lcd.print("                                      ");
  lcd.print("SISTEMA Encendido :3");
  delay(2000);
  Serial.println("Componentes encendidos");
}

bool leerSensorInfrarojo() {  
  valor = digitalRead(pinSensorInfrarojo);

  if (valor == LOW && buzzerEncendido) {
    Serial.println("Movimiento detectado");
    if (buzzerEncendido) {
      reproducirMelodiaEstrellita();
    }
    return true;  // Indica que se detectó movimiento
  }
  return false;  // No se detectó movimiento
}

void setup() {

  Serial.begin(115200);
  pinMode(pinSensorInfrarojo, INPUT);
  pinMode(pingBuzzer, OUTPUT);
  pinMode(ledSensorInfrarojo, OUTPUT);
  pinMode(btnApagado, INPUT_PULLUP);
  dht.begin();  // Iniciar el sensor DHT11

  // Inicializa la pantalla LCD
  lcd.init();
  lcd.backlight();  // Enciende la luz de fondo
  lcd.setCursor(0, 0);  // Coloca el cursor en la primera línea

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
    lcd.setCursor(0, 0); 
    lcd.print("Conectado a WiFi");
    delay(2000);
    lcd.setCursor(0, 0); 
    lcd.print("                   ");
  }
  
  Serial.println("Conectado a WiFi");
  timeClient.begin();
  Serial.print("Conectado a WiFi. Dirección IP: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(1, 0); 
  lcd.print("IP: ");
  lcd.setCursor(0, 1); 
  lcd.print(WiFi.localIP().toString()); 
  delay(2000);

  // Configurar la ruta HTTP para devolver los datos en JSON
  server.on("/estados", HTTP_GET, handleEstados); 
  server.on("/toggleInfrarrojo", handleInfrarrojoToggle);
  server.on("/toggleTemperatura", handleTemperaturaToggle);
  server.on("/toggleAlarma", handleAlarmaToggle);
  server.on("/encender", handleEncenderComponentes);  
  server.on("/apagar", handleApagarComponentes);      
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  
  server.handleClient();
  timeClient.update();  // Actualiza la hora NTP

  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  lcd.setCursor(0, 0); 

  if (temperaturaEncendido) {
    if (isnan(temperatura) || isnan(humedad)) {
      Serial.println("Error al leer temperatura o humedad");
      lcd.print("Sensor Temperatura Error   "); 
    } else {
      // Mostrar temperatura y humedad
      lcd.print("T:");
      lcd.print(temperatura);
      lcd.print("C° ");
      lcd.setCursor(8, 0); // Mover a la segunda mitad de la línea
      lcd.print(" H:");
      lcd.print(humedad);
      lcd.print("% ");
      lcd.setCursor(0,1);
      lcd.print("Hora: ");
      lcd.print(timeClient.getFormattedTime());
    }
  } else {
    // Mostrar hora en lugar de temperatura y humedad
    lcd.print("Hora: ");
    lcd.print(timeClient.getFormattedTime());
    lcd.print("    ");
    lcd.setCursor(0,1);
    lcd.print("                            ");
  }
  

  if (infrarojoEncendido && leerSensorInfrarojo()) {
    lcd.setCursor(0, 1);  // Segunda línea
    lcd.print("Movimiento!     "); // Mensaje de movimiento
  } else {
    
  }

 // Lógica para el botón
    if (digitalRead(btnApagado) == LOW) { // Si el botón está presionado (estado LOW)
        pulsaciones++; // Incrementar contador de pulsaciones
        tiempoUltimaPulsacion = millis(); // Registrar el tiempo de la última pulsación
        Serial.println("Botón presionado");
    }

    // Comprobar si ha pasado tiempo desde la última pulsación
    if (millis() - tiempoUltimaPulsacion > tiempoLimite) {
        // Reiniciar contador de pulsaciones si ha pasado el tiempo límite
        if (pulsaciones >= 2) {
            encenderComponentes();
            Serial.println("Sistema Encendido");
        }
        pulsaciones = 0; // Reiniciar contador
    }

    // Comprobar si el buzzer está apagado
    if (digitalRead(btnApagado) == LOW) { // Si el botón está presionado (estado LOW)
        apagarComponentes(); 
        Serial.println("Sistema Apagado"); // Mensaje en el monitor serie
    }

  activarBuzzerPorTemperatura();

  controlarLed();

}
