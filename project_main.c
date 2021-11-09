//lol
/* C Standard library */
#include <stdio.h>
#include <time.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"

/* PIN */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>


/*Power PIN config for gyro*/

static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;

static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};



/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];

//Ohjelma aloittaa tilassa IDLE, napista painamalla siirryt��n tilaan COLLECT
enum state { SLEEP=1, COLLECT, DATA_READY};
enum state programState = SLEEP;

// JTKJ: Teht�v� 3. Valoisuuden globaali muuttuja
// JTKJ: Exercise 3. Global variable for ambient light
double ambientLight = -1000.0;


// RTOS:n globaalit muuttujat pinnien k�ytt��n
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

// Pinnien alustukset, molemmille pinneille oma konfiguraatio
// Vakio BOARD_BUTTON_0 vastaa toista painonappia
PIN_Config buttonConfig[] = { //Button-asetustaulukko
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, 
   PIN_TERMINATE 
};

PIN_Config ledConfig[] = { //Pin-asetustaulukko
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX, 
   PIN_TERMINATE 
};



void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
	//Napin k�sittelij�funktio
	uint_t pinValue = PIN_getOutputValue( Board_LED0 );
	pinValue = !pinValue;
	PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
	if (programState == SLEEP) {
		programState = COLLECT;
    	System_printf("programState is COLLECT\n");
    	System_flush();
	} else {
		programState = SLEEP;
	    System_printf("programState is SLEEP\n");
    	System_flush();
	}
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1) {

    // JTKJ: Teht�v� 4. Lis�� UARTin alustus: 9600,8n1
    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1

    while (1) {

        // JTKJ: Teht�v� 3. Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Print out sensor data as string to debug window if the state is correct
        //       Remember to modify state

        // JTKJ: Teht�v� 4. L�het� sama merkkijono UARTilla
        // JTKJ: Exercise 4. Send the same sensor data string with UART

        // Just for sanity check for exercise, you can comment this out
        //System_printf("uartTask\n");
        //System_flush();

        // Once per second, you can modify this
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    //GYRO


    float ax, ay, az, gx, gy, gz;

    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    //i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

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

    Task_sleep(100000 / Clock_tickPeriod);
    opt3001_setup(&i2cLUX);

    System_printf("OPT3001: Setup and calibration OK\n");
    System_flush();

    I2C_close(i2cLUX); //Opened and closed i2c for LUX

    System_printf("LUX and GYRO setup OK\n");
    System_flush();

    while (1) {
        if (programState != SLEEP) {

            float valoisuusarvo;
            char merkkijono_valoisuus[30];
            float liike;
            char merkkijono_liike[30];



            i2cLUX = I2C_open(Board_I2C, &i2cParamsLUX);    //Ask LUX for values
            valoisuusarvo = opt3001_get_data(&i2c);
            I2C_close(i2cLUX);

            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);  //Ask GYRO for values
            I2C_close(i2cMPU);



            sprintf(merkkijono_valoisuus, "%f   LUX\n", valoisuusarvo);
            sprintf(merkkijono_liike, "%f   GYRO\n", liike);


            System_printf(merkkijono_valoisuus);
            System_flush();
            System_printf(merkkijono_liike);
            System_flush();

            ambientLight = valoisuusarvo;

            Task_sleep(100000 / Clock_tickPeriod); //100ms
        }
    }
}

Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();
    
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }

    // JTKJ: Teht�v� 2. Ota i2c-v�yl� k�ytt��n ohjelmassa
    // JTKJ: Exercise 2. Initialize i2c bus
    // JTKJ: Teht�v� 4. Ota UART k�ytt��n ohjelmassa
    // JTKJ: Exercise 4. Initialize UART
	
	
   buttonHandle = PIN_open(&buttonState, buttonConfig);
   if(!buttonHandle) {
      System_abort("Error initializing button pins\n");
   }
   ledHandle = PIN_open(&ledState, ledConfig);
   if(!ledHandle) {
      System_abort("Error initializing LED pins\n");
   }

   // Asetetaan painonappi-pinnille keskeytyksen k�sittelij�ksi
   // funktio buttonFxn
   if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
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

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}

