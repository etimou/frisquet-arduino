#include <SerialTerminal.hpp>

#define TRANSCEIVER_TX_PIN      14    // Set the pin that receives data from your 433.42 Mhz Receiver
#define TRANSCEIVER_MODE_PIN    15    // Aurel transceivers have a pin that let the hardware switch to RX and TX mode
#define TRANSCEIVER_ENABLE_PIN  22    // Aurel transceivers have a pin that must be set to HIGH to enable the transmitter

// Variables
int long_pulse = 825;
byte message[17] = {0x00, 0x00, 0x00, 0x7E, 0xFF, 0xFF, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0x00, 0xFF, 0x80};
byte old_state, num_byte;
byte bitstuff = 0;

void writeBit(bool Bit) {
  old_state = !old_state;
  digitalWrite(TRANSCEIVER_TX_PIN, old_state);
  delayMicroseconds(long_pulse);

  if (Bit) {
    old_state = !old_state;
    digitalWrite(TRANSCEIVER_TX_PIN, old_state);
  }
  else {
    digitalWrite(TRANSCEIVER_TX_PIN, old_state);
  }
  delayMicroseconds(long_pulse);
}

void conversion(byte input) {
  for (byte n = 0; n < 8; n++) { // boucle pour chaque bit
    writeBit(bitRead(input, n));
    if (num_byte >= 4 && num_byte <= 14) {
      if (bitRead(input, n) == 1) bitstuff++;  // incrémente le compteur bitstuffing
    }
    if (bitRead(input, n) == 0) bitstuff = 0;
    if (bitstuff >= 5) {
      writeBit(0);
      bitstuff = 0;
    }
  }
}

void commande(byte mode, byte temperature) {
  if ((temperature <= 100) and ((mode==0) or (mode==3) or (mode==4))) {
    for (byte x = 0; x < 3; x++) { // boucle de 3 messages
      old_state = 0;
      
      // Message 9 --> numero de message 0 à 2
      message[9] = x;  
      
      // Message : 10 -> mode
      if (x == 2) message[10] = mode;
      else message[10] = mode + 0x80;

      // Message : 11 -> temperature
      message[11] = temperature;
      
      // Messages 13 et 14 -> checksum
      int checksum = 0;
      for (int i = 4; i <= 12; i++) {
        checksum -= message[i];
      }
      message[13] = highByte(checksum);
      message[14] = lowByte(checksum);

      Serial.print("Trame ");
      Serial.print(x);
      Serial.print(" : ");
      
      for (int i = 1; i <= 16; i++) {
        if (message[i] <= 0x0F) Serial.print("0");
        Serial.print(message[i], HEX);
      }
      Serial.println("");
      
      for (int i = 1; i <= 16; i++) { // boucle de 16 bytes
          conversion(message[i]);
      }
      
      digitalWrite(TRANSCEIVER_TX_PIN, LOW);
      delay(33);
    }
    digitalWrite(TRANSCEIVER_TX_PIN, LOW);
  } else {
    return;
  }
}

void setup() {
  // set the Aurel transceiver to RX mode
  pinMode(TRANSCEIVER_MODE_PIN, OUTPUT);
  digitalWrite(TRANSCEIVER_MODE_PIN, HIGH);

  // enable Aurel transmitter
  pinMode(TRANSCEIVER_ENABLE_PIN, OUTPUT);
  digitalWrite(TRANSCEIVER_ENABLE_PIN, HIGH);

  delay(500);

  // assign an interrupt on pin x on state change
  pinMode(TRANSCEIVER_TX_PIN, OUTPUT);
  digitalWrite(TRANSCEIVER_TX_PIN, LOW);
  
  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
   
  Serial.begin(9600) ;

}

void loop() {

  String chaineCaractere = "" ;
  String mode = "" ;
  String modeTexte = "" ;
  String temperature = "" ;
  char   acquisDonnees = "" ;
  int    nbTerme = 0 ;
  
  while (Serial.available() > 0) {
    acquisDonnees = Serial.read() ;
    delay(80) ;
    
    if ( acquisDonnees != 32 && acquisDonnees != 10 )  chaineCaractere = chaineCaractere + acquisDonnees  ;
    else {
      switch (nbTerme) {
        case 0:
          Serial.println("##### Reception de parametres #####");
          chaineCaractere = "" ;
          nbTerme++ ;
          break;
        case 1 :  // 1ere option  --> remontée du mode
          mode = chaineCaractere ;
          chaineCaractere = "" ;
          nbTerme++ ;
          switch (mode.toInt()) {
            case 0: 
              modeTexte = "Réduit" ;
              break ;
            case 3:
              modeTexte = "Confort" ;
              break ;
            case 4:
              modeTexte = "Hors gel" ;
              break ;            
          }
          break ;  
        case 2 :  // 2nd option  --> remontée de la temperature
          temperature = chaineCaractere.toInt() ;
          chaineCaractere = "" ;
          nbTerme++ ;
          break ;  
       }
    }

    if ( nbTerme == 3 ) { 
      Serial.println("Lancement commande avec parametres : ");
      Serial.println("     Mode            --> " + mode + " - " + modeTexte );
      Serial.println("     Temperature eau --> " + temperature + "°C"); 
      commande(mode.toInt(),temperature.toInt()) ;
      Serial.println("##### Commandes envoyées #####");
      Serial.println("");
    }
  }     
}
