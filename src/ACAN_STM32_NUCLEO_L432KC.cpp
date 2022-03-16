#include <ACAN_STM32.h>

//----------------------------------------------------------------------------------------------------------------------
//  STM32L432
//    CAN_RX : PA11 == D10
//    CAN_TX : PA12 == D2
//----------------------------------------------------------------------------------------------------------------------

#ifdef ARDUINO_NUCLEO_L432KC

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

extern "C" void CAN1_RX0_IRQHandler (void) ;
extern "C" void CAN1_RX1_IRQHandler (void) ;
extern "C" void CAN1_TX_IRQHandler (void) ;

//----------------------------------------------------------------------------------------------------------------------

void CAN1_RX0_IRQHandler (void) {
  can.message_isr_rx0 () ;
}

//----------------------------------------------------------------------------------------------------------------------

void CAN1_RX1_IRQHandler (void){
  can.message_isr_rx1 () ;
}

//----------------------------------------------------------------------------------------------------------------------

void CAN1_TX_IRQHandler (void){
  can.message_isr_tx () ;
}

//----------------------------------------------------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------------------------------------------------
