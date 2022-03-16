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
  filters.addExtendedDual (0x5555, false, // Extended data frame, identifier 0x5555
                           0x6666, true,  // Extended remote frame, identifier 0x6666
                           ACAN_STM32::FIFO0) ;
//--- Add quad standard filter (4 matching frames)
  filters.addStandardQuad (0x123, false, // Standard data frame, identifier 0x123
                           0x234, true,  // Standard remote frame, identifier 0x234
                           0x345, true,  // Standard remote frame, identifier 0x345
                           0x456, false, // Standard data frame, identifier 0x456
                           ACAN_STM32::FIFO1) ;
//--- Add extended mask filter (32 matching data frames)
  filters.addExtendedMask (0x6789, 0x1FFF67BD, ACAN_STM32::DATA, ACAN_STM32::FIFO1) ;
//--- Add extended mask filter (32 matching remote frames)
  filters.addExtendedMask (0x6789, 0x1FFF67BD, ACAN_STM32::REMOTE, ACAN_STM32::FIFO0) ;
//--- Add extended mask filter (32 matching data frames, 32 matching remote frames)
  filters.addExtendedMask (0x4789, 0x1FFF67BD, ACAN_STM32::DATA_OR_REMOTE, ACAN_STM32::FIFO0) ;
//--- Add standard dual mask filter
  filters.addStandardMasks (0x405, 0x7D5, ACAN_STM32::DATA, // 8 Standard data frames
                            0x605, 0x7D5, ACAN_STM32::REMOTE, // 8 Standard remote frames
                            ACAN_STM32::FIFO1) ;
//--- Add standard dual mask filter
  filters.addStandardMasks (0x705, 0x7D5, ACAN_STM32::DATA_OR_REMOTE, // 4 Standard data frames, 4 Standard remote frames
                            0x505, 0x7D5, ACAN_STM32::DATA_OR_REMOTE, // 4 Standard data frames, 4 Standard remote frames
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
}

//-----------------------------------------------------------------

static const uint32_t PERIOD = 1000 ;
static uint32_t gBlinkDate = PERIOD ;
static uint32_t gSentIdentifierAndFormat = 0 ;
static uint32_t gReceiveCountFIFO0 = 0 ;
static uint32_t gReceiveCountFIFO1 = 0 ;
static bool gOk = true ;
static bool gSendExtended = false ;

//-----------------------------------------------------------------

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

//-----------------------------------------------------------------

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
  CANMessage frame ;
  if (gOk && can.receive0 (frame)) {
    gReceiveCountFIFO0 += 1 ;
  }
  if (gOk && can.receive1 (frame)) {
    gReceiveCountFIFO1 += 1 ;
  }
//--- Blink led and display
  if (gBlinkDate <= millis ()) {
    gBlinkDate += PERIOD ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    Serial.print ("Sent: ") ;
    Serial.print (gSentIdentifierAndFormat) ;
    printCount (gReceiveCountFIFO0, 2 + 32 + 64 + 8 + 8) ;
    printCount (gReceiveCountFIFO1, 4 + 32 + 16) ;
    Serial.println () ;
  }
}

//-----------------------------------------------------------------
