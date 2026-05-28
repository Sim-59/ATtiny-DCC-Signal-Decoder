// DCC extended Accessories decoder for simple HL-signals of DR
// (c) Michael Hochmuth https://github.com/Sim-59                          2026-05-28
// 4 output ports
// CV reading at programming track (PT) is possible with a temporarily circuit for 60 mA load at one port
//
// Decoder-Address = LSB + MSB*64
// CV1 = 6 bit LSB, default 1
// x x x x  x x x x
//     +-+--+-+-+-+----- LSB 0 ... 63
//
// CV9 = 3 Bit MSB, default 0
// x x x x  x x x x
//            +-+-+----- MSB (0 ... 7) * 64
//
// CV34, default 0b00000100 (4) for 1 sec blink frequency
// x x x x  x x x x
//          +-+-+-+------ 4 bit for blinking periode in s (0.25 ... 3.75 sec) 
//
// CV29 = Konfiguration, default 192
// x x x x  x x x x 
// | +------------------ "0" = Decoder Address Mode, "1" = (64) Output Address Mode 
// +-------------------- "1" = (128) Accessory Decoder Mode, is set for accessories
//
// writing to CV8 is resetting the decoder
//    CV1 = 1 default accessory address-LSB 
//    CV9 = 0 default accessory address-MSB 
//    CV29 = 192
//    CV34 = 4
//

#include <NmraDcc.h>
NmraDcc Dcc;

//define the destination board
//#define UNO
#define PCB_10
//#define PCB_11

#if defined UNO
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     12    // 60 mA-circuit for CV reading
  #define PORT1_PIN       3     // red LED
  #define PORT2_PIN       4     // green LED
  #define PORT3_PIN       5     // yellow LED top
  #define PORT4_PIN       6     // yellow LED bottom
  #define PROG_NEXT_PIN   8	    // programming port
  #define DEBUG
  long int debounce;
  
#elif defined PCB_10            // for ATtiny85 with obsolate PCB version MuFu4P 1.0
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     3     // 60 mA-circuit can be temporarily connected to pin 4 for CV reading
  #define PORT1_PIN       0     // ws - red LED
  #define PORT2_PIN       1     // ge - green LED
  #define PORT3_PIN       3     // gn (optional ACK) - yellow LED top
  #define PORT4_PIN       4     // vi - yellow LED button

#elif defined PCB_11            // for ATtiny85 PCB MuFu4P 1.1 and  MuFuDec 1.0
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     3     // 60 mA-circuit can be temporarily connected to pin 4 for CV reading
  #define PORT1_PIN       0     // ws - red LED
  #define PORT2_PIN       4     // ge - green LED
  #define PORT3_PIN       1     // gn - yellow LED top
  #define PORT4_PIN       3     // vi (optinal ACK) - yellow LED bottom
#endif

#define CV_BLINK_TIME     34
 
struct CVPair {
  uint16_t CV;
  uint8_t Value;
};

int AccDecoderAddr, BlinkPeriod;
bool ProgModeActivated = false;
bool BlinkPort1 = false;
bool BlinkPort2 = false;
bool BlinkPort3 = false;
bool BlinkPort4 = false;

unsigned long currentPortMillis, startBlinkMillis, startPort1Millis, startPort2Millis;

// This function is called whenever a DCC Signal Aspect Packet is received
void notifyDccSigOutputState( uint16_t Addr, uint8_t State) {
  #if defined DEBUG
    Serial.print("notifyDccSigOutputState: ") ;
    Serial.print(Addr,DEC) ;
    Serial.print(',');
    Serial.println(State, HEX) ;
  #endif
  Run_HL_ASPECT(Addr, State);
}

void Run_HL_ASPECT( uint16_t SIGN_addr, uint8_t SIGN_aspect ) {
 
  startBlinkMillis = millis();

// check if the command is for our address and output
  if(SIGN_addr == AccDecoderAddr)  {

    switch (SIGN_aspect) {
    case 0:                           // Hp0 (HL13) rot
      digitalWrite(PORT1_PIN, HIGH);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, LOW);
      digitalWrite(PORT4_PIN, LOW);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect 0");
      #endif

      break;
    case 1:                           // HL1 grün - Strecken-Höchstgeschw.
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, HIGH);
      digitalWrite(PORT3_PIN, LOW);
      digitalWrite(PORT4_PIN, LOW);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect 1");
      #endif
      break;

    case 2:                           // HL3a grün-gelb - Fahrt 40 km/h, dann mit Strecken-Höchstgeschw.
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, HIGH);
      digitalWrite(PORT3_PIN, LOW);
      digitalWrite(PORT4_PIN, HIGH);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect 2");
      #endif
      break;

    case 3:                           // HL7 gelb(blink) - Fahrt auf 40 km/h reduzieren
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, HIGH);
      digitalWrite(PORT4_PIN, LOW);
      BlinkPort2 = false;
      BlinkPort3 = true;
      #if defined DEBUG  
        Serial.println("Aspect 3");
      #endif
      break;

    case 4:                           // HL9a gelb(blink)-gelb - Fahrt mit 40 km/h, 40 km/h erwarten
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, HIGH);
      digitalWrite(PORT4_PIN, HIGH);
      BlinkPort2 = false;
      BlinkPort3 = true;
      #if defined DEBUG  
        Serial.println("Aspect 4");
      #endif
      break;

    case 5:                           // HL10 gelb-oben - Halt erwarten
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, HIGH);
      digitalWrite(PORT4_PIN, LOW);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect 5");
      #endif
      break;

    case 6:                           // HL12a gelb-gelb - Fahrt mit 40 km/h, Halt erwarten
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, HIGH);
      digitalWrite(PORT4_PIN, HIGH);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect 6");
      #endif
      break;

    default:
      digitalWrite(PORT1_PIN, LOW);
      digitalWrite(PORT2_PIN, LOW);
      digitalWrite(PORT3_PIN, LOW);
      digitalWrite(PORT4_PIN, LOW);
      BlinkPort2 = false;
      BlinkPort3 = false;
      #if defined DEBUG  
        Serial.println("Aspect default");
      #endif
      break;

    }
  }
}

CVPair FactoryDefaultCVs[] = {
  // These two CVs define the Long DCC Address, CV1 = 6 bit LSB, CV9 = 3 bit MSB
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ACCESSORY_DECODER_ADDRESS},
  {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE},
  {CV_BLINK_TIME,4},                                  // 1s blink frequeny
};

//uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
// set to 0, to ensure that no reset init is performed within the loop
uint8_t FactoryDefaultCVIndex = 0;

void notifyCVResetFactoryDefault() {
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};

// This function is called by the NmraDcc library when a DCC ACK needs to be sent
// Calling this function should cause an increased 60ma current drain on the power supply for 6ms to ACK a CV Read 
void notifyCVAck(void) {
  digitalWrite(DCC_ACK_PIN, HIGH );
  delay( 8 );  
  digitalWrite(DCC_ACK_PIN, LOW );
  #if defined DEBUG  
    Serial.println("notifyCVAck") ;
  #endif
}

void setup() {

  int cv29_Bits;

  // Serial output for debugging
  #if defined DEBUG  
    Serial.begin(115200);
    Serial.println("DCC Signal Aspect Decoder");
    Serial.println();
  #endif


  if ((Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB) == 255) || (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB) == 255)) {
    FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
  } else {
    AccDecoderAddr = (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB & 0x03F)) + (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB) << 6);
    cv29_Bits = Dcc.getCV(CV_29_CONFIG);
    BlinkPeriod = (Dcc.getCV(CV_BLINK_TIME) & 0x0F);

    #if defined DEBUG  
        Serial.print(Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB));
        Serial.print("-");
        Serial.println(Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB));
        Serial.print("Adresse:      "); Serial.println(AccDecoderAddr);
        Serial.print("CV29:         "); Serial.println(cv29_Bits,BIN);
    #endif
  }

  // configure pins
  pinMode(PORT1_PIN, OUTPUT);
  pinMode(PORT2_PIN, OUTPUT);
  pinMode(PORT3_PIN, OUTPUT);
  pinMode(PORT4_PIN, OUTPUT);
  #if defined UNO
    pinMode(PROG_NEXT_PIN, INPUT_PULLUP);
  #endif
  digitalWrite(PORT1_PIN, LOW);
  digitalWrite(PORT2_PIN, LOW);
  digitalWrite(PORT3_PIN, LOW);
  digitalWrite(PORT4_PIN, LOW);

  pinMode(DCC_ACK_PIN, OUTPUT);
  digitalWrite(DCC_ACK_PIN, LOW);

  // init NmraDcc library (PIN, manufacturer, version...) 
  Dcc.pin(digitalPinToInterrupt(DCC_PIN), DCC_PIN, 1);
  Dcc.initAccessoryDecoder(MAN_ID_DIY, 10, cv29_Bits & FLAGS_OUTPUT_ADDRESS_MODE, 0);   // CV8=Manufacturer-ID=13, CV7=Manufacturer-VERS=10
  
//  delay(200);  // uncomment this if Micronucleus Bootloader is used.

  #if defined DEBUG  
    Serial.println("Decoder initialized");
  #endif
}

void loop() {
  Dcc.process();

  currentPortMillis = millis();

  // Set to default values
  if (FactoryDefaultCVIndex && Dcc.isSetCVReady()) {
    FactoryDefaultCVIndex--;  // Decrement first as initially it is the size of the array
    Dcc.setCV(FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
    #if defined DEBUG  
        Serial.println("Reset");
    #endif
  }

  // BlinkPort PORT1
  if (BlinkPeriod && BlinkPort1) {
    if (((currentPortMillis-startBlinkMillis)  % (BlinkPeriod*250)) < BlinkPeriod*125) {
      digitalWrite(PORT1_PIN, HIGH);
    } else {
      digitalWrite(PORT1_PIN, LOW);
    }
  }

  // BlinkPort PORT2
  if (BlinkPeriod && BlinkPort2) {
    if (((currentPortMillis-startBlinkMillis)  % (BlinkPeriod*250)) < BlinkPeriod*125) {
      digitalWrite(PORT2_PIN, HIGH);
    } else {
      digitalWrite(PORT2_PIN, LOW);
    }
  }

  // BlinkPort PORT3
  if (BlinkPeriod && BlinkPort3) {
    if (((currentPortMillis-startBlinkMillis) % (BlinkPeriod*250)) < BlinkPeriod*125) {
      digitalWrite(PORT3_PIN, HIGH);
    } else {
      digitalWrite(PORT3_PIN, LOW);
    }
  }

  // BlinkPort PORT4
  if (BlinkPeriod && BlinkPort4) {
    if (((currentPortMillis-startBlinkMillis) % (BlinkPeriod*250)) < BlinkPeriod*125) {
      digitalWrite(PORT4_PIN, HIGH);
    } else {
      digitalWrite(PORT4_PIN, LOW);
    }
  }

  // Wait for the first command to programme the address after setting PROG_NEXT_PIN to LOW and releasing
  #if defined UNO
    // Set address to first incoming command
    if (debounce > 0) debounce--;
    if ((digitalRead(PROG_NEXT_PIN) == 0) && (ProgModeActivated == false) && (debounce == 0)) {
      ProgModeActivated = true;
      debounce = 30000;
      Dcc.setAccDecDCCAddrNextReceived(1);
      #if defined DEBUG  
          Serial.println("ProgModeKey pressed");
      #endif
    }
    if ((digitalRead(PROG_NEXT_PIN) == 1) && (debounce == 0) && (ProgModeActivated == true)) {
      ProgModeActivated = false;    
      debounce = 30000;
      #if defined DEBUG  
          Serial.println("ProgModeKey released");
      #endif
    }
  #endif
}
