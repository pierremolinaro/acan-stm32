#include <ACAN_STM32.h>

//----------------------------------------------------------------------------------------
//  STM32F303
//    CAN_RX : PA11 == D10
//    CAN_TX : PA12 == D2
//----------------------------------------------------------------------------------------
// extern const PinMap PinMap_CAN_RD[];
// extern const PinMap PinMap_CAN_TD[];
// HAL_CAN_MODULE_ENABLED

#if defined (ARDUINO_NUCLEO_F303K8) || defined (ARDUINO_GENERIC_F303K8TX)

//----------------------------------------------------------------------------------------

ACAN_STM32 can (
  & (RCC->APB1ENR), // Enable CAN Clock Register address
  RCC_APB1ENR_CANEN_Pos, // Enable CAN Clock bit offset in Enable CAN Clock Register
  & (RCC->APB1RSTR), // Reset CAN peripheral Register address
  RCC_APB1RSTR_CANRST_Pos, // Reset CAN Clock bit offset in Reset CAN peripheral Register
  CAN, // CAN Peripheral base address
  CAN_TX_IRQn,  // Transmit interrupt
  CAN_RX0_IRQn, // RX0 receive interrupt
  CAN_RX1_IRQn, // RX1 receive interrupt
  GPIOA, 12, 9, // Tx Pin, AF9
  GPIOA, 11, 9  // Rx Pin, AF9
) ;

//----------------------------------------------------------------------------------------

extern "C" void CAN_RX0_IRQHandler (void) ;
extern "C" void CAN_RX1_IRQHandler (void) ;
extern "C" void CAN_TX_IRQHandler (void) ;

//----------------------------------------------------------------------------------------

void CAN_RX0_IRQHandler (void) {
  can.message_isr_rx0 () ;
}

//----------------------------------------------------------------------------------------

void CAN_RX1_IRQHandler (void){
  can.message_isr_rx1 () ;
}

//----------------------------------------------------------------------------------------

void CAN_TX_IRQHandler (void){
  can.message_isr_tx () ;
}

//----------------------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------------------
