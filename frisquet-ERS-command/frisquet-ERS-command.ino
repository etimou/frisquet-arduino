byte ERS_pin = 3;
int long_pulse = 825;
byte message[17] = {0x00, 0x00, 0x00, 0x7E, 0xBE, 0xE7, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0x00, 0xFF, 0x80};
byte old_state, num_byte;
byte bitstuff = 0;

void writeBit(bool Bit) {
  old_state = !old_state;
  digitalWrite(ERS_pin, old_state);
  delayMicroseconds(long_pulse);

  if (Bit) {
    old_state = !old_state;
    digitalWrite(ERS_pin, old_state);
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

void commande(byte prechauffage, byte chauffage) {
  Serial.print("Nouvelle commande de chauffage : (");
  Serial.print(prechauffage);
  Serial.print(",");
  Serial.print(chauffage);
  Serial.println(")");
  Serial.flush();

  if ((chauffage <= 100) and ((prechauffage == 0) or (prechauffage == 3) or (prechauffage == 4))) {
    for (byte x = 0; x < 3; x++) { // boucle de 3 messages
      old_state = 0;
      message[9] = x;  // numero message : 0 à 2
      if (x == 2) {
        message[10] = prechauffage;
      } else {
        message[10] = prechauffage + 0x80;
      }
      message[11] = chauffage;

      int checksum = 0;
      for (int i = 4; i <= 12; i++) {
        checksum -= message[i];
      }
      message[13] = highByte(checksum);
      message[14] = lowByte(checksum);

      for (num_byte = 1; num_byte < 17; num_byte++) { // boucle de 16 bytes
        conversion(message[num_byte]);
      }
      //digitalWrite(ERS_pin, HIGH);
      digitalWrite(ERS_pin, LOW);
      delay(33);
    }
    digitalWrite(ERS_pin, LOW);

  } else {
    return;
  }
}


void setup() {
  pinMode(ERS_pin, OUTPUT);
  digitalWrite(ERS_pin, LOW);
}

void loop() {
  commande(0,0);   // reduit 0, confort 3, hors gel 4, chauffage 0 à 100
  delay(240000);  // boucle toutes les 4 minutes
}
