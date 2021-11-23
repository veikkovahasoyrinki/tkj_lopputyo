/*
 * own_functions.h
 *
 *  Created on: 11.11.2021
 *      Author: Eemil ja Veikko
 */

#ifndef OWN_FUNCTIONS_H_
#define OWN_FUNCTIONS_H_

int liiku_check(float *ax, float *ay, float *az);
int data_activate(float *valoisuusarvo, float *ax, float *ay);
void syo_check(float *az,float *ay, float *ax, int *laskuri_az, int *flag1);
int leiki_check(float *gx, float *gy, float *gz);

#endif /* OWN_FUNCTIONS_H_ */
