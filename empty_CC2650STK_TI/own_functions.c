/*
 * own_functions.c
 *
 *  Created on: 11.11.2021
 *      Author: Eemil ja Veikko
 */

#include <math.h>
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
#include <ti/drivers/i2c/I2CCC26XX.h>

/* Board Header files */
#include "Board.h"

int laskuri_gx = 0;
int laskuri_akt = 0;
int flag2 = 0;

void syo_check(float *az, int *laskuri_az, int *flag1) {
    if (abs(*az + 1) >= 3) {
        *flag1 = 1;
    }
    if (*flag1 == 1) {
        if (*laskuri_az <= 20) {
            *laskuri_az = *laskuri_az + 1;
        }
        else {
            *laskuri_az = 0;
            *flag1 = 0;
        }
    }
}

/* Tarkistaa t�yttyyk� leikkimisen ja liikkumisen kynnysehdot */

int leiki_check(float *gx) {
    if (abs(*gx) >= 40) {
        laskuri_gx++;
        if (laskuri_gx > 10) {
            laskuri_gx = 0;
            System_printf("Leiki\n");
            System_flush();
            return 3;
        }
    } else {
        laskuri_gx = 0;
    }
    return 0;


}

int liiku_check(float *ax, float *ay) {

    if ((abs(*ax) >= 3) || (abs(*ay) >= 3)) {
        System_printf("Liiku\n");
        System_flush();
        return 3;
    } else {
        return 0;
    }

}

//77char cha[30];

int data_activate(float *valoisuusarvo, float *ax, float *ay) {
    if ((abs(*ax) >= 3) || (abs(*ay) >= 3)) {
        flag2 = 1;
        System_printf("Give me light....");
        System_flush();
    }
    if (flag2 == 1) {
        laskuri_akt++;
        if (laskuri_akt > 30){
            flag2 = 0;
            laskuri_akt = 0;
        }

        //sprintf(cha, "%f   LUX\n", *valoisuusarvo);
        //System_printf(cha);
        //System_flush();

        if ((laskuri_akt <= 30) && (*valoisuusarvo >= 2000.0)){
            System_printf("AKTIVOI\n");
            System_flush();
            laskuri_akt = 0;
            flag2 = 0;
            return 1;
        }
    } else {
        return 0;
    }
}
