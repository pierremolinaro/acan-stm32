// LoopBackDemo

// This demo runs on NUCLEO_L432KC and NUCLEO_F303K8
// The CAN module is configured in external loop back mode: it
// internally receives every CAN frame it sends, and emitted frames
// can be observed on TxCAN pin (D2, e.g. PA12).

// No external hardware is required.
//-----------------------------------------------------------------

#include <ACAN_STM32.h>

//-----------------------------------------------------------------

void setup () {
  pinMode (LED_BUILTIN, OUTPUT) ;
  Serial.begin (115200) ;
  while (!Serial) {
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    delay (50) ;
  }

  ACAN_STM32_Settings settings (125 * 1000) ; // 125 kbit/s
 
  Serial.print ("Bit Rate prescaler: ") ;
  Serial.println (settings.mBitRatePrescaler) ;
  Serial.print ("Phase segment 1: ") ;
  Serial.println (settings.mPhaseSegment1) ;
  Serial.print ("Phase segment 2: ") ;
  Serial.println (settings.mPhaseSegment2) ;
  Serial.print ("RJW: ") ;
  Serial.println (settings.mRJW) ;
  Serial.print ("Actual Bit Rate: ") ;
  Serial.print (settings.actualBitRate ()) ;
  Serial.println (" bit/s") ;
  Serial.print ("Sample point: ") ;
  Serial.print (settings.samplePointFromBitStart ()) ;
  Serial.println ("%") ;
  Serial.print ("Exact Bit Rate ? ") ;
  Serial.println (settings.exactBitRate () ? "yes" : "no") ;

  settings.mModuleMode = ACAN_STM32_Settings::EXTERNAL_LOOP_BACK ;
  
  const uint32_t errorCode = can.begin (settings) ;
  if (0 == errorCode) {
    Serial.println ("can ok") ;
  }else{
    Serial.print ("Error can: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//-----------------------------------------------------------------

static uint32_t gSendDate = 0 ;
static uint32_t gSentCount = 0 ;
static uint32_t gReceivedCount = 0 ;

//-----------------------------------------------------------------

void loop () {
  CANMessage message ;
  if (gSendDate < millis ()) {
    message.id = 0x542 ;
    message.len = 8 ;
    message.data [0] = 0 ;
    message.data [1] = 1 ;
    message.data [2] = 2 ;
    message.data [3] = 3 ;
    message.data [4] = 4 ;
    message.data [5] = 5 ;
    message.data [6] = 6 ;
    message.data [7] = 7 ;
    const bool ok = can.tryToSendReturnStatus (message) ;
    if (ok) {
      digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
      gSendDate += 1000 ;
      gSentCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentCount) ;
    }
  }
  if (can.receive0 (message)) {
    gReceivedCount += 1 ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedCount) ;
  }
}
