int ledPin = 10;                 // LED connected to digital pin 13

#define Pin13LED         13


void setup()
{
  pinMode(ledPin, OUTPUT);      // sets the digital pin as output
}

void loop()
{
  digitalWrite(ledPin, HIGH);   // sets the LED on
       TXLED0;
  delay(4000);                  // waits for a second
  digitalWrite(ledPin, LOW);    // sets the LED off
        TXLED1;
  delay(4000);                  // waits for a second
}

