// DCC extended Accessories decoder for simple HL-signals (EZMG) of DR
// (c) Michael Hochmuth https://github.com/Sim-59                                2026-11-03
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
// CV34, default 13 (13 x 100 ms, ca. 46 blink impulses pro minute)
// x x x x  x x x x
//       +--+-+-+-+------ 5 bit for blinking periode in s (0.1 ... 3.1 sec) 
//
// CV35, default 35 (35% of time in CV34) 
//       +--+-+-+-+------ 5 bit for blink percent (2 ... 98 %, fade-in and on time, without fade-out)
//
// CV36 ... CV39, default 15 for fading in and out
// x x x x  x x x x
//       +--+-+-+-+------ 5 bit for fading (expected time is this value multiplied with dim value in milliseconds)
//                        if dim value is defined greater, then define the fade value smaller
//
// CV49, Fade-Pause configuration, default 3
// x x x x  x x x x 
//              |-+----- Bit0=1 => Suppress the pause before the red fade-in  
//              +------- Bit1=1 => Fade-out of green and yellow after the red fade-in  
//
// CV50, default 10 for pause for fade-in after aspect command in 25 milliseconds
//
// CV51, CV52, CV53, CV54, default 15
// x x x x  x x x x
//       +--+-+-+-+------ 5 bit for dim value für LV, LR, AUX1, AUX2, PB3 (AUX2) no PWM -> Soft-Dim in Loop
//                        Bit0 ... Bit4  (0 ... 31) 1=dark to 31=full brightness, 0 is set to 1
//
// writing to CV8 is resetting the decoder
//    CV1 = 1 default accessory address-LSB 
//    CV9 = 0 default accessory address-MSB 
//    CV29 = 192, Accessory Decoder Mode, Output Address Mode 
//    CV34 = 13 blink periode
//    CV35 = 35 blink percentage (fade-in and on time, without fade-out)
//    CV36 = 15 Fade value red
//    CV37 = 15 Fade value green
//    CV38 = 15 Fade value yellow top
//    CV39 = 15 Fade value yellow buttom
//	  CV49 = 03 Fade_Pause configuration
//    CV50 = 10 Pause after signal command
//    CV51 = 15 PWM dim red
//    CV52 = 15 PWM dim green
//    CV53 = 15 PWM dim yellow top
//    CV54 = 15 SoftDim yellow bottom
//
// with these values for fading the time is (15 * 15) 225 ms
// the pause from command start for fading in is (10 * 25 ms) 250 ms, so the fade in begins after fade out of the other lights.

#include <NmraDcc.h>
NmraDcc Dcc;

//define the destination board
//#define UNO

#if defined UNO
  #define DCC_PIN          2     // DCC-Signal
  #define DCC_ACK_PIN      12    // ACK for CV reading
  #define PORT_OUT1        3     // PWM possible
  #define PORT_OUT2        5     // PWM possible
  #define PORT_OUT3        6     // PWM possible
  #define PORT_OUT4        4     // no-PWM-Port for Soft-Dim testing
  #define PROG_NEXT_PIN    13    // programming port
  #define DEBUG
  
#else                           // for ATtiny85 PCB MuFu4P 1.1 and  MuFuDec 1.0, (MuFu4P 1.0 in brackets) 
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     3     // 60 mA-circuit can be temporarily connected to pin 4 for CV reading
  #define PORT_OUT1       0     // ws - red LED             (ws - red LED)
  #define PORT_OUT2       4     // ge - green LED           (vi - yellow LED bottom)
  #define PORT_OUT3       1     // gn - yellow LED top      (ge - yellow LED top)
  #define PORT_OUT4       3     // vi - yellow LED bottom   (gn - green LED) - SoftDim, optinal ACK
#endif

#define MAN_VERSION       62   // used in init call, is read in CV#7

#define CV_BLINK_PERIOD   34   // blink period in 0.1s
#define CV_BLINK_PERC     35   // blink percentage
#define CV_FADE_OUT1      36   // Fade time out1 - red
#define CV_FADE_OUT2      37   // Fade time out2 - green
#define CV_FADE_OUT3      38   // Fade time out3 - yellow top
#define CV_FADE_OUT4      39   // Fade time out4 - yellow bot

#define CV_FADE_PAUSE_CFG 49   // Fade_Pause configuration
#define CV_FADE_PAUSE     50   // Pause after Fade Out
#define CV_OUT1_DIM       51   // Dimm value
#define CV_OUT2_DIM       52   // Dimm value
#define CV_OUT3_DIM       53   // Dimm value
#define CV_OUT4_DIM       54   // Dimm value - Soft-Dim
 
struct CVPair {
  uint16_t CV;
  uint8_t Value;
};

int AccDecoderAddr, cv_blink_period;
bool ProgModeActivated = false;
bool Blink_out3 = false;
uint8_t cv_out1_dim, cv_out2_dim, cv_out3_dim, cv_out4_dim;
uint8_t cv_fade_out1, cv_fade_out2, cv_fade_out3, cv_fade_out4, cv_fade_pause;
uint8_t out1_fade_pause, out2_fade_pause, out3_fade_pause, out4_fade_pause;
uint8_t cv_blink_perc, cv_fade_pause_cfg;

uint8_t out4_dim_pos = 0; 
bool out1_fadeInSet, out1_fadeOutSet, out2_fadeInSet, out2_fadeOutSet, out3_fadeInSet, out3_fadeOutSet, out4_fadeInSet, out4_fadeOutSet;
bool fadePauseSet;
bool fadePause = false;
bool out1_on = false;
bool out2_on = false;
bool out3_on = false;
bool out1_fadeIn = false;
bool out1_fadeOut = false;
bool out2_fadeIn = false;
bool out2_fadeOut = false;
bool out3_fadeIn = false;
bool out3_fadeOut = false;
bool out4_fadeIn = false;
bool out4_fadeOut = false;
uint8_t out2_fade = 0;
uint8_t out1_fade = 0;
uint8_t out3_fade = 0;
uint8_t out4_fade = 0;
uint8_t pause = 0;

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
 
// check if the command is for our address and output
  if(SIGN_addr == AccDecoderAddr)  {
    startBlinkMillis = millis();

    switch (SIGN_aspect) {
    case 0:                           // Hp0 (HL13) rot
      out1_fadeIn = true;
      out1_fadeInSet = true;
      out1_fadeOut = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeOut = true;
      out3_fadeOutSet = true;
      out3_fadeIn = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 0");
      #endif
      break;

    case 1:                           // HL1 grün - Strecken-Höchstgeschw.
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeIn = true;
      out2_fadeInSet = true;
      out2_fadeOut = false;
      out3_fadeOut = true;
      out3_fadeOutSet = true;
      out3_fadeIn = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 1");
      #endif
      break;

    case 2:                           // HL3a grün-gelb - Fahrt 40 km/h, dann mit Strecken-Höchstgeschw.
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeIn = true;
      out2_fadeInSet = true;
      out2_fadeOut = false;
      out3_fadeOut = true;
      out3_fadeOutSet = true;
      out3_fadeIn = false;
      out4_fadeIn = true;
      out4_fadeInSet = true;
      out4_fadeOut = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 2");
      #endif
      break;

    case 3:                           // HL7 gelb(blink) - Fahrt auf 40 km/h reduzieren
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeIn = true;
      out3_fadeInSet = true;
      out3_fadeOut = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = true;
      out3_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt

      #if defined DEBUG  
        Serial.println("Aspect 3");
      #endif
      break;

    case 4:                           // HL9a gelb(blink)-gelb - Fahrt mit 40 km/h, 40 km/h erwarten
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeIn = true;
      out3_fadeInSet = true;
      out3_fadeOut = false;
      out4_fadeIn = true;
      out4_fadeInSet = true;
      out4_fadeOut = false;
      fadePause = true;
      Blink_out3 = true;
      out3_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt

      #if defined DEBUG  
        Serial.println("Aspect 4");
      #endif
      break;

    case 5:                           // HL10 gelb-oben - Halt erwarten
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeIn = true;
      out3_fadeInSet = true;
      out3_fadeOut = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 5");
      #endif
      break;

    case 6:                           // HL12a gelb-gelb - Fahrt mit 40 km/h, Halt erwarten
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeIn = true;
      out3_fadeInSet = true;
      out3_fadeOut = false;
      out4_fadeIn = true;
      out4_fadeInSet = true;
      out4_fadeOut = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 6");
      #endif
      break;

    case 7:                           // Option nur, wenn HL7 ... HL12a am Signal nicht genutzt werden, 
                                      // z.B. reduziertes EZMG-Ausfahrsignal mit nur Hp0, HL1, HL3a
                                      // Verdrahtung statt "Gelb oben" dann 2x Weiß für RA12
      out1_fadeIn = true;
      out1_fadeInSet = true;
      out1_fadeOut = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeIn = true;
      out3_fadeInSet = true;
      out3_fadeOut = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = false;

      #if defined DEBUG  
        Serial.println("Aspect 7");
      #endif
      break;

    default:
      out1_fadeOut = true;
      out1_fadeOutSet = true;
      out1_fadeIn = false;
      out2_fadeOut = true;
      out2_fadeOutSet = true;
      out2_fadeIn = false;
      out3_fadeOut = true;
      out3_fadeOutSet = true;
      out3_fadeIn = false;
      out4_fadeOut = true;
      out4_fadeOutSet = true;
      out4_fadeIn = false;
      fadePause = true;
      Blink_out3 = false;
      
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
  {CV_BLINK_PERIOD,13},
  {CV_BLINK_PERC,35},
  {CV_FADE_OUT1,15},
  {CV_FADE_OUT2,15},
  {CV_FADE_OUT3,15},
  {CV_FADE_OUT4,15},
  {CV_FADE_PAUSE_CFG,0x03},
  {CV_FADE_PAUSE,10},
  {CV_OUT1_DIM,15},
  {CV_OUT2_DIM,15},
  {CV_OUT3_DIM,15},
  {CV_OUT4_DIM,15},
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

void FadePause() {
  if (((currentPortMillis-startBlinkMillis) % 25) == 0) {
    if (fadePauseSet)  {
      pause++;
      fadePauseSet = false;
      if (pause >= cv_fade_pause) {
        pause = 0;
        fadePause = false;
        startBlinkMillis = millis();    // wegen Fade-Pause nochmal setzen, sonst fängt das Blinken mit zu kurzem Puls an
      }
    }
  } else fadePauseSet=true;
}

void out1_FadeIn() {
  //red fadeIn OUT1
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out1) == 0) {
    if ((out1_fade < cv_out1_dim) && out1_fadeInSet) {
      out1_fade++;
      analogWrite(PORT_OUT1,out1_fade<<3);
      out1_fadeInSet = false;
    } else {
      if (out1_fade >= cv_out1_dim) {
        out1_fadeIn = false;
      }
    }
  } else out1_fadeInSet=true;
}

void out1_FadeOut() {
  //red fadeOut OUT1
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out1) == 0) {
    if ((out1_fade > 0) && out1_fadeOutSet) {
      out1_fade--;
      out1_fadeOutSet = false;
      analogWrite(PORT_OUT1,out1_fade<<3);
    } else {
      if (out1_fade == 0) out1_fadeOut = false;
    }
  } else out1_fadeOutSet=true;
}

void out2_FadeIn() {
  //green fadeIn OUT2
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out2) == 0) {
    if ((out2_fade < cv_out2_dim) && out2_fadeInSet) {
      out2_fade++;
      out2_fadeInSet = false;
      analogWrite(PORT_OUT2,out2_fade<<3);
    } else {
      if (out2_fade >= cv_out2_dim) {
        out2_fadeIn = false;
      }
    }
  } else out2_fadeInSet=true;
}

void out2_FadeOut() {
  //green fadeOut OUT2
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out2) == 0) {
    if ((out2_fade > 0) && out2_fadeOutSet) {
      out2_fade--;
      out2_fadeOutSet = false;
      analogWrite(PORT_OUT2,out2_fade<<3);
    } else {
      if (out2_fade == 0) out2_fadeOut = false;
    }
  } else out2_fadeOutSet=true;
}

void out3_FadeIn() {
  //yellow top fadeIn OUT3
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out3) == 0) {
    if ((out3_fade < cv_out3_dim) && out3_fadeInSet) {
      out3_fade++;
      out3_fadeInSet = false;
      analogWrite(PORT_OUT3,out3_fade<<3);
    } else {
      if (out3_fade >= cv_out3_dim) {
        out3_fadeIn = false;
      }
    }
  } else out3_fadeInSet=true;
}

void out3_FadeOut() {
  //yellow top_fadeOut OUT3
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out3) == 0) {
    if ((out3_fade > 0) && out3_fadeOutSet) {
      out3_fade--;
      out3_fadeOutSet = false;
      analogWrite(PORT_OUT3,out3_fade<<3);
    } else {
      if (out3_fade == 0) out3_fadeOut = false;
    }
  } else out3_fadeOutSet=true;
}

void out4_FadeIn() {
  //yellow buttom fadeIn OUT4
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out4) == 0) {
    if ((out4_fade < cv_out4_dim) && out4_fadeInSet) {
      out4_fade++;
      out4_fadeInSet = false;
    } else {
      if (out4_fade >= cv_out4_dim) {
        out4_fadeIn = false;
      }
    }
  } else out4_fadeInSet=true;
}

void out4_FadeOut() {
  //yellow buttom fadeOut OUT4
  if (((currentPortMillis-startBlinkMillis) % cv_fade_out4) == 0) {
    if ((out4_fade > 0) && out4_fadeOutSet) {
      out4_fade--;
      out4_fadeOutSet = false;
    } else {
      if (out4_fade == 0) out4_fadeOut = false;
    }
  } else out4_fadeOutSet=true;
}

void setup() {

  int cv29_Bits;

  // Serial output for debugging
  #if defined DEBUG  
    Serial.begin(115200);
    Serial.print("DCC Signal Aspect Decoder Version ");
    Serial.println(MAN_VERSION);
    Serial.println();
  #endif


  if ((Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB) == 255) || (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB) == 255)) {
    FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
  } else {
    AccDecoderAddr = (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_LSB)) + (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS_MSB) << 8);
    cv29_Bits = Dcc.getCV(CV_29_CONFIG);

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

  pinMode(PORT_OUT1, OUTPUT);
  pinMode(PORT_OUT2, OUTPUT);
  pinMode(PORT_OUT3, OUTPUT);
  pinMode(PORT_OUT4, OUTPUT);
  #if defined UNO
    pinMode(PROG_NEXT_PIN, INPUT_PULLUP);
  #endif
  digitalWrite(PORT_OUT1, LOW);
  digitalWrite(PORT_OUT2, LOW);
  digitalWrite(PORT_OUT3, LOW);
  digitalWrite(PORT_OUT4, LOW);

  cv_blink_period = (Dcc.getCV(CV_BLINK_PERIOD) & 0x1F);

  cv_blink_perc = Dcc.getCV(CV_BLINK_PERC);
  if (cv_blink_perc < 2) cv_blink_perc = 2;
  if (cv_blink_perc > 98) cv_blink_perc = 98;

  cv_fade_out1 = Dcc.getCV(CV_FADE_OUT1);
  if (cv_fade_out1 < 2) cv_fade_out1 = 2;
  if (cv_fade_out1 > 31) cv_fade_out1 = 31;

  cv_fade_out2 = Dcc.getCV(CV_FADE_OUT2);
  if (cv_fade_out2 < 2) cv_fade_out2 = 2;
  if (cv_fade_out2 > 31) cv_fade_out2 = 31;

  cv_fade_out3 = Dcc.getCV(CV_FADE_OUT3);
  if (cv_fade_out3 < 2) cv_fade_out3 = 2;
  if (cv_fade_out3 > 31) cv_fade_out3 = 31;

  cv_fade_out4 = Dcc.getCV(CV_FADE_OUT4);
  if (cv_fade_out4 < 2) cv_fade_out4 = 2;
  if (cv_fade_out4 > 31) cv_fade_out4 = 31;

  cv_fade_pause = Dcc.getCV(CV_FADE_PAUSE);
  if (cv_fade_pause < 1) cv_fade_pause = 1;
  pause = cv_fade_pause;

  cv_fade_pause_cfg = Dcc.getCV(CV_FADE_PAUSE_CFG);

  cv_out1_dim = Dcc.getCV(CV_OUT1_DIM);
  if (cv_out1_dim < 1) cv_out1_dim = 1;
  if (cv_out1_dim > 31) cv_out1_dim = 31;

  cv_out2_dim = Dcc.getCV(CV_OUT2_DIM);
  if (cv_out2_dim < 1) cv_out2_dim = 1;
  if (cv_out2_dim > 31) cv_out2_dim = 31;

  cv_out3_dim = Dcc.getCV(CV_OUT3_DIM);
  if (cv_out3_dim < 1) cv_out3_dim = 1;
  if (cv_out3_dim > 31) cv_out3_dim = 31;

  cv_out4_dim = Dcc.getCV(CV_OUT4_DIM);
  if (cv_out4_dim < 1) cv_out4_dim = 1;
  if (cv_out4_dim > 31) cv_out4_dim = 31;

  pinMode(DCC_PIN, INPUT);          // DCC Eingang

  #if defined UNO
    pinMode(PROG_NEXT_PIN, INPUT_PULLUP);
  #endif

  // init NmraDcc library (PIN, manufacturer, version...) 
  Dcc.pin(digitalPinToInterrupt(DCC_PIN), DCC_PIN, 1);
  Dcc.initAccessoryDecoder(MAN_ID_DIY, MAN_VERSION, cv29_Bits & FLAGS_OUTPUT_ADDRESS_MODE, 0);   // CV8=Manufacturer-ID=13, CV7=Manufacturer-VERSION
  
  #if defined UNO
    if (digitalRead(PROG_NEXT_PIN) == 0) {
      #if defined DEBUG  
          Serial.println("ProgModeKey pressed");
      #endif
      Dcc.setAccDecDCCAddrNextReceived(1);
    }
  #endif

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

  if(fadePause) FadePause();

  if ((cv_fade_pause_cfg & 0x01) == 0) {
    if(out1_fadeIn && !fadePause) out1_FadeIn();
  } else {
    if(out1_fadeIn) out1_FadeIn();      // The red light comes on immediately after the command without pause
  }
  if(out1_fadeOut) out1_FadeOut();

  if(out2_fadeIn && !fadePause) out2_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(out2_fadeOut) out2_FadeOut();
  } else {
    if(out2_fadeOut && !out1_fadeIn) out2_FadeOut();
  }

  if(out3_fadeIn && !fadePause) out3_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(out3_fadeOut) out3_FadeOut();
  } else {
    if(out3_fadeOut && !out1_fadeIn) out3_FadeOut();
  }

  if(out4_fadeIn && !fadePause) out4_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(out4_fadeOut) out4_FadeOut();
  } else {
    if(out4_fadeOut && !out1_fadeIn) out4_FadeOut();
  }

  // Soft-Dim OUT4 (PB3)
  if (out4_dim_pos < 31) out4_dim_pos+=1; else out4_dim_pos = 0;
  if (out4_dim_pos < out4_fade) digitalWrite(PORT_OUT4, 1); 
  else digitalWrite(PORT_OUT4, 0);

  // BlinkPort OUT3 yellow top
  if (cv_blink_period && Blink_out3) {
    if (((currentPortMillis-startBlinkMillis) % (cv_blink_period*100)) < cv_blink_period*cv_blink_perc) {
      if (out3_on == false) {
        out3_fadeIn = true;
        out3_fadeInSet = true;
        out3_fadeOut = false;
        out3_on = true;
      }
    } else {
      if (out3_on == true) {
        out3_fadeOut = true;
        out3_fadeOutSet = true;
        out3_fadeIn = false;
        out3_on = false;
      }
    }
  }
}