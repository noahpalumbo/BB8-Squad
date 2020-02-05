/*
* BB-8 Base Transceiver Module
* RF chip used: nRF24L01
* Updated: Feb 2020 by Noah Palumbo
*/

#include "receiverLib.h"

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 1;

// Create servo objects
Servo SERVO1;
Servo SERVO2;

// Set initial motor values
int currMotorY = 0;
int currMotorX = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(8, 53);
/**********************************************************/

byte addresses[][6] = {"1Node", "2Node"};

// Used to control whether this node is sending or receiving
// NOTE we will always be reading, so we don't need to use role.
// bool role = 0;

void setup() {
  Serial.begin(115200);

  radio.begin();

  // Set the PA Level low to prevent power supply related issues and 
  // bc of the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);

  // Open a writing and reading pipe on each radio, with opposite addresses
  if (radioNumber) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  }

  // Start the radio listening for data
  radio.startListening();

  //initialize servos and start them in the center
  SERVO1.attach(s1pin);
  SERVO2.attach(s2pin);
  SERVO1.write(81);     // 
  SERVO2.write(46);     // may need tuning as we make changes
}

void loop() {
  /****************** Pong Back Role ***************************/

  uint8_t payload[5]; // payload variable 5 bytes

  int motorY;     //Joystick 1 Y-axis
  int motorX;     //Joystick 1 X-axis
  int neckX;      // Joystick 2 X-axis
  int neckY;      // Joystick 2 Y-axis
  int rotate;     // Head rotation

  //servo variables
  int servo1angle;
  int servo2angle;
  
  if (radio.available()) {
    //Serial.print("Radio received!");
    // Variable for the received timestamp
    while (radio.available()) {                             // While there is data ready
      radio.read( &payload, sizeof(uint8_t)*5 );             // Get the payload
      //Serial.println(payload, BIN);
    }

    // FOR USE IF CONTROLLER NEEDS FEEDBACK
    //radio.stopListening();                                        // First, stop listening so we can talk
    //radio.write( &payload, sizeof(unsigned long) );               // Send the final one back.
    //radio.startListening();                                       // Now, resume listening so we catch the next packets.

    //Decode our message into individual motor and servo signals
    // Payload structure:
    //  [ ---- ---- , ---- ---- , ---- ---- , ---- ---- , 0000 00-- ]
    //     Motor X     Motor Y      Neck X      Neck Y   Head Rotation
    //-------------------------------------------------------

    /* Parse Payload into array fragments (bytes) */
    motorX = payload[0];
    motorY = payload[1];
    neckX = payload[2];
    neckY = payload[3];
    rotate = payload[4];    // 0 & 3 do nothing, 1 and 2 rotate in respective directions

    // Print Payload Fragments
    printPayload(payload);
    
    // Joystick values can range -127 and +127, set from 127 to 0 by subtracting 127
    motorY -= 127;
    motorX -= 127;
    neckY -= 127;
    neckX -= 40;

    // Modified to Zero Center Payload Debug Prints
    // values roughly (-127 <-> 127)
    /*Serial.print("motorX: ");
    Serial.println(motorX);
    Serial.print("motorY: ");
    Serial.println(motorY);
    Serial.print("neckX: ");
    Serial.println(neckX);
    Serial.print("neckY: ");
    Serial.println(neckY);
    Serial.print("rotate: ");
    Serial.println(rotate);
    Serial.println();*/
    

    // Motor Damping Debug Prints
    /*Serial.print("motorY: ");
    Serial.println(motorY);
    Serial.print("currMotorY: ");
    Serial.println(currMotorY);
    
    Serial.print("motorX: ");
    Serial.println(motorX);
    Serial.print("currMotorX: ");
    Serial.println(currMotorX);
    
    Serial.println();*/

    
    // scaling to account for overflow
    neckX = neckX / 2;
    neckY = neckY / 1.5;
    
    //SERVO OUTPUT CODE
    servo1angle = neckY;
    servo2angle = neckX;
    servo1angle = map(servo1angle, 127, -127, 37, 128);
    servo2angle = map(servo2angle, -40, 22, 0, 92);
    SERVO1.write(servo1angle); // write neckY
    SERVO2.write(servo2angle); // write neckX
    /* If movement too janky may need to use damping code similar to motors */

    // MOTOR OUTPUT CODE
        // Motor Y Direction and currMotorY update
        if (motorY < -5)                    // if user input is negative Y
          currMotorY = negativeInputDamp(motorY, currMotorY);
        else if (motorY > 5)                // if user input is positive Y
          currMotorY = positiveInputDamp(motorY, currMotorY);
        else     // user input is zero
          currMotorY = zeroInputDamp(motorY, currMotorY);


        // Send currMotorY to correct pins
        if(currMotorY < 0)
        {         
          digitalWrite(Y_DIR2_PIN, 0);
          digitalWrite(Y_DIR2_PIN2, 1);
          analogWrite(Y_PWM_PIN, abs(currMotorY) * 2);
          analogWrite(Y_PWM_PIN2, abs(currMotorY) * 2);
        }
        else if (currMotorY > 0)
        {
          digitalWrite(Y_DIR2_PIN, 1);
          digitalWrite(Y_DIR2_PIN2, 0);
          analogWrite(Y_PWM_PIN, abs(currMotorY) * 2);
          analogWrite(Y_PWM_PIN2, abs(currMotorY) * 2);
        }
        else
        {
           digitalWrite(Y_DIR2_PIN, 0);
           analogWrite(Y_PWM_PIN, 0);
           digitalWrite(Y_DIR2_PIN2, 0);
           analogWrite(Y_PWM_PIN2, 0);
        }     
        

        // Motor X Direction and currMotorX update
        if (motorX < -5)                     // if user input is negative X
          currMotorX = negativeInputDamp(motorX, currMotorX);
        else if (motorX > 5)                 // if user input is positive X
          currMotorX = positiveInputDamp(motorX, currMotorX);
        else                                 // user input is zero
          currMotorX = zeroInputDamp(motorX, currMotorX);
        
        // Send currMotorX to correct pins
        if(currMotorX < 0)
        {         
          digitalWrite(X_DIR1_PIN, 0);
          digitalWrite(X_DIR1_PIN2, 1);
          analogWrite(X_PWM_PIN, abs(currMotorX) * 2);
          analogWrite(X_PWM_PIN2, abs(currMotorX) * 2);
        }
        else if (currMotorX > 0)
        {
          digitalWrite(X_DIR1_PIN, 1);
          digitalWrite(X_DIR1_PIN2, 0);
          analogWrite(X_PWM_PIN, abs(currMotorX) * 2);
          analogWrite(X_PWM_PIN2, abs(currMotorX) * 2);
        }
        else
        {
           digitalWrite(X_DIR1_PIN, 0);
           analogWrite(X_PWM_PIN, 0);
           digitalWrite(X_DIR1_PIN2, 0);
           analogWrite(X_PWM_PIN2, 0);
        }

                 
        /*Serial.println("BEFORE");
        Serial.print("Rotate: ");
        Serial.println(rotate);
        Serial.print("rotEn: ");
        Serial.println(rotEn);
        Serial.print("rotDir1: ");
        Serial.println(rotDir1);
        Serial.print("rotDir2: ");
        Serial.println(rotDir2);
        Serial.println();*/

        //Serial.println(rotate);
        if (rotate == 1)
        {
            digitalWrite(rotDir1, HIGH);
            digitalWrite(rotDir2, LOW);
            // set speed to 50 out of possible range 0~255
            analogWrite(rotEn, 150);
        }
        else if (rotate == 2)
        {
            digitalWrite(rotDir1, LOW);
            digitalWrite(rotDir2, HIGH);
            analogWrite(rotEn, 150);
        }
        else 
        {
            digitalWrite(rotDir1, LOW);
            digitalWrite(rotDir2, LOW);
        }

        /*Serial.println("AFTER");
        Serial.print("rotEn: ");
        Serial.println(rotEn);
        Serial.print("rotDir1: ");
        Serial.println(rotDir1);
        Serial.print("rotDir2: ");
        Serial.println(rotDir2);
        Serial.println();*/
        
      }

} // Loop

//Motor 1 is janky
