
/*
  Communication between an arduino, xsens sensor and pressure sensor.
  The data from pressure sensor and arduino is

  In order to listen on a software port, you call port.listen().
  When using two software serial ports, you have to switch ports
  by listen()ing on each one in turn. Pick a logical time to switch
  ports, like the end of an expected transmission, or when the
  buffer is empty. This example switches ports when there is nothing
  more to read from a port

  Note:
  Not all pins on the Mega and Mega 2560 support change interrupts,
  so only the following can be used for RX:
  10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69

  Not all pins on the Leonardo support change interrupts,
  so only the following can be used for RX:
  8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).

  created 18 Apr. 2011
  modified 19 March 2016
  by Tom Igoe
  based on Mikal Hart's twoPortRXExample

  This example code is in the public domain.

*/
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SparkFun_MS5803_I2C.h>
#include <xsensmessage.h>
#include <LowPower.h>


/*XSENS COMMS*/
#define XSENS_BAUD_RATE     38400  //Baud rate for xsens sensor. 

// software serial #1: RX = digital pin 10, TX = digital pin 11
#define XSENSSerialRX        9  //Serial Receive pin 
#define XSENSSerialTX        8  //Serial Transmit pin

//define the size of the data buffer.
#define ARRAY_SIZE 255

#define XSENS_PWR 10

SoftwareSerial XSensSerial(XSENSSerialRX, XSENSSerialTX);

/*SOUNDTRAP COMMS rs458*/
#define ST_BAUD_RATE     38400  //Baud rate for xsens sensor. 

#define SSerialRX        14  //Serial Receive pin
#define SSerialTX        15 //Serial Transmit pin

#define SSerialTxControl 5   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

/*PRESSURE SENSOR*/
MS5803 sensor(ADDRESS_HIGH);

/**LED **/
#define LED_2         17 //Orange RX LED

/******XSENS COMMUNICATION*********/

//raw output in mt communication prtotocol to send to SoundTrap or xsens sensor.
uint8_t rawMessageOut[64];

//the length of the output message
uint8_t outLen = 0;

//the current incoming message
XBusMessage messageIn;

//the current outgoing message
XBusMessage messageOut;

//the last byte read from the buffer
uint8_t inByte;

/**PRESSURE SENSOR COMMUNICATION**/

//uinion to easily convert between types for pressure sensor data.
typedef union
{
  double numberd;
  float numberf;
  uint8_t bytes[4];
} PT_UNION_t;

PT_UNION_t ptUnion;

//the temperature
float temperature_c;

//the absolute pressure in millibar.
double pressure_abs;

//baseline pressure i.e. when out of water- this allows more accurate depth measurments (only a minor correction though )
double pressure_baseline;

/******SOUNDTRAP COMMUNICATION*********/


/************MISC VARIABLES************/
unsigned long time1;

unsigned long time2;

unsigned long timetask;

/*******Measurment state*****/
//keep track of whether the xsens is in measurment sytate or not.
bool measure = false;

//total number of orientation data reads.
long countOrient = 0;

//the number of pressure and tempoerature to message per recieved orientation message
int ptRate = 10;

//the number of millis to check status if SoundTrap.
long checkMillisRate = 30 * 60000; //every 30 mintues

float ori[4];


/**
   Setup the xsens sensor.Starty with config mode.
*/
void setupXsens() {
  XSensSerial.begin(XSENS_BAUD_RATE);
  pinMode(XSENS_PWR, OUTPUT);      // sets the digital pin as output

  //portOne.write(goToConfig, sizeof(goToConfig));
  //sendCommand(goToConfig);

  sendXBusCommand(&XSensSerial, XMID_GotoConfig);
  delay(50);  //need to delay to make this reliable- meh
  listenOnPort(&XSensSerial, -1);
  sendXBusCommand(&XSensSerial, XMID_ReqPeriod);
  delay(20);
  listenOnPort(&XSensSerial, -1);
}

/**
   Setup the pressure sensor communication with I2C
*/
void setupPressure() {
  //Retrieve calibration constants for conversion math.
  sensor.reset();
  sensor.begin();
  pressure_baseline = sensor.getPressure(ADC_4096);
}


void setupST() {

  //  pinMode(Pin13LED, OUTPUT);
  //  pinMode(SSerialTxControl, OUTPUT);
  //
  //  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver

  // Start the software serial port, to another device
  pinMode(SSerialTxControl, OUTPUT);      // sets the digital pin as output
  RS485Serial.begin(ST_BAUD_RATE);   // set the data rate

}

/**
   Wait for the next available or until timeout.
*/
int readNextAvailable(SoftwareSerial* portOne) {
  int count = 0;
  while (portOne->available() == 0 && count < 50) {
    //do nothing;
    count++;
    delay(1);
  }
  return portOne->read();
}


/**
   Read the xbus header  and data. Th header values and data are passed
   into an xbus_message struct..
*/
bool readXBusMessage(SoftwareSerial* portOne, XBusMessage* mtMessage) {
  //BID
  mtMessage->bid = readNextAvailable(portOne);

  //MID
  mtMessage->mid = readNextAvailable(portOne);

  //LEN
  mtMessage->len = readNextAvailable(portOne);

  //      //print some stuff out.
  //      Serial.print(" BID: ");
  //      Serial.print(mtMessage->bid);
  //      Serial.print(" MID: ");
  //      Serial.print(mtMessage->mid);
  //      Serial.print(" LEN: ");
  //      Serial.print(mtMessage->len);


  if (mtMessage->len >= ARRAY_SIZE)
  {
    return false;
  }

  if (mtMessage->len > 0)
  {
    int bytes = portOne->readBytes(mtMessage->charBufferRx, mtMessage->len);
    //       int a;
    //       for( a = 0; a < mtMessage->len; a++ )
    //       {
    //          Serial.print(" ");
    //          Serial.print(mtMessage->charBufferRx[a]);
    //       }
    //       Serial.println(" bytes read");
    if (bytes != mtMessage->len) {
      return false;
    }
  }

  return true;

}


/**
   Parse a recieved message, either from the SoundTrap or the Xsens sensor.
*/
void parseMessage(XBusMessage* mtMessage) {
  switch (mtMessage->mid)
  {
    case XMID_MtData2:
      //MTData2 message
      parseMTData2(mtMessage);
      break;
    case XMID_ReqSTMessageAck:
      //SoundTrap Acknowledge. Means connected to SoundTrap and SoundTrap is ready to go.
      sendXBusCommand(&XSensSerial, XMID_GotoMeasurement);
      measure = true;
      break;

    /****Acknowledgement without data*****/
    case XMID_ResetAck:
    case XMID_GotoMeasurementAck:
      //Serial.print("Go To Measurment Ack ");
      measure = true;
    case XMID_GotoConfigAck:
      //Serial.print("Config Ack ");
      break;

    /****Acknowledgement with data*****/
    case XMID_ReqPeriodAck:
      //Serial.print("Req Period Acknowledge ");
      //                  double period = 1000*(bitShiftCombine(mtMessage->charBufferRx[0], mtMessage->charBufferRx[1])/115200.);
      //                  Serial.print("Period is: ");
      //                  Serial.print(period);
      //                  Serial.print(" ms ");
      break;
    case XMID_DeviceId:
      break;

    /****Commands from SoundTrap to XSENS*****/
    case XMID_Wakeup:
      sendXBusCommand(&XSensSerial, XMID_Wakeup);
      break;
    case XMID_ReqDid:
      sendXBusCommand(&XSensSerial, XMID_ReqDid);
      break;
    case XMID_GotoConfig:
      //Serial.println("Go To Config");
      sendXBusCommand(&XSensSerial, XMID_GotoConfig);
      break;
    case XMID_Reset:
      sendXBusCommand(&XSensSerial, XMID_Reset);
      break;
    case XMID_ReqPeriod:
      sendXBusCommand(&XSensSerial, XMID_ReqPeriod);
      break;
    case XMID_GotoMeasurement:
      //Commands to pass on to xsens
      //Serial.print("Go To Measurment");
      sendXBusCommand(&XSensSerial, XMID_GotoMeasurement);
      measure = true;
      break;
    /****Error message****/
    case XMID_Error:
      //Error sent
      break;
    default:
      break;
  }
}


/**
   Parse an MTData 2 message. Extracts usefulk info form from the message and send on to SoundTrap as quickly as possible.
   Afterwards sends pressure and temperature data.
*/
void parseMTData2(XBusMessage* mtMessage) {

  //uint8_t ori[19];

  uint8_t* dptr;
  bool messageSent = false;

  //Find quaternion data and add to the soundtrap array. Send orientation data to ST
  //Want to do this quickly so send pressure after
  if (XbusMessage_getDataItemRaw(messageOut.charBufferRx, XDI_Quaternion, mtMessage, true))
  {
    messageOut.bid = XBUS_MASTERDEVICE;
    messageOut.mid = XMID_MtData2;
    messageOut.len = 19;

    sendSTData(&messageOut);

    messageSent = true;
    countOrient++;
  }
  else if (XbusMessage_getDataItemRaw(messageOut.charBufferRx, XDI_EulerAngles, mtMessage, true))
  {
    messageOut.bid = XBUS_MASTERDEVICE;
    messageOut.mid = XMID_MtData2;
    messageOut.len = 19 - 4;

    sendSTData(&messageOut);

    messageSent = true;
    countOrient++;
  }

  //    ///TEST print data//
  //    Serial.print(" Orientation: ");
  //
  //    float* p = ori;
  //    XbusMessage_getDataItem(ori, XDI_Quaternion, &messageOut);
  //
  //    int a;
  //    for ( a = 0; a < 4; a++ )
  //    {
  //      Serial.print(ori[a]);
  //      Serial.print(" ");
  //    }
  //    Serial.println(" ");
  //
  //    int a;
  //    for ( a = 0; a < messageOut.len; a++ )
  //    {
  //      Serial.print(messageOut.charBufferRx[a]);
  //      Serial.print(" ");
  //    }
  //    Serial.println(" ");
  //    ///TEST print data//

  if (countOrient % ptRate == 0 && messageSent)
  {
    /****Send Pressure and Temperature****/
    // Read temperature from the sensor in deg C. This operation takes about
    temperature_c = sensor.getTemperature(CELSIUS, ADC_512);
    // Read pressure from the sensor in mbar.
    pressure_abs = sensor.getPressure(ADC_4096);

    //assign pointer to data buffer
    dptr = messageOut.charBufferRx;

    ptUnion.numberf = temperature_c;
    dptr = writeMTData2PT(dptr, &ptUnion, XDI_Temperature);

    ptUnion.numberd = pressure_abs; //overwrites float in memory
    dptr = writeMTData2PT(dptr, &ptUnion, XDI_Pressure);

    messageOut.len = dptr - messageOut.charBufferRx;

    sendSTData(&messageOut);

    //print statements.
    //  Serial.print(" Message len: ");
    //  Serial.print( messageOut.len);
    //  Serial.println("");

    //   Serial.print(" PTData:    : ");
    //   for( a = 0; a < messageOut.len; a++ )
    //    {
    //       Serial.print(messageOut.charBufferRx[a]);
    //       Serial.print(" ");
    //    }
    //    Serial.println(" ");
  }

}

/**
   Convert a pressure/temperature value to bytes and add to an array of bytes. Add length and message identifer.
*/
uint8_t* writeMTData2PT(uint8_t* out, PT_UNION_t* ptUnion, XsDataIdentifier id) {
  int a;

  out = XbusUtility_writeU16(out, id);
  *out++ = 4; //length
  for (a = 0; a < 4; a++) {
    *out++ = ptUnion->bytes[3 - a];
  }

  return out;
}

/**
   Send a command to the xbus sensor. Note, only works when command message contains no data.
*/
void sendXBusCommand(SoftwareSerial* portOne, XsMessageId messageId) {
  portOne->listen(); //make sure the port is listening, otherwise difficult to switch ports.
  messageOut.mid = messageId;
  messageOut.len = 0;
  outLen = XbusMessage_format(rawMessageOut, &messageOut);
  writeXBusMessage(portOne, rawMessageOut, outLen);
}

/*
   Send raw command to xsens sensor and await reply
*/
void writeXBusMessage(SoftwareSerial* portOne, byte command[], int size) {
  //Serial.println("Send Command");
  portOne->write(command, size);

  //  delay(20); //need to delay to make this reliable;=.
  //  while(portOne->available()==0){
  //    //do nothing;
  //    delay(10);
  //  }
  //      while(portOne->available()){
  //        inByte = portOne->read();
  //        if (inByte==0xFA){
  //           readXBusMessage(portOne, &messageIn);
  //           parseMessage(&messageIn);
  //        break;
  //      }
  //    }
}

/**********SoundTrap Functions*************/

/**
   Send a data packet to the SoundTrap
*/
void sendSTData(XBusMessage* mtMessageOut) {
  outLen = XbusMessage_format(rawMessageOut, mtMessageOut);
  writeSTMessage(rawMessageOut, outLen);
}

/*
   Send raw data to SoundTrap
*/
void writeSTMessage(byte data[], int size) {
  digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit

  int a;
  for (a = 0; a < size ; a++)
  {
    RS485Serial.write(data[a]);
  }
}


/**
    Wait for an incomming message from the SoundTrap
*/
void configState() {

  RS485Serial.listen();

  digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit

  listenOnPort(&RS485Serial, -1);
  //Serial.println("Awaiting command");

}


/***
   Request a status from the soundtrap. Checks comms with SopundTrap are still open.
*/
bool checkSTStatus() {

  RS485Serial.listen();

  digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit

  sendXBusCommand(&RS485Serial, XMID_ReqSTMessage);

  digitalWrite(SSerialTxControl, RS485Receive);  // Enable RS485 Recieve
  delay(200);  //need to delay to make this reliable- meh

  //listens for a command. If the SoundTrapAck command is recieved then automatically goes to measurment state.
  listenOnPort(&RS485Serial, -1);

  Serial.println("Awaiting command");

}

long timePort;
/**
   Lisen on a port for xbus data and send a message to parseXBusMessage function. If no bytes are available the function returns
   /*! \portOne - the serial port to read from
*/
void listenOnPort(SoftwareSerial* portOne, long timeout) {

  timePort = millis();
  // Now listen on the second port
  portOne->listen();
  // while there is data coming in, read it
  // and send to the hardware serial port:

  while (portOne->available() > 0 && (timetask < timeout || timeout < 0)) {

    inByte = portOne->read();
    Serial.print(inByte);  Serial.print(" ");

    if (inByte == XBUS_PREAMBLE)
    {
      //read all bytes in message
      if (readXBusMessage(portOne, &messageIn))
      {
        //analyse an xbus message.
        if (messageIn.bid == XBUS_MASTERDEVICE) {
          parseMessage(&messageIn);
        }
      }
    }
    Serial.println(timetask);
    timetask = millis() - timePort;
  }

}


void setup() {
  //setup communication with soundtrap;
  setupST(); //needs to be called before xsens or doesn't work.
  //start the xsens serial port.
  setupXsens();
  //setup communication with pressure sensor.
  setupPressure();

  //set pin mode to power up the xsens sensor.
  pinMode(XSENS_PWR, OUTPUT);

  //set orange LED to display
  pinMode(LED_2, OUTPUT);  // Set RX LED as an output

  /****For testing*****/
  //begin serial.
  Serial.begin(38400);
  //  digitalWrite(XSENS_PWR, HIGH);   // sets the XSENS on
  //  sendXBusCommand(&XSensSerial, XMID_GotoMeasurement);
  /********************/

}


int ledcount = 0;
#define LED_DIV_RATE 10000  //LED divisor 

void loop() {

  if (measure == false) {

    digitalWrite(XSENS_PWR, LOW);   // sets the XSENS OFF to save power

    //check the SoundTrap to see if it's on and sending a messsages. This can take a few attempts
    checkSTStatus();

    if (!Serial.available())
    {
      digitalWrite(LED_2, LOW);   // sets orange LED on
      delay(50); //allow some time to see LED. The LED is longer in this instance.
      digitalWrite(LED_2, HIGH);   // sets orange LED off

      //Before sleep, detach the USB port
      Serial.flush();
      USBDevice.detach();
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
      USBDevice.attach();
      //some time for comms
      delay(1000);
      //Serial.println("Waiting for ST connection after sleep");
    }
    else
    {
      TXLED1;
      delay(50);
      TXLED0;
      Serial.println("Connected to serial port");
      delay(5000);
    }

    //    //wait a while-don't want to bombard the SoundTrap with stuff.
    //    delay(5000);
  }

  else {
    digitalWrite(XSENS_PWR, HIGH);   // sets the XSENS ON
    delay(200); //wait for XSENS to boot up
    sendXBusCommand(&XSensSerial, XMID_GotoMeasurement);
    delay(20); //wait for measurment to start

    time1 = millis(); // record the start time
    while (timetask < checkMillisRate) {
      timetask = millis() - time1;
      listenOnPort(&XSensSerial, checkMillisRate);

      //LED Flashes
      ledcount = ledcount + 1;
      //if the LED count is a divisor of LEDDiv then show LED. This means
      //LED shows briefly at a longish interval.
      if (ledcount % LED_DIV_RATE == 0) {
        TXLED1;
        delay(5);
        TXLED0;
      }
    }


    //    //set measurment to false. Now has to poll ST and recieve reply
    //    //to start measuring again
    //    if (!Serial.available()){
    //      Serial.println("Moving out of measure mode") ;
    //    }

    measure = false;
  }


  ///****For testing*****/
  ////digitalWrite(XSENS_PWR, HIGH);
  //    TXLED1;  // Show activity
  //    time1=millis();
  //    while (timetask<checkMillisRate){
  //      timetask=millis()-time1;
  //      listenOnPort(&XSensSerial, checkMillisRate);
  //      delay(2);
  //    }
  //     TXLED0;
  //    delay(20);
  ///*************************/

}


