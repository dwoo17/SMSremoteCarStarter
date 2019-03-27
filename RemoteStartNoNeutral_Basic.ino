// Remote Start program
//===============================================
// Peripherals:
// 1. ignition 1
// 2. ignition 2 (for AC/Heater fan)
// 3. starter
// also has brake safety shut-off.
//===============================================
// Does not have parking lights or lock/unlock
// no neutral
// no tachometer

#include <SoftwareSerial.h>


// PIN NUMBER
String PIN = "1234";
//String senderPhoneNum = "17753974138";  //put default number here: Dallin: 17753881511, my num: 17753974138
String senderPhoneNum = "19703939252";

SoftwareSerial mySerial(2, 3); // RX, TX

// gprs sheild pwr pin:
int GPRS_PWR_KEY_PIN = 6;
//inputs
int BRAKE_PIN = A0;
//int NEUTRAL_PIN = A1;
//outputs
int IGNITION1_PIN = A2;
int START_PIN = A3;
int AC_FAN_PIN = A4;
//int PLIGHTS_PIN = A4;
//int LOCK_PIN = A5;

int BrakeState = LOW;
long BAUD = 9600;
long runningTime = 0;  // time that the car has been running
long ignition_delay;
//long lightsOnTime = 0;
long MAX_RUN_TIME = 20L*60L*1000L;  // 20min * 60sec * 1000ms = 15 min.
//long MAX_LIGHTS_ON_TIME = 5L*60L*1000L;  // 5min
boolean running = false;        // car is not running
//boolean lightsOn = false;
//boolean allMsgs = false;

String RTC_date = "";

String txtToSend = "";
int state = 0;
boolean sendMsg = false;
boolean msgSent = true;
String msgReceived = "";
//String msgQueue = "";
String msgId;              // mem location of sms to read

enum COMMAND_TYPES
{
  NEW_SMS_ALERT,
  SMS_READ,
  RTC_TIME
};
  
  


void setup()
{
  pinMode(BRAKE_PIN, INPUT);
  //pinMode(NEUTRAL_PIN, INPUT);
  pinMode(IGNITION1_PIN, OUTPUT);
  pinMode(START_PIN, OUTPUT);
  //pinMode(PLIGHTS_PIN, OUTPUT); 
  pinMode(AC_FAN_PIN, OUTPUT);
  //pinMode(LOCK_PIN, OUTPUT);
  
  //---------------------------------------
  //  Start-up sequence for GSM module:
  //---------------------------------------
  //pinMode(GPRS_PWR_KEY_PIN, OUTPUT);
  //digitalWrite(GPRS_PWR_KEY_PIN, HIGH);
  //delay(3100);
  //digitalWrite(GPRS_PWR_KEY_PIN, LOW);
  //delay(7000);
  
  //---------------------------------------
  //    End of start-up sequence
  //  GSM is ready for communication!
  //---------------------------------------
//  mySerial.begin(19200);
//  mySerial.print("\r");
//  delay(1000);
//  mySerial.print("AT+IPR=");
//  mySerial.print(BAUD);
//  Serial.print("AT+IPR=");
//  Serial.print(BAUD);
//  delay(1000);
  
  Serial.begin(BAUD);
  mySerial.begin(BAUD);  //Default serial port setting for the GPRS modem is 19200bps 8-N-1
  
  mySerial.print("\r");
  delay(1000);                    //Wait for a second while the modem sends an "OK"
  mySerial.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  delay(1000);
  mySerial.print("AT+CMGDA=\"DEL ALL\"\r");  // delete all sms messages
  delay(1000);
  
  //  send a text message: 
  //composeText("17753728341", "Car Remote Start device restarted!\r");
}

//=======================================================================================
//  Main loop
//=======================================================================================
void loop()
{
    int brake = digitalRead(BRAKE_PIN);
//    String msgReceived = ""; //#msgReadFix
    if(brake == HIGH && BrakeState == LOW)
    {
      //if(allMsgs)
      //{
        //String text = "Brake applied";
        //composeText(senderPhoneNum, text);
        //delay(10);
      //}
      killIgnition();
    }
    BrakeState = brake;
    
    if(Serial.available())
    {
         mySerial.write(Serial.read());
    }  
    if(mySerial.available())
    {   
        char data;
        while(mySerial.available() > 0)
        {
          data = mySerial.read();
          msgReceived += data;  // build the rcvd msg string
//          int END = msgQueue.indexOf(0x0A);
//          if(END > = 0)
//          {
//            msgReceived = msgQueue.substring(0,
//            Serial.write(data);
//         }
          if( state == 0 && data == '>')
          {
            //Serial.println("Setting state=1");
             state = 1;
          }
          
          else if(state == 1 && data == ' ')
          {
            if(!msgSent)
            {
              sendTxt();
              //Serial.println("delay");
              delay(1000);
              delSMSTxt(msgId);
            }
            else
              Serial.println("text sent=true, did not call sendTxt()");
            state = 0;
          }
          else
          {
            state = 0;
            //Serial.println("setting state=0");
          }
        }
    }
   parseMsg(); 
   
   if(running && (elapsedTime(runningTime) > MAX_RUN_TIME))
   {
     //String text = "Warm-up time-out";
     //Serial.println(text);
     
     //composeText(senderPhoneNum, text);
     //delay(10);
     //Serial.println(MAX_RUN_TIME);
     killIgnition();
   }
   /*if(lightsOn && !running && (elapsedTime(lightsOnTime) > MAX_LIGHTS_ON_TIME))
   {
     String text = "Lights time-out.  Turning parking lights off!";
     Serial.print(text);
     composeText(senderPhoneNum, text);
     digitalWrite(PLIGHTS_PIN, LOW);
     lightsOn = false;
   }*/
}

void parseMsg()
{
    // for cmds begining with '+'
    int plus_indx = msgReceived.indexOf('+');
    //-----------------------------------------------------------------------
    //  parse only after whole cmd has been received (ending with 0x0D 0x0A)
    if(plus_indx >= 0 && msgReceived.indexOf(0x0A, plus_indx) > 0)
    {
      //Serial.println("\nmsgReceived uart: ");
      Serial.println(msgReceived);
      String cmd = msgReceived.substring(plus_indx, msgReceived.indexOf(':', plus_indx));
      
      //Serial.println("\nswitch on cmd type");
      String msg = msgReceived.substring(1+msgReceived.indexOf(':'));
      switch(getCmdType(cmd))
      {
        case NEW_SMS_ALERT:
              msgId = getSMSMemIndex(msg);
              //if(msgId != -1)
              readSMSTxt(msgId);
              msgReceived = "";
              break;
         
        case SMS_READ:
             //Serial.println("SMS_READ");
             if(msg.length() > 54)
             {
               //Serial.println("parse sms");
               parseSMS(msg);
               //delSMSTxt(msgId);
               msgReceived = "";
             }
             break;   
             
//        case RTC_TIME:
//            timeUpdate(msg);
//            msgReceived = "";
//            break;
            
        default:
              msgReceived = msgReceived.substring(msgReceived.indexOf(0x0A, plus_indx));
              break;
      }
              
    }
}

int getCmdType(String cmd)  // COMMAND_TYPES
{
  Serial.print("\ncmd: ");
  Serial.print(cmd);
  if(cmd.equals("+CMTI"))
  {
    Serial.println("=NEW_SMS_ALERT");
    return NEW_SMS_ALERT;
  }
  else if(cmd.equals("+CMGR"))
  {
    //Serial.println("=SMS_READ");
    return SMS_READ;
  }
//  else if(cmd.equals("+CCLK"))
//  {
//    Serial.println("Time update cmd");
//    return RTC_TIME;
//  }
  else 
  {
    Serial.println("\ncmd not recognized");
    return -1;
  }
}

String getSMSMemIndex(String msgReceived)
{
  int numStrt = 1 + msgReceived.indexOf(',');  // inclusive of beginning index
  int endNum = msgReceived.indexOf(0x0D);    //exclusive of ending index
  
  String numString =  msgReceived.substring(numStrt, endNum);
  
  return numString;
}

//================================================
//  String to Int
int stringToInt(String numString)
{
  int length = numString.length();
  
  int numVal = 0;
  int factor = 1;
  
  for(int i = length - 1; i >= 0; i--)
  {
    numVal += factor*(numString.charAt(i) - 30);
    factor *= 10;
  }
  
  return numVal;
}

//================================================
//  Read SMS message at mem index
void readSMSTxt(String mem_index)
{
  mySerial.print("AT+CMGR=");
  mySerial.print(mem_index);
  mySerial.print("\r");
}

//================================================
//  Delete SMS message at mem index
void delSMSTxt(String mem_index)
{
  Serial.print("AT+CMGD=");
  Serial.println(mem_index);
  mySerial.print("AT+CMGD=");
  mySerial.print(mem_index);
  mySerial.print("\r");
  Serial.println("deleted sms");
}

//void timeUpdate(String dateTime)
//{
//  //Serial.println("\n" + dateTime);
//  int dateStartIndex = dateTime.indexOf('"') + 1;
//  int dateEndIndex = dateTime.indexOf(',');
//  RTC_date = dateTime.substring(dateStartIndex, dateEndIndex);
//  String time = dateTime.substring(dateEndIndex+1, dateTime.indexOf('"', dateEndIndex));
//  Serial.println("rtc date: " + RTC_date);
//  //Serial.println("time: " + time);
//  int endHourIndex = time.indexOf(':');
//  int hour = stringToInt(time.substring(0, time.indexOf(':')));
//  int minute = stringToInt(time.substring(endHourIndex + 1, time.indexOf(':', endHourIndex)));
////  String number []= {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
////                   "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
////                   "twenty","","","","","","","","","",
////                   "","","","","","","","","","",
////                   "","","","","","","","","","",
////                   "fifty", "fifty-one","fifty-two","fifty-three","fifty-four","fifty-five","fifty-six","fifty-seven","fifty-eight","fifty-nine"};
////   Serial.println("hour: " + hour );  //number[hour]);
////   Serial.println("minute: " + minute);  //number[minute]);
//}

//================================================
// 
//  parse a string for *[PIN][CMD]&
void parseSMS(String sms)
{
  String header = "*";
  //Serial.println("sms string to parse: ");
  //Serial.println(sms);
  
  //Serial.print("length of sms msg just read: ");
  //Serial.println(sms.length());
  int endPhNumIndex = sms.indexOf('"', sms.indexOf('+'));
  senderPhoneNum = sms.substring(sms.indexOf('+')+1, endPhNumIndex);
  int dateStartIndex = sms.indexOf("\",\"", endPhNumIndex+1)+3;
  int dateEndIndex = sms.indexOf(',', dateStartIndex);
  String date = sms.substring(dateStartIndex, dateEndIndex);
  String time = sms.substring(dateEndIndex+1, sms.indexOf('"', dateEndIndex));
  //Serial.println("sender phone number: " + senderPhoneNum);
  Serial.print("date: ");
  Serial.println(date);
  //Serial.print("time: ");
  Serial.println(time);
  
  header += PIN;
  if(sms.indexOf('*') == -1)
  {
    String text = "Incorrect command syntax";
    //Serial.println("No '*' char in string");
    composeText(senderPhoneNum, text);
  }
  else
    Serial.println("String strt w header: " + sms.substring(sms.indexOf('*')));
  
  if(sms.indexOf('*') == -1 || sms.indexOf('*') + 5 >= sms.length())
      return;
  if(header.equals(sms.substring(sms.indexOf('*'), sms.indexOf('*') + 5)))
  {
    // PIN OK
    Serial.println("\nPIN OK");
    String cmd = sms.substring(sms.indexOf('*') + 5, sms.lastIndexOf('&'));  //sms.indexOf('&'));//
    executeCmd(cmd);
  }
  else
  {
    String text = "Incorrect Pin";
    composeText(senderPhoneNum, text);
    Serial.println("PIN does not match");
    //String expected = "expected: " + header;
    //String received = "received: " + sms.substring(sms.indexOf('*'), sms.indexOf('*') + 5);
    //Serial.println(expected);
    //Serial.println(received);
  }
}

void executeCmd(String cmd)
{
  String text = "";
  //Serial.println("ex cmd phone number: " + senderPhoneNum);
  //Serial.println("executing command...");
  Serial.println(cmd);
  if(cmd.equalsIgnoreCase("Start"))
  {
    digitalWrite(IGNITION1_PIN, HIGH);  // turn car on to read neutral switch
    delay(300);
    if(/*digitalRead(NEUTRAL_PIN) == HIGH && */digitalRead(BRAKE_PIN) == LOW)
    {
      if(!running)
        startCar();
      else
      {
        text = "Car is already running";
        Serial.println(text);
        composeText(senderPhoneNum, text);
      }
    }
    else
    {
      digitalWrite(IGNITION1_PIN, LOW);  // turn car off
      Serial.println("Car not started because it is not in neutral or the brake is applied");
      text = "Car not started because it is not in neutral or the brake is applied";
      composeText(senderPhoneNum, text);
    }
  }
  else if(cmd.equalsIgnoreCase("Kill"))
  {
    killIgnition();
  }
//  else if(cmd.equalsIgnoreCase("Lock"))
//  {
//    Serial.println("Locking doors!");
//    digitalWrite(LOCK_PIN, HIGH);
//  }
//  else if(cmd.equalsIgnoreCase("Unlock"))
//  {
//    Serial.println("Unlocking doors!");
//    digitalWrite(LOCK_PIN, LOW);
//  }

//--------------------------------------------
  /*else if(cmd.equalsIgnoreCase("Lights On"))
  {
    text = "Turning parking lights on!";
    Serial.println(text);
    composeText(senderPhoneNum, text);
    digitalWrite(PLIGHTS_PIN, HIGH);
    lightsOn = true;
    startTimer(&lightsOnTime);
  }
  else if(cmd.equalsIgnoreCase("Lights Off"))
  {
    text = "Turning parking lights off!";
    Serial.print(text);
    composeText(senderPhoneNum, text);
    digitalWrite(PLIGHTS_PIN, LOW);
    lightsOn = false;
  }*/
  
  else if(cmd.indexOf('&') != -1 && cmd.substring(0, cmd.indexOf('&')).equalsIgnoreCase("Change Pin"))
  {
    PIN = cmd.substring(cmd.indexOf('&')+1);  //, cmd.lastIndexOf('&'));
    text = "pin changed to: " + PIN;
    Serial.print(text);
    composeText(senderPhoneNum, text);
  }
  else
  {
    text = "Unrecognized command: " + cmd;
    Serial.print(text);
    composeText(senderPhoneNum, text);
  }
}

void startCar()
{
  boolean abort = false;
  String text = "Starting car!";
  Serial.println(text);
  composeText(senderPhoneNum, text);
  //digitalWrite(PLIGHTS_PIN, HIGH);    // parking lights on
  digitalWrite(IGNITION1_PIN, HIGH);  // turn car on
  //delay(10000);                        // delay 10s
  startTimer(&ignition_delay);
  while(elapsedTime(ignition_delay) < 10000L)
  {
    if(digitalRead(BRAKE_PIN) == HIGH)
    {
      //Serial.println("abort start");
      //String text = "aborting start...";
      //composeText(senderPhoneNum, text);
      return;
    }
  }
  //digitalWrite(PLIGHTS_PIN, LOW);     // turn off prk lights for start pwr
  digitalWrite(START_PIN, HIGH);      // crank engine
  //delay(1500);                        // delay 1.5s
  startTimer(&ignition_delay);
  while(elapsedTime(ignition_delay) < 1500L)  //IGNITION_DELAY_TIME)
  {
    if(digitalRead(BRAKE_PIN) == HIGH)
    {
      //Serial.println("abort starter");
      //String text = "aborting start...";
      //composeText(senderPhoneNum, text);
      return;
    }
  }
  digitalWrite(START_PIN, LOW);       // stop cranking
  //digitalWrite(PLIGHTS_PIN, HIGH);    // turn pk light back on (running lights)
  digitalWrite(AC_FAN_PIN, HIGH);    // turn on ac fan
  
  // start run timer to keep track of how long vehicle has been running:
  running = true;
  //lightsOn = true;
  startTimer(&runningTime);
  // verify start?
  // send start notification:
  //composeText(USER_PHONE_NUM, "Car started!");
}

void killIgnition()
{
  String text = "Car Shutting Off!";
  composeText(senderPhoneNum, text);
  Serial.println(text);
  digitalWrite(START_PIN, LOW);
  digitalWrite(IGNITION1_PIN, LOW);
  //digitalWrite(PLIGHTS_PIN, LOW);
  digitalWrite(AC_FAN_PIN, LOW);
  running = false;
  //lightsOn = false;
  // send kill notification:
  //composeText(USER_PHONE_NUM, "Car ignition killed!");  //will this work in an isr?
}


//=======================================================================================
//  starts a timer by storing the current time in ms in the timer variable passed in
//=======================================================================================
void startTimer(long* timer)
{
  *timer = millis();
}

//=======================================================================================
//  Returns the elapsed time of the timer passed in
//=======================================================================================
long elapsedTime(long timer)
{
  long elapsedTime =  millis()-timer;
  if(elapsedTime < 0)
    elapsedTime += 1000;
    
  return elapsedTime;
}

//=======================================================================================
//  Compose a text message
//  params:
//       String phone_num   string of destination phone including country and area code
//       String msg         message string to send
//=======================================================================================
void composeText(String phone_num, String &msg)
{
  String strtTxt = "AT+CMGS=\"";
  //Serial.println("composing text...");
  if(msgSent)  // this is a new text
  {
    strtTxt += phone_num;
    strtTxt += "\"\r";
    //Serial.println("phone number: " + phone_num);
    //Serial.println(strtTxt);
    mySerial.print(strtTxt);
    msgSent = false;
    txtToSend = msg;
  }
  else  // append to unsent text:
    txtToSend += "\n" + msg;  
}

//=======================================================================================
//  This function is to be called when the GSM device is actually ready for the text of
//  the message, the text has already been initialized with a phone number by the 
//  composeText function.
//=======================================================================================
void sendTxt()
{
  //Serial.println("sendTxt()");
  if(txtToSend.compareTo("") != 0)
  {
    //Serial.print(txtToSend);
    mySerial.print(txtToSend);
    mySerial.write(0x1A);
    mySerial.write(0x0D); //print("\r");
    
    //Serial.println("End text message");
    
    txtToSend = "";
    msgSent = true;
    //Serial.println("txt sent");
  }
  //Serial.println("end send txt");
}
