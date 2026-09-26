

//######################################################################
//# License: BSD-3-Clause
//# Sample of my own proprietary over the UART protocol implemented                                     #
//# on two boards of Arduino Uno. Tx and Rx (with Adafruit TFT).
//# Currently Tx can send a float number towards Rx, which is the check
//# of the protocol integrity. The transmission begins with
//# synchronization symbol (in logs as: initial).
//# All are seen on RS232 by USB port monitor.
//# Tx file. With synchronization symbol.                                                                                     #
//# The cables connection setup is as follows:                                                                             #
//#       Board 1           Board 2                                                                                                   #
//#          GND --------- GND                                                                                                     #
//#          TX -------\                                                                                                                   #
//#                          \---RX                                                                                                          #
//#          RX------\                                                                                                                      #
//#                        \---- TX                                                                                                          #
//#     
//# *a - is the floating point number sent
//# *b is the floating point numer received
//# *a is set in the Tx code, but not in Rx.
//#  Values of those shall be equal.
//######################################################################


#define TX 8
#define RX 9
#define DELAY 130
#define INITIALNULLBITS 32
#define BYTEBITES 8
#define FLOATBITLENGTH 8
#define LOWBYTE 255
#define HIGHBYTE 65280
#define NULL32BIT 4294967295
#define SENDBUFFERSIZE 12
#define PROTOCOLSIZE 256

// 205 204 204 63

/**
 * *longintp=
1070386381
*a =
1.60
 * 8
205
true
false
true
true
false
false
true
true
Sending from buffer index =
9
204
false
false
true
true
false
false
true
true
Sending from buffer index =
10
204
false
false
true
true
false
false
true
true
Sending from buffer index =
11
63
true
true
true
true
true
true
false
false

 */

byte eightbytebuffertosend[SENDBUFFERSIZE];

byte protocol[PROTOCOLSIZE];

// 0 - incomming line check - must be LOW to transmit from local TX
#define PROTINIT 255 //repeat 8 times
// LENGTH value in bytes
#define ADDRESS 254 // + address line
#define BODYFRAME 253
#define FRTYPECLEAR 1
#define FRTYPEASKFORSQUARE 2
#define FRTYPEASKFORELLIPSE 3
#define FRTYPESQUARE 4
#define VALUESQUARELENGTH 64 //2 floates (4 bytes) - point * 4 - square, no color right now
#define FRTYPEELLIPSE 5
#define VALUESQUARELENGTH //2 floates - center, 2 floates - radiuses

/** Graphics renderer
 * 
 * 
 */

boolean dec;
float xe = 0;
float ye = 0;
float ze = 5;
float squareXY[] = { -0.5,-0.5,-0.5,  0.5,-0.5,-0.5,  0.5, 0.5,-0.5, -0.5,0.5,-0.5};
float squareXYback[] = { -0.5,-0.5,-1.5,  0.5,-0.5,-1.5,  0.5, 0.5,-1.5, -0.5,0.5,-1.5 };
float squareXZ[] = {0,0,0, 0,0,100, 100,0,100, 100, 0, 0};
float squareYZ[] = {0,0,0, 0,100,0, 0,100,100, 0,100,0};

float * render(float x, float y, float z, float * xy) {
    float deltax = x - xe;
    float deltay = y - ye;
    float deltaz = z - ze;
    float tgalpha = deltax / deltaz;
    float tgbeta = deltay / deltaz;
    xy[0] = ze * tgalpha;
    xy[1] = ze * tgbeta;
    return xy;
}

/************
 *  Sender receiver, slavetype
 */

 typedef union {
  float f;
  struct {
    unsigned int mantissa : 16;
    unsigned int empty : 7;
    unsigned int sign : 1;
    unsigned int exponent : 8;
  } parts;
} floatmask1;

typedef union {
  struct {
    unsigned int mantissa : 16;
    unsigned int empty : 7;
    unsigned int sign : 1;
    unsigned int exponent : 8;
  } parts;
  int32_t  fractionmask;
} floatmask2;

boolean checkSendIndex(int ebbstartindex) {
  if (ebbstartindex < SENDBUFFERSIZE) {
    return true;
  } else {
    return false;
  }
}

boolean checkEndIndex(int ebbstartindex, int offset) {
  if ((ebbstartindex + offset) <= SENDBUFFERSIZE) {
    return true;
  } else {
    Serial.println("End index exceeded =");
    Serial.println(ebbstartindex + offset);
    return false;
  }
}

boolean sendMask(int ebbindex, int tx, int delayperiod) {
  if (checkSendIndex(ebbindex)) {
    byte maskfull = eightbytebuffertosend[ebbindex];
    //Serial.println("Mask sending");
    //Serial.println(maskfull);
    byte mask;
    int i = 0;
    for (i=0; i < 8; i++) {
      mask = maskfull << 7;
      mask = mask >> 7;
      if (mask == 1) {
        digitalWrite(tx,HIGH);
        Serial.println("true");
      } else if (mask == 0) {
        digitalWrite(tx,LOW); // w tej funkcji chyba ida zawsze LOW
        Serial.println("false");
      } else {
        Serial.println("Mask NOT sent");
        return false;
      }
     maskfull = maskfull >> 1;
     delay(delayperiod);
    }
    return true;  
  } else {
    Serial.println("Mask NOT sent - buffer exceeded");
    return false;
  }
}

int write2byte(uint16_t longmask, int ebbstartindex) {
  if (checkEndIndex(ebbstartindex,2)) {
    uint16_t tempmask = longmask << 8;
    tempmask = tempmask >> 8;
    eightbytebuffertosend[ebbstartindex + 0] = (byte) tempmask;
    tempmask = longmask;
    tempmask = tempmask >> 8;
    eightbytebuffertosend[ebbstartindex + 1] = (byte) tempmask; 
    return (ebbstartindex + 2);
  } else {
    return 0;
  }
}

int write4byte(uint32_t longmask, int ebbstartindex) {
  if (checkEndIndex(ebbstartindex,4)) {
    uint32_t tempmask = longmask << 24;
    tempmask = tempmask >> 24;
    eightbytebuffertosend[ebbstartindex] = (byte) tempmask;
    tempmask = longmask << 16;
    tempmask = tempmask >> 24;
    ebbstartindex += 1;
    eightbytebuffertosend[ebbstartindex] = (byte) tempmask;
    tempmask = longmask << 8;
    tempmask = tempmask >> 24;
    ebbstartindex += 1;
    eightbytebuffertosend[ebbstartindex] = (byte) tempmask;
    tempmask = longmask >> 24;
    ebbstartindex += 1;
    eightbytebuffertosend[ebbstartindex] = (byte) tempmask;
    ebbstartindex += 1;
    return ebbstartindex;
  } else {
    return 0;
  }
}

int send8byte(int ebbstartindex, int tx, int delayperiod) {
  int i;
  for (i = ebbstartindex; i<(ebbstartindex + 8); i++) {
    Serial.println("Sending from buffer index =");
    Serial.println(i);
    Serial.println(eightbytebuffertosend[i]);
    sendMask(i,tx,delayperiod);
  }
  return i;
}

int send4byte(int ebbstartindex, int tx, int delayperiod) {
  int i;
  for (i = ebbstartindex; i<(ebbstartindex + 4); i++) {
    Serial.println("Sending from buffer index =");
    Serial.println(i);
    Serial.println(eightbytebuffertosend[i]);
    sendMask(i,tx,delayperiod);
  }
  return i;
}

int writeFloat(float *a, uint32_t * longintp, int ebbstartindex) {
    int ind = ebbstartindex;
    longintp = (uint32_t*)memcpy(((void*)longintp),((void*)a),4);
    Serial.println("*longintp=");
    Serial.println(*longintp);
    a = (float*) memcpy(((void*)a),((void*)longintp),4);
    Serial.println("*a =");
    Serial.println((float)(*a));
    ind = write4byte((*longintp),ind);
    return ind;
}

void setup() {
  Serial.begin(9600);
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
}
 
void loop() {
  float *a;
  uint32_t * longintp;
  a = (float *) malloc(sizeof(float));
  longintp = (uint32_t *) malloc(sizeof(uint32_t));
  (*a) = (float) 1.6;
  Serial.println((float)(*a));
  int ebbstartindexwrite = 0;
  int ebbstartindexsend = 0;
  Serial.println("Init");
  ebbstartindexwrite = write4byte(NULL32BIT,ebbstartindexwrite);
  ebbstartindexwrite = write4byte(0,ebbstartindexwrite);
  ebbstartindexwrite = writeFloat(a,longintp,ebbstartindexwrite);
  Serial.println("SEND index");
  Serial.println(ebbstartindexsend);
  ebbstartindexsend = send8byte(ebbstartindexsend,TX,DELAY);
  ebbstartindexsend = send4byte(ebbstartindexsend,TX,DELAY);
  free(a);
  free(longintp);
  Serial.println("SEND again");
}
