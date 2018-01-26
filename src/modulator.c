/*
 ============================================================================
 Name        : modulator.c
 Author      : md
 Version     :
 Copyright   : SPAL
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define PI 3.14159
#define FREQ 20e6
#define MAX_SPEED 120.0
#define LU 74.7e-6
#define LV 74.7e-6
#define LW 74.7e-6
#define R 15e-3
#define AMP 0.6
#define AMP_OFFSET 0.2
#define PWM_LEN 1024
#define VBATT 13
#define SIM_TIME 3
#define DIODE 1.3
#define DEADTIME 10
#define INERTIA 15e-3
#define KE 5.03e-3
#define POLES 6
#define U_VECT (0.0 / 3 * 3.14159)
#define V_VECT (2.0 / 3 * 3.14159)
#define W_VECT (4.0 / 3 * 3.14159)
#define FILE_PATH1 "C:\\Users\\MUNARID\\Desktop\\data.txt"
#define FILE_PATH2 "data.txt"

int main(void) {
	// puts("Hi"); /* prints Hi */
	int i;

	unsigned int phase = 0;
	unsigned int phase_revs = 0;
	unsigned int dphase = (unsigned int) (MAX_SPEED / FREQ * PWM_LEN * (1 << 16)
			* (1 << 16));  // 90.0
	double dphase_inc;
	unsigned short sineU_ix = 0;
	unsigned short sineV_ix = 0;
	unsigned short sineW_ix = 0;
	short sineU_mod = 0;
	short sineV_mod = 0;
	short sineW_mod = 0;
	float sineU_bri = 0;
	float sineV_bri = 0;
	float sineW_bri = 0;
	short sineddpmU = 0;
	short sineddpmV = 0;
	short sineddpmW = 0;
	short minphase = 0;
	double diu = 0;
	double div = 0;
	double diw = 0;
	double iu = 0;
	double iv = 0;
	double iw = 0;
	double rotorAngle = 0;
	double rotorSpeed = 0;
	double rotorAlpha = 0;
	double bemfU;
	double bemfV;
	double bemfW;
	double deltaAngle_rad = 0;
	double speed_target;
	double stator_voltage;

	FILE* data_file;

	printf("%d\n", sizeof(unsigned int));
	printf("%d\n", sizeof(unsigned short));
	printf("%d\n", sizeof(unsigned char));

	data_file = fopen(FILE_PATH2, "w+");

	for (i = 0; i < SIM_TIME * FREQ; i++) {
		sineU_ix = phase >> 22;
		sineV_ix = (phase - 0x55555555) >> 22;
		sineW_ix = (phase - 0xAAAAAAAA) >> 22;

		speed_target = i / (SIM_TIME * FREQ);
		if (speed_target > 0.666)
			speed_target = 0.666;
		stator_voltage = speed_target + AMP_OFFSET;

		sineU_mod = floor(
				(cos(2 * 3.14159 * sineU_ix / PWM_LEN) + 1) / 2 * AMP *
				PWM_LEN * stator_voltage);
		sineV_mod = floor(
				(cos(2 * 3.14159 * sineV_ix / PWM_LEN) + 1) / 2 * AMP *
				PWM_LEN * stator_voltage);
		sineW_mod = floor(
				(cos(2 * 3.14159 * sineW_ix / PWM_LEN) + 1) / 2 * AMP *
				PWM_LEN * stator_voltage);

		minphase = MIN(sineU_mod, sineV_mod);
		minphase = MIN(minphase, sineW_mod);

		sineddpmU = sineU_mod - minphase;
		sineddpmV = sineV_mod - minphase;
		sineddpmW = sineW_mod - minphase;

		if ((i % PWM_LEN) < (sineddpmU - DEADTIME))
			sineU_bri = VBATT;
		else if ((i % PWM_LEN) > (sineddpmU + DEADTIME))
			sineU_bri = 0;
		else if (iu < 0)
			sineU_bri = VBATT + DIODE;
		else
			sineU_bri = -DIODE;

		if ((i % PWM_LEN) < (sineddpmV - DEADTIME))
			sineV_bri = VBATT;
		else if ((i % PWM_LEN) > (sineddpmV + DEADTIME))
			sineV_bri = 0;
		else if (iv < 0)
			sineV_bri = VBATT + DIODE;
		else
			sineV_bri = -DIODE;

		if ((i % PWM_LEN) < (sineddpmW - DEADTIME))
			sineW_bri = VBATT;
		else if ((i % PWM_LEN) > (sineddpmW + DEADTIME))
			sineW_bri = 0;
		else if (iw < 0)
			sineW_bri = VBATT + DIODE;
		else
			sineW_bri = -DIODE;

		// Rotor

		bemfU = sin(U_VECT - rotorAngle) * KE * rotorSpeed;
		bemfV = sin(V_VECT - rotorAngle) * KE * rotorSpeed;
		bemfW = sin(W_VECT - rotorAngle) * KE * rotorSpeed;

		diu = ((sineU_bri - 1.5 * R * iu - bemfU) / (1.5 * LU) / FREQ);
		div = ((sineV_bri - 1.5 * R * iv - bemfV) / (1.5 * LV) / FREQ);
		diw = ((sineW_bri - 1.5 * R * iw - bemfW) / (1.5 * LW) / FREQ);

		iu += diu - (div + diw) / 2;
		iv += div - (diw + diu) / 2;
		iw += diw - (diu + div) / 2;

		// Rotor
		rotorAlpha = sin(U_VECT - rotorAngle) * iu * KE / INERTIA * POLES
				* POLES;
		rotorAlpha += sin(V_VECT - rotorAngle) * iv * KE / INERTIA * POLES
				* POLES;
		rotorAlpha += sin(W_VECT - rotorAngle) * iw * KE / INERTIA * POLES
				* POLES;

		rotorSpeed += (rotorAlpha / FREQ);
		rotorAngle += (rotorSpeed / FREQ);

		if ((i % PWM_LEN) == 0) {
			dphase_inc = ((float) dphase * speed_target);
			if (0xFFFFFFFF - phase < dphase_inc)
				phase_revs++;
			phase += dphase_inc;
		}

		deltaAngle_rad = 2 * PI
				* (phase_revs + (double) phase / (1 << 16) / (1 << 16))
				- rotorAngle;



		if ((i % 400) == 0) {
			// printf("%d %d %d\n", sineU_mod, sineU_mod, sineU_mod);
			// fprintf(data_file, " %f %d %d %d\n", i / FREQ, sineddpmU, sineddpmV,
			// sineddpmW);
			// printf("%f %f %f\n", iu, iv, iw);
			fprintf(data_file, "%f %f %f %f %f %f %f\n", i / FREQ, iu, iv, iw,
					rotorAngle, rotorSpeed, deltaAngle_rad);
		}
		fflush(stdout);
	}
	fclose(data_file);
	printf("%d\n", dphase);

	return EXIT_SUCCESS;
}
