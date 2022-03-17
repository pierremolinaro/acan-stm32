#pragma once

//----------------------------------------------------------------------------------------------------------------------

#include <ACAN_STM32_Settings.h>
#include <ACAN_STM32_FIFO.h>
#include <Arduino.h>

//----------------------------------------------------------------------------------------------------------------------

class ACAN_STM32 {

//--------------------------------------------------------------------------------------------------
//    Private Dynamic Array
//--------------------------------------------------------------------------------------------------

  private: template <typename T> class DynamicArray {
  //--- Default constructor
    public: DynamicArray (void) { }

  //--- Destructor
    public: ~ DynamicArray (void) { delete [] mArray ; }

  //--- Append
    public: void append (const T & inObject) {
      if (mCapacity == mCount) {
        mCapacity += 64 ;
        T * newArray = new T [mCapacity] ;
        for (uint32_t i=0 ; i<mCount ; i++) {
          newArray [i] = mArray [i] ;
        }
        delete [] mArray ;
        mArray = newArray ;
      }
      mArray [mCount] = inObject ;
      mCount += 1 ;
    }

  //--- Methods
    public: void free (void) {
      delete [] mArray ;
      mArray = nullptr ;
      mCount = 0 ;
      mCapacity = 0 ;
    }

    public: void setCapacity (const uint32_t inNewCapacity) {
      if (mCapacity < inNewCapacity) {
        mCapacity = inNewCapacity ;
        T * newArray = new T [mCapacity] ;
        for (uint32_t i=0 ; i<mCount ; i++) {
          newArray [i] = mArray [i] ;
        }
        delete [] mArray ;
        mArray = newArray ;
      }
    }

    public: void copyTo (DynamicArray <T> & outArray) const {
      outArray.free () ;
      if (count () > 0) {
        outArray.setCapacity (count ()) ;
        for (uint32_t i=0 ; i<count () ; i++) {
          outArray.append (mArray [i]) ;
        }
      }
    }

  //--- Access
    public: uint32_t count () const { return mCount ; }
    public: T operator [] (const uint32_t inIndex) const { return mArray [inIndex] ; }

  //--- Private properties
    private: uint8_t mCapacity = 0 ;
    private: uint8_t mCount = 0 ;
    private: T * mArray = nullptr ;

  //--- No copy
    private : DynamicArray (const DynamicArray &) = delete ;
    private : DynamicArray & operator = (const DynamicArray &) = delete ;
  } ;

//--------------------------------------------------------------------------------------------------
//    Filters
//··································································································

  public: typedef enum { FIFO0, FIFO1 } Action ;
  public: typedef enum { DATA, REMOTE, DATA_OR_REMOTE } Format ;

//--------------------------------------------------------------------------------------------------

  public: class Filters {
  //--- Default constructor
    public: Filters (void) { }

  //--- Append filter
    public: bool addStandardMasks (const uint16_t inBase1,
                                   const uint16_t inMask1,
                                   const Format inFormat1,
                                   const uint16_t inBase2,
                                   const uint16_t inMask2,
                                   const Format inFormat2,
                                   const ACAN_STM32::Action inAction) ;

    public: bool addStandardMasks (const uint16_t inBase1,
                                   const uint16_t inMask1,
                                   const Format inFormat1,
                                   const ACANCallBackRoutine inCallBack1,
                                   const uint16_t inBase2,
                                   const uint16_t inMask2,
                                   const Format inFormat2,
                                   const ACANCallBackRoutine inCallBack2,
                                   const ACAN_STM32::Action inAction) ;

    public: bool addExtendedMask (const uint32_t inBase,
                                  const uint32_t inMask,
                                  const Format inFormat,
                                  const ACAN_STM32::Action inAction) ;

    public: bool addExtendedMask (const uint32_t inBase,
                                  const uint32_t inMask,
                                  const Format inFormat,
                                  const ACANCallBackRoutine inCallBack,
                                  const ACAN_STM32::Action inAction) ;

    public: bool addExtendedDual (const uint32_t inIdentifier1,
                                  const bool inRTR1,
                                  const uint32_t inIdentifier2,
                                  const bool inRTR2,
                                  const ACAN_STM32::Action inAction) ;

    public: bool addExtendedDual (const uint32_t inIdentifier1,
                                  const bool inRTR1,
                                  const ACANCallBackRoutine inCallBack1,
                                  const uint32_t inIdentifier2,
                                  const bool inRTR2,
                                  const ACANCallBackRoutine inCallBack2,
                                  const ACAN_STM32::Action inAction) ;

    public: bool addStandardQuad (const uint16_t inIdentifier1,
                                  const bool inRTR1,
                                  const uint16_t inIdentifier2,
                                  const bool inRTR2,
                                  const uint16_t inIdentifier3,
                                  const bool inRTR3,
                                  const uint16_t inIdentifier4,
                                  const bool inRTR4,
                                  const ACAN_STM32::Action inAction) ;

    public: bool addStandardQuad (const uint16_t inIdentifier1,
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
                                  const ACAN_STM32::Action inAction) ;

  //--- Access
    public: uint32_t count () const { return mFR1Array.count () ; }
    public: uint32_t fr1AtIndex (const uint32_t inIndex) const { return mFR1Array [inIndex] ; }
    public: uint32_t fr2AtIndex (const uint32_t inIndex) const { return mFR2Array [inIndex] ; }
    public: uint32_t fm1r (void) const { return mFM1R ; }
    public: uint32_t fs1r (void) const { return mFS1R ; }
    public: uint32_t ffa1r (void) const { return mFFA1R ; }
    public: void copyFIFO0CallBackArrayTo (DynamicArray <ACANCallBackRoutine> & outArray) const {
      mFIFO0CallBackArray.copyTo (outArray) ;
    }
    public: void copyFIFO1CallBackArrayTo (DynamicArray <ACANCallBackRoutine> & outArray) const {
      mFIFO1CallBackArray.copyTo (outArray) ;
    }

  //--- Private properties
    private: DynamicArray <uint32_t> mFR1Array ;
    private: DynamicArray <uint32_t> mFR2Array ;
    private: uint16_t mFM1R = 0 ; // By default Mask mode
    private: uint16_t mFS1R = 0 ; // By default, dual 16-bit scale
    private: uint16_t mFFA1R = 0 ; // By default, filters assigned to FIFO 0
    private: DynamicArray < ACANCallBackRoutine > mFIFO0CallBackArray ;
    private: DynamicArray < ACANCallBackRoutine > mFIFO1CallBackArray ;

  //--- No copy
    private : Filters (const Filters &) = delete ;
    private : Filters & operator = (const Filters &) = delete ;
  } ;

//--------------------------------------------------------------------------------------------------

//--- Constructor
  public: ACAN_STM32 (volatile uint32_t * inClockEnableRegisterAddress,
                      const uint8_t inClockEnableBitOffset,
                      volatile uint32_t * inResetRegisterPointer,
                      const uint8_t inResetBitOffset,
                      volatile CAN_TypeDef * inPeripheralModuleBasePointer,
                      const IRQn_Type in_TX_IRQ,
                      const IRQn_Type in_RX0_IRQn,
                      const IRQn_Type in_RX1_IRQn,
                      GPIO_TypeDef * inTxPinGPIO,
                      const uint8_t inTxPinIndex,
                      const uint8_t inTxPinAlternateMode,
                      GPIO_TypeDef * inRxPinGPIO,
                      const uint8_t inRxPinIndex,
                      const uint8_t inRxPinAlternateMode) ;

//--- begin; returns a result code:
//  0 : Ok
//  other: every bit denotes an error
//  The error code are thoses returned by ACAN_STM32_Settings::CANBitSettingConsistency
//  and the following one
  public: static const uint32_t kActualBitRateTooFarFromDesiredBitRate = 1 << 16 ;

  public: uint32_t begin (const ACAN_STM32_Settings & inSettings,
                          const ACAN_STM32::Filters & inFilters = ACAN_STM32::Filters ()) ;

//--- end: stop CAN controller
  public: void end (void) ;

//--- Transmitting messages
  public: bool sendBufferNotFullForIndex (const uint32_t inBufferIndex) ;
  public: uint32_t tryToSendReturnStatus (const CANMessage & inMessage) ;
  public: static const uint32_t kTransmitBufferIndexTooLarge = 1 ;
  public: static const uint32_t kTransmitBufferOverflow      = 2 ;

  public: inline uint32_t driverTransmitFIFOSize (void) const { return mDriverTransmitFIFO.size () ; }
  public: inline uint32_t driverTransmitFIFOCount (void) const { return mDriverTransmitFIFO.count () ; }
  public: inline uint32_t driverTransmitFIFOPeakCount (void) const { return mDriverTransmitFIFO.peakCount () ; }

//--- Receiving messages
  public: bool available0 (void) const ;
  public: bool receive0 (CANMessage & outMessage) ;
  public: bool dispatchReceivedMessage0 (void) ;
  public: bool available1 (void) const ;
  public: bool receive1 (CANMessage & outMessage) ;
  public: bool dispatchReceivedMessage1 (void) ;
  public: bool dispatchReceivedMessage (void) ;

//--- Call back function array
  private: DynamicArray < ACANCallBackRoutine > mFIFO0CallBackArray ;
  private: DynamicArray < ACANCallBackRoutine > mFIFO1CallBackArray ;

//--- Driver receive Fifos
  private: ACAN_STM32_FIFO mDriverReceiveFIFO0 ;
  public: inline uint32_t driverReceiveFIFO0Size (void) const { return mDriverReceiveFIFO0.size () ; }
  public: inline uint32_t driverReceiveFIFO0Count (void) const { return mDriverReceiveFIFO0.count () ; }
  public: inline uint32_t driverReceiveFIFO0PeakCount (void) const { return mDriverReceiveFIFO0.peakCount () ; }

  private: ACAN_STM32_FIFO mDriverReceiveFIFO1 ;
  public: inline uint32_t driverReceiveFIFO1Size (void) const { return mDriverReceiveFIFO1.size () ; }
  public: inline uint32_t driverReceiveFIFO1Count (void) const { return mDriverReceiveFIFO1.count () ; }
  public: inline uint32_t driverReceiveFIFO1PeakCount (void) const { return mDriverReceiveFIFO1.peakCount () ; }

//--- Driver transmit buffer
  private: ACAN_STM32_FIFO mDriverTransmitFIFO ;
  private: void writeTxRegisters (const CANMessage & inMessage, const uint32_t inMBIndex) ;

//--- Message interrupt service routines
  public: void message_isr_rx0 (void) ; // interrupt on FIFO 0
  public: void message_isr_rx1 (void) ; // interrupt on FIFO 1
  public: void message_isr_tx (void) ;  // interrupt on transmission

//--- Private properties
  private: volatile uint32_t * mClockEnableRegisterPointer ;
  private: volatile uint32_t * mResetRegisterPointer ;
  private: volatile CAN_TypeDef * mCAN ;
  private: GPIO_TypeDef * mTxPinGPIO ;
  private: GPIO_TypeDef * mRxPinGPIO ;
  private: const uint8_t mClockEnableBitOffset ;
  private: const uint8_t mResetBitOffset ;
  private: const IRQn_Type m_TX_IRQn ;
  private: const IRQn_Type m_RX0_IRQn ;
  private: const IRQn_Type m_RX1_IRQn ;
  private: const uint8_t mTxPinIndex ;
  private: const uint8_t mTxPinAlternateMode ;
  private: const uint8_t mRxPinIndex ;
  private: const uint8_t mRxPinAlternateMode ;


//--- Private methods
  private: uint32_t internalBegin (const ACAN_STM32_Settings & inSettings,
                                   const ACAN_STM32::Filters & inFilters) ;
  private: void internalDispatchReceivedMessage (const CANMessage & inMessage,
                                                 const DynamicArray < ACANCallBackRoutine > & inCallBackArray) ;


 //--- No copy
  private : ACAN_STM32 (const ACAN_STM32 &) = delete ;
  private : ACAN_STM32 & operator = (const ACAN_STM32 &) = delete ;
} ;

//----------------------------------------------------------------------------------------------------------------------

#ifdef ARDUINO_NUCLEO_F303K8
  extern ACAN_STM32 can ;
#elif ARDUINO_NUCLEO_L432KC
  extern ACAN_STM32 can ;
#endif

//----------------------------------------------------------------------------------------------------------------------
