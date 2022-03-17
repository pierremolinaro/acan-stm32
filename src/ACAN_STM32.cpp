#include <ACAN_STM32.h>

//----------------------------------------------------------------------------------------------------------------------
//    imin template function
//----------------------------------------------------------------------------------------------------------------------

template <typename T> static inline T imin (const T inA, const T inB) {
  return (inA <= inB) ? inA : inB ;
}

//----------------------------------------------------------------------------------------------------------------------
//    Constructor
//----------------------------------------------------------------------------------------------------------------------

ACAN_STM32::ACAN_STM32 (volatile uint32_t * inClockEnableRegisterPointer,
                        const uint8_t inClockEnableBitOffset,
                        volatile uint32_t * inResetRegisterPointer,
                        const uint8_t inResetBitOffset,
                        volatile CAN_TypeDef * inPeripheralModuleBasePointer,
                        const IRQn_Type in_TX_IRQn,
                        const IRQn_Type in_RX0_IRQn,
                        const IRQn_Type in_RX1_IRQn,
                        GPIO_TypeDef * inTxPinGPIO,
                        const uint8_t inTxPinIndex,
                        const uint8_t inTxPinAlternateMode,
                        GPIO_TypeDef * inRxPinGPIO,
                        const uint8_t inRxPinIndex,
                        const uint8_t inRxPinAlternateMode) :
mClockEnableRegisterPointer (inClockEnableRegisterPointer),
mResetRegisterPointer (inResetRegisterPointer),
mCAN (inPeripheralModuleBasePointer),
mTxPinGPIO (inTxPinGPIO),
mRxPinGPIO (inRxPinGPIO),
mClockEnableBitOffset (inClockEnableBitOffset),
mResetBitOffset (inResetBitOffset),
m_TX_IRQn (in_TX_IRQn),
m_RX0_IRQn (in_RX0_IRQn),
m_RX1_IRQn (in_RX1_IRQn),
mTxPinIndex (inTxPinIndex),
mTxPinAlternateMode (inTxPinAlternateMode),
mRxPinIndex (inRxPinIndex),
mRxPinAlternateMode (inRxPinAlternateMode) {
}

//----------------------------------------------------------------------------------------------------------------------
//    end
//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::end (void) {
//--- Disable interrupts
  NVIC_DisableIRQ (m_RX0_IRQn);
  NVIC_DisableIRQ (m_RX1_IRQn);
  NVIC_DisableIRQ (m_TX_IRQn);
//--- Free receive FIFOs
  mDriverReceiveFIFO0.free () ;
  mDriverReceiveFIFO1.free () ;
//--- Free transmit FIFO
  mDriverTransmitFIFO.free () ;
//--- Free callback function array
  mFIFO0CallBackArray.free () ;
  mFIFO1CallBackArray.free () ;
}

//----------------------------------------------------------------------------------------------------------------------
//    begin method
//----------------------------------------------------------------------------------------------------------------------

static const uint32_t MAX_FILTER_COUNT = 14 ;

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32::begin (const ACAN_STM32_Settings & inSettings,
                            const ACAN_STM32::Filters & inFilters) {
  uint32_t errorCode = inSettings.CANBitSettingConsistency () ;
//--- No configuration if CAN bit settings are incorrect
  if ((errorCode == 0) && !inSettings.mBitRateClosedToDesiredRate) {
    errorCode = kActualBitRateTooFarFromDesiredBitRate ;
  }
  if (0 == errorCode) {
    errorCode = internalBegin (inSettings, inFilters) ;
  }
  return errorCode ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32::internalBegin (const ACAN_STM32_Settings & inSettings,
                                    const ACAN_STM32::Filters & inFilters) {
  uint32_t errorCode = 0 ; // No error

//---------------------------------------------- Allocate buffers
  mDriverReceiveFIFO0.initWithSize (inSettings.mDriverReceiveFIFO0Size) ;
  mDriverReceiveFIFO1.initWithSize (inSettings.mDriverReceiveFIFO1Size) ;
  mDriverTransmitFIFO.initWithSize (inSettings.mDriverTransmitFIFOSize) ;

//---------------------------------------------- Allocate call back function array
  inFilters.copyFIFO0CallBackArrayTo (mFIFO0CallBackArray) ;
  inFilters.copyFIFO1CallBackArrayTo (mFIFO1CallBackArray) ;


//---------------------------------------------- Enable CAN clock
  *mClockEnableRegisterPointer |= 1U << mClockEnableBitOffset ; // Enable clock for CAN
  const uint32_t unused1 __attribute__ ((unused)) = *mClockEnableRegisterPointer ; // Wait until done

//---------------------------------------------- Reset CAN peripheral
  *mResetRegisterPointer |=    (1U << mResetBitOffset) ; // Reset CAN
  *mResetRegisterPointer &=  ~ (1U << mResetBitOffset) ;

//---------------------------------------------- Configure TxPin
  const uint32_t txPinMask = 1U << mTxPinIndex ;
  LL_GPIO_SetPinMode  (mTxPinGPIO, txPinMask, LL_GPIO_MODE_ALTERNATE) ;
  LL_GPIO_SetPinOutputType (mTxPinGPIO, txPinMask, inSettings.mOpenCollectorOutput ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL) ;
  LL_GPIO_SetPinSpeed (mTxPinGPIO, txPinMask, LL_GPIO_SPEED_FREQ_HIGH) ;
  if (mTxPinIndex < 8) {
    LL_GPIO_SetAFPin_0_7 (mTxPinGPIO, txPinMask, mTxPinAlternateMode) ;
  }else{
    LL_GPIO_SetAFPin_8_15 (mTxPinGPIO, txPinMask, mTxPinAlternateMode) ;
  }

//---------------------------------------------- Configure RxPin
  const uint32_t rxPinMask = 1U << mRxPinIndex ;
  LL_GPIO_SetPinMode  (mRxPinGPIO, rxPinMask, LL_GPIO_MODE_ALTERNATE) ;
  if (mRxPinIndex < 8) {
    LL_GPIO_SetAFPin_0_7 (mRxPinGPIO, rxPinMask, mRxPinAlternateMode) ;
  }else{
    LL_GPIO_SetAFPin_8_15 (mRxPinGPIO, rxPinMask, mRxPinAlternateMode) ;
  }

//---------------------------------------------- Init CAN
// set INRQ bit in MCR and wait until INAK bit in MSR is Ok.
// INRQ: Initialization request, NART: No automatic retransmission
  mCAN->MCR = CAN_MCR_INRQ | CAN_MCR_NART ; // all other fields to 0 (reset state).
  while ((mCAN->MSR & CAN_MSR_INAK) == 0) {}
  mCAN->IER = 0; // no interrupt enabled.

//---------------------------------------------- BTR
//--- Can bit timing
  uint32_t btr =
    ((inSettings.mBitRatePrescaler - 1)  << CAN_BTR_BRP_Pos) // 1 tq each mBitRatePrescaler input clock
  |
    ((inSettings.mPhaseSegment1 - 1) << CAN_BTR_TS1_Pos) // #TQ in BS1 = PropagationSegment + PhaseSegment1
  |
    ((inSettings.mPhaseSegment2 - 1)  << CAN_BTR_TS2_Pos) // #TQ in BS2 = PhaseSegment2
  |
    ((inSettings.mRJW - 1)  << CAN_BTR_SJW_Pos) // mRJW tq to jump for resync.
  ;
//--- Mode
  switch (inSettings.mModuleMode) {
  case ACAN_STM32_Settings::NORMAL :
    break ;
  case ACAN_STM32_Settings::INTERNAL_LOOP_BACK :
    btr |= CAN_BTR_LBKM | CAN_BTR_SILM ;
    break ;
  case ACAN_STM32_Settings::EXTERNAL_LOOP_BACK :
    btr |= CAN_BTR_LBKM ;
    break ;
  case ACAN_STM32_Settings::SILENT :
    btr |= CAN_BTR_SILM ;
    break ;
  }
//--- Set BTR register
  mCAN->BTR = btr ;

//---------------------------------------------- Setup filters
//--- Start filter config
  mCAN->FMR = CAN_FMR_FINIT ;
  mCAN->FA1R = 0 ; // All filters inactive (put filter inactive for configuration)
  mCAN->FS1R = 0 ; // filter 0 scale config on single 32-bit
  mCAN->FM1R = 0 ; // reset to zero filter mode bits
  mCAN->FFA1R = 0 ; // Assign all filters to FIFO 0
  mCAN->FA1R = 0 ;  // Filter 0 inactive
//--- If no filter is provided, configure for accepting all valid frames in FIFO0
  if (inFilters.count () == 0) {
    mCAN->FS1R = 1 ; // filter 0 scale config on single 32-bit
    mCAN->sFilterRegister [0].FR1 = 0 ; // identifier
    mCAN->sFilterRegister [0].FR2 = 0 ; // mask, accept any valid frame
    mCAN->FA1R = 1 ;  // Filter 0 active
  }else{
  //--- Setup filters
    for (uint32_t i = 0 ; i < inFilters.count () ; i++) {
      mCAN->sFilterRegister [i].FR1 = inFilters.fr1AtIndex (i) ;
      mCAN->sFilterRegister [i].FR2 = inFilters.fr2AtIndex (i) ;
    }
    mCAN->FM1R = inFilters.fm1r () ;
    mCAN->FS1R = inFilters.fs1r () ;
    mCAN->FFA1R = inFilters.ffa1r () ;
    mCAN->FA1R |= (1U << inFilters.count ()) - 1 ;
  }
//--- End filter config
  mCAN->FMR = 0 ;

//---------------------------------------------- Setup Interrupts
//Rx interrupt on FIFO0
  uint8_t ier = CAN_IER_FMPIE0;  //FIFO 0 message pending interrupt enable
  ier |= CAN_IER_FFIE0;   //FIFO 0 full interrupt enable
  ier |= CAN_IER_FOVIE0;  //FIFO 0 overrun interrupt enable
//Rx interrupt on FIFO1
  ier |= CAN_IER_FMPIE1;  //FIFO 1 message pending interrupt enable
  ier |= CAN_IER_FFIE1;   //FIFO 1 full interrupt enable
  ier |= CAN_IER_FOVIE1;  //FIFO 1 overrun interrupt enable
//Tx interrupt on transmision
  ier |= CAN_IER_TMEIE;  //Transmit mailbox empty interrupt enable
  mCAN->IER = ier ;

//---------------------------------------------- Leave init mode
//ok, now leave init mode (and remove the NART bit).
//We just set ABOM (Automatic Bus Off recovery, in case of big pb…)
  uint32_t mcr = CAN_MCR_ABOM ;
  if (inSettings.mTransmitPriority == ACAN_STM32_Settings::BY_REQUEST_ORDER) {
    mcr |= CAN_MCR_TXFP ;
  }
  mCAN->MCR = mcr ;
  while ((mCAN->MCR & CAN_MCR_INRQ) != 0) {} // Wait until it is ok.
  while ((mCAN->TSR & CAN_TSR_TME0) == 0) {} // Wait until Transmit box is empty.

//---------------------------------------------- Enable interrupts
  if (errorCode == 0) {
//  NVIC_SetPriority (m_RX0_IRQn, inSettings.mMessageIRQPriority);
    NVIC_EnableIRQ (m_RX0_IRQn) ;
//  NVIC_SetPriority (m_RX1_IRQn, inSettings.mMessageIRQPriority);
    NVIC_EnableIRQ (m_RX1_IRQn) ;
//  NVIC_SetPriority (m_TX_IRQn, inSettings.mMessageIRQPriority);
    NVIC_EnableIRQ (m_TX_IRQn) ;
  }

//---------------------------------------------- Return
   return errorCode ;
}

//----------------------------------------------------------------------------------------------------------------------
//   RECEPTION
//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::available0 (void) const {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO0.count () > 0 ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::receive0 (CANMessage & outMessage) {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO0.remove (outMessage) ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::available1 (void) const {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO1.count () > 0 ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::receive1 (CANMessage & outMessage) {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO1.remove (outMessage) ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::internalDispatchReceivedMessage (const CANMessage & inMessage,
                                                  const DynamicArray < ACANCallBackRoutine > & inCallBackArray) {
  const uint32_t filterIndex = inMessage.idx ;
  if (filterIndex < inCallBackArray.count ()) {
    ACANCallBackRoutine callBack = inCallBackArray [filterIndex] ;
    if (nullptr != callBack) {
      callBack (inMessage) ;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::dispatchReceivedMessage (void) {
  CANMessage receivedMessage ;
  bool hasReceived = false ;
  if (receive0 (receivedMessage)) {
    internalDispatchReceivedMessage (receivedMessage, mFIFO0CallBackArray) ;
    hasReceived = true ;
  }
  if (receive1 (receivedMessage)) {
    internalDispatchReceivedMessage (receivedMessage, mFIFO1CallBackArray) ;
    hasReceived = true ;
  }
  return hasReceived ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::dispatchReceivedMessage0 (void) {
  CANMessage receivedMessage ;
  const bool hasReceived = receive0 (receivedMessage) ;
  if (hasReceived) {
    internalDispatchReceivedMessage (receivedMessage, mFIFO0CallBackArray) ;
  }
  return hasReceived ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::dispatchReceivedMessage1 (void) {
  CANMessage receivedMessage ;
  const bool hasReceived = receive1 (receivedMessage) ;
  if (hasReceived) {
    internalDispatchReceivedMessage (receivedMessage, mFIFO1CallBackArray) ;
  }
  return hasReceived ;
}

//----------------------------------------------------------------------------------------------------------------------
//   EMISSION
//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::sendBufferNotFullForIndex (const uint32_t inBufferIndex) {
  bool ok = false ;
  if (inBufferIndex == 0) {
    ok = !mDriverTransmitFIFO.isFull () ;
  }else if (inBufferIndex == 1) {
    ok = (mCAN->TSR & CAN_TSR_TME1) != 0 ;
  }else if (inBufferIndex == 2) {
    ok = (mCAN->TSR & CAN_TSR_TME2) != 0 ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32::tryToSendReturnStatus (const CANMessage & inMessage) {
  uint32_t sendStatus = 0 ; // Means ok

  noInterrupts () ;
  switch (inMessage.idx) {
  case 0 : // FIFO
    if (mDriverTransmitFIFO.isEmpty () && ((mCAN->TSR & CAN_TSR_TME0) != 0)) {
      writeTxRegisters (inMessage, 0) ;
    }else{
      const bool ok = mDriverTransmitFIFO.append (inMessage) ;
      if (!ok) {
        sendStatus = kTransmitBufferOverflow ;
      }
    }
    break ;
  case 1 : // Mailbox 1
    if ((mCAN->TSR & CAN_TSR_TME1) != 0) {
      writeTxRegisters (inMessage, 1) ;
    }else{
      sendStatus = kTransmitBufferOverflow ;
    }
    break ;
  case 2 : // Mailbox 2
    if ((mCAN->TSR & CAN_TSR_TME2) != 0) {
      writeTxRegisters (inMessage, 2) ;
    }else{
      sendStatus = kTransmitBufferOverflow ;
    }
    break ;
  default :
    sendStatus = kTransmitBufferIndexTooLarge ;
    break ;
  }
  interrupts () ;
  return sendStatus ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::writeTxRegisters (const CANMessage & inMessage, const uint32_t inBufferIndex) {
//--- Write rtr, ext, identifier
  if (inMessage.ext) {
    mCAN->sTxMailBox [inBufferIndex].TIR = (inMessage.rtr << 1) | (inMessage.ext << 2) | (inMessage.id << 3) ;
  }else{
    mCAN->sTxMailBox [inBufferIndex].TIR = (inMessage.rtr << 1) | (inMessage.ext << 2) | (inMessage.id << 21) ;
  }

//--- Write length
  mCAN->sTxMailBox [inBufferIndex].TDTR = inMessage.len & 0xF ;

//---  Write data
  mCAN->sTxMailBox [inBufferIndex].TDLR = inMessage.data32 [0] ;
  mCAN->sTxMailBox [inBufferIndex].TDHR = inMessage.data32 [1] ;

//--- Set TXRQ to request the transmission for the corresponding mailbox
  mCAN->sTxMailBox [inBufferIndex].TIR |= 1 ;
}

//----------------------------------------------------------------------------------------------------------------------
//   MESSAGE INTERRUPT SERVICE ROUTINES
//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::message_isr_rx0 (void) {
  if ((mCAN->RF0R & 0x3) != 0) { //case 1: FIFO 0 message pending
    CANMessage message ;
    const uint32_t rir  = mCAN->sFIFOMailBox [0].RIR ;
    const uint32_t rdtr = mCAN->sFIFOMailBox [0].RDTR ;
  //-- Get rtr, ext, len, identifier
    message.rtr = ((rir >> 1) & 0x1) != 0 ;
    message.ext = ((rir >> 2) & 0x1) != 0 ;
    message.len = rdtr & 0x0F ;
    if (message.ext) { // extended message
      message.id = (rir >> 3) & 0x1FFFFFFF ;
    }else{ //standard message
      message.id = (rir >> 21) & 0x7FF ;
    }
  //-- Get data
    message.data32 [0] = mCAN->sFIFOMailBox [0].RDLR ;
    message.data32 [1] = mCAN->sFIFOMailBox [0].RDHR ;
  //-- Get filter index
    message.idx = (rdtr >> 8) & 0xFF ;
  //-- Store the message
    mDriverReceiveFIFO0.append (message) ;
    //Ok message received: set RFR.RFOM bit to release FIFO
    //if fifo is empty, it acks the interrupt (i.e. FPM returns to 0).
    mCAN->RF0R = CAN_RF0R_RFOM0 ;
  }

  //case 2: FIFO 0 full
  if ((mCAN->RF0R & CAN_RF0R_FULL0) >> CAN_RF0R_FULL0_Pos == 1){
    mCAN->RF0R &= ~CAN_RF0R_FULL0;
  }

  //case 3: FIFO 0 overrun
  if ((mCAN->RF0R & CAN_RF0R_FOVR0) >> CAN_RF0R_FOVR0_Pos == 1){
    mCAN->RF0R &= ~CAN_RF0R_FOVR0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::message_isr_rx1 (void) {
//case 1: FIFO 1 message pending
  if ((mCAN->RF1R & 0x3) != 0) {
    CANMessage message ;
    const uint32_t rir  = mCAN->sFIFOMailBox [1].RIR ;
    const uint32_t rdtr = mCAN->sFIFOMailBox [1].RDTR ;
  //-- Get rtr, ext, len, identifier
    message.rtr = ((rir >> 1) & 0x1) != 0 ;
    message.ext = ((rir >> 2) & 0x1) != 0 ;
    message.len = rdtr & 0x0F ;
    if (message.ext) { // extended message
      message.id = (rir >> 3) & 0x1FFFFFFF ;
    }else{ //standard message
      message.id = (rir >> 21) & 0x7FF ;
    }
  //-- Get data
    message.data32 [0] = mCAN->sFIFOMailBox [1].RDLR ;
    message.data32 [1] = mCAN->sFIFOMailBox [1].RDHR ;
 //-- Get filter index
    message.idx = (rdtr >> 8) & 0xFF ;
  //-- Store the message
    mDriverReceiveFIFO1.append (message) ;
  //Ok message received: set RFR.RFOM bit to release FIFO
  //if fifo is empty, it acks the interrupt (i.e. FPM returns to 0).
    mCAN->RF1R = CAN_RF1R_RFOM1 ;
  }

  //case 2: FIFO 1 full
  if ((mCAN->RF1R & CAN_RF1R_FULL1) >> CAN_RF1R_FULL1_Pos == 1){
    mCAN->RF1R &= ~CAN_RF1R_FULL1;
  }

  //case 3: FIFO 1 overrun
  if ((mCAN->RF1R & CAN_RF1R_FOVR1) >> CAN_RF1R_FOVR1_Pos == 1){
    mCAN->RF1R &= ~CAN_RF1R_FOVR1;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN_STM32::message_isr_tx (void) {
  //interrupt acks when a message has been succesfully transmitted
  //check if there is a message in the transmit buffer
  if (mDriverTransmitFIFO.count () > 0) {

    //check if there is a transmit mailbox available
    if ((mCAN->TSR  & CAN_TSR_TME) != 0) {
      //there is at least one empty Tx buffer.
      //get one
      const int bufferId = (mCAN->TSR & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos ;
      //and set the message.
      CANMessage message ;
      mDriverTransmitFIFO.remove (message) ;
      writeTxRegisters (message, bufferId);
    }
  }

//interrput handled: set bit TSR.RQCPx bits to reinitialize interrupts
  mCAN->TSR |= CAN_TSR_RQCP0 ; //mailbox 0
}

//----------------------------------------------------------------------------------------------------------------------
//   FILTERS
//----------------------------------------------------------------------------------------------------------------------

static const uint32_t EXTENDED_IDENTIFIER_MAX = 0x1FFFFFFF ;

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addStandardMasks (const uint16_t inBase1,
                                            const uint16_t inMask1,
                                            const Format inFormat1,
                                            const uint16_t inBase2,
                                            const uint16_t inMask2,
                                            const Format inFormat2,
                                            const ACAN_STM32::Action inAction) {
  return addStandardMasks (inBase1, inMask1, inFormat1, nullptr,
                           inBase2, inMask2, inFormat2, nullptr,
                           inAction) ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addStandardMasks (const uint16_t inBase1,
                                            const uint16_t inMask1,
                                            const Format inFormat1,
                                            const ACANCallBackRoutine inCallBack1,
                                            const uint16_t inBase2,
                                            const uint16_t inMask2,
                                            const Format inFormat2,
                                            const ACANCallBackRoutine inCallBack2,
                                            const ACAN_STM32::Action inAction) {
  const uint32_t n = mFR1Array.count () ;
  const bool ok = (inBase1 <= 0x7FF)
               && (inMask1 <= 0x7FF)
               && ((inBase1 & inMask1) == inBase1)
               && (inBase2 <= 0x7FF)
               && (inMask2 <= 0x7FF)
               && ((inBase2 & inMask2) == inBase2)
               && (n < 14) ;
  if (ok) {
    switch (inAction) {
    case ACAN_STM32::FIFO0 :
      mFIFO0CallBackArray.append (inCallBack1) ;
      mFIFO0CallBackArray.append (inCallBack2) ;
      break ;
    case ACAN_STM32::FIFO1 :
      mFFA1R |= (1U << n) ;
      mFIFO1CallBackArray.append (inCallBack1) ;
      mFIFO1CallBackArray.append (inCallBack2) ;
      break ;
    }
    uint32_t fr1 = (uint32_t (inMask1) << 21) | (uint32_t (inBase1) << 5) | (1 << 20) ;
    switch (inFormat1) {
    case DATA :
      fr1 |= (1 << 20) ;
      break ;
    case REMOTE :
      fr1 |= (1 << 20) | (1 << 4) ;
      break ;
    case DATA_OR_REMOTE :
      break ;
    }
    mFR1Array.append (fr1) ;
    uint32_t fr2 = (uint32_t (inMask2) << 21) | (uint32_t (inBase2) << 5) | (1 << 20) ;
    switch (inFormat2) {
    case DATA :
      fr2 |= (1 << 20) ;
      break ;
    case REMOTE :
      fr2 |= (1 << 20) | (1 << 4) ;
      break ;
    case DATA_OR_REMOTE :
      break ;
    }
    mFR2Array.append (fr2) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addStandardQuad (const uint16_t inIdentifier1,
                                           const bool inRTR1,
                                           const uint16_t inIdentifier2,
                                           const bool inRTR2,
                                           const uint16_t inIdentifier3,
                                           const bool inRTR3,
                                           const uint16_t inIdentifier4,
                                           const bool inRTR4,
                                           const ACAN_STM32::Action inAction) {
  return addStandardQuad (inIdentifier1, inRTR1, nullptr,
                          inIdentifier2, inRTR2, nullptr,
                          inIdentifier3, inRTR3, nullptr,
                          inIdentifier4, inRTR4, nullptr,
                          inAction) ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addStandardQuad (const uint16_t inIdentifier1,
                                           const bool inRTR1,
                                           const ACANCallBackRoutine inCallBack1,
                                           const uint16_t inIdentifier2,
                                           const bool inRTR2,
                                           const ACANCallBackRoutine inCallBack2,
                                           const uint16_t inIdentifier3,
                                           const bool inRTR3,
                                           const ACANCallBackRoutine inCallBack3,
                                           const uint16_t inIdentifier4,
                                           const bool inRTR4,
                                           const ACANCallBackRoutine inCallBack4,
                                           const ACAN_STM32::Action inAction) {
  const uint32_t n = mFR1Array.count () ;
  const bool ok = (inIdentifier1 <= 0x7FF)
               && (inIdentifier2 <= 0x7FF)
               && (inIdentifier3 <= 0x7FF)
               && (inIdentifier4 <= 0x7FF)
               && (n < 14) ;
  if (ok) {
    mFM1R |= (1U << n) ; // Identifier list mode
    switch (inAction) {
    case ACAN_STM32::FIFO0 :
      mFIFO0CallBackArray.append (inCallBack1) ;
      mFIFO0CallBackArray.append (inCallBack2) ;
      mFIFO0CallBackArray.append (inCallBack3) ;
      mFIFO0CallBackArray.append (inCallBack4) ;
      break ;
    case ACAN_STM32::FIFO1 :
      mFFA1R |= (1U << n) ;
      mFIFO1CallBackArray.append (inCallBack1) ;
      mFIFO1CallBackArray.append (inCallBack2) ;
      mFIFO1CallBackArray.append (inCallBack3) ;
      mFIFO1CallBackArray.append (inCallBack4) ;
      break ;
    }
    uint32_t fr1 = (uint32_t (inIdentifier2) << 21) | (uint32_t (inIdentifier1) << 5) ;
    if (inRTR2) {
      fr1 |= (1 << 20) ;
    }
    if (inRTR1) {
      fr1 |= (1 << 4) ;
    }
    mFR1Array.append (fr1) ;
    uint32_t fr2 = (uint32_t (inIdentifier4) << 21) | (uint32_t (inIdentifier3) << 5) ;
    if (inRTR4) {
      fr2 |= (1 << 20) ;
    }
    if (inRTR3) {
      fr2 |= (1 << 4) ;
    }
    mFR2Array.append (fr2) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addExtendedMask (const uint32_t inBase,
                                           const uint32_t inMask,
                                           const Format inFormat,
                                           const ACAN_STM32::Action inAction) {
  return addExtendedMask (inBase, inMask, inFormat, nullptr, inAction);
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addExtendedMask (const uint32_t inBase,
                                           const uint32_t inMask,
                                           const Format inFormat,
                                           const ACANCallBackRoutine inCallBack,
                                           const ACAN_STM32::Action inAction) {
  const uint32_t n = mFR1Array.count () ;
  const bool ok = (inBase <= EXTENDED_IDENTIFIER_MAX)
               && (inMask <= EXTENDED_IDENTIFIER_MAX)
               && ((inBase & inMask) == inBase)
               && (n < 14) ;
  if (ok) {
    mFS1R |= (1U << n) ; // Single 32-bit scale
    switch (inAction) {
    case ACAN_STM32::FIFO0 :
      mFIFO0CallBackArray.append (inCallBack) ;
      break ;
    case ACAN_STM32::FIFO1 :
      mFFA1R |= (1U << n) ;
      mFIFO1CallBackArray.append (inCallBack) ;
      break ;
    }
    uint32_t fr1 = (inBase << 3) | (1 << 2) ;
    uint32_t fr2 = (inMask << 3) | (1 << 2) ;
    switch (inFormat) {
    case DATA :
      fr2 |= (1 << 1) ;
      break ;
    case REMOTE :
      fr1 |= (1 << 1) ;
      fr2 |= (1 << 1) ;
      break ;
    case DATA_OR_REMOTE :
      break ;
    }
    mFR1Array.append (fr1) ;
    mFR2Array.append (fr2) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addExtendedDual (const uint32_t inIdentifier1,
                                           const bool inRTR1,
                                           const uint32_t inIdentifier2,
                                           const bool inRTR2,
                                           const ACAN_STM32::Action inAction) {
  return addExtendedDual (inIdentifier1, inRTR1, nullptr,
                          inIdentifier2, inRTR2, nullptr,
                          inAction) ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32::Filters::addExtendedDual (const uint32_t inIdentifier1,
                                           const bool inRTR1,
                                           const ACANCallBackRoutine inCallBack1,
                                           const uint32_t inIdentifier2,
                                           const bool inRTR2,
                                           const ACANCallBackRoutine inCallBack2,
                                           const ACAN_STM32::Action inAction) {
  const uint32_t n = mFR1Array.count () ;
  const bool ok = (inIdentifier1 <= EXTENDED_IDENTIFIER_MAX)
               && (inIdentifier2 <= EXTENDED_IDENTIFIER_MAX)
               && (n < 14) ;
  if (ok) {
    mFM1R |= (1U << n) ; // Identifier list mode
    mFS1R |= (1U << n) ; // Single 32-bit scale
    switch (inAction) {
    case ACAN_STM32::FIFO0 :
      mFIFO0CallBackArray.append (inCallBack1) ;
      mFIFO0CallBackArray.append (inCallBack2) ;
      break ;
    case ACAN_STM32::FIFO1 :
      mFFA1R |= (1U << n) ;
      mFIFO1CallBackArray.append (inCallBack1) ;
      mFIFO1CallBackArray.append (inCallBack2) ;
      break ;
    }
    uint32_t fr1 = (inIdentifier1 << 3) | (1 << 2) ;
    if (inRTR1) {
      fr1 |= (1 << 1) ;
    }
    mFR1Array.append (fr1) ;
    uint32_t fr2 = (inIdentifier2 << 3) | (1 << 2) ;
    if (inRTR2) {
      fr2 |= (1 << 1) ;
    }
    mFR2Array.append (fr2) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------
