/* recepteur de telecommande de porte garage pour actionner le portail
 *  433MHZ chips EG301 copie chinoise du HCS301
 *  1 bouton pour aprentissage
 *  1 bouton pour tout effacer
 *  1 led de signal aprentissage et reception signal
 *  1 sortie pour relay
 *  arduino micro
 */
#include <RFControl.h>
#include <EEPROM.h>

//recepteur 433 sur pin "2"
#define pin_rx 2
#define pin_relay1 6
#define pin_relay2 7
#define pin_relay3 8
#define pin_relay4 9
#define pin_learn 4
#define pin_led 5
#define pin_erase 3
#define max_nb_key 20
//-----------------------
int last_nb_key;
char keys[20][27];
//char key1[0][27] = "0162991031518399103127";

void setup() {
  Serial.begin(115200);
/*  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/
  pinMode(pin_relay1, OUTPUT);
  pinMode(pin_relay2, OUTPUT);
  pinMode(pin_relay3, OUTPUT);
  pinMode(pin_relay4, OUTPUT);
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_relay1, LOW);
  digitalWrite(pin_relay2, LOW);
  digitalWrite(pin_relay3, LOW);
  digitalWrite(pin_relay4, LOW);
  pinMode(pin_rx, INPUT);
  pinMode(pin_erase, INPUT_PULLUP);
  pinMode(pin_learn, INPUT_PULLUP);
  RFControl::startReceiving(pin_rx);
  EEPROM.get(0, last_nb_key);
  Serial.println("stock keys :");
  for(int i=0;i<20;i++){
    readFromEEPROM(sizeof(int)+i*27,keys[i],27);
    Serial.write(keys[i]);
    Serial.print(" ");
  }
}

void(* resetFunc) (void) = 0;//declare reset function at address 0

void writeToEEPROM(int addrOffset, char* value, int len )
{
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + i, value[i]);
  }
}
void readFromEEPROM(int addrOffset, char* dest, int len)
{
  for (int i = 0; i < len; i++)
  {
    dest[i] = EEPROM.read(addrOffset + i);
  }
}

//stock une clef
void store_key(char* key){
  if(last_nb_key >= (max_nb_key)){
    last_nb_key = 0;
  }
  else{
    last_nb_key++;
  }
  int eeadr = sizeof(int) + (last_nb_key*27);
  writeToEEPROM(eeadr,key, 27);
  EEPROM.put(0,last_nb_key);
  Serial.write("stock clef n: ");
  Serial.print(last_nb_key);
  Serial.write('\n');
  Serial.write(key);
  Serial.write('\n');
  digitalWrite(pin_led, HIGH);
  delay(500);
  digitalWrite(pin_led, LOW);
  delay(500);
  digitalWrite(pin_led, HIGH);
  delay(500);
  digitalWrite(pin_led, LOW);
}

//check si la clef recu match une clef stockée
// renvoi 1 si ok
int match_key(char* key){
    for(int i=0;i<20;i++){
      if (strcmp(key,keys[i])==0){
        return 1;
      }
    }
    return 0;
}

void loop() {
  char keytest[27] = "";
  if(RFControl::hasData()) {
    unsigned int *timings;
    unsigned int timings_size;
    unsigned long mean_timing;
    //unsigned int pulse_length_divider = RFControl::getPulseLengthDivider();
    RFControl::getRaw(&timings, &timings_size);   //recupere le signal sous forme de tableau de durée
    mean_timing=0;
    //calcul la durée moyenne
    for(unsigned int i=0; i < timings_size; i++) {
      mean_timing += timings[i];
    }
    mean_timing = mean_timing  / timings_size;
    unsigned int hexvalue=0;
    unsigned int num_bit=0;
    int j=0;
    //convertit les timings en octet et ascii
    for(unsigned int i=0; i < timings_size; i++) {
      if(timings[i]>mean_timing){
        if(timings[i]<(mean_timing*3)){
          int val = pow(2,(7-num_bit));
          hexvalue+=val;
        }
      }
      num_bit++;
      //récupere uniquement l'ID de telecommande et l'état des boutons pour un chips HCS301
      if(((i+1)%8 == 0) && (i > 11*8)) {
        char convertitoa[3];
        itoa(hexvalue,convertitoa,10);
        strcat(keytest,convertitoa);;
        j+=3;
        hexvalue = 0;
        num_bit = 0;
      };
    };
    //recupere le dernier octet
    char convertitoa[3];
    itoa(hexvalue,convertitoa,10);
    strcat(keytest,convertitoa);
    //----------------//
    Serial.write("ascii ");
    Serial.write(keytest);
    Serial.write('\n');
    if (digitalRead(pin_erase) == LOW){
      //efface toutes les clefs stockées
      for (unsigned int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 255);
      }
      digitalWrite(pin_led, HIGH);
      delay(500);
      digitalWrite(pin_led, LOW);
      delay(100);
      while(digitalRead(pin_erase) == LOW){
        digitalWrite(pin_led, HIGH);
        delay(500);
        digitalWrite(pin_led, LOW);
        delay(500);
      }
      resetFunc(); //call reset
    }
    if (digitalRead(pin_learn) == LOW){
      //stock la clef si appuis sur bouton
      if(match_key(keytest)==0){
        store_key(keytest);
        digitalWrite(pin_led, HIGH);
        delay(500);
        digitalWrite(pin_led, LOW);
        delay(500);
        digitalWrite(pin_led, HIGH);
        delay(500);
        digitalWrite(pin_led, LOW);
        resetFunc(); //call reset        
      }
    }
    else{
      //detecte un match et active le relay
      if(match_key(keytest)){
        Serial.write("match clef : ");
        Serial.write(keytest);
        Serial.write('\n');
        digitalWrite(pin_led, HIGH);
        digitalWrite(pin_relay1, HIGH);
        digitalWrite(pin_relay2, HIGH);
        digitalWrite(pin_relay3, HIGH);
        digitalWrite(pin_relay4, HIGH);
        delay(500);
        digitalWrite(pin_led, LOW);
        digitalWrite(pin_relay1, LOW);
        digitalWrite(pin_relay2, LOW);
        digitalWrite(pin_relay3, LOW);
        digitalWrite(pin_relay4, LOW);
        delay(3000);
      }
    }
    RFControl::continueReceiving();
  }
}
