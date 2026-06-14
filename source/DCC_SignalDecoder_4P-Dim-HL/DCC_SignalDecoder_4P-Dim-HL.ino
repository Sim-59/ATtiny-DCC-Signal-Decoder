// DCC extended Accessories decoder for simple HL-signals of DR
// (c) Michael Hochmuth https://github.com/Sim-59                                2026-06-13
// 4 output ports
// CV reading at programming track (PT) is possible with a temporarily circuit for 60 mA load at one port
//
// Decoder-Address in Output-Address-Mode = LSB + MSB*256
// CV1 = 8 bit LSB, default 1
// x x x x  x x x x
// +-+-+-+--+-+-+-+----- LSB 0 ... 255
//
// CV9 = 3 Bit MSB, default 0
// x x x x  x x x x
//            +-+-+----- MSB (0 ... 7) * 256
//
// CV29 = Konfiguration, default 192
// x x x x  x x x x 
// | +------------------ "0" = Decoder Address Mode, "1" = (64) Output Address Mode 
// +-------------------- "1" = (128) Accessory Decoder Mode, is set for accessories
//
// CV34, default 0b00000100 (4) for 1 sec blink frequency
// x x x x  x x x x
//          +-+-+-+------ 4 bit for blinking periode in s (0.25 ... 3.75 sec) 
//
// CV51, CV52, CV53, CV54, default 15
// x x x x  x x x x
//       +--+-+-+-+------ 5 bit for dim value für LV, LR, AUX1, AUX2, PB3 (AUX2) no PWM -> Soft-Dim in Loop
//                        Bit0 ... Bit4  (0 ... 31) 1=dark to 31=full brightness, 0 is set to 1
//
// writing to CV8 is resetting the decoder
//    CV1 = 1 default accessory address-LSB 
//    CV9 = 0 default accessory address-MSB 
//    CV29 = 192
//    CV34 = 4  blink periode
//    CV51 = 15 PWM dim red
//    CV52 = 15 PWM dim green
//    CV53 = 15 PWM dim yellow top
//    CV54 = 15 SoftDim yellow bottom
//

#include <NmraDcc.h>
NmraDcc Dcc;

//define the destination board
//#define UNO

#if defined UNO
  #define DCC_PIN          2     // DCC-Signal
  #define DCC_ACK_PIN      12    // ACK for CV reading
  #define PORT_RED         3     // PWM possible
  #define PORT_GREEN       5     // PWM possible
  #define PORT_YTOP        6     // PWM possible
  #define PORT_YBOT        4     // no PWM, Soft-Dim
  #define PROG_NEXT_PIN    13    // programming port
  #define DEBUG
  
#else                           // for ATtiny85 PCB MuFu4P 1.1 and  MuFuDec 1.0, (MuFu4P 1.0 in brackets) 
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     3     // 60 mA-circuit can be temporarily connected to pin 4 for CV reading
  #define PORT_RED        0     // ws - red LED             (ws)
  #define PORT_GREEN      4     // ge - green LED           (vi)
  #define PORT_YTOP       1     // gn - yellow LED top      (ge)
  #define PORT_YBOT       3     // vi - yellow LED bottom   (gn) - SoftDim, optinal ACK
#endif

#define CV_BLINK_PERIOD   34   // blink period in 0.25s
#define CV_RED_DIM        51   // Dimmwert PORT1
#define CV_GREEN_DIM      52   // Dimmwert PORT2
#define CV_YTOP_DIM       53   // Dimmwert PORT3
#define CV_YBOT_DIM       54   // Dimmwert PORT4 - Soft-Dim
 
struct CVPair {
  uint16_t CV;
  uint8_t Value;
};

int AccDecoderAddr, BlinkPeriod;
bool ProgModeActivated = false;
bool Blink_red = false;
bool Blink_green = false;
bool Blink_ytop = false;
bool Blink_ybot = false;
uint8_t cv_red_dim, cv_green_dim, cv_ytop_dim, cv_ybot_dim;
uint8_t ybot_dim_pos = 0; 
bool ybot_enable = false; 
bool red_on = false;
bool green_on = false;
bool ytop_on = false;
bool ybot_on = false;

unsigned long currentPortMillis, startBlinkMillis;

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
      analogWrite(PORT_RED,cv_red_dim);
      digitalWrite(PORT_GREEN, LOW);
      digitalWrite(PORT_YTOP, LOW);
      ybot_enable = false;            // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop 
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 0");
      #endif

      break;
    case 1:                           // HL1 grün - Strecken-Höchstgeschw.
      digitalWrite(PORT_RED, LOW);
      analogWrite(PORT_GREEN,cv_green_dim);
      digitalWrite(PORT_YTOP, LOW);
      ybot_enable = false;            // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop 
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 1");
      #endif
      break;

    case 2:                           // HL3a grün-gelb - Fahrt 40 km/h, dann mit Strecken-Höchstgeschw.
      digitalWrite(PORT_RED, LOW);
      analogWrite(PORT_GREEN,cv_green_dim);
      digitalWrite(PORT_YTOP, LOW);
      ybot_enable = true;             // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 2");
      #endif
      break;

    case 3:                           // HL7 gelb(blink) - Fahrt auf 40 km/h reduzieren
      digitalWrite(PORT_RED, LOW);
      digitalWrite(PORT_GREEN, LOW);
      analogWrite(PORT_YTOP,cv_ytop_dim);
      ybot_enable = false;            // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      ytop_on = false;
      Blink_green = false;
      Blink_ytop = true;
      ytop_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt
      #if defined DEBUG  
        Serial.println("Aspect 3");
      #endif
      break;

    case 4:                           // HL9a gelb(blink)-gelb - Fahrt mit 40 km/h, 40 km/h erwarten
      digitalWrite(PORT_RED, LOW);
      digitalWrite(PORT_GREEN, LOW);
      analogWrite(PORT_YTOP,cv_ytop_dim);
      ybot_enable = true;             // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = true;
      ytop_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt
      #if defined DEBUG  
        Serial.println("Aspect 4");
      #endif
      break;

    case 5:                           // HL10 gelb-oben - Halt erwarten
      digitalWrite(PORT_RED, LOW);
      digitalWrite(PORT_GREEN, LOW);
      analogWrite(PORT_YTOP,cv_ytop_dim);
      ybot_enable = false;            // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 5");
      #endif
      break;

    case 6:                           // HL12a gelb-gelb - Fahrt mit 40 km/h, Halt erwarten
      digitalWrite(PORT_RED, LOW);
      digitalWrite(PORT_GREEN, LOW);
      analogWrite(PORT_YTOP,cv_ytop_dim);
      ybot_enable = true;             // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 6");
      #endif
      break;

    case 7:                           // Option nur, wenn HL7 ... HL12a am Signal nicht genutzt werden, 
                                      // z.B. reduziertes EZMG-Ausfahrsignal mit nur Hp0, HL1, HL3a
                                      // Verdrahtung statt "Gelb oben" dann 2x Weiß für RA12
      analogWrite(PORT_RED,cv_red_dim);
      digitalWrite(PORT_GREEN, LOW);
      analogWrite(PORT_YTOP,cv_ytop_dim);
      ybot_enable = false;             // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = false;
      #if defined DEBUG  
        Serial.println("Aspect 7");
      #endif
      break;

    default:
      digitalWrite(PORT_RED, LOW);
      digitalWrite(PORT_GREEN, LOW);
      digitalWrite(PORT_YTOP, LOW);
      ybot_enable = false;            // digitalWrite(PORT_YBOT, x) in Soft-Dim in loop
      Blink_green = false;
      Blink_ytop = false;
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
  {CV_BLINK_PERIOD,4},                                  // 1s blink frequeny
  {CV_RED_DIM,15},
  {CV_GREEN_DIM,15},
  {CV_YTOP_DIM,15},
  {CV_YBOT_DIM,15},
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
    AccDecoderAddr = (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB)) + (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB) << 8);
    cv29_Bits = Dcc.getCV(CV_29_CONFIG);
    BlinkPeriod = (Dcc.getCV(CV_BLINK_PERIOD) & 0x0F);

    #if defined DEBUG  
        Serial.print(Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB));
        Serial.print("-");
        Serial.println(Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB));
        Serial.print("Adresse:      "); Serial.println(AccDecoderAddr);
        Serial.print("CV29:         "); Serial.println(cv29_Bits,BIN);
    #endif
  }

  // configure pins
  pinMode(DCC_ACK_PIN, OUTPUT);
  digitalWrite(DCC_ACK_PIN, LOW);

  pinMode(PORT_RED, OUTPUT);
  pinMode(PORT_GREEN, OUTPUT);
  pinMode(PORT_YTOP, OUTPUT);
  pinMode(PORT_YBOT, OUTPUT);
  #if defined UNO
    pinMode(PROG_NEXT_PIN, INPUT_PULLUP);
  #endif
  digitalWrite(PORT_RED, LOW);
  digitalWrite(PORT_GREEN, LOW);
  digitalWrite(PORT_YTOP, LOW);
  digitalWrite(PORT_YBOT, LOW);

  cv_red_dim = Dcc.getCV(CV_RED_DIM);
  if (cv_red_dim < 1) cv_red_dim = 1;
  if (cv_red_dim > 31) cv_red_dim = 31;
  cv_red_dim = cv_red_dim << 3;

  cv_green_dim = Dcc.getCV(CV_GREEN_DIM);
  if (cv_green_dim < 1) cv_green_dim = 1;
  if (cv_green_dim > 31) cv_green_dim = 31;
  cv_green_dim = cv_green_dim << 3;

  cv_ytop_dim = Dcc.getCV(CV_YTOP_DIM);
  if (cv_ytop_dim < 1) cv_ytop_dim = 1;
  if (cv_ytop_dim > 31) cv_ytop_dim = 31;
  cv_ytop_dim = cv_ytop_dim << 3;

  cv_ybot_dim = Dcc.getCV(CV_YBOT_DIM);
  if (cv_ybot_dim < 1) cv_ybot_dim = 1;
  if (cv_ybot_dim > 31) cv_ybot_dim = 31;

  pinMode(DCC_PIN, INPUT);          // DCC Eingang

  #if defined UNO
    pinMode(PROG_NEXT_PIN, INPUT_PULLUP);
  #endif

  // init NmraDcc library (PIN, manufacturer, version...) 
  Dcc.pin(digitalPinToInterrupt(DCC_PIN), DCC_PIN, 1);
  Dcc.initAccessoryDecoder(MAN_ID_DIY, 50, cv29_Bits & FLAGS_OUTPUT_ADDRESS_MODE, 0);   // CV8=Manufacturer-ID=13, CV7=Manufacturer-VERS=50
  
  #if defined UNO
    if (digitalRead(PROG_NEXT_PIN) == 0) {
      #if defined DEBUG  
          Serial.println("ProgModeKey pressed");
      #endif
      Dcc.setAccDecDCCAddrNextReceived(1);
    }
  #endif

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

  // Soft-Dim PORT4 (PB3)
  ybot_dim_pos+=1;
  if (ybot_dim_pos > 31) ybot_dim_pos = 1;
  if (ybot_enable && (ybot_dim_pos <= cv_ybot_dim)) digitalWrite(PORT_YBOT, 1); else digitalWrite(PORT_YBOT, 0);

  // BlinkPort PORT1
  if (BlinkPeriod && Blink_red) {
    if (((currentPortMillis-startBlinkMillis)  % (BlinkPeriod*250)) < BlinkPeriod*125) {
//      digitalWrite(PORT_RED, HIGH);
      analogWrite(PORT_RED,cv_red_dim);
    } else {
      digitalWrite(PORT_RED, LOW);
    }
  }

  // BlinkPort PORT2
  if (BlinkPeriod && Blink_green) {
    if (((currentPortMillis-startBlinkMillis) % (BlinkPeriod*250)) < BlinkPeriod*125) {
      if (green_on == false) {
        analogWrite(PORT_GREEN,cv_green_dim);
        green_on = true;
      }
    } else {
      if (green_on == true) {
        digitalWrite(PORT_GREEN, LOW);
        green_on = false;
      }
    }
  }

  // BlinkPort PORT3
  if (BlinkPeriod && Blink_ytop) {
    if (((currentPortMillis-startBlinkMillis) % (BlinkPeriod*250)) < BlinkPeriod*125) {
      if (ytop_on == false) {
        analogWrite(PORT_YTOP,cv_ytop_dim);
        ytop_on = true;
      }
    } else {
      if (ytop_on == true) {
        digitalWrite(PORT_YTOP, LOW);
        ytop_on = false;
      }
    }
  }

  // BlinkPort PORT4 (PB3)
  if (BlinkPeriod && Blink_ybot) {
    if (((currentPortMillis-startBlinkMillis) % (BlinkPeriod*250)) < BlinkPeriod*125) {
      ybot_enable = true;
    } else {
      ybot_enable = false;
    }
  }

}
