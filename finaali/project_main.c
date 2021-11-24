/*A sensortag program for tamagotchi. The program collects and sends data with 2.4GHZ wireless network

musiikki() function was borrowed from https://github.com/robsoncouto/arduino-songs/tree/master/starwars
modified by us

Authors: Eemil Kulmala, Veikko Vähäsöyrinki */


//Header files
#include <time.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"
#include "sensors/tmp007.h"
#include "buzzer.h"
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <own_functions.h>
#include <wireless/comm_lib.h>
#include <music.h>

//Global variables
int laskuri_az = 0;
int flag1 = 0;
int flag_music = 0;
int leiki_data = 0;
int liiku_data = 0;
int syo_data = 0;
int activate_data = 0; 
float valoisuusarvo;
float lampotila;
char merkkijono_valoisuus[30];
char merkkijono_liike[30];
int sekunti = 1;
int tempo = 108;


char liiku_viesti[] = "id:44,EXERCISE:3\0";
char leiki_viesti[] = "id:44,PET:2\0";
char syo_viesti[] = "id:44,EAT:2\0";
char act_viesti[] = "id:44,ACTIVATE:3;3;3\0";



//PINS
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;
static PIN_State buzzerState;
static PIN_Handle buzzerHandle;
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle button1Handle;
static PIN_State button1State;
static PIN_Handle ledHandle;
static PIN_State ledState;


/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
//Char uartTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

//State machine
//Ohjelma aloittaa tilassa IDLE, napista painamalla siirryt��n tilaan COLLECT
enum state { SLEEP=1, COLLECT, DATA_READY, ACTIVATE, WAIT};
enum state programState = SLEEP;


// Configuration arrays for PINS
PIN_Config buttonConfig[] = { //Button-asetustaulukko
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE 
};

PIN_Config button1Config[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};

PIN_Config ledConfig[] = { //LED-asetustaulukko
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX, 
   PIN_TERMINATE 
};

PIN_Config buzzerConfig[] = { //BUZZER-asetustaulukko
    Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};


//Functions
void buzz(int freq);

void musiikki();

Void buzz(int freq) { //Buzzer
    time_t t1 = time(NULL);
    buzzerOpen(buzzerHandle);
    buzzerSetFrequency(freq);
    while (1) {
            time_t t2 = time(NULL);
            if (t1 - t2 > 1) {
                break;
            }
    }

    buzzerClose();
}

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

void musiikki() { //Function to play music, originally made by: Robson Couto
    int thisNote;
    int notes;
    notes = sizeof(melody) / sizeof(melody[0]) / 2;


    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 4) / tempo;

    int divider = 0, noteDuration = 0;
      // iterate over the notes of the melody.
      // Remember, the array is twice the number of notes (notes + durations)
      for (thisNote = 0; thisNote < notes * 2, flag_music == 1; thisNote = thisNote + 2) {

        // calculates the duration of each note
        divider = melody[thisNote + 1];
        if (divider > 0) {
          // regular note, just proceed
          noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
          // dotted notes are represented with negative durations!!
          noteDuration = (wholenote) / abs(divider);
          noteDuration *= 1.5; // increases the duration in half for dotted notes
        }

        // we only play the note for 90% of the duration, leaving 10% as a pause
        //tone(buzzer, melody[thisNote], noteDuration*0.9);

        // Wait for the specief duration before playing the next note.
        //delay(noteDuration);

        // stop the waveform generation before the next note.
        //noTone(buzzer);
        buzzerSetFrequency(melody[thisNote]);
        Task_sleep(noteDuration*1000 / Clock_tickPeriod);
      }

}


void buttonFxn(PIN_Handle handle, PIN_Id pinId) { //Button0 handler function
		uint_t pinValue = PIN_getOutputValue( Board_LED0 );
		pinValue = !pinValue;
		PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
		if (programState == SLEEP) {
			programState = COLLECT;
			flag_music = 0;
		System_printf("programState is COLLECT\n");
		System_flush();
		}
		else {
			programState = SLEEP;
		    System_printf("programState is SLEEP\n");
		System_flush();
		}
}

Void button1Fxn(PIN_Handle handle, PIN_Id pinId) { //Button1 handler function
    //Napin k�sittelij�funktio
    if (programState == COLLECT) {
        if ((0 < laskuri_az) && (laskuri_az <= 20)) {
            programState = DATA_READY;
            System_printf("programState is DATA_READY\n");
            System_flush();
            System_printf("SY�\n");
            System_flush();
            syo_data = 1;
            laskuri_az = 0;
            flag1 = 0;

        }
        else {
            programState = ACTIVATE;
            System_printf("programState is AKTIVOI\n");
            System_flush();
        }
    }
    else if (programState == ACTIVATE){
        System_printf("programState is COLLECT\n");
        System_flush();
        programState = COLLECT;
    }
    else if (programState == SLEEP){
        if(flag_music == 0){
            flag_music = 1;
            System_printf("MUSIIKKI PÄÄLLE\n");
            System_flush();
        }
        else {
            flag_music = 0;
            System_printf("MUSIIKKI POIS\n");
            System_flush();
        }

    }
}

Void commTask(UArg arg0, UArg arg1) { //Wireless commtask for recieving and handling incoming messages

   char payload[80]; // viestipuskuri
   uint16_t senderAddr;

   // Radio alustetaan vastaanottotilaan
   int32_t result = StartReceive6LoWPAN();
   if(result != true) {
      System_abort("Wireless receive start failed");
   }
   char *res;
   const char address[] = "44,BEEP";

   // Vastaanotetaan viestejä loopissa
   while (true) {

        if (GetRXFlag()) {

           memset(payload,0,80);
           Receive6LoWPAN(&senderAddr, payload, 80);


           res = strstr(payload, address); //Check if recieved message contains our address
           if (res != NULL) {  //variable res will contain something else than NULL if message is meant for us
               System_printf("viesti\n");
               System_flush();
               buzz(2000);   //Notify the user we have recieved a message
           }
         }
      }
}

void sensorTaskFxn(UArg arg0, UArg arg1) { //Temperature, gyro and lux sensor task, also data transmitting and playing music happens here

    //GYRO
    float ax, ay, az, gx, gy, gz;

    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 100ms for the MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();

    mpu9250_setup(&i2cMPU);

    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();

    I2C_close(i2cMPU); //Opened and closed i2c for gyro

    //LUX
    I2C_Handle      i2cLUX;
    I2C_Params      i2cParamsLUX;


    I2C_Params_init(&i2cParamsLUX);
    i2cParamsLUX.bitRate = I2C_400kHz;

    // Avataan yhteys
    i2cLUX = I2C_open(Board_I2C, &i2cParamsLUX);
    if (i2cLUX == NULL) {
       System_abort("Error Initializing I2C\n");
    }

    System_printf("OPT3001: Setup and calibration...\n");
    System_flush();

    opt3001_setup(&i2cLUX);

    System_printf("OPT3001: Setup and calibration OK\n");
    System_flush();

    I2C_close(i2cLUX); //Opened and closed i2c for LUX

    //TEMP
    I2C_Handle      i2cTEMP;
    I2C_Params      i2cParamsTEMP;

    I2C_Params_init(&i2cParamsTEMP);
    i2cParamsTEMP.bitRate = I2C_400kHz;

    // Avataan yhteys
    i2cTEMP = I2C_open(Board_I2C, &i2cParamsTEMP);
    if (i2cTEMP == NULL) {
       System_abort("Error Initializing I2C\n");
    }

    System_printf("TMP007: Setup and calibration...\n");
    System_flush();

    tmp007_setup(&i2cTEMP);

    System_printf("TMP007: Setup and calibration OK\n");
    System_flush();

    I2C_close(i2cTEMP); //Opened and closed i2c for TEMP

    System_printf("Sensors OK, SensorTag ready\n");
    System_flush();
    char valoisuus_day[] = "MSG1:Day";
    char valoisuus_night[] = "MSG1:Night";

    char lampotila_cold[] = "MSG2:Cold";
    char lampotila_warm[] = "MSG2:Hot";


    while (1) {
        if (programState == COLLECT || programState == ACTIVATE) { //If state is COLLECT or ACTIVATE ask sensors for values


            if ( sekunti % 10 == 0) {  //Using modulo to find out if second has passed, can't query opt3001 sensor too much because it will throw DATA NOT READY error
                i2cLUX = I2C_open(Board_I2C, &i2cParamsLUX);    
                valoisuusarvo = opt3001_get_data(&i2cLUX);
                I2C_close(i2cLUX);
																		//Ask opt and tmp sensor for values, save them in a variable
                i2cTEMP = I2C_open(Board_I2C, &i2cParamsTEMP);    
                lampotila = tmp007_get_data(&i2cTEMP);
                I2C_close(i2cTEMP);


                if (lampotila > 37) {

                    Send6LoWPAN(IEEE80154_SERVER_ADDR,lampotila_warm, strlen(lampotila_warm)); // MSG1: temperature
                    StartReceive6LoWPAN();
                } else if (lampotila < 37) {
                    Send6LoWPAN(IEEE80154_SERVER_ADDR,lampotila_cold, strlen(lampotila_cold));
                    StartReceive6LoWPAN();
                }



                if (valoisuusarvo > 50) {

                    Send6LoWPAN(IEEE80154_SERVER_ADDR,valoisuus_day, strlen(valoisuus_day)); //MSG2: light
                    StartReceive6LoWPAN();
                } else if (valoisuusarvo < 50) {
                    Send6LoWPAN(IEEE80154_SERVER_ADDR,valoisuus_night, strlen(valoisuus_night));
                    StartReceive6LoWPAN();
                }

            }

            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);  //Ask GYRO for values, save them to variables
            I2C_close(i2cMPU);

            if (programState != ACTIVATE) {
                leiki_data = leiki_check(&gx,&gy, &gz);          //Functions for checking if movement had happened, if treshold values have been broken,
                liiku_data = liiku_check(&ax, &ay, &az);			//leiki_data and liiku_data will return != 0
                syo_check(&az, &ay, &ax, &laskuri_az, &flag1);
                if (leiki_data != 0 || liiku_data != 0) {
                    programState = DATA_READY;                            //State change 
                    System_printf("programState is DATA_READY\n");
                    System_flush();
                }
            }

            else if (programState == ACTIVATE) {

                if (data_activate(&valoisuusarvo, &ax, &ay) == 1) {
                    activate_data = 1;
                    programState = DATA_READY;
                    System_printf("programState is DATA_READY\n");
                    System_flush();
                }
            }
        }

    if (programState == DATA_READY) { //Sending pet, move, eat, activate messages
        if (liiku_data != 0) {
            Send6LoWPAN(IEEE80154_SERVER_ADDR,liiku_viesti, strlen(liiku_viesti));
            buzz(1000);   //Notify the user a message has been sent
            liiku_data = 0; //Reset the variable
        }

        if (leiki_data != 0) {
            Send6LoWPAN(IEEE80154_SERVER_ADDR,leiki_viesti, strlen(leiki_viesti));
            buzz(1000);
            leiki_data = 0;
        }

       if (syo_data != 0) {
           Send6LoWPAN(IEEE80154_SERVER_ADDR,syo_viesti, strlen(syo_viesti));
           buzz(1000);
           syo_data = 0;
       }

       if (activate_data != 0) {
           Send6LoWPAN(IEEE80154_SERVER_ADDR,act_viesti, strlen(act_viesti));
           buzz(2000);
           buzz(1000);
           activate_data = 0;
       }
       StartReceive6LoWPAN();  //Set the radio back to recieve
       programState = COLLECT;
       System_printf("programState is COLLECT\n");
       System_flush();
    }
    sekunti++;            //10 times a sec this variable is incremented
    Task_sleep(100000 / Clock_tickPeriod); //100ms wait before the while loop is run again
    if (programState == SLEEP){
        while(flag_music == 1) { //Play music if state is sleep and music flag is set to 1
            buzzerOpen(buzzerHandle);
            musiikki();
            buzzerClose();
        }
    }
  }
}

//MAIN
int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Params commTaskParams;
    Task_Handle commTaskHandle;
	
    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();
    
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }

	
   buttonHandle = PIN_open(&buttonState, buttonConfig);
   if(!buttonHandle) {
      System_abort("Error initializing button pins\n");
   }

   button1Handle = PIN_open(&button1State, button1Config);
   if(!button1Handle) {
      System_abort("Error initializing button1 pins\n");
   }

   ledHandle = PIN_open(&ledState, ledConfig);
   if(!ledHandle) {
      System_abort("Error initializing LED pins\n");
   }

   buzzerHandle = PIN_open(&buzzerState, buzzerConfig);
   if (!buzzerHandle) {
       System_abort("Error initializing BUZZER pins\n");
   }


   if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
      System_abort("Error registering button callback function");
   }

   if (PIN_registerIntCb(button1Handle, &button1Fxn) != 0) {
        System_abort("Error registering button callback function");
   }

    /* Tasks */

   Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&commTaskParams);

    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority = 1;
    commTaskHandle = Task_create((Task_FuncPtr)commTask, &commTaskParams, NULL);
    if (commTaskHandle == NULL) {
       System_abort("Task create failed");
    }


	
    System_printf("Hello world!\n");
    System_flush();
    BIOS_start();

    return (0);
}
