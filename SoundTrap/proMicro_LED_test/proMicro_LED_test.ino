#define RXLED 17  // The RX LED has a defined Arduino pin

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);
//  while (!Serial) {
//     ; // wait for serial port to connect. Needed for native USB port only
//   }

 pinMode(RXLED, OUTPUT);  // Set RX LED as an output
 }

void loop() {
  //Serial.println("LED Low");
     TXLED0;
    delay(1000);
      //Serial.println("LED High");
      TXLED1;
    delay(2000);


  digitalWrite(RXLED, HIGH);   // sets the XSENS on
    delay(1000);
      //Serial.println("LED High");
  digitalWrite(RXLED, LOW);   // sets the XSENS on
    delay(2000);
}
