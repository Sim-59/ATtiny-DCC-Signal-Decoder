// DCC extended Accessories decoder for simple HL-signals (EZMG) of DR
// (c) Michael Hochmuth https://github.com/Sim-59                                2026-10-12
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
  #define PORT_RED         3     // PWM possible
  #define PORT_GREEN       5     // PWM possible
  #define PORT_YTOP        6     // PWM possible
  #define PORT_YBOT        4     // no PWM, Soft-Dim
  #define PROG_NEXT_PIN    13    // programming port
  #define DEBUG
  
#else                           // for ATtiny85 PCB MuFu4P 1.1 and  MuFuDec 1.0, (MuFu4P 1.0 in brackets) 
  #define DCC_PIN         2     // DCC signal
  #define DCC_ACK_PIN     3     // 60 mA-circuit can be temporarily connected to pin 4 for CV reading
  #define PORT_RED        0     // ws - red LED             (ws - red LED)
  #define PORT_GREEN      4     // ge - green LED           (vi - yellow LED bottom)
  #define PORT_YTOP       1     // gn - yellow LED top      (ge - yellow LED top)
  #define PORT_YBOT       3     // vi - yellow LED bottom   (gn - green LED) - SoftDim, optinal ACK
#endif

#define MAN_VERSION       62   // used in init call, is read in CV#7

#define CV_BLINK_PERIOD   34   // blink period in 0.1s
#define CV_BLINK_PERC     35   // blink percentage
#define CV_FADE_RED       36   // Fade time red
#define CV_FADE_GREEN     37   // Fade time green
#define CV_FADE_YTOP      38   // Fade time yellow top
#define CV_FADE_YBOT      39   // Fade time yellow bot

#define CV_FADE_PAUSE_CFG 49   // Fade_Pause configuration
#define CV_FADE_PAUSE     50   // Pause after Fade Out
#define CV_RED_DIM        51   // Dimm value PORT1
#define CV_GREEN_DIM      52   // Dimm value PORT2
#define CV_YTOP_DIM       53   // Dimm value PORT3
#define CV_YBOT_DIM       54   // Dimm value PORT4 - Soft-Dim
 
struct CVPair {
  uint16_t CV;
  uint8_t Value;
};

int AccDecoderAddr, cv_blink_period;
bool ProgModeActivated = false;
bool Blink_ytop = false;
uint8_t cv_red_dim, cv_green_dim, cv_ytop_dim, cv_ybot_dim;
uint8_t cv_fade_red, cv_fade_green, cv_fade_ytop, cv_fade_ybot, cv_fade_pause;
uint8_t red_fade_pause, green_fade_pause, ytop_fade_pause, ybot_fade_pause;
uint8_t cv_blink_perc, cv_fade_pause_cfg;

uint8_t ybot_dim_pos = 0; 
bool red_fadeInSet, red_fadeOutSet, green_fadeInSet, green_fadeOutSet, ytop_fadeInSet, ytop_fadeOutSet, ybot_fadeInSet, ybot_fadeOutSet;
bool fadePauseSet;
bool fadePause = false;
bool red_on = false;
bool green_on = false;
bool ytop_on = false;
bool red_fadeIn = false;
bool red_fadeOut = false;
bool green_fadeIn = false;
bool green_fadeOut = false;
bool ytop_fadeIn = false;
bool ytop_fadeOut = false;
bool ybot_fadeIn = false;
bool ybot_fadeOut = false;
uint8_t green_fade = 0;
uint8_t red_fade = 0;
uint8_t ytop_fade = 0;
uint8_t ybot_fade = 0;
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
      red_fadeIn = true;
      red_fadeInSet = true;
      red_fadeOut = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeOut = true;
      ytop_fadeOutSet = true;
      ytop_fadeIn = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 0");
      #endif
      break;

    case 1:                           // HL1 grün - Strecken-Höchstgeschw.
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeIn = true;
      green_fadeInSet = true;
      green_fadeOut = false;
      ytop_fadeOut = true;
      ytop_fadeOutSet = true;
      ytop_fadeIn = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 1");
      #endif
      break;

    case 2:                           // HL3a grün-gelb - Fahrt 40 km/h, dann mit Strecken-Höchstgeschw.
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeIn = true;
      green_fadeInSet = true;
      green_fadeOut = false;
      ytop_fadeOut = true;
      ytop_fadeOutSet = true;
      ytop_fadeIn = false;
      ybot_fadeIn = true;
      ybot_fadeInSet = true;
      ybot_fadeOut = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 2");
      #endif
      break;

    case 3:                           // HL7 gelb(blink) - Fahrt auf 40 km/h reduzieren
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeIn = true;
      ytop_fadeInSet = true;
      ytop_fadeOut = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
      Blink_ytop = true;
      ytop_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt

      #if defined DEBUG  
        Serial.println("Aspect 3");
      #endif
      break;

    case 4:                           // HL9a gelb(blink)-gelb - Fahrt mit 40 km/h, 40 km/h erwarten
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeIn = true;
      ytop_fadeInSet = true;
      ytop_fadeOut = false;
      ybot_fadeIn = true;
      ybot_fadeInSet = true;
      ybot_fadeOut = false;
      fadePause = true;
      Blink_ytop = true;
      ytop_on = false;                // damit beim ersten Aufruf im Blinkmode "ein" erfolgt

      #if defined DEBUG  
        Serial.println("Aspect 4");
      #endif
      break;

    case 5:                           // HL10 gelb-oben - Halt erwarten
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeIn = true;
      ytop_fadeInSet = true;
      ytop_fadeOut = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 5");
      #endif
      break;

    case 6:                           // HL12a gelb-gelb - Fahrt mit 40 km/h, Halt erwarten
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeIn = true;
      ytop_fadeInSet = true;
      ytop_fadeOut = false;
      ybot_fadeIn = true;
      ybot_fadeInSet = true;
      ybot_fadeOut = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 6");
      #endif
      break;

    case 7:                           // Option nur, wenn HL7 ... HL12a am Signal nicht genutzt werden, 
                                      // z.B. reduziertes EZMG-Ausfahrsignal mit nur Hp0, HL1, HL3a
                                      // Verdrahtung statt "Gelb oben" dann 2x Weiß für RA12
      red_fadeIn = true;
      red_fadeInSet = true;
      red_fadeOut = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeIn = true;
      ytop_fadeInSet = true;
      ytop_fadeOut = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
      Blink_ytop = false;

      #if defined DEBUG  
        Serial.println("Aspect 7");
      #endif
      break;

    default:
      red_fadeOut = true;
      red_fadeOutSet = true;
      red_fadeIn = false;
      green_fadeOut = true;
      green_fadeOutSet = true;
      green_fadeIn = false;
      ytop_fadeOut = true;
      ytop_fadeOutSet = true;
      ytop_fadeIn = false;
      ybot_fadeOut = true;
      ybot_fadeOutSet = true;
      ybot_fadeIn = false;
      fadePause = true;
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
  {CV_BLINK_PERIOD,13},
  {CV_BLINK_PERC,35},
  {CV_FADE_RED,15},
  {CV_FADE_GREEN,15},
  {CV_FADE_YTOP,15},
  {CV_FADE_YBOT,15},
  {CV_FADE_PAUSE_CFG,0x03},
  {CV_FADE_PAUSE,10},
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

void red_FadeIn() {
  //red fadeIn PORT1
  if (((currentPortMillis-startBlinkMillis) % cv_fade_red) == 0) {
    if ((red_fade < cv_red_dim) && red_fadeInSet) {
      red_fade++;
      analogWrite(PORT_RED,red_fade<<3);
      red_fadeInSet = false;
    } else {
      if (red_fade >= cv_red_dim) {
        red_fadeIn = false;
      }
    }
  } else red_fadeInSet=true;
}

void red_FadeOut() {
  //red fadeOut PORT1
  if (((currentPortMillis-startBlinkMillis) % cv_fade_red) == 0) {
    if ((red_fade > 0) && red_fadeOutSet) {
      red_fade--;
      red_fadeOutSet = false;
      analogWrite(PORT_RED,red_fade<<3);
    } else {
      if (red_fade == 0) red_fadeOut = false;
    }
  } else red_fadeOutSet=true;
}

void green_FadeIn() {
  //green fadeIn PORT2
  if (((currentPortMillis-startBlinkMillis) % cv_fade_green) == 0) {
    if ((green_fade < cv_green_dim) && green_fadeInSet) {
      green_fade++;
      green_fadeInSet = false;
      analogWrite(PORT_GREEN,green_fade<<3);
    } else {
      if (green_fade >= cv_green_dim) {
        green_fadeIn = false;
      }
    }
  } else green_fadeInSet=true;
}

void green_FadeOut() {
  //green fadeOut PORT2
  if (((currentPortMillis-startBlinkMillis) % cv_fade_green) == 0) {
    if ((green_fade > 0) && green_fadeOutSet) {
      green_fade--;
      green_fadeOutSet = false;
      analogWrite(PORT_GREEN,green_fade<<3);
    } else {
      if (green_fade == 0) green_fadeOut = false;
    }
  } else green_fadeOutSet=true;
}

void ytop_FadeIn() {
  //yellow top fadeIn PORT3
  if (((currentPortMillis-startBlinkMillis) % cv_fade_ytop) == 0) {
    if ((ytop_fade < cv_ytop_dim) && ytop_fadeInSet) {
      ytop_fade++;
      ytop_fadeInSet = false;
      analogWrite(PORT_YTOP,ytop_fade<<3);
    } else {
      if (ytop_fade >= cv_ytop_dim) {
        ytop_fadeIn = false;
      }
    }
  } else ytop_fadeInSet=true;
}

void ytop_FadeOut() {
  //yellow top_fadeOut PORT3
  if (((currentPortMillis-startBlinkMillis) % cv_fade_ytop) == 0) {
    if ((ytop_fade > 0) && ytop_fadeOutSet) {
      ytop_fade--;
      ytop_fadeOutSet = false;
      analogWrite(PORT_YTOP,ytop_fade<<3);
    } else {
      if (ytop_fade == 0) ytop_fadeOut = false;
    }
  } else ytop_fadeOutSet=true;
}

void ybot_FadeIn() {
  //yellow buttom fadeIn PORT4
  if (((currentPortMillis-startBlinkMillis) % cv_fade_ybot) == 0) {
    if ((ybot_fade < cv_ybot_dim) && ybot_fadeInSet) {
      ybot_fade++;
      ybot_fadeInSet = false;
    } else {
      if (ybot_fade >= cv_ybot_dim) {
        ybot_fadeIn = false;
      }
    }
  } else ybot_fadeInSet=true;
}

void ybot_FadeOut() {
  //yellow buttom fadeOut PORT4
  if (((currentPortMillis-startBlinkMillis) % cv_fade_ybot) == 0) {
    if ((ybot_fade > 0) && ybot_fadeOutSet) {
      ybot_fade--;
      ybot_fadeOutSet = false;
    } else {
      if (ybot_fade == 0) ybot_fadeOut = false;
    }
  } else ybot_fadeOutSet=true;
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

  cv_blink_period = (Dcc.getCV(CV_BLINK_PERIOD) & 0x1F);

  cv_blink_perc = Dcc.getCV(CV_BLINK_PERC);
  if (cv_blink_perc < 2) cv_blink_perc = 2;
  if (cv_blink_perc > 98) cv_blink_perc = 98;

  cv_fade_red = Dcc.getCV(CV_FADE_RED);
  if (cv_fade_red < 2) cv_fade_red = 2;
  if (cv_fade_red > 31) cv_fade_red = 31;

  cv_fade_green = Dcc.getCV(CV_FADE_GREEN);
  if (cv_fade_green < 2) cv_fade_green = 2;
  if (cv_fade_green > 31) cv_fade_green = 31;

  cv_fade_ytop = Dcc.getCV(CV_FADE_YTOP);
  if (cv_fade_ytop < 2) cv_fade_ytop = 2;
  if (cv_fade_ytop > 31) cv_fade_ytop = 31;

  cv_fade_ybot = Dcc.getCV(CV_FADE_YBOT);
  if (cv_fade_ybot < 2) cv_fade_ybot = 2;
  if (cv_fade_ybot > 31) cv_fade_ybot = 31;

  cv_fade_pause = Dcc.getCV(CV_FADE_PAUSE);
  if (cv_fade_pause < 1) cv_fade_pause = 1;
  pause = cv_fade_pause;

  cv_fade_pause_cfg = Dcc.getCV(CV_FADE_PAUSE_CFG);

  cv_red_dim = Dcc.getCV(CV_RED_DIM);
  if (cv_red_dim < 1) cv_red_dim = 1;
  if (cv_red_dim > 31) cv_red_dim = 31;

  cv_green_dim = Dcc.getCV(CV_GREEN_DIM);
  if (cv_green_dim < 1) cv_green_dim = 1;
  if (cv_green_dim > 31) cv_green_dim = 31;

  cv_ytop_dim = Dcc.getCV(CV_YTOP_DIM);
  if (cv_ytop_dim < 1) cv_ytop_dim = 1;
  if (cv_ytop_dim > 31) cv_ytop_dim = 31;

  cv_ybot_dim = Dcc.getCV(CV_YBOT_DIM);
  if (cv_ybot_dim < 1) cv_ybot_dim = 1;
  if (cv_ybot_dim > 31) cv_ybot_dim = 31;

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
    if(red_fadeIn && !fadePause) red_FadeIn();
  } else {
    if(red_fadeIn) red_FadeIn();      // The red light comes on immediately after the command without pause
  }
  if(red_fadeOut) red_FadeOut();

  if(green_fadeIn && !fadePause) green_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(green_fadeOut) green_FadeOut();
  } else {
    if(green_fadeOut && !red_fadeIn) green_FadeOut();
  }

  if(ytop_fadeIn && !fadePause) ytop_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(ytop_fadeOut) ytop_FadeOut();
  } else {
    if(ytop_fadeOut && !red_fadeIn) ytop_FadeOut();
  }

  if(ybot_fadeIn && !fadePause) ybot_FadeIn();
  if ((cv_fade_pause_cfg & 0x02) == 0) {
    if(ybot_fadeOut) ybot_FadeOut();
  } else {
    if(ybot_fadeOut && !red_fadeIn) ybot_FadeOut();
  }

  // Soft-Dim PORT4 (PB3)
  if (ybot_dim_pos < 31) ybot_dim_pos+=1; else ybot_dim_pos = 0;
  if (ybot_dim_pos < ybot_fade) digitalWrite(PORT_YBOT, 1); 
  else digitalWrite(PORT_YBOT, 0);

  // BlinkPort PORT3 yellow top
  if (cv_blink_period && Blink_ytop) {
    if (((currentPortMillis-startBlinkMillis) % (cv_blink_period*100)) < cv_blink_period*cv_blink_perc) {
      if (ytop_on == false) {
        ytop_fadeIn = true;
        ytop_fadeInSet = true;
        ytop_fadeOut = false;
        ytop_on = true;
      }
    } else {
      if (ytop_on == true) {
        ytop_fadeOut = true;
        ytop_fadeOutSet = true;
        ytop_fadeIn = false;
        ytop_on = false;
      }
    }
  }
}