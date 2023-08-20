//----------------------------------------------------------------------------------------
// This demo runs on NUCLEO_L432KC and NUCLEO_F303K8
// The CAN module is configured in external loop back mode: it
// internally receives every CAN frame it sends, and emitted frames
// can be observed on TxCAN pin (D2, e.g. PA12).

// No external hardware is required.
//----------------------------------------------------------------------------------------

#include <ACAN_STM32.h>

//----------------------------------------------------------------------------------------

static bool gOk = true ;

//----------------------------------------------------------------------------------------

static void filterError (const CANMessage & inMessage,
                         const char * inCallBackName) {
  Serial.print ("*** Filter error for '") ;
  Serial.print (inCallBackName) ;
  Serial.println ("' callback:") ;
  Serial.print ("  Identifier: 0x") ;
  Serial.println (inMessage.id, HEX) ;
  Serial.print ("  Format: ") ;
  Serial.println (inMessage.ext ? "extended" : "standard") ;
  Serial.print ("  Type: ") ;
  Serial.println (inMessage.rtr ? "remote" : "data") ;
  gOk = false ;
}

//----------------------------------------------------------------------------------------

static uint32_t gMatch1 = 0 ;
static uint32_t gMatch2 = 0 ;
static uint32_t gMatch3 = 0 ;
static uint32_t gMatch4 = 0 ;
static uint32_t gMatch5 = 0 ;
static uint32_t gMatch6 = 0 ;
static uint32_t gMatch7 = 0 ;
static uint32_t gMatch8 = 0 ;
static uint32_t gMatch9 = 0 ;
static uint32_t gMatch10 = 0 ;
static uint32_t gMatch11 = 0 ;
static uint32_t gMatch12 = 0 ;
static uint32_t gMatch13 = 0 ;

//----------------------------------------------------------------------------------------

static void callBack1 (const CANMessage & inMessage) {
  if (inMessage.ext && (inMessage.id == 0x5555) && !inMessage.rtr) {
    gMatch1 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack2 (const CANMessage & inMessage) {
  if (inMessage.ext && (inMessage.id == 0x6666) && inMessage.rtr) {
    gMatch2 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack3 (const CANMessage & inMessage) {
  if (!inMessage.ext && (inMessage.id == 0x123) && !inMessage.rtr) {
    gMatch3 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack4 (const CANMessage & inMessage) {
  if (!inMessage.ext && (inMessage.id == 0x234) && inMessage.rtr) {
    gMatch4 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack5 (const CANMessage & inMessage) {
  if (!inMessage.ext && (inMessage.id == 0x345) && inMessage.rtr) {
    gMatch5 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack6 (const CANMessage & inMessage) {
  if (!inMessage.ext && (inMessage.id == 0x456) && !inMessage.rtr) {
    gMatch6 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack7 (const CANMessage & inMessage) {
  if (inMessage.ext && ((inMessage.id & 0x1FFF67BD) == 0x6789) && !inMessage.rtr) {
    gMatch7 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack8 (const CANMessage & inMessage) {
  if (inMessage.ext && ((inMessage.id & 0x1FFF67BD) == 0x6789) && inMessage.rtr) {
    gMatch8 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack9 (const CANMessage & inMessage) {
  if (inMessage.ext && ((inMessage.id & 0x1FFF67BD) == 0x4789)) {
    gMatch9 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack10 (const CANMessage & inMessage) {
  if (!inMessage.ext && ((inMessage.id & 0x7D5) == 0x405) && !inMessage.rtr) {
    gMatch10 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack11 (const CANMessage & inMessage) {
  if (!inMessage.ext && ((inMessage.id & 0x7D5) == 0x605) && inMessage.rtr) {
    gMatch11 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack12 (const CANMessage & inMessage) {
  if (!inMessage.ext && ((inMessage.id & 0x7D5) == 0x705)) {
    gMatch12 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

static void callBack13 (const CANMessage & inMessage) {
  if (!inMessage.ext && ((inMessage.id & 0x7D5) == 0x505)) {
    gMatch13 += 1 ;
  }else{
    filterError (inMessage, __FUNCTION__) ;
  }
}

//----------------------------------------------------------------------------------------

void setup () {
  pinMode (LED_BUILTIN, OUTPUT) ;
  Serial.begin (38400) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
  Serial.println ("CAN loopback filter test") ;
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

  ACAN_STM32::Filters filters ;
//--- Add dual extended filter: identifier, false -> data, true -> rtr (2 matching frames)
  filters.addExtendedDual (0x5555, false, callBack1, // Extended data frame, identifier 0x5555
                           0x6666, true,  callBack2, // Extended remote frame, identifier 0x6666
                           ACAN_STM32::FIFO0) ;
//--- Add quad standard filter (4 matching frames)
  filters.addStandardQuad (0x123, false, callBack3, // Standard data frame, identifier 0x123
                           0x234, true,  callBack4, // Standard remote frame, identifier 0x234
                           0x345, true,  callBack5, // Standard remote frame, identifier 0x345
                           0x456, false, callBack6, // Standard data frame, identifier 0x456
                           ACAN_STM32::FIFO1) ;
//--- Add extended mask filter (32 matching data frames)
  filters.addExtendedMask (0x6789, 0x1FFF67BD, ACAN_STM32::DATA, callBack7, ACAN_STM32::FIFO1) ;
//--- Add extended mask filter (32 matching remote frames)
  filters.addExtendedMask (0x6789, 0x1FFF67BD, ACAN_STM32::REMOTE, callBack8, ACAN_STM32::FIFO0) ;
//--- Add extended mask filter (32 matching data frames, 32 matching remote frames)
  filters.addExtendedMask (0x4789, 0x1FFF67BD, ACAN_STM32::DATA_OR_REMOTE, callBack9, ACAN_STM32::FIFO0) ;
//--- Add standard dual mask filter
  filters.addStandardMasks (0x405, 0x7D5, ACAN_STM32::DATA, callBack10, // 8 Standard data frames
                            0x605, 0x7D5, ACAN_STM32::REMOTE, callBack11, // 8 Standard remote frames
                            ACAN_STM32::FIFO1) ;
//--- Add standard dual mask filter
  filters.addStandardMasks (0x705, 0x7D5, ACAN_STM32::DATA_OR_REMOTE, callBack12, // 4 Standard data frames, 4 Standard remote frames
                            0x505, 0x7D5, ACAN_STM32::DATA_OR_REMOTE, callBack13, // 4 Standard data frames, 4 Standard remote frames
                            ACAN_STM32::FIFO0) ;

//--- Allocate FIFO 1
  settings.mDriverReceiveFIFO1Size = 10 ; // By default, 0

  const uint32_t errorCode = can.begin (settings, filters) ;

  if (0 == errorCode) {
    Serial.println ("can configuration ok") ;
  }else{
    Serial.print ("Error can configuration: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
  Serial.print ("CAN clock: ") ;
  Serial.print (HAL_RCC_GetPCLK1Freq ()) ;
  Serial.println (" Hz") ;
}

//----------------------------------------------------------------------------------------

static const uint32_t PERIOD = 1000 ;
static uint32_t gBlinkDate = PERIOD ;
static uint32_t gSentIdentifierAndFormat = 0 ;
static bool gSendExtended = false ;

//----------------------------------------------------------------------------------------

static void printCount (const uint32_t inActualCount, const uint32_t inExpectedCount) {
  Serial.print (", ") ;
  if (inActualCount == inExpectedCount) {
    Serial.print ("ok") ;
  }else{
    Serial.print (inActualCount) ;
    Serial.print ("/") ;
    Serial.print (inExpectedCount) ;
  }
}

//----------------------------------------------------------------------------------------

void loop () {
//--- Send standard frame ?
  if (!gSendExtended && gOk && (gSentIdentifierAndFormat <= 0xFFF) && can.sendBufferNotFullForIndex (0)) {
    CANMessage frame ;
    frame.id = gSentIdentifierAndFormat >> 1 ;
    frame.rtr = (gSentIdentifierAndFormat & 1) != 0 ;
    gSentIdentifierAndFormat += 1 ;
    const uint32_t sendStatus = can.tryToSendReturnStatus (frame) ;
    if (sendStatus != 0) {
      gOk = false ;
      Serial.print ("Sent error 0x") ;
      Serial.println (sendStatus) ;
    }
  }
//--- All standard frame have been sent ?
  if (!gSendExtended && gOk && (gSentIdentifierAndFormat > 0xFFF)) {
    gSendExtended = true ;
    gSentIdentifierAndFormat = 0 ;
  }
//--- Send extended frame ?
  if (gSendExtended && gOk && (gSentIdentifierAndFormat <= 0x3FFFFFFF) && can.sendBufferNotFullForIndex (0)) {
    CANMessage frame ;
    frame.id = gSentIdentifierAndFormat >> 1 ;
    frame.rtr = (gSentIdentifierAndFormat & 1) != 0 ;
    frame.ext = true ;
    gSentIdentifierAndFormat += 1 ;
    const uint32_t sendStatus = can.tryToSendReturnStatus (frame) ;
    if (sendStatus != 0) {
      gOk = false ;
      Serial.print ("Sent error 0x") ;
      Serial.println (sendStatus) ;
    }
  }
//--- Receive frame
  can.dispatchReceivedMessage () ;
//--- Blink led and display
  if (gBlinkDate <= millis ()) {
    gBlinkDate += PERIOD ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    if (gOk) {
      Serial.print ("Sent: ") ;
      Serial.print (gSentIdentifierAndFormat) ;
      printCount (gMatch1, 1) ;
      printCount (gMatch2, 1) ;
      printCount (gMatch3, 1) ;
      printCount (gMatch4, 1) ;
      printCount (gMatch5, 1) ;
      printCount (gMatch6, 1) ;
      printCount (gMatch7, 32) ;
      printCount (gMatch8, 32) ;
      printCount (gMatch9, 64) ;
      printCount (gMatch10, 8) ;
      printCount (gMatch11, 8) ;
      printCount (gMatch12, 8) ;
      printCount (gMatch13, 8) ;
      Serial.println () ;
    }
  }
}

//----------------------------------------------------------------------------------------
