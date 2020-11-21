

#define PIN_BSF_0                   22                                          // Board Specific Function lijn-0
#define PIN_BSF_1                   23                                          // Board Specific Function lijn-1
#define PIN_BSF_2                   24                                          // Board Specific Function lijn-2
#define PIN_BSF_3                   25                                          // Board Specific Function lijn-3
#define PIN_RF_TX_VCC               15                                          // +5 volt / Vcc power to the transmitter on this pin
#define PIN_RF_TX_DATA              14                                          // Data to the 433Mhz transmitter on this pin
#define PIN_RF_RX_VCC               16                                          // Power to the receiver on this pin
#define PIN_RF_RX_DATA              19                                          // On this input, the 433Mhz-RF signal is received. LOW when no signal.

volatile long duration = 0;
volatile long prev_time = 0;
volatile boolean data_dispo = false;
byte input = PIN_RF_RX_DATA;//3;
String trame[3];
byte message = 0;
byte start = true;
byte intro = 0;
String prefixe = "1100";
 
void setup() {
  Serial.begin(57600);
  pinMode(PIN_RF_RX_DATA, INPUT);                                               // Initialise in/output ports
  pinMode(PIN_RF_TX_DATA, OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_TX_VCC,  OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_RX_VCC,  OUTPUT);                                              // Initialise in/output ports    
  digitalWrite(PIN_RF_RX_VCC,HIGH);                                             // turn VCC to RF receiver ON
  digitalWrite(PIN_RF_RX_DATA,INPUT_PULLUP);                                    // pull-up resister on (to prevent garbage)
  
  pinMode(PIN_BSF_0,OUTPUT);                                                    // rflink board switch signal
  digitalWrite(PIN_BSF_0,HIGH);                                                 // rflink board switch signal
  attachInterrupt(digitalPinToInterrupt(input), declenche, CHANGE);
}
 
void loop() {
  if (data_dispo == true) {
    int bit_state = digitalRead(input);
    if ((duration > 700) && (duration < 950)) { // pulse off court
      trame[message] = trame[message] + !bit_state;
    } else if ((duration > 1400) && (duration < 1950)) { // pulse off long
      // verifie qu'on commence avec un pulse long à 1
      if ((start == true) && (!bit_state == HIGH)) {
        trame[message] = trame[message] + !bit_state + !bit_state;
        start = false;
      } else if (start == false) {
        trame[message] = trame[message] + !bit_state + !bit_state;
      }
    } else {
      start = true; // debut d'une nouvelle trame
      if (trame[message].length() > 200) {
        message++;
        if (message > 2) {
          for (message = 0; message < 3; message++) {
            for (int x = 0; x <= 50; x=x+4) {
              if (trame[message].substring(x,x+4).compareTo(prefixe) == 0) {
                intro++;
              } else {
                break;
              }
            }
            String sortie = "";
            if (intro > 6 ) {
              // décodage des bits de la trame: 00=0, 11=0, 10=1, 01=1
              for (int x = (intro*4); x <= trame[message].length(); x=x+2) {
                if ((trame[message].substring(x,x+2) == "00") or (trame[message].substring(x,x+2) == "11")) {
                  sortie = sortie + "0";
                } else {
                  sortie = sortie + "1";
                }
              }

              // repartition en bytes
              byte id1 = convert(sortie.substring(0,8));
              byte id2 = convert(sortie.substring(8,16));
              byte id3 = convert(sortie.substring(16,24));
              byte num = convert(sortie.substring(48,56));
              byte prechauff = convert(sortie.substring(56,64));
              byte chauffage = convert(sortie.substring(64,72));
              Serial.print("ID: ");
              Serial.print(id1, HEX);
              Serial.print("-");
              Serial.print(id2, HEX);
              Serial.print("-");
              Serial.println(id3, HEX);
              Serial.print("num: ");
              Serial.println(num);
              Serial.print("prechauff: ");
              Serial.println(prechauff);
              Serial.print("chauffage: ");
              Serial.println(chauffage);
              intro = 0;
            }
          }  
          trame[0] = "";
          trame[1] = "";
          trame[2] = "";
          message = 0;
        }
      } else {
        // Re-initialise la trame s'il n'y avait pas de données juste avant      
        if ((message == 0) || (trame[message-1].length() < 200)) { 
          trame[0] = "";
          trame[1] = "";
          trame[2] = "";
          message = 0;
        }
      }
    }
    data_dispo = false;
  }
}

void declenche() {
  duration = micros()-prev_time;
  prev_time = micros();
  data_dispo = true;
}

byte convert(String entree) {
  byte result = 0;
  for (int f = 0; f <= 7 ; f++) {
    if (entree.substring(f,f+1) == "1") bitSet(result, f);
  }
return result;
}
