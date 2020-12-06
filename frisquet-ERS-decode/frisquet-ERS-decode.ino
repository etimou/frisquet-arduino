// A commenter pour avoir la trame brute. 
#define STUFFING_MNG

volatile long duration = 0;
volatile long prev_time = 0;
volatile boolean data_dispo = false;
byte input = 3;
String trame[3];
byte message = 0;
byte start = true;
byte intro = 0;
String prefixe = "1100";
 
void setup() {
  Serial.begin(500000);
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
             
              byte nbOne = 0;
             
              // décodage des bits de la trame: 00=0, 11=0, 10=1, 01=1
              for (int x = (intro*4); x <= trame[message].length(); x=x+2) {
                if ((trame[message].substring(x,x+2) == "00") or (trame[message].substring(x,x+2) == "11")) {
#ifdef STUFFING_MNG                  
                  if (nbOne != 5) sortie = sortie + "0";    // gestion du Stuffing pour tous les octets de la trame
#else
                  sortie = sortie + "0";
#endif                   
                  nbOne = 0;
                } else {
                  sortie = sortie + "1";
                  nbOne ++;
                }
              }

              // repartition en bytes
              String id1 = sortie.substring(0,8);
              String id2 = sortie.substring(8,16);
              String id3 = sortie.substring(16,24);
              byte num = convert(sortie.substring(48,56));
              byte prechauff = convert(sortie.substring(56,64));
              byte chauffage = convert(sortie.substring(64,72));
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
