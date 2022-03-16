// This demo runs on NUCLEO_L432KC and NUCLEO_F303K8
// The CAN module is configured in external loop back mode: it
// internally receives every CAN frame it sends, and emitted frames
// can be observed on TxCAN pin (D2, e.g. PA12).

// No external hardware is required.
//-----------------------------------------------------------------

#include <ACAN_STM32.h>

//-----------------------------------------------------------------

static ACAN_STM32_FIFO gBuffer ;

//-----------------------------------------------------------------

void setup () {
  gBuffer.initWithSize (100) ;
  pinMode (LED_BUILTIN, OUTPUT) ;
  Serial.begin (115200) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
  Serial.println ("CAN loopback test") ;
  ACAN_STM32_Settings settings (1000 * 1000) ;

  settings.mModuleMode = ACAN_STM32_Settings::EXTERNAL_LOOP_BACK ;

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

  const uint32_t errorCode = can.begin (settings) ;

  if (0 == errorCode) {
    Serial.println ("can configuration ok") ;
  }else{
    Serial.print ("Error can configuration: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//-----------------------------------------------------------------

static uint32_t pseudoRandomValue (void) {
  static uint32_t gSeed = 0 ;
  gSeed = 8253729U * gSeed + 2396403U ;
  return gSeed ;
}

//-----------------------------------------------------------------

static const uint32_t PERIOD = 1000 ;
static uint32_t gBlinkDate = PERIOD ;
static uint32_t gSentCount = 0 ;
static uint32_t gReceiveCount = 0 ;
static bool gOk = true ;
static uint32_t gCANRemoteFrameCount = 0 ;
static uint32_t gCanDataFrameCount = 0 ;
static uint32_t gStandardFrameCount = 0 ;
static uint32_t gExtendedFrameCount = 0 ;

//-----------------------------------------------------------------

void loop () {
  if (gBlinkDate <= millis ()) {
    gBlinkDate += PERIOD ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    Serial.print ("Sent ") ;
    Serial.print (gSentCount) ;
    Serial.print (", received ") ;
    Serial.print (gReceiveCount) ;
    Serial.print (" (") ;
    Serial.print (gCANRemoteFrameCount) ;
    Serial.print (", ") ;
    Serial.print (gCanDataFrameCount) ;
    Serial.print (", ") ;
    Serial.print (gStandardFrameCount) ;
    Serial.print (", ") ;
    Serial.print (gExtendedFrameCount) ;
    Serial.println (")") ;
  }
//--- Send buffer index
//    0: fifo
//    1 ... mailbox 1
//    2 ... mailbox 2
  const uint8_t sendBufferIndex = 0 ;
//--- Send frame
  if (gOk && !gBuffer.isFull () && can.sendBufferNotFullForIndex (sendBufferIndex)) {
    CANMessage frame ;
    frame.idx = sendBufferIndex ;
    const uint32_t r = pseudoRandomValue () ;
    frame.ext = (r & (1 << 29)) != 0 ;
    frame.rtr = (r & (1 << 30)) != 0 ;
    frame.id = r & 0x1FFFFFFF ;
    if (frame.ext) {
      gExtendedFrameCount += 1 ;
    }else{
      gStandardFrameCount += 1 ;
      frame.id &= 0x7FF ;
    }
    if (frame.rtr) {
      gCANRemoteFrameCount += 1 ;
      frame.len = pseudoRandomValue () % 9 ;
    }else{
      gCanDataFrameCount += 1 ;
      frame.len = pseudoRandomValue () % 9 ;
      for (uint32_t i=0 ; i<frame.len ; i++) {
        frame.data [i] = uint8_t (pseudoRandomValue ()) ;
      }
    }
    gBuffer.append (frame) ;
    const uint32_t sendStatus = can.tryToSendReturnStatus (frame) ;
    if (sendStatus == 0) {
      gSentCount += 1 ;
    }else{
      gOk = false ;
      Serial.print ("Send status error 0x") ;
      Serial.println (sendStatus, HEX) ;
    }
  }
//--- Receive frame
  CANMessage receivedFrame ;
  if (gOk && can.receive0 (receivedFrame)) {
    CANMessage storedFrame ;
    gBuffer.remove (storedFrame) ;
    gReceiveCount += 1 ;
    bool sameFrames = storedFrame.id == receivedFrame.id ;
    if (sameFrames) {
      sameFrames = storedFrame.ext == receivedFrame.ext ;
    }
    if (sameFrames) {
      sameFrames = storedFrame.rtr == receivedFrame.rtr ;
    }
    if (sameFrames) {
      sameFrames = storedFrame.len == receivedFrame.len ;
    }
    if (!storedFrame.rtr) {
      for (uint32_t i=0 ; (i<receivedFrame.len) && sameFrames ; i++) {
        sameFrames = storedFrame.data [i] == receivedFrame.data [i] ;
      }
    }
    if (!sameFrames) {
      gOk = false ;
      Serial.println ("Receive error") ;
      Serial.print ("  IDF: 0x") ;
      Serial.print (storedFrame.id, HEX) ;
      Serial.print (" :: 0x") ;
      Serial.println (receivedFrame.id, HEX) ;
      Serial.print ("  EXT: ") ;
      Serial.print (storedFrame.ext) ;
      Serial.print (" :: ") ;
      Serial.println (receivedFrame.ext) ;
      Serial.print ("  RTR: ") ;
      Serial.print (storedFrame.rtr) ;
      Serial.print (" :: ") ;
      Serial.println (receivedFrame.rtr) ;
      Serial.print ("  LENGTH: ") ;
      Serial.print (storedFrame.len) ;
      Serial.print (" :: ") ;
      Serial.println (receivedFrame.len) ;     
    }
  }
}

//-----------------------------------------------------------------
