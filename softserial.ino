#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
int mark=0;
void setup() {
  // put your setup code here, to run once:
mySerial.begin(115200);
Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  while (Serial.available() > 0)
  {
    Serial.read();
    mark = 1;
  }
if(mark==1)
{
  mark=0;
  mySerial.print("AT+TTS=发现猫咪\r");
}
}
