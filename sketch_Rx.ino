//######################################################################
//# License: BSD-3-Clause
//# Rx file of th program. Tine, transport like an UART protocol in GPIO
//# receives the sychronization symbol (in logs as: initial)
//# and a floating number which currently
//# is hardcoded in Tx as 1.60 . Rx file doesn't store this value.
//# is it received from Tx and shown in the RS232 by USB port monitor.
//# *a - is the floating point number sent
//# *b is the floating point numer received
//# *a is set in the Tx code, but not in Rx.
//#  Values of those shall be equal.
//######################################################################

#include <SPI.h>

#define TX 8
#define RX 9
#define DELAY 130
#define INITIALNULLBITS 32
#define BYTEBITES 8
#define FLOATBITLENGTH 8
#define LOWBYTE 255
#define HIGHBYTE 65280
#define RECEIVEBUFFERSIZE 4
#define PROTOCOLSIZE 256

// 205 #CD  204  #CC 204 #CC 63 #3F 3452750911

/*
 * 
 * 
 * 
true
false
true
true
false
false
true
true
mask 
205
false
false
true
true
false
false
true
true
mask 
204
false
false
true
true
false
false
true
true
mask 
204
true
true
true
true
true
true
false
false
mask 
63
ovf
 */

byte eightbytebufferreceived [RECEIVEBUFFERSIZE];

byte protocol[PROTOCOLSIZE];

void setup() {
   
  // put your setup code here, to run once:
  Serial.begin(9600);

  Serial.println("PIN modes");  
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT); 
}

/************
 *  Sender receiver, mastertype
 */

int waitnull(int rx, int delayperiod) {
  int i = 0;
  int counter = 0;
  int fullcounter = INITIALNULLBITS;
  while (counter < fullcounter) {
    if (digitalRead(rx) == true) {
      counter ++ ;
      Serial.println("initial true");
    } else {
      counter = 0;
      Serial.println("noninitial false");
    }
    Serial.println("noninitial counter =");
    Serial.println(counter);
    delay(delayperiod);
  }
  counter = 0;
  while (counter < fullcounter) {
    if (digitalRead(rx) == false) {
      counter ++ ;
      Serial.println("initial false");
    } else {
      counter = 0;
      Serial.println("noninitial true");
    }
    Serial.println("noninitial counter =");
    Serial.println(counter);
    delay(delayperiod);
  }
  return 0; // is not storing NULL
}

boolean checkSendIndexRec(int ebbstartindex) {
  if (ebbstartindex < RECEIVEBUFFERSIZE) {
    return true;
  } else {
    return false;
  }
}

boolean checkEndIndexRec(int ebbstartindex, int offset) {
  if ((ebbstartindex + offset) <= RECEIVEBUFFERSIZE) {
    return true;
  } else {
    Serial.println("End index exceeded =");
    Serial.println(ebbstartindex + offset);
    return false;
  }
}

int receiveMask(int ebbindex, int rx, int delayperiod) {
  if (checkSendIndexRec(ebbindex)) {
    byte fullmask = 0;
    byte mask = 1;
    int i = 0;
    int counter = 0;
    int fullcounter = 8;
  
    while (counter < fullcounter) {
      if (digitalRead(rx) == true) {
        fullmask = fullmask | mask;
        mask = mask << 1;
        Serial.println("true");
      } else {
        mask = mask << 1;
        Serial.println("false");
      }
      counter++;
      delay(delayperiod);
    }
    eightbytebufferreceived[ebbindex] = fullmask;
    //Serial.println("mask ");
    //Serial.println(eightbytebufferreceived[ebbindex]);
    return (++ebbindex);
  } else {
    return 0;
  }
}

int receive8byte(int *ebbindex, int rx, int delayperiod) {
  if (checkEndIndexRec(ebbindex,8)) {
    for (int i = 0; i<4; i++){
      ebbindex = receiveMask(ebbindex, rx, delayperiod);
    }
    return ebbindex;
  } else {
    return 0;
  }
}

int receive4byte(int *ebbindex, int rx, int delayperiod) {
  if (checkEndIndexRec(ebbindex,4)) {
    for (int i = 0; i<4; i++){
      ebbindex = receiveMask(ebbindex, rx, delayperiod);
    }
    return ebbindex;
  } else {
    return 0;
  }
}

int read4byte(int32_t *longmask, int ebbstartindex) {
  if (checkEndIndexRec(ebbstartindex,4)) {
    int ind = ebbstartindex;
    uint32_t mask0 = eightbytebufferreceived[ind];
    Serial.println("mask0 ");
    Serial.println(mask0);
    //mask0;
    ind++;
    uint32_t mask1 = eightbytebufferreceived[ind];
    Serial.println("mask1 ");
    Serial.println(mask1);
    mask1 = mask1 << 8; 
    ind++;
    uint32_t mask2 = eightbytebufferreceived[ind];
    Serial.println("mask2 ");
    Serial.println(mask2);
    mask2 = mask2 << 16;
    ind++;
    uint32_t mask3 = eightbytebufferreceived[ind];
    Serial.println("mask3 ");
    Serial.println(mask3);
    mask3 = mask3 << 24;
    ind++;
    (*longmask) = mask3 + mask2 + mask1 + mask0;
    return ind;
  } else {
    return 0;
  }
};

int read2byte(uint16_t *longmask, int ebbstartindex) {
  if (checkEndIndexRec(ebbstartindex,4)) {
    int ind = ebbstartindex;
    uint16_t mask0 = eightbytebufferreceived[ind];
    Serial.println("mask0 ");
    Serial.println(mask0);
    //mask0;
    ind++;
    uint16_t mask1 = eightbytebufferreceived[ind];
    Serial.println("mask1 ");
    Serial.println(mask1);
    mask1 = mask1 << 8; 
    (*longmask) = mask1 + mask0;
    ind++;
  } else {
    return 0;
  }
};

int readFloat(float *b, uint32_t * longintp,int  ebbindex) {
  int ind = ebbindex;
  ind = read4byte(longintp,ebbindex);
  b = (float*) memcpy(((void*)b),((void*)longintp),4);
  Serial.println("*longintp=");
  Serial.println(*longintp);
  Serial.println("*b =");
  Serial.println((float)(*b));
  return ind;
};


void loop() {
  float *b;
  uint32_t * longintp;
  int eightbuffereceiveindex = 0;
  int eightbufferreadindex = 0;
  b = (float*)malloc(sizeof(float));
  longintp = (uint32_t*)malloc(sizeof(uint32_t));
  Serial.println("INIT LISTENING");
  waitnull(RX,DELAY);
  eightbuffereceiveindex = receive4byte(eightbuffereceiveindex,RX,DELAY);
  //eightbytebufferreceived[0] = 205;
  //eightbytebufferreceived[1] = 204;
  //eightbytebufferreceived[2] = 204;
  //eightbytebufferreceived[3] = 63;
  eightbufferreadindex = readFloat(b,longintp,eightbufferreadindex);
  Serial.println((float)(*b));
  delay(1000);
  free(b);
  free(longintp);
}
