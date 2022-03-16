## CAN Library for STM32

### Supported boards

Currently, the following boards are supported: `NUCLEO-F303K8`, `NUCLEO-L432KC`.

### Compatibility with other ACAN libraries

This library is fully compatible with the MCP2515 CAN Controller ACAN2515, ACAN2515Tiny, ACAN2517 and ACAN2517FD libraries, it uses a very similar API and the same `CANMessage` class for handling messages.

### Demo sketchs

Four sketches are provided; the use the `EXTERNAL_LOOP_BACK` mode, no external hardware is required, and sent frames can be observed on `TxCAN` pin. They demonstrate frame sending and receiving capabilities:

* **LoopBackDemo** sketch, basic example for sending and receiving frames;
* **LoopBackDemoIntensive** sketch which sends random frames and checks reception;
* **LoopBackDemoFilters** sketch, basic example of reception filters;
* **LoopBackDemoDispatch** sketch, basic example for using callback functions associated with filters.


### ACAN_STM32 library description
ACAN_STM32 is a driver for the bxCAN module built into several STM32 microcontroller.

The driver supports many bit rates, as standard 62.5 kbit/s, 125 kbit/s, 250 kbit/s, 500 kbit/s, and 1 Mbit/s. An efficient CAN bit timing calculator finds settings for them, but also for exotic bit rates as 842 kbit/s. If the wished bit rate cannot be achieved, the `begin` method does not configure the hardware and returns an error code.

> Driver API is fully described by the PDF file in the `extras` directory.

### LoopBackDemo demo Sketch

Configuration is a four-step operation.

1. Instanciation of the `settings` object: the constructor has one parameter: the wished CAN bit rate. The `settings` is fully initialized.
2. You can override default settings. Here, we set the `mModuleMode` property to `ACAN_STM32_Settings::EXTERNAL_LOOP_BACK`, enabling to run demo code without any additional hardware (no CAN transceiver needed). We can also for example change the receive buffer size by setting the `mDriverReceiveFIFO0Size` property.
3. Calling the `begin` method configures the driver and starts CAN bus participation. Any message can be sent, any frame on the bus is received. No default filter to provide.
4. You check the `errorCode` value to detect configuration error(s).

```cpp
void setup () {
  pinMode (LED_BUILTIN, OUTPUT) ;
  Serial.begin (115200) ;
  while (!Serial) {
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    delay (50) ;
  }

  ACAN_STM32_Settings settings (125 * 1000) ; // 125 kbit/s
 
  settings.mModuleMode = ACAN_STM32_Settings::EXTERNAL_LOOP_BACK ;
  
  const uint32_t errorCode = can.begin (settings) ;
  if (0 == errorCode) {
    Serial.println ("can ok") ;
  }else{
    Serial.print ("Error can: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}
```

Now, an example of the `loop` function. As we have selected loop back mode, every sent frame is received.

```cpp
static unsigned gSendDate = 0 ;
static unsigned gSentCount = 0 ;
static unsigned gReceivedCount = 0 ;

void loop () {
  CANMessage message ;
  if (gSendDate < millis ()) {
    message.id = 0x542 ;
    message.len = 8 ;
    message.data [0] = 0 ;
    message.data [1] = 1 ;
    message.data [2] = 2 ;
    message.data [3] = 3 ;
    message.data [4] = 4 ;
    message.data [5] = 5 ;
    message.data [6] = 6 ;
    message.data [7] = 7 ;
    const bool ok = can.tryToSendReturnStatus (message) ;
    if (ok) {
      digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
      gSendDate += 1000 ;
      gSentCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentCount) ;
    }
  }
  if (can.receive0 (message)) {
    gReceivedCount += 1 ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedCount) ;
  }
}
```
`CANMessage` is the class that defines a CAN message. The `message` object is fully initialized by the default constructor. Here, we set the `id` to `0x542` for sending a standard data frame, without data, with this identifier.

The `can.tryToSendReturnStatus` tries to send the message. It returns `0` if the message has been sucessfully added to the driver transmit buffer.

The `gSendDate` variable handles sending a CAN message every 1000 ms.

`(can.receive0` returns `true` if a message has been received, and assigned to the `message`argument.

### Use of Optional Reception Filtering

The bxCAN module accepts up to 14 filter banks. A filter bank can be either:

* a standard quad filter bank;
* a standard mask dual filter bank;
* an extended dual filter bank;
* an extended mask single filter bank.

The `LoopBackDemoFilters` sketch is a basic demo of filters:

```cpp
 ACAN_STM32_Settings settings (1000 * 1000) ;

  settings.mModuleMode = ACAN_STM32_Settings::EXTERNAL_LOOP_BACK ;

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
```
