#pragma once

//----------------------------------------------------------------------------------------

#include <Arduino.h>

//----------------------------------------------------------------------------------------

#if !defined (STM32L4xx) && !defined(STM32F303x8) && !defined(STM32F103xB)
  #error "Unhandled STM32 Board"
#endif

//----------------------------------------------------------------------------------------

class ACAN_STM32_Settings {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Enumerations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum {
    NORMAL,
    INTERNAL_LOOP_BACK,
    EXTERNAL_LOOP_BACK,
    SILENT
  } ModuleMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum {
    BY_IDENTIFIER,
    BY_REQUEST_ORDER
  } TransmitPriority ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//--- Constructor for a given baud rate
  public: explicit ACAN_STM32_Settings (const uint32_t inWhishedBitRate,
                                        const uint32_t inTolerancePPM = 1000) ;

//--- CAN bit timing (default values correspond to 250 kb/s)
  public: uint32_t mWhishedBitRate ; // In kb/s
  public: uint16_t mBitRatePrescaler = 1024 ; // 1...1024
  public: uint8_t mPhaseSegment1 = 16 ; // 1...16
  public: uint8_t mPhaseSegment2 = 8 ;  // 1...8
  public: uint8_t mRJW = 4 ; // 1...4
  public: bool mTripleSampling = true ;
  public: bool mBitRateClosedToDesiredRate = false ;

//--- Silent mode
  public: ModuleMode mModuleMode = NORMAL ;

//--- Open collector output
  public: bool mOpenCollectorOutput = false ; // true --> open collector, false --> push / pull

//--- Transmit Priority
  public: TransmitPriority mTransmitPriority = BY_IDENTIFIER ;

//--- Receive FIFO sizes
  public: uint16_t mDriverReceiveFIFO0Size = 32 ;
  public: uint16_t mDriverReceiveFIFO1Size = 0 ;

//--- Transmit buffer size
  public: uint16_t mDriverTransmitFIFOSize = 16 ;

//--- Compute actual bit rate
  public: uint32_t actualBitRate (void) const ;

//--- Exact bit rate ?
  public: bool exactBitRate (void) const ;

//--- Distance between actual bit rate and requested bit rate (in ppm, part-per-million)
  public: uint32_t ppmFromWishedBitRate (void) const ;

//--- Distance of sample point from bit start (in ppc, part-per-cent, denoted by %)
  public: uint32_t samplePointFromBitStart (void) const ;

//--- Bit settings are consistent ? (returns 0 if ok)
  public: uint32_t CANBitSettingConsistency (void) const ;

//--- Constants returned by CANBitSettingConsistency
  public: static const uint32_t kBitRatePrescalerIsZero            = 1 <<  0 ;
  public: static const uint32_t kBitRatePrescalerIsGreaterThan1024 = 1 <<  1 ;
  public: static const uint32_t kPhaseSegment1IsZero               = 1 <<  2 ;
  public: static const uint32_t kPhaseSegment1IsGreaterThan16      = 1 <<  3 ;
  public: static const uint32_t kPhaseSegment2IsZero               = 1 <<  4 ;
  public: static const uint32_t kPhaseSegment2IsGreaterThan8       = 1 <<  5 ;
  public: static const uint32_t kRJWIsZero                         = 1 <<  6 ;
  public: static const uint32_t kRJWIsGreaterThan4                 = 1 <<  7 ;
  public: static const uint32_t kRJWIsGreaterThanPhaseSegment2     = 1 <<  8 ;
  public: static const uint32_t kPhaseSegment1Is1AndTripleSampling = 1 <<  9 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//----------------------------------------------------------------------------------------
