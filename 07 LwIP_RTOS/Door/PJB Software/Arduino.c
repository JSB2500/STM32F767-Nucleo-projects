#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>

#define DEBUG 0

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
EthernetClient client;
IPAddress server(137, 74, 43, 80);
int ARDUINO_PORT = 18753;
IPAddress ip(192, 168, 0, 102);
IPAddress myDns(192, 168, 0, 254);
IPAddress gateway(192, 168, 0, 254);
IPAddress subnet(255, 255, 255, 0);

#define PIN_SERVO 2
#define PIN_HALL_SENSOR 3
#define PIN_RELAY 4
#define PIN_LED_SERIAL 5
#define PIN_LED_ETHERNET 6
#define PIN_LED_CONNECTION 7
#define PIN_LED_DOOR_UNLOCKED 8
#define PIN_LED_ACTIVE 9

Servo servo;
bool locked = false;
bool open = true;
void updateServo()
{
  servo.attach(PIN_SERVO);
  servo.write(locked ? 180 : 0);
  delay(1000);
  servo.detach();
}
void toggleServo()
{
  locked = !locked;
  updateServo();
}
void blink()
{
  digitalWrite(PIN_RELAY, digitalRead(PIN_HALL_SENSOR));
  open = !open;
}

void setup()
{
  #if DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif
  if (Serial)
    Serial.println("Serial connected.");

  if (Serial)
    Serial.println("Setting LED outputs...");
  pinMode(PIN_LED_SERIAL, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_ETHERNET, OUTPUT);
  pinMode(PIN_LED_CONNECTION, OUTPUT);
  pinMode(PIN_LED_DOOR_UNLOCKED, OUTPUT);
  pinMode(PIN_LED_ACTIVE, OUTPUT);
  pinMode(PIN_HALL_SENSOR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL_SENSOR), blink, CHANGE);
  if (Serial)
    Serial.println("LED outputs set.");
  digitalWrite(PIN_LED_SERIAL, Serial);

  if (Serial)
    Serial.println("Initialising Ethernet...");
  if (Ethernet.begin(mac) == 0)
  {
    if (Serial)
    {
      Serial.println("Failed to initialise Ethernet!");
      Serial.println("Trying using fixed IP instead.");
    }
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  else
  {
    if (Serial)
      Serial.println("Ethernet initialised.");
    digitalWrite(PIN_LED_ETHERNET, 1);
  }
  digitalWrite(10, HIGH);

  if (Serial)
    Serial.print("IP address: ");
  for (byte i = 0; i < 4; ++i)
  {
    if (Serial)
    {
      Serial.print(Ethernet.localIP()[i], DEC);
      Serial.print(".");
    }
  }
  if (Serial)
    Serial.println();
  updateServo();
}

void loop()
{
  if (!client.connected())
  {
    client.stop();
    client.connect(server, ARDUINO_PORT);
    client.print("GET /arduino HTTP/1.1\r\n\r\n");
  }
  digitalWrite(PIN_LED_CONNECTION, client.connected());
  digitalWrite(PIN_LED_DOOR_UNLOCKED, !locked);

  if (client.available())
  {
    char c = (char) client.read();
    if (Serial)
      Serial.println(c);
    switch (c)
    {
      case 'p':
        if (Serial)
          Serial.println("Ping.");
        client.write(!locked ? 'o' : 'c');
        break;
      case 'o':
        if (Serial)
          Serial.print("Door ");
        toggleServo();
        if (Serial)
          Serial.println(!locked ? "unlocked." : "locked.");
        break;
    }
  }

  digitalWrite(PIN_LED_SERIAL, Serial);
  digitalWrite(PIN_LED_ACTIVE, !digitalRead(PIN_LED_ACTIVE));
  delay(50);
}
