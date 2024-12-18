//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------

#include <ACAN_STM32_CANMessage.h>

//------------------------------------------------------------------------------

class ACAN_STM32_FIFO {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Default constructor
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: ACAN_STM32_FIFO (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Destructor
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: ~ ACAN_STM32_FIFO (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private properties
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: CANMessage * mBuffer ;
  private: uint16_t mSize ;
  private: uint16_t mReadIndex ;
  private: uint16_t mCount ;
  private: uint16_t mPeakCount ; // > mSize if overflow did occur

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline uint16_t size (void) const { return mSize ; }
  public: inline uint16_t count (void) const { return mCount ; }
  public: inline bool isEmpty (void) const { return mCount == 0 ; }
  public: inline bool isFull (void) const { return mCount == mSize ; }
  public: inline uint16_t peakCount (void) const { return mPeakCount ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // initWithSize
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void initWithSize (const uint16_t inSize) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // append
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool append (const CANMessage & inMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Remove
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool remove (CANMessage & outMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Free
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void free (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Reset Peak Count
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline void resetPeakCount (void) { mPeakCount = mCount ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // No copy
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: ACAN_STM32_FIFO (const ACAN_STM32_FIFO &) = delete ;
  private: ACAN_STM32_FIFO & operator = (const ACAN_STM32_FIFO &) = delete ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//------------------------------------------------------------------------------
