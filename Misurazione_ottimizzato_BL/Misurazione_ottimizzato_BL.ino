#include <SdFat.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ArduinoBLE.h>
#include <vector>
#include <algorithm>
#include <Arduino.h>

#define CS_PIN 10  // Pin SS per la microSD

SdFat sd;
File file;

//cose per BL
BLEService customService("12345678-1234-5678-1234-56789abcdef0");
BLEStringCharacteristic dataCharacteristic("12345678-1234-5678-1234-56789abcdef1", BLERead | BLEWrite, 100);


//stimoli 
const int Buzzer = 9;
const int led1 = A1;
const int led2 = A2;
const int led3 = A3;

unsigned long uA;
unsigned long stamp=300;
int tempo = 0;
int k = 0;

int i;
int stim;
bool start;
int cont = 0;
int esercizio;
bool pause = 1;
bool ese_completato=0;
int cicli = 4;
int visual;


// VETTORE STIMOLI
std::vector<int> vecstim = {1,2,3,4};

//pulsanti
const int pulsb = A4;
const int pulsl1 = A5;
const int pulsl2 = A6;
const int pulsl3 = A7;
const int pulsStart = 8;

//costanti tempo 
unsigned long periodoIniziale;
unsigned long periodo;
unsigned long t;
unsigned long t1; //tempo di inizio buzzer
unsigned long t2; //tempo di schiaccio del pulsante
unsigned long tv;
unsigned long deltaTime;
unsigned long tempi[30];

//variabili pulsanti
bool pressl1 = 0;
bool pressl2 = 0;
bool pressl3 = 0;
bool pressB = 0;
bool presStart = 0;
//Display LCD
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

bool buzzerParte = 0;

bool ex = 0;
bool bl = 1;


//temporaneo 2 = pulsante
void setup() {
// set up the LCD's number of columns and rows:
lcd.begin(16, 2);
lcd.setCursor(0, 0);   // colonna 0, riga 0
lcd.print("Esercizio");
lcd.setCursor(0, 1);   // colonna 0, riga 1
lcd.print("Nano 33 BLE");

Serial.begin(115200);

//inizializzazione BL

if (!BLE.begin()) {

 Serial.println("BL non va");
 while (1);
}

BLE.setLocalName("Nano33BLE");
BLE.setAdvertisedService(customService);
customService.addCharacteristic(dataCharacteristic);
BLE.addService(customService);
BLE.advertise();



// put your setup code here, to run once:
//periodoIniziale = random(3000,10000);

//iniz OUTPUT stimoli
pinMode(Buzzer, OUTPUT);
pinMode(led1,OUTPUT);
pinMode(led2,OUTPUT);
pinMode(led3,OUTPUT);


pinMode(pulsb, INPUT_PULLUP); // PULLUP = si usa la resistenza interna del pulsante, sembrerebbe che sul nano 33 funzionino soltanto i PULLUP
pinMode(pulsl1, INPUT_PULLUP); // PULLUP = si usa la resistenza interna del pulsante, sembrerebbe che sul nano 33 funzionino soltanto i PULLUP
pinMode(pulsl2, INPUT_PULLUP); // PULLUP = si usa la resistenza interna del pulsante, sembrerebbe che sul nano 33 funzionino soltanto i PULLUP
pinMode(pulsl3, INPUT_PULLUP); // PULLUP = si usa la resistenza interna del pulsante, sembrerebbe che sul nano 33 funzionino soltanto i PULLUP
pinMode(pulsStart, INPUT_PULLUP); // PULLUP = si usa la resistenza interna del pulsante, sembrerebbe che sul nano 33 funzionino soltanto i PULLUP

//iniz pulsanti
attachInterrupt(digitalPinToInterrupt(pulsb), presb, FALLING); 
attachInterrupt(digitalPinToInterrupt(pulsl1), presl1, FALLING); 
attachInterrupt(digitalPinToInterrupt(pulsl2), presl2, FALLING); 
attachInterrupt(digitalPinToInterrupt(pulsl3), presl3, FALLING); 
attachInterrupt(digitalPinToInterrupt(pulsStart), Start, FALLING);

randomSeed(analogRead(A0));

    // Inizializza la SD
if (!sd.begin(CS_PIN, SD_SCK_MHZ(10))) {  // Usa 10 MHz come velocità SPI
      Serial.println("Errore nell'inizializzazione della microSD!");
    return;
    }

    Serial.println("Scheda SD inizializzata con successo!");

}

void loop() {
if (ex && cont < cicli) {


    i = random(0, vecstim.size());
    stim = vecstim[i];

    periodo = random(3000, 10000) + millis();
    ex = 0;
    ese_completato = 0;
    k=0;
    start = 1;
    Serial.print("Numero LED: ");
    Serial.println(stim);


} 
else if (cont == cicli && pause == 0) {

    ex = 0;
    Serial.println("stim azzerata");
    stim = 0;

    for (int j = 0; j < cicli; j++) {
        Serial.println(tempi[j]);
    }
    ResetPressStates();
    WriteDataToSD();
    esercizio++;
    pause = 1;
    ese_completato=1;
    visual = cicli;
    cicli = 4;

    vecstim.clear();
    vecstim = {1,2,3,4};

} 
else if (pause && presStart) 
{
    cont = 0;
    pause = 0;
    ex = 1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Esercizio");
    lcd.setCursor(0, 1);
    lcd.print("in corso...");
}
    else if (pause && ese_completato)
{
    timevis();
    BLmanage();
}
else if (bl)
{
    BLmanage();
}

t = millis();
HandleEvent(stim);



}

void ResetPressStates() {
    pressB = pressl1 = pressl2 = pressl3 = presStart = 0;
}



void WriteDataToSD() {
    // === LEGGI il contatore da "counter.txt"
    File counterFile = sd.open("counter.txt", FILE_READ);
    if (counterFile) {
        esercizio = counterFile.parseInt(); // legge il numero della sessione precedente
        counterFile.close();
        Serial.print("Sessione precedente: ");
        Serial.println(esercizio);
    } else {
        // Se il file non esiste, inizializza da 0
        esercizio = 0;
    }


    Serial.println(esercizio);
    esercizio++; // nuova sessione

    // === RIMUOVI e riscrivi il file contatore aggiornato


    sd.remove("counter.txt"); 


    counterFile = sd.open("counter.txt", FILE_WRITE);
    if (counterFile) {
        counterFile.println(esercizio);
        counterFile.close();
    } else {
        Serial.println("Errore nella scrittura del contatore.");
        return;
    }

    // === CREA nome file tipo "dati_1.txt"
    char filename[20];
    sprintf(filename, "dati_%d.txt", esercizio);

    // === SALVA i dati nel file
    file = sd.open(filename, FILE_WRITE);
    if (file) {
        file.print("SERIE NUMERO: "); file.println(esercizio);
        for (int j = 0; j < cicli; j++) {
            file.print("tempo "); file.print(j + 1); file.print(": "); file.println(tempi[j]);
        }
        file.close();
        Serial.print("Scrittura completata nel file: ");
        Serial.println(filename);
    } else {
        Serial.println("Errore nell'apertura del file.");
    }
}





void HandleEvent(int index) {
    if (index == 0) return;
    
    bool *pressPtr;
    int led;
    switch (index) {
        case 1: pressPtr = &pressB; break;
        case 2: pressPtr = &pressl1; led = led1; break;
        case 3: pressPtr = &pressl2; led = led2; break;
        case 4: pressPtr = &pressl3; led = led3; break;
    }
    
    if (t <= periodo) {
        *pressPtr = 0;
    } else {
        if (index == 1) analogWrite(Buzzer, 200);
        else digitalWrite(led, HIGH);
        
        if (start) {
            t1 = millis();
            start = 0;
        } else if (*pressPtr) {
            *pressPtr = 0;
            t2 = millis();
            deltaTime = t2 - t1;
            Serial.println(deltaTime);
            ex = 1;
            tempi[cont++] = deltaTime;
            
            Stimolioff();
        }
    }
}


void Stimolioff() {
    analogWrite(Buzzer, 0);
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
}


void timevis()
{
    if (t > tv) {
        k++;
        if (k > visual + 1) {
            k = 1;
        }
        lcd.clear();
        tv = millis() + 2000; // aggiorna anche qui per evitare sfarfallii
    }
    if (k <visual + 1){
      lcd.setCursor(0, 0);
      lcd.print("T");
      lcd.print(k);
      lcd.print(": ");
      lcd.setCursor(4, 0);
      lcd.print(tempi[k - 1]);
      lcd.print(" ms");
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("Esercizio finito");
      lcd.setCursor(0, 1);
      lcd.print("Premi Start");
    }
}



void BLmanage()
{
BLEDevice central = BLE.central();
  if (central) {
    if (dataCharacteristic.written()) {
      
      String valore = dataCharacteristic.value();  // ricevi stringa
      
      if (valore == "start")
      {
        presStart = 1;
      }
      else {
      if (valore.length() > 1 )
      {
        vecstim.clear();
      }
      
    for (int y = 0; y < valore.length(); y++) {
    char c = valore[y];

    if (!isDigit(c)) {
        Serial.println("Carattere non valido: " + String(c));
        continue;
    }

    int inputi = c - '0';

    if (y == 0) {
        cicli = inputi;
    } else {
        if (inputi > 4 || inputi < 1) {
        Serial.println("Valore non valido");
        } else {
        vecstim.push_back(inputi);
        Serial.println("inserito stimolo " + String(inputi));
        }
    }
    }


      Serial.println("Ricevuto: " + valore);
      bl = 0;
      }
    }
  }
}

// Funzioni di gestione pulsanti
void presb() { pressB = 1; }

void presl1() { pressl1 = 1; }

void presl2() { pressl2 = 1; }

void presl3() { pressl3 = 1; }

void Start() { presStart = 1; }