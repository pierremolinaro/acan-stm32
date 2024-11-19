#include <ACAN_STM32.h>

//------------------------------------------------------------------------------
//  STM32L432
//    CAN_RX : PA11 == D10
//    CAN_TX : PA12 == D2
//------------------------------------------------------------------------------

#ifdef STM32L4xx

//------------------------------------------------------------------------------

ACAN_STM32 can (
  & (RCC->APB1ENR1), // Enable CAN Clock Register address
  RCC_APB1ENR1_CAN1EN_Pos, // Enable CAN Clock bit offset in Enable CAN Clock Register
  & (RCC->APB1RSTR1), // Reset CAN peripheral Register address
  RCC_APB1RSTR1_CAN1RST_Pos, // Reset CAN Clock bit offset in Reset CAN peripheral Register
  CAN1, // CAN Peripheral base address
  CAN1_TX_IRQn,  // Transmit interrupt
  CAN1_RX0_IRQn, // RX0 receive interrupt
  CAN1_RX1_IRQn, // RX1 receive interrupt
  GPIOA, 12, 9, // Tx Pin, AF9
  GPIOA, 11, 9  // Rx Pin, AF9
) ;

//------------------------------------------------------------------------------

extern "C" void CAN1_RX0_IRQHandler (void) ;
extern "C" void CAN1_RX1_IRQHandler (void) ;
extern "C" void CAN1_TX_IRQHandler (void) ;

//------------------------------------------------------------------------------

void CAN1_RX0_IRQHandler (void) {
  can.message_isr_rx0 () ;
}

//------------------------------------------------------------------------------

void CAN1_RX1_IRQHandler (void){
  can.message_isr_rx1 () ;
}

//------------------------------------------------------------------------------

void CAN1_TX_IRQHandler (void){
  can.message_isr_tx () ;
}

//------------------------------------------------------------------------------

void ACAN_STM32::configureTxPin (const bool inOpenCollector) {
  const uint32_t txPinMask = 1U << mTxPinIndex ;
  LL_GPIO_SetPinMode  (mTxPinGPIO, txPinMask, LL_GPIO_MODE_ALTERNATE) ;
  LL_GPIO_SetPinOutputType (mTxPinGPIO, txPinMask, inOpenCollector ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL) ;
  LL_GPIO_SetPinSpeed (mTxPinGPIO, txPinMask, LL_GPIO_SPEED_FREQ_HIGH) ;
  if (mTxPinIndex < 8) {
    LL_GPIO_SetAFPin_0_7 (mTxPinGPIO, txPinMask, mTxPinAlternateMode) ;
  }else{
    LL_GPIO_SetAFPin_8_15 (mTxPinGPIO, txPinMask, mTxPinAlternateMode) ;
  }
}

//------------------------------------------------------------------------------

void ACAN_STM32::configureRxPin (void) {
  const uint32_t rxPinMask = 1U << mRxPinIndex ;
  LL_GPIO_SetPinMode  (mRxPinGPIO, rxPinMask, LL_GPIO_MODE_ALTERNATE) ;
  if (mRxPinIndex < 8) {
    LL_GPIO_SetAFPin_0_7 (mRxPinGPIO, rxPinMask, mRxPinAlternateMode) ;
  }else{
    LL_GPIO_SetAFPin_8_15 (mRxPinGPIO, rxPinMask, mRxPinAlternateMode) ;
  }
}

//------------------------------------------------------------------------------

#endif

//------------------------------------------------------------------------------
