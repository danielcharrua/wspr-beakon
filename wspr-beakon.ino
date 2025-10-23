#include <si5351.h>
#include "Wire.h"
#include <JTEncode.h>
#include <int.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <RotaryEncoder.h>
#include <TinyGPS++.h>

//******************************************************************
//                      USER CONFIGURATION
//******************************************************************

// Comment out if you do not want Serial console output
// Comment out DEVMODE in a production environment as it will degrade performance!
#define DEVMODE // Serial port debug

// WSPR transmitter configuration
#define WSPR_CALL "EA1REX" // 4-6 digit callsign
#define WSPR_LOC "IN53"    // 4 digit locator  
#define WSPR_DBM 20        // WSPR TX power
#define WSPR_TX_EVERY 4    // Transmit every x minutes

// SI5351 drive strength options: SI5351_DRIVE_2MA, SI5351_DRIVE_4MA, SI5351_DRIVE_6MA, SI5351_DRIVE_8MA
// Tested values 2mA = 1mW, 4mA = 2mW, 6mA = 5mW, 8mA = 10mW
#define SI5351_DRIVE_LEVEL SI5351_DRIVE_4MA

// WiFi networks configuration
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

const WiFiNetwork wifiNetworks[] = {
  {"Your_SSID_1", "password1"},
  {"Your_SSID_2", "password2"},
  {"Your_SSID_3", "password3"}
};
const int numWifiNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Available frequencies
const struct
{
  unsigned long freq;
  unsigned long crystalFreq;
  const char *label;
  int filterPin;

  // For filters controlled with the integrated UDN2981
  // we are going to use ESP32 pins 12, 14, 27, 26, 25, 33, 32
  // Filter pins set to 0 are ignored (no filter control)

  // All frecuencies in this chart are calculated for our radio module
  // You need to calculate your own. Every radio module has a different crystal.

} wsprFrequencies[] = {
  // Frequency(Hz), Crystal_Freq(Hz), Label, Relay_Pin
  {144489000UL, 25000000UL, "144.489 MHz 2m",  0},    // 2m band (not supported by the Si5351)
  {70105048UL,  25000000UL, "70.091 MHz 4m",   0},    // 4m band (not supported by the Si5351)
  {50303500UL,  25000000UL, "50.293 MHz 6m",   0},    // 6m band (not supported by the Si5351)
  {28131120UL,  25000000UL, "28.124 MHz 10m",  12},   // 10m band
  {24924600UL,  25000000UL, "24.924 MHz 12m",  12},   // 12m band
  {21099330UL,  25000000UL, "21.094 MHz 15m",  14},   // 15m band
  {18104600UL,  25000000UL, "18.104 MHz 17m",  14},   // 17m band
  {14099615UL,  25000000UL, "14.095 MHz 20m",  27},   // 20m band
  {10142033UL,  25000000UL, "10.138 MHz 30m",  27},   // 30m band
  {7041356UL,   25000000UL, "7.038 MHz 40m",   26},   // 40m band
  {5287200UL,   25000000UL, "5.287 MHz 60m",   26},   // 60m band
  {3570732UL,   25000000UL, "3.568 MHz 80m",   25},   // 80m band
  {1838426UL,   25000000UL, "1.836 MHz 160m",  33},   // 160m band
  {475786UL,    25000000UL, "0.474 MHz 630m",  32},   // 630m band
  {136000UL,    25000000UL, "0.136 MHz 2200m", 32}    // 2200m band
};

//******************************************************************
//                             WSPR
//******************************************************************

// WSPR protocol configuration
#define WSPR_TONE_SPACING 146 // ~1.46 Hz
#define WSPR_DELAY 683        // Delay value for WSPR
#define WSPR_MESSAGE_BUFFER_SIZE 255

const int numFrequencies = sizeof(wsprFrequencies) / sizeof(wsprFrequencies[0]);

unsigned long selectedFreq;
unsigned long selectedCrystalFreq;
int selectedFilter;
int selectedIndex = 0;

//******************************************************************
//                           NTP server
//******************************************************************

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

bool ntpInitialized = false; // Flag para asegurarnos de inicializar NTP solo una vez

//******************************************************************
//                          SI5351 + TX
//******************************************************************

#define SI5351_CAL_FACTOR 0   // SI5351 Calibration factor obtained from Calibration firmware plugged in here.

uint64_t transmissionFrequency;        // Store the transmission frequency
uint64_t transmissionCrystalFrequency; // Store the crystal transmission frequency
uint8_t tx_buffer[WSPR_MESSAGE_BUFFER_SIZE];
bool warmup = 0; // Warmup status

Si5351 si5351;
JTEncode jtencode;

//******************************************************************
//                         Rotary encoder
//******************************************************************

#define ENCODER_CLK_PIN 4
#define ENCODER_DT_PIN 5
#define ENCODER_SWITCH_PIN 18

RotaryEncoder encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN, RotaryEncoder::LatchMode::FOUR3);

bool inMenu = false; // Flag para saber si estamos en el menú
int encoderPos = 0;

//******************************************************************
//                          Preferences
//******************************************************************

Preferences preferences;

//******************************************************************
//                              LCD
//******************************************************************

int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

unsigned long previousMillis = 0;
const long interval = 1000; // Intervalo de 1 segundo

String freqInMHz; // Variable para almacenar la frecuencia en MHz

unsigned long scrollPreviousMillis = 0; // Variable específica para el scroll
int scrollPos = 0;                      // Posición actual en el mensaje
int scrollRow = 0;                      // Fila del LCD donde se hace scroll
int scrollDelay = 0;                    // Tiempo de retraso entre desplazamientos
int scrollColumns = 0;                  // Número de columnas del LCD
String scrollMessage = "";              // Mensaje ya preparado (se inicializará más adelante)
String messageToScroll = "";            // Mensaje a desplazar

//******************************************************************
//                             WiFi
//******************************************************************

WiFiMulti wifiMulti; // Create WiFiMulti object to connect to multiple SSIDs

//******************************************************************
//                              GPS
//******************************************************************

// Define the RX and TX pins for Serial 2
#define RXD2 16       // Connect GPS TX pin to ESP32 PIN 16
#define TXD2 17       // Connect GPS RX pin to ESP32 PIN 17
#define GPS_BAUD 9600 // GPS baud rate

TinyGPSPlus gps;             // Create a TinyGPS++ object
HardwareSerial gpsSerial(2); // Create a HardwareSerial object for GPS

//******************************************************************
//                           Time Sync
//******************************************************************

bool timeSynced = false;         // Flag to indicate if time has been synced
unsigned long lastSyncCheck = 0; // Last time the time was checked

//******************************************************************
//                           Functions
//******************************************************************

void setFilterPins()
{
  // Set filter pins as outputs
  for (int i = 0; i < numFrequencies; i++)
  {
    pinMode(wsprFrequencies[i].filterPin, OUTPUT);
  }
 
  #if defined(DEVMODE)
    Serial.println("Setup filter pins");
  #endif
}
 
void loadStoredFrequency() {
  preferences.begin("wspr", true); // Solo lectura
  selectedFreq   = preferences.getULong("frequency", wsprFrequencies[3].freq);
  selectedCrystalFreq = preferences.getULong("crystalFreq", wsprFrequencies[3].crystalFreq);
  selectedFilter = preferences.getInt("filterPin", wsprFrequencies[3].filterPin);
  preferences.end();

  // Actualizar la frecuencia de transmisión y la frecuencia del cristal
  setTransmissionFrequency(selectedFreq);
  setTransmissionCrystalFrequency(selectedCrystalFreq);

  // Establecer el filtro de transmisión
  setTransmissionFilter(selectedFilter);

  // Encontrar el índice correspondiente
  for (int i = 0; i < numFrequencies; i++) {
    if (wsprFrequencies[i].freq == selectedFreq) {
      selectedIndex = i;
      break;
    }
  }

  // Actualizar el mensaje a desplazar
  updateScrollMessage(); 
}
 
void storeFrequency(unsigned long freq, unsigned long crystalFreq, int filterPin) {
  preferences.begin("wspr", false); // Escritura
  preferences.putULong("frequency", freq);
  preferences.putULong("crystalFreq", crystalFreq);
  preferences.putInt("filterPin", filterPin);
  preferences.end();

  // Establecer la frecuencia de transmisión y la frecuencia del cristal
  setTransmissionFrequency(freq);
  setTransmissionCrystalFrequency(crystalFreq);

  // Establecer el filtro de transmisión
  setTransmissionFilter(filterPin);

  // Actualizar el mensaje a desplazar
  updateScrollMessage();

  // Actualizar el mensaje del LCD ante cambio de frecuencia
  setScrollText(0, 450, lcdColumns); // Fila 0, retraso de 450ms, LCD de 16 columnas

  // Reiniciar el SI5351 con la nueva frecuencia y valor de cristal
  reinitSi5351();
}

bool reinitSi5351() {
  
  // The crystal load value needs to match in order to have an accurate calibration
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, transmissionCrystalFrequency, SI5351_CAL_FACTOR);

  // Set freq and CLK0 output
  si5351.set_freq(transmissionFrequency, SI5351_CLK0);

  // Set SI5351_DRIVE_8MA for max power, 10dbm output
  // Tested values 2MA = 0.6 mW, 4MA = 2.5 mW, 6MA = 5 mW, 8MA = 10 mW
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_LEVEL);

  // Turn off the output after reinitializing
  si5351.set_clock_pwr(SI5351_CLK0, 0);

  #if defined(DEVMODE)
    Serial.println("Si5351 initialized successfully");
  #endif

  return true;
}

void handleMenu() {

  // Detectar presión del botón del encoder
  if (digitalRead(ENCODER_SWITCH_PIN) == LOW) {
    delay(200); // Antirrebote
    while (digitalRead(ENCODER_SWITCH_PIN) == LOW);
    delay(200); // Antirrebote

    if (!inMenu) {
      // Entrar al menú
      inMenu = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Freq");
      lcd.setCursor(0, 1);
      lcd.print(wsprFrequencies[selectedIndex].label);
      handleEncoderInMenu();
    }
  }
}
 
void handleEncoderInMenu() {
  #if defined(DEVMODE)
    Serial.println("Entered menu");
  #endif

  while (inMenu) {

    encoder.tick();
    int newPos = encoder.getPosition();

    if (encoderPos != newPos) {

      selectedIndex       = abs(newPos) % numFrequencies;
      selectedFreq        = wsprFrequencies[selectedIndex].freq;
      selectedCrystalFreq = wsprFrequencies[selectedIndex].crystalFreq;
      selectedFilter      = wsprFrequencies[selectedIndex].filterPin;

      // Mostrar la frecuencia actualizada en el menú
      lcd.setCursor(0, 1);
      lcd.print(wsprFrequencies[selectedIndex].label);
      lcd.print("      ");

      encoderPos = newPos;
    }

    if (digitalRead(ENCODER_SWITCH_PIN) == LOW) {
      delay(200); // Antirrebote
      while (digitalRead(ENCODER_SWITCH_PIN) == LOW);
      delay(200); // Antirrebote

      // Salir del menú
      inMenu = false;
      storeFrequency(selectedFreq, selectedCrystalFreq, selectedFilter);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Frequency Set");
      lcd.setCursor(0, 1);
      lcd.print(wsprFrequencies[selectedIndex].label);

      delay(2000);

      // Regresar al modo normal
      lcd.clear();

      #if defined(DEVMODE)
        Serial.println("Exited menu");
      #endif
    }
  }
}
 
// Función para iniciar el scroll en el LCD
void setScrollText(int row, int delayTime, int lcdColumns) {
  // Usamos el mensaje global messageToScroll
  scrollMessage = messageToScroll;

  // Agregar espacios al inicio y al final del mensaje para el efecto de scroll
  for (int i = 0; i < lcdColumns; i++) {
    scrollMessage = " " + scrollMessage;
    scrollMessage = scrollMessage + " ";
  }
  
  // Inicializar variables globales
  scrollRow = row;
  scrollDelay = delayTime;
  scrollColumns = lcdColumns;
  scrollPos = 0; // Reiniciar la posición
}
 
// Función no bloqueante para hacer scroll del texto
void updateScrollText() {
  unsigned long currentMillis = millis();

  // Solo actualizar si ha pasado suficiente tiempo (delayTime)
  if (currentMillis - scrollPreviousMillis >= scrollDelay) {
    scrollPreviousMillis = currentMillis; // Guardar el tiempo actual

    // Mostrar la subcadena correspondiente en la fila
    lcd.setCursor(0, scrollRow);
    lcd.print(scrollMessage.substring(scrollPos, scrollPos + scrollColumns));

    // Incrementar la posición para el siguiente ciclo
    scrollPos++;
    
    // Si se ha mostrado todo el mensaje, reiniciar la posición
    if (scrollPos >= scrollMessage.length() - scrollColumns) {
      scrollPos = 0;
    }
  }
}
 
void updateScrollMessage(){
  // Calcular la frecuencia en MHz a partir de la frequencia seleccionada
  String label = wsprFrequencies[selectedIndex].label;

  #if defined(DEVMODE)
    Serial.println("Label: " + label);
  #endif

  // Actualizar el mensaje a desplazar
  messageToScroll = String(WSPR_CALL) + " " + String(WSPR_LOC) + " " + String(label) + " " + String(WSPR_DBM) + "dBm TX every " + String(WSPR_TX_EVERY) + "min";
}
 
/**
* @brief Intenta sincronizar la hora usando NTP (Network Time Protocol).
* 
* Esta función consulta un servidor NTP para obtener la hora actual en función de la zona horaria configurada.
* Si la sincronización es exitosa, se actualiza la hora del sistema utilizando la librería TimeLib.
* 
* @return true  Si la sincronización con el servidor NTP fue exitosa.
* @return false Si la sincronización con el servidor NTP falló.
*/
bool syncTimeWithNTP() {
  #if defined(DEVMODE)
    Serial.println("Trying to sync time via NTP...");
  #endif

  // Iniciar NTPClient solo la primera vez
  if (!ntpInitialized) {
    timeClient.begin();
    ntpInitialized = true;
  }

  // Obtener la hora del servidor NTP
  bool synced = timeClient.update();

  // Pequeña pausa para asegurar que el cálculo de getEpochTime()
  // incluya al menos 1 segundo desde la última sincronización.
  // Evita que se registre un segundo "truncado" por redondeo con millis().
  delay(100);

  // Validar si está sincronizado
  if ( synced ) {
    unsigned long epochTime = timeClient.getEpochTime();  // Obtener tiempo en formato UNIX

    // Convertir tiempo UNIX a fecha y hora
    setTime(hour(epochTime), minute(epochTime), second(epochTime), 
            day(epochTime), month(epochTime), year(epochTime));

    #if defined(DEVMODE)
      Serial.println("Time synced via NTP.");
      Serial.print("Fecha/hora establecida en el sistema: ");
      Serial.print(year());
      Serial.print(F("-"));
      Serial.print(month());
      Serial.print(F("-"));
      Serial.print(day());
      Serial.print(F(" "));
      Serial.print(hour());
      Serial.print(F(":"));
      Serial.print(minute());
      Serial.print(F(":"));
      Serial.println(second());
    #endif

    return true;
  } else {
    #if defined(DEVMODE)
      Serial.println("Failed to sync time via NTP.");
    #endif
    return false;
  }
}

/*
 * Tarea que mantiene actualizado el GPS
 * Se ejecuta en paralelo usando FreeRTOS, independientemente del loop()
 * Lee bytes del puerto serial del GPS y los entrega a gps.encode()
 * El vTaskDelay(1) es crítico para no bloquear el sistema y ceder CPU a otras tareas
 */
void mantenerGPS(void *pvParameters) {
  for (;;) {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
    vTaskDelay(1);  // cede tiempo a FreeRTOS, evita watchdog
  }
}
 
/**
 * @brief Intenta sincronizar la hora usando un módulo GPS NEO-7.
 *
 * Si la luz del GPS esta fija no tiene señal fixed, si parpadea tiene.
 * Al iniciarse el NEO 7M suele demorar 5 min aprox para coger señal.
 *
 * Cuando el sistema esta en warmup o transmitiendo, la luz del GPS queda fija.
 * Entendemos que con esto la RF podría estar molestando al GPS pero no da fallos en la lectura.
 * Hicimos la prueba de apagar la transmision por si era algo del código pero no sucede, solo
 * sucede cuando hay RF.
 * 
 * Otro comportamiento detetectado es que si el GPS se inicia sin señal (sin satelites) así permanece, pero si 
 * inicia con satelites y luego le quitamos la antena, se ve que almacena algo interno que le permite seguir teniendo
 * señal. Por ejemplo si lo iniciamos en una mesa a 2 metros de una ventana nunca llega a ver satelites, pero si
 * lo acercamos a una ventana, una vez que los ha visto, si lo volvemos a mover a la mesa, sigue con el fix y 
 * los satelites.
 *
 * Esta función interpreta los datos del GPS con TinyGPS+.
 * Si los datos son correctos, se actualiza la hora del sistema utilizando la librería TimeLib.
 *
 * @return true  Si la sincronización con el GPS fue exitosa.
 * @return false Si la sincronización con el GPS falló.
 */
bool syncTimeWithGPS()
{

  #if defined(DEVMODE)
    Serial.println("Trying to sync time via GPS...");
  #endif

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Reading GPS...");
  
  // Si los datos son váidos y actualizados, y hay más de 3 satélites conectados (se necesitan 4 para calcular la hora con presicion)
  // La validacion es importante ya que el GPS puede tener una hora incorrecta al inicio
  // almacenada en la memoria (con la pila de respaldo)

  if (gps.date.isValid() && gps.time.isValid() && gps.satellites.isValid() && gps.satellites.value() >= 4)
  {

    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());

    #if defined(DEVMODE)
      Serial.println("Time synced via GPS");
      Serial.print("Fecha/hora establecidas en el sistema: ");
      Serial.print(year());
      Serial.print(F("-"));
      Serial.print(month());
      Serial.print(F("-"));
      Serial.print(day());
      Serial.print(F(" "));
      Serial.print(hour());
      Serial.print(F(":"));
      Serial.print(minute());
      Serial.print(F(":"));
      Serial.println(second());
      GPSdebug();
    #endif

    return true;

  } else {
    // Si no hay datos válidos, esperar un poco antes de volver a intentar
    #if defined(DEVMODE)
      Serial.println("GPS data not valid or not enough satellites connected.");
      GPSdebug();
    #endif

    return false;
  }
}

/**
 * @brief Imprime información de depuración del GPS en el Serial.
 *
 * Esta función imprime la fecha, hora, número de satélites y sus edades.
 * Se utiliza para verificar el estado del GPS y su sincronización.
 */
void GPSdebug(){

  Serial.println("--------------GPS Debug---------------");

  // Datos de fecha
  Serial.print("Date: ");
  char dateBuffer[11]; // "YYYY-MM-DD" + null
  sprintf(dateBuffer, "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
  Serial.print(dateBuffer);
  Serial.print(" | Valid: ");
  Serial.print(gps.date.isValid());
  Serial.print(" | Age: ");
  Serial.println(gps.date.age());

  // Datos de hora
  Serial.print("Time: ");
  char buffer[9]; // hh:mm:ss + null
  sprintf(buffer, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  Serial.print(buffer);
  Serial.print("   | Valid: ");
  Serial.print(gps.time.isValid());
  Serial.print(" | Age: ");
  Serial.println(gps.time.age());

  // Datos de satelites
  Serial.print("Sate: ");
  Serial.print(gps.satellites.value());
  Serial.print("          | Valid: ");
  Serial.print(gps.satellites.isValid());
  Serial.print(" | Age: ");
  Serial.println(gps.satellites.age());

  Serial.println("--------------------------------------");
}
 
void encodeWSPRMessage()
{
  memset(tx_buffer, 0, WSPR_MESSAGE_BUFFER_SIZE);

  JTEncode jtencode;
  jtencode.wspr_encode(WSPR_CALL, WSPR_LOC, WSPR_DBM, tx_buffer);
}
 
void setTransmissionFrequency(unsigned long freq)
{
  transmissionFrequency = freq * SI5351_FREQ_MULT;
  
  #if defined(DEVMODE)
    Serial.println("Frequency set to: " + String(transmissionFrequency));
  #endif
}

void setTransmissionCrystalFrequency(unsigned long crystalFreq)
{
  transmissionCrystalFrequency = crystalFreq;
  
  #if defined(DEVMODE)
    Serial.println("Crystal frequency set to: " + String(transmissionCrystalFrequency));
  #endif
}
 
void setTransmissionFilter(int filterPin)
{
  // Set all filters LOW
  for (int i = 0; i < numFrequencies; i++) {

    // discard filters with 0
    if (wsprFrequencies[i].filterPin != 0) {
      digitalWrite(wsprFrequencies[i].filterPin, LOW);
    }
  }

  // Set transmission filter
  digitalWrite(filterPin, HIGH);

  #if defined(DEVMODE)
    Serial.println("Filter pin set HIGH: " + filterPin);
  #endif
}
 
void startWarmup()
{
  warmup = 1; // warm up started, bypass this if for the next 30 seconds

  #if defined(DEVMODE)
    Serial.println("Warmup started");
  #endif

  si5351.set_freq(transmissionFrequency, SI5351_CLK0);

  // Turn on the output
  si5351.set_clock_pwr(SI5351_CLK0, 1);
}
 
void transmitWSPRMessage()
{
  uint8_t i;

  #if defined(DEVMODE)
    Serial.println("TX ON - Starting transmission");
  #endif

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Transmitting");
  lcd.setCursor(0, 1);
  lcd.print("WSPR message");

  // Now do the rest of the message
  for(i = 0; i < WSPR_SYMBOL_COUNT; i++)
  {
    si5351.set_freq(transmissionFrequency + (tx_buffer[i] * WSPR_TONE_SPACING), SI5351_CLK0);
    delay(WSPR_DELAY);
  }
      
  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);

  // Reset warmup cycle
  warmup = 0;

  #if defined(DEVMODE)
    Serial.println("TX OFF - End of transmission");
  #endif

  lcd.clear();
}
 
bool ssidConnect()
{
  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  #if defined(DEVMODE)
    Serial.println("Connecting to Wifi...");
  #endif

  uint32_t startAttemptTime = millis();

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Connecting Wifi");

  // Wait until Wi-Fi is connected or timeout occurs
  while ((wifiMulti.run() != WL_CONNECTED) && 
        (millis() - startAttemptTime < 10000)) {
  }

  if (wifiMulti.run() == WL_CONNECTED) {
  
    #if defined(DEVMODE)
      Serial.println("\nWi-Fi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    #endif

    lcd.setCursor(0, 1);
    lcd.print("Wifi connected");

    return true;

  } else {

    #if defined(DEVMODE)
      Serial.println("\nFailed to connect to Wi-Fi within 10 seconds");
    #endif

    lcd.setCursor(0, 1);
    lcd.print("Wifi error");

    return false;
  }
}

void updateTXCountdown() {
  // Obtener el minuto y segundo actual
  int currentMinute = minute();
  int currentSecond = second();

  // Obtener el siguiente minuto de transmisión correcto
  int nextTXMinute = getNextTXMinute(currentMinute, WSPR_TX_EVERY);

  // Calcular tiempo restante
  int minutesLeft = nextTXMinute - currentMinute;
  if (minutesLeft < 0) minutesLeft += 60;  // Ajustar si es negativo
  int secondsLeft = 60 - currentSecond;

  if (secondsLeft == 60) {
      secondsLeft = 0;
  } else {
      minutesLeft--;  // Ajuste porque los segundos cuentan en reversa
  }

  // Actualizar el LCD con el tiempo restante
  if (millis() - previousMillis >= interval) {
      previousMillis = millis();  // Reiniciar el contador de tiempo

      lcd.setCursor(0, 1);
      lcd.print(minutesLeft);
      lcd.print(":");
      if (secondsLeft < 10) lcd.print("0");
      lcd.print(secondsLeft);
      lcd.print(" to TX");  // Indicar que es el tiempo hasta TX
  }
}

int getNextTXMinute(int currentMinute, int interval) {
  int remainder = currentMinute % interval;
  int nextTXMinute = currentMinute + (interval - remainder);
  return (nextTXMinute < 60) ? nextTXMinute : 0;  // Reiniciar si es 60
}
 
void checkAndSyncTime( int checkEvery ) {
  // Intentamos sincronizar cada checkEvery siempre que no esté en warmup
  // Encontramos un posible bug que si confunde al sistema y al GPS si consulta la hora al mismo
  // tiempo que esta encendido el módulo de RF (en warmup)

  checkEvery = checkEvery * 1000; // Convertir a milisegundos

  if (millis() - lastSyncCheck > checkEvery && !warmup) {  
    lastSyncCheck = millis(); // Actualizar la última verificación

    if ((WiFi.status() == WL_CONNECTED && syncTimeWithNTP()) || syncTimeWithGPS()) {
      #if defined(DEVMODE)
        Serial.println("Hora sincronizada correctamente.");
      #endif
      timeSynced = true;
    } else {
      #if defined(DEVMODE)
        Serial.println("No se pudo sincronizar la hora. Entrando en modo reintento...");
      #endif
      timeSynced = false;
    }
  }

  // Si la sincronización falló, entramos en modo reintento
  while (!timeSynced) {
    if (ssidConnect() && syncTimeWithNTP()) {
      timeSynced = true;
    } else if (syncTimeWithGPS()) {
      timeSynced = true;
    } else {
      #if defined(DEVMODE)
        Serial.println("Retrying time sync in 5 seconds...");
      #endif
      delay(5000);
    }
  }
}

void setup()
{
  // Setup and start serial
  Serial.begin(115200);
  while (!Serial);

  // Setup GPS on Serial2
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

  #if defined(DEVMODE)
    Serial.println("WSPR BEACON v0.1 Initializing...");
  #endif

  // Set filter pins as outputs
  setFilterPins();

  // Cargar frecuencia almacenada
  loadStoredFrequency();

  // Initialize encoder
  pinMode(ENCODER_SWITCH_PIN, INPUT_PULLUP);
  encoder.setPosition(selectedIndex);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WSPR BEACON v0.1");

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  
  // Add all WiFi networks from configuration array
  for (int i = 0; i < numWifiNetworks; i++) {
    wifiMulti.addAP(wifiNetworks[i].ssid, wifiNetworks[i].password);
  }

  /*
  * Lanza la función mantenerGPS() como una tarea de FreeRTOS en el núcleo 1 (Core 1)
  * Esto permite que la lectura del GPS ocurra en paralelo al loop(), incluso si este contiene delays o es bloqueante
  * Se usa Core 1 para evitar conflictos con WiFi, que corre en Core 0
  * La prioridad 1 es suficiente para mantener la tarea fluida sin acaparar CPU
  * El stack size (4096 palabras = 16 KB) está ajustado para operaciones UART continuas con TinyGPS++
  */
  xTaskCreatePinnedToCore(
      mantenerGPS, // función GPS
      "GPS_Task",  // nombre
      4096,        // stack size
      NULL,        // parámetros
      1,           // prioridad
      NULL,        // handle
      1            // Core 1 (el mismo que loop, pero como otra tarea)
  );

  // Intentar sincronizar la hora, si falla, se reintentará cada 5 segundos
  checkAndSyncTime(5);

  // Initialize the Si5351
  #if defined(DEVMODE)
    Serial.println("Initializing radio module");
  #endif

  lcd.setCursor(0, 1);
  lcd.print("Starting radio");

  reinitSi5351();

  // Test the output for 30 seconds
  #if defined(DEVMODE)
    Serial.println("Testing signal for 30 seconds");

    lcd.setCursor(0, 1);
    lcd.print("Testing signal  ");

    si5351.set_clock_pwr(SI5351_CLK0, 1);
    delay(15000);

    // Disable CLK0 output initially
    si5351.set_clock_pwr(SI5351_CLK0, 0);

    Serial.println("Radio Module setup successful");
  #endif

  lcd.clear();

  // Iniciar el scroll de texto
  setScrollText(0, 450, lcdColumns); // Fila 0, retraso de 450ms, LCD de 16 columnas

  // RAM-intensive operation, generate WSPR message only once, at device startup
  encodeWSPRMessage();

  #if defined(DEVMODE)
    Serial.println("Finished setup");
  #endif
}

void loop()
{

  // Actualizar el scroll de texto en el LCD
  updateScrollText();

  // Lógica del menu
  handleMenu();

  // Transmitir si la hora se mantiene sincronizada
  if (timeSynced) {

    // Mostrar el countdown en el LCD para la próxima transmisión
    updateTXCountdown();

    // 30 seconds before TX enable SI5351 output to eliminate startup drift
    if ((minute() + 1) % WSPR_TX_EVERY == 0 && second() == 30 && !warmup)
    {
      startWarmup();
    }

    // WSPR should start on the 1st second of the even minute stated in WSPR_TX_EVERY
    // If WSPR_TX_EVERY is 4, it should transmit on minute 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56
    if (minute() % WSPR_TX_EVERY == 0 && second() == 0)
    {
      transmitWSPRMessage();
    }

  } else {
    // Si la hora no está sincronizada, mostrar mensaje en el LCD
    lcd.setCursor(0, 1);
    lcd.print("Time not synced");
  }

  // Verificar y re-sincronizar la hora cada 30 minutos (1800 segundos)
  checkAndSyncTime(1800);

}
 