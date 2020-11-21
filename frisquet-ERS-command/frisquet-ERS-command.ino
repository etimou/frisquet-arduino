
#define PIN_BSF_0                   22                                          // Board Specific Function lijn-0
#define PIN_BSF_1                   23                                          // Board Specific Function lijn-1
#define PIN_BSF_2                   24                                          // Board Specific Function lijn-2
#define PIN_BSF_3                   25                                          // Board Specific Function lijn-3
#define PIN_RF_TX_VCC               15                                          // +5 volt / Vcc power to the transmitter on this pin
#define PIN_RF_TX_DATA              14                                          // Data to the 433Mhz transmitter on this pin
#define PIN_RF_RX_VCC               16                                          // Power to the receiver on this pin
#define PIN_RF_RX_DATA              19                                          // On this input, the 433Mhz-RF signal is received. LOW when no signal.

byte ERS_pin = PIN_RF_TX_DATA;//3;
int long_pulse = 825;
byte message[17] = {0x00, 0x00, 0x00, 0x58, 0x0C, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0x00, 0xFF, 0x00};
byte old_state, num_byte;
byte bitstuff = 0;

void pulse (byte state) {
  digitalWrite(ERS_pin, state);
  delayMicroseconds(long_pulse);
}

void conversion(byte input) {
  for (byte n = 0; n < 8; n++) { // boucle pour chaque bit
    if (bitRead(input, n) == 1) bitstuff++;  // incrémente le compteur bitstuffing
    if ((bitstuff > 5) && (num_byte > 9) && (num_byte < 15)) { // bitstuffing bit 10 à 14
      pulse(!old_state);
      pulse(!old_state);
      old_state = !old_state;
      bitstuff = 0;
    }

    pulse(!old_state);
    if (bitRead(input, n) == 1) {
      old_state = !old_state;
    } else {
      bitstuff = 0;
    }
    pulse(!old_state);
    old_state = !old_state;
  }
}

void commande(byte prechauffage, byte chauffage) {
  if ((chauffage <= 100) and ((prechauffage==0) or (prechauffage==3) or (prechauffage==4))) {
    for (byte x = 0; x < 3; x++) { // boucle de 3 messages
      old_state = 0;
      message[9] = x;  // numero message : 0 à 2
      if (x == 2) {
        message[10] = prechauffage;
      } else {
        message[10] = prechauffage + 0x80;
      }
      message[11] = chauffage;
      message[14] = 0x13B-message[9]-message[10]-message[11]-message[12];
      for (num_byte = 1; num_byte < 17; num_byte++) { // boucle de 16 bytes
        conversion(message[num_byte]);
      }
      digitalWrite(ERS_pin, HIGH);
      delay(33);
    }
    digitalWrite(ERS_pin, LOW);
  } else {
    return;
  }
}

void setup() {
  pinMode(PIN_RF_RX_DATA, INPUT);                                               // Initialise in/output ports
  pinMode(PIN_RF_TX_DATA, OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_TX_VCC,  OUTPUT);                                              // Initialise in/output ports
  pinMode(PIN_RF_RX_VCC,  OUTPUT);                                              // Initialise in/output ports    
  digitalWrite(PIN_RF_RX_VCC,HIGH);                                             // turn VCC to RF receiver ON
  digitalWrite(PIN_RF_RX_DATA,INPUT_PULLUP);                                    // pull-up resister on (to prevent garbage)
  
  pinMode(ERS_pin, OUTPUT);
  digitalWrite(ERS_pin, LOW);
}

void loop() {
  commande(3,30);   // reduit 0, confort 3, hors gel 4, chauffage 0 à 100
  delay(240000);  // boucle toutes les 4 minutes
}
