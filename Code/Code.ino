#include "WiFi.h"
#define DEBUG true


//******************* Pin Configurations *******************//

#define A9G_PON     D10
#define A9G_LOWP    D2
#define SOS_Button D3


//******************* Necessary Variables *******************//
boolean stringComplete = false;
String inputString = "";
String fromGSM = "";
bool CALL_END = 1;
char* response = " ";
String res = "";
int c = 0;
String msg;
String custom_message;

//******************* SIM Paramaters *******************//


String SOS_NUM = "+91xxxxxxxxxx";



//******************* SOS Button Press  *******************//
int SOS_Time = 5; // Press the button 5 sec

void A9G_Ready_msg();

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, D0, D1);

  pinMode(A9G_PON, OUTPUT);//LOW LEVEL ACTIVE
  //  pinMode(A9G_RST, OUTPUT);//HIGH LEVEL ACTIVE
  pinMode(A9G_LOWP, OUTPUT);//LOW LEVEL ACTIVE

  //  digitalWrite(A9G_RST, LOW);
  digitalWrite(A9G_LOWP, HIGH);
  digitalWrite(A9G_PON, HIGH);
  delay(1000);
  digitalWrite(A9G_PON, LOW);
  delay(10000);

  // Making Radio OFF for power saving
  WiFi.mode(WIFI_OFF);  // WiFi OFF
  btStop();   // Bluetooth OFF

  pinMode(SOS_Button, INPUT_PULLUP);
  pinMode(A9G_LOWP, OUTPUT);

  // Waiting for A9G to setup everything for 20 sec
  delay(20000);


  digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF

  // Just Checking
  msg = "";
  msg = sendData("AT", 1000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT", 1000, DEBUG);
    Serial.println("Trying");
  }


  // Turning ON GPS
  msg = "";
  msg = sendData("AT+GPS=1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPS=1", 1000, DEBUG);
    Serial.println("Trying");
  }

  // GPS low power
  msg = "";
  msg = sendData("AT+GPSLP = 2", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPSLP = 2", 1000, DEBUG);
    Serial.println("Trying");
  }

  // Configuring Sleep Mode to 1
  msg = "";
  msg = sendData("AT+SLEEP = 1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+SLEEP = 1", 1000, DEBUG);
    Serial.println("Trying");
  }

  // For SMS
  msg = "";
  msg = sendData("AT+CMGF = 1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CMGF = 1", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = "";
  msg = sendData("AT+CSMP  = 17,167,0,0 ", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CSMP  = 17,167,0,0 ", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = "";
  msg = sendData("AT+CPMS = \"SM\",\"ME\",\"SM\" ", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CPMS = \"SM\",\"ME\",\"SM\" ", 1000, DEBUG);
    Serial.println("Trying");
  }


  // For Speaker
  msg = "";
  msg = sendData("AT+SNFS=2", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+SNFS=2", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = "";
  msg = sendData("AT+CLVL=8", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CLVL=8", 1000, DEBUG);
    Serial.println("Trying");
  }

  A9G_Ready_msg(); // Sending Ready Msg to SOS Number

  digitalWrite(A9G_LOWP, HIGH); // Sleep Mode ON
}

void loop()
{

  {
    //listen from GSM Module
    if (Serial1.available())
    {
      char inChar = Serial1.read();

      if (inChar == '\n') {

        //check the state
        if (fromGSM == "SEND LOCATION\r" || fromGSM == "send location\r" || fromGSM == "Send Location\r")
        {
          Get_gmap_link(0);  // Send Location without Call
          digitalWrite(A9G_LOWP, HIGH);// Sleep Mode ON

        }

        //check the state
        else if (fromGSM == "BATTERY?\r" || fromGSM == "battery?\r" || fromGSM == "Battery?\r")
        {
          digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF
          Serial.println("---------Battery Status-------");
          msg = "";
          msg = sendData("AT+CBC?", 2000, DEBUG);
          while ( msg.indexOf("OK") == -1 ) {
            msg = sendData("AT+CBC?", 1000, DEBUG);
            Serial.println("Trying");
          }

          msg = msg.substring(19, 24);
          response = &msg[0];

          Serial.print("Recevied Data - "); Serial.println(response); // printin the String in lower character form
          Serial.println("\n");


          custom_message = response;
          Send_SMS(custom_message);



        }

        // For Auto Call Recieve
        else if (fromGSM == "RING\r")
        {
          digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF
          Serial.println("---------ITS RINGING-------");
          Serial1.println("ATA");
        }

        else if (fromGSM == "NO CARRIER\r")
        {
          Serial.println("---------CALL ENDS-------");
          CALL_END = 1;
          digitalWrite(A9G_LOWP, HIGH);// Sleep Mode ON
        }

        //write the actual response
        Serial.println(fromGSM);
        //clear the buffer
        fromGSM = "";

      }
      else
      {
        fromGSM += inChar;
      }
      delay(20);
    }

    // read from port 0, send to port 1:
    if (Serial.available()) {
      int inByte = Serial.read();
      Serial1.write(inByte);
    }

    // When SOS button is pressed
    if (digitalRead(SOS_Button) == LOW && CALL_END == 1)
    {
      Serial.print("Calling In.."); // Waiting for 5 sec
      for (c = 0; c < SOS_Time; c++)
      {
        Serial.println((SOS_Time - c));
        delay(1000);
        if (digitalRead(SOS_Button) == HIGH)
          break;
      }

      if (c == 5)
      {
        Get_gmap_link(1);  // Send Location with Call
      }

      //only write a full message to the GSM module
      if (stringComplete)
      {
        Serial1.print(inputString);
        inputString = "";
        stringComplete = false;
      }

    }
  }
}


//---------------------------------------------  Getting Location and making Google Maps link of it. Also making call if needed
void Get_gmap_link(bool makeCall)
{


  digitalWrite(A9G_LOWP, LOW);
  delay(1000);
  Serial1.println("AT+LOCATION = 2");
  Serial.println("AT+LOCATION = 2");

  while (!Serial1.available());
  while (Serial1.available())
  {
    char add = Serial1.read();
    res = res + add;
    delay(1);
  }

  res = res.substring(17, 38);
  response = &res[0];

  Serial.print("Recevied Data - "); Serial.println(response); // printin the String in lower character form
  Serial.println("\n");

  if (strstr(response, "GPS NOT"))
  {
    Serial.println("No Location data");
    //------------------------------------- Sending SMS without any location
    custom_message = "Unable to fetch location. Please try again";
    Send_SMS(custom_message);
  }
  else
  {

    int i = 0;
    while (response[i] != ',')
      i++;

    String location = (String)response;
    String lat = location.substring(2, i);
    String longi = location.substring(i + 1);
    Serial.println(lat);
    Serial.println(longi);

    String Gmaps_link = "I'm Here " + ( "http://maps.google.com/maps?q=" + lat + "+" + longi); //http://maps.google.com/maps?q=38.9419+-78.3020
    //------------------------------------- Sending SMS with Google Maps Link with our Location


    custom_message = Gmaps_link;
    Send_SMS(custom_message);

  }
  response = "";
  res = "";
  if (makeCall)
  {
    Serial.println("Calling Now");
    Serial1.println("ATD" + SOS_NUM);
    CALL_END = 0;
  }
}

void A9G_Ready_msg()
{

  custom_message = "A9G Ready!!";
  Send_SMS(custom_message);

}

String sendData(String command, const int timeout, boolean debug)
{
  String temp = "";
  Serial1.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (Serial1.available())
    {
      char c = Serial1.read();
      temp += c;
    }
  }
  if (debug)
  {
    Serial.print(temp);
  }
  return temp;
}


void Send_SMS(String message)
{
  //for (int i = 0; i < Total_Numbers; i++)
  {
    Serial1.println("AT+CMGF=1");
    delay(1000);
    Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"\r");
    delay(1000);

    Serial1.println (message);
    delay(1000);
    Serial1.println((char)26);
    delay(1000);

    Serial1.println("AT+CMGD=1,4"); // delete stored SMS to save memory
    delay(3000);
  }
}
