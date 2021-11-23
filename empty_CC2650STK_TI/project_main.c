/*A sensortag program for tamagotchi. The program collects and sends data with UART to gateway program, 
which then sends the data forward to a backend server through internet.

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
#include "buzzer.h"
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <own_functions.h>
#include <wireless/comm_lib.h>

//Global variables
int laskuri_az = 0;
int flag1 = 0;
int leiki_data = 0;
int liiku_data = 0;
int syo_data = 0;
int activate_data = 0; 
float valoisuusarvo;
char merkkijono_valoisuus[30];
char merkkijono_liike[30];
int sekunti = 1;
double ambientLight = 123123;

char liiku_viesti[] = "id:44,EXERCISE:3\0";
char leiki_viesti[] = "id:44,PET:2\0";
char syo_viesti[] = "id:44,EAT:2\0";
char act_viesti[] = "id:44,ACTIVATE:4;4;4\0";



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


Void buzz(int freq) {
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
/*
static void uartFxn(UART_Handle uart, char *uartBuffer, size_t len) {
    char *result;
    const char address[] = "44,BEEP";

    // System_printf(uartBuffer);
    // System_flush();

    result = strstr(uartBuffer, address);
    if (result != NULL) {
        System_printf("viesti\n");
        System_flush();
        // buzz(4000);
    }

    UART_read(uart, uartBuffer, 80);
}
*/


static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
		uint_t pinValue = PIN_getOutputValue( Board_LED0 );
		pinValue = !pinValue;
		PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
		if (programState == SLEEP) {
			programState = COLLECT;
		System_printf("programState is COLLECT\n");
		System_flush();
		}
		else {
			programState = SLEEP;
		    System_printf("programState is SLEEP\n");
		System_flush();
		}
}

Void button1Fxn(PIN_Handle handle, PIN_Id pinId) {
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
        System_printf("MUSIIKKI POIS\n");
        System_flush();
    }
}

Void commTask(UArg arg0, UArg arg1) {

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


           res = strstr(payload, address);
           if (res != NULL) {
               System_printf("viesti\n");
               System_flush();
               buzz(2000);
           }

         }
      }
}

/*Void myTaskFxn(UArg arg0, UArg arg1) {

   // Taskin ikuinen elämä
   while (1) {
      if (programState == DATA_READY) {








      }
      Task_sleep(100000 / Clock_tickPeriod);
   }
}*/

/* Task Functions */
/*
void uartTaskFxn(UArg arg0, UArg arg1) {


    char liiku_viesti[22] = "id:44,EXERCISE:3\0";
    char leiki_viesti[15] = "id:44,PET:2\0";
    char syo_viesti[15] = "id:44,EAT:2\0";
    char act_viesti[25] = "id:44,ACTIVATE:4;4;4\0";

    UART_Handle uart;
    UART_Params uartParams;

    //UARTin alustus: 9600,8n1

    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.readMode=UART_MODE_CALLBACK;
    uartParams.readCallback  = &uartFxn;
    uartParams.baudRate = 9600; // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1

    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL) {
       System_abort("Error opening the UART");
    }
    else {
        System_printf("UART OK\n");
        System_flush();
    }
    UART_read(uart, uartBuffer, 80);

    while (1) {
        if (programState == DATA_READY) {

            if (liiku_data != 0) {
                UART_write(uart,liiku_viesti, strlen(liiku_viesti)+1);
                liiku_data = 0;
            }

            if (leiki_data != 0) {
                UART_write(uart,leiki_viesti, strlen(leiki_viesti)+1);
                leiki_data = 0;
            }

            if (syo_data != 0) {
                UART_write(uart,syo_viesti, strlen(syo_viesti)+1);
                syo_data = 0;
            }

            if (activate_data != 0) {
                UART_write(uart,act_viesti, strlen(act_viesti)+1);
                activate_data = 0;
            }

            //printf(teksti, "%f\n\r", ambientLight);
            //UART_write(uart,teksti, strlen(teksti));
            programState = COLLECT;
            System_printf("ProgramState is COLLECT\n");
            System_flush();
        }

        // Once per second, you can modify this
        Task_sleep(100000 / Clock_tickPeriod);
    }
}
*/
void sensorTaskFxn(UArg arg0, UArg arg1) {

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

    System_printf("LUX and GYRO setup OK\n");
    System_flush();


    while (1) {
        if (programState == COLLECT || programState == ACTIVATE) {


            if ( sekunti % 10 == 0) {
                i2cLUX = I2C_open(Board_I2C, &i2cParamsLUX);    //Ask LUX for values
                valoisuusarvo = opt3001_get_data(&i2cLUX);
                I2C_close(i2cLUX);

            }

            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);  //Ask GYRO for values
            I2C_close(i2cMPU);

            if (programState != ACTIVATE) {
                leiki_data = leiki_check(&gx);
                liiku_data = liiku_check(&ax, &ay);
                syo_check(&az, &laskuri_az, &flag1);
                if (leiki_data != 0 || liiku_data != 0) {
                    programState = DATA_READY;
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

            //sprintf(merkkijono_liike, "%.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", ax, ay, az, gx, gy, gz);

            ////System_printf(merkkijono_liike);
            ///System_flush();

        }

    if (programState == DATA_READY) {

        if (liiku_data != 0) {
            Send6LoWPAN(IEEE80154_SERVER_ADDR,liiku_viesti, strlen(liiku_viesti));
            buzz(1000);
            liiku_data = 0;
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
       StartReceive6LoWPAN();
       programState = COLLECT;
       System_printf("programState is COLLECT\n");
       System_flush();
    }
    sekunti++;
    Task_sleep(100000 / Clock_tickPeriod); //100ms
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
    //Board_initUART();
    
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


   // Asetetaan painonappi-pinnille keskeytyksen k�sittelij�ksi
   // funktio buttonFxn
   if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
      System_abort("Error registering button callback function");
   }

   if (PIN_registerIntCb(button1Handle, &button1Fxn) != 0) {
        System_abort("Error registering button callback function");
   }


    /* Task */

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
/*

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }
*/

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}

