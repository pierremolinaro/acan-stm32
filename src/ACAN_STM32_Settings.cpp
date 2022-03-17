#include <ACAN_STM32_Settings.h>

//----------------------------------------------------------------------------------------------------------------------
//    CAN Settings
//----------------------------------------------------------------------------------------------------------------------

ACAN_STM32_Settings::ACAN_STM32_Settings (const uint32_t inWhishedBitRate,
                                          const uint32_t inTolerancePPM) :
mWhishedBitRate (inWhishedBitRate) {
  uint32_t TQCount = 25 ; // TQCount: 5 ... 25
  uint32_t smallestError = UINT32_MAX ;
  uint32_t bestBRP = 1024 ; // Setting for slowest bit rate
  uint32_t bestTQCount = 25 ; // Setting for slowest bit rate
  uint32_t BRP = CAN_CLOCK_FREQUENCY / inWhishedBitRate / TQCount ;

//--- Loop for finding best BRP and best TQCount
  while ((TQCount >= 5) && (BRP <= 1024)) {
  //--- Compute error using BRP (caution: BRP should be > 0)
    if (BRP > 0) {
      const uint32_t error = CAN_CLOCK_FREQUENCY - inWhishedBitRate * TQCount * BRP ; // error is always >= 0
      if (error < smallestError) {
        smallestError = error ;
        bestBRP = BRP ;
        bestTQCount = TQCount ;
      }
    }
  //--- Compute error using BRP+1 (caution: BRP+1 should be <= 1024)
    if (BRP < 1024) {
      const uint32_t error = inWhishedBitRate * TQCount * (BRP + 1) - CAN_CLOCK_FREQUENCY ; // error is always >= 0
      if (error < smallestError) {
        smallestError = error ;
        bestBRP = BRP + 1 ;
        bestTQCount = TQCount ;
      }
    }
  //--- Continue with next value of TQCount
    TQCount -- ;
    BRP = CAN_CLOCK_FREQUENCY / inWhishedBitRate / TQCount ;
  }
//--- Set the BRP
  mBitRatePrescaler = uint16_t (bestBRP) ;

//--- Compute PS2
  const uint32_t PS2 = 1 + 2 * bestTQCount / 7 ; // Always 2 <= PS2 <= 8
  mPhaseSegment2 = uint8_t (PS2) ;

//--- Compute the remaining number of TQ once PS2 and SyncSeg are removed
  const uint32_t propSegmentPlusPhaseSegment1 = bestTQCount - PS2 - 1 /* Sync Seg */ ;

//--- Set PS1 to half of remaining TQCount
  mPhaseSegment1 = uint8_t (propSegmentPlusPhaseSegment1) ; // Always 1 <= PS1 <= 16

//--- Set RJW to PS2, with a maximum value of 4
  mRJW = (mPhaseSegment2 >= 4) ? 4 : mPhaseSegment2 ; // Always 2 <= RJW <= 4, and RJW <= mPhaseSegment2

//--- Triple Sampling
  mTripleSampling = (inWhishedBitRate <= 125000) && (mPhaseSegment1 > 1) ;

//--- Final check of the configuration
  const uint32_t W = bestTQCount * mWhishedBitRate * mBitRatePrescaler ;
  const uint64_t diff = (CAN_CLOCK_FREQUENCY > W) ? (CAN_CLOCK_FREQUENCY - W) : (W - CAN_CLOCK_FREQUENCY) ;
  const uint64_t ppm = uint64_t (1000 * 1000) ;
  mBitRateClosedToDesiredRate = (diff * ppm) <= (uint64_t (W) * inTolerancePPM) ;
} ;

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32_Settings::actualBitRate (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mPhaseSegment1 + mPhaseSegment2 ;
  return CAN_CLOCK_FREQUENCY / mBitRatePrescaler / TQCount ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN_STM32_Settings::exactBitRate (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mPhaseSegment1 + mPhaseSegment2 ;
  return CAN_CLOCK_FREQUENCY == (mBitRatePrescaler * mWhishedBitRate * TQCount) ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32_Settings::ppmFromWishedBitRate (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mPhaseSegment1 + mPhaseSegment2 ;
  const uint32_t W = TQCount * mWhishedBitRate * mBitRatePrescaler ;
  const uint64_t diff = (CAN_CLOCK_FREQUENCY > W) ? (CAN_CLOCK_FREQUENCY - W) : (W - CAN_CLOCK_FREQUENCY) ;
  const uint64_t ppm = uint64_t (1000 * 1000) ;
  return (uint32_t) ((diff * ppm) / W) ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32_Settings::samplePointFromBitStart (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mPhaseSegment1 + mPhaseSegment2 ;
  const uint32_t samplePoint = 1 /* Sync Seg */ + mPhaseSegment1 ;
  const uint32_t partPerCent = 100 ;
  return (samplePoint * partPerCent) / TQCount ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN_STM32_Settings::CANBitSettingConsistency (void) const {
  uint32_t errorCode = 0 ; // Means no error
  if (mBitRatePrescaler == 0) {
    errorCode |= kBitRatePrescalerIsZero ;
  }else if (mBitRatePrescaler > 1024) {
    errorCode |= kBitRatePrescalerIsGreaterThan1024;
  }
  if (mPhaseSegment1 == 0) {
    errorCode |= kPhaseSegment1IsZero ;
  }else if (mPhaseSegment1 > 16) {
    errorCode |= kPhaseSegment1IsGreaterThan16 ;
  }
  if (mPhaseSegment2 == 0) {
    errorCode |= kPhaseSegment2IsZero ;
  }else if (mPhaseSegment2 > 8) {
    errorCode |= kPhaseSegment2IsGreaterThan8 ;
  }
  if (mRJW == 0) {
    errorCode |= kRJWIsZero ;
  }else if (mRJW > 4) {
    errorCode |= kRJWIsGreaterThan4 ;
  }
  if (mRJW > mPhaseSegment2) {
    errorCode |= kRJWIsGreaterThanPhaseSegment2 ;
  }
  if (mTripleSampling && ((mWhishedBitRate > 125000) || (mPhaseSegment1 == 1))) {
    errorCode |= kPhaseSegment1Is1AndTripleSampling ;
  }
  return errorCode ;
}

//----------------------------------------------------------------------------------------------------------------------
