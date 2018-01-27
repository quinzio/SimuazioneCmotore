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
#define MAX_SPEED 250.0
#define MIN_SPEED 90.0
#define INITIAL_SPEED 5.0
#define SPEED_INCREASE (MAX_SPEED / ACCEL_TIME)
#define ACCEL_TIME 7
#define PHASE_INIT 0
#define LU 74.7e-6
#define LV 74.7e-6
#define LW 74.7e-6
#define R 20.2e-3
#define AMP
#define AMP_OFFSET 0.4
#define PWM_LEN 1024
#define VBATT 13
#define SIM_TIME 15
#define DIODE 1.3
#define DEADTIME 5
#define INERTIA 15.1e-3
#define KE 5.03e-3
#define POLES 6
#define U_VECT (0.0 / 3 * 3.14159)
#define V_VECT (2.0 / 3 * 3.14159)
#define W_VECT (4.0 / 3 * 3.14159)
#define POWER_LOAD_AT_REF_SPEED -500.0
#define REF_SPEED (2*PI*250)
#define ROTOR_INITAL_ANGLE 0.4
#define IU_INITIAL 3

#define FILE_PATH1 "C:\\Users\\MUNARID\\Desktop\\data.txt"
#define FILE_PATH2 "data.txt"

int main(void) {
	// puts("Hi"); /* prints Hi */
	int i;

	unsigned int phase = PHASE_INIT;
	unsigned int phase_revs = 0;
	unsigned int dphase = (unsigned int) ((double) PWM_LEN / FREQ * (1 << 16)
			* (1 << 16));
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
	double iu = IU_INITIAL;
	double iv = 0;
	double iw = 0;
	double rotorAngle = ROTOR_INITAL_ANGLE;
	double rotorSpeed = 0;
	double rotorAlpha = 0;
	double bemfU;
	double bemfV;
	double bemfW;
	double deltaAngle_rad = 0;
	double speed_target;
	double speed_target_smooth = 0;
	double speed_target_tod = 0;
	double stator_voltage;
	double currentAngle = 0;
	double currentAnglerevs = 0;
	double currentYold = 0;
	double currentX = 0;
	double currentY = 0;
	double statotVoltageReg = 0;
	double powersunkFilter = 0;
	double powersunk = 0;
	double todGain = 0;

	FILE* data_file;

	printf("%d\n", sizeof(unsigned int));
	printf("%d\n", sizeof(unsigned short));
	printf("%d\n", sizeof(unsigned char));

	data_file = fopen(FILE_PATH2, "w+");

	for (i = 0; i < SIM_TIME * FREQ; i++) {
		sineU_ix = phase >> 22;
		sineV_ix = (phase - 0x55555555) >> 22;
		sineW_ix = (phase - 0xAAAAAAAA) >> 22;

		speed_target = INITIAL_SPEED + SPEED_INCREASE * i / FREQ;

		if (speed_target > MIN_SPEED + 5)
			speed_target = MIN_SPEED + 5;

		speed_target_smooth += 0.3 * (speed_target - speed_target_smooth) * 2
				* PI / FREQ;

		// Regolatore tensione statore
		if (speed_target >= MIN_SPEED) {
			if (stator_voltage * sqrt(3) * 512 / 700 < VBATT) {
				statotVoltageReg += 0.5 * (currentAngle - (rotorAngle + PI / 2))
						* 2 * PI / FREQ;
			}
		}
		// TOD
		speed_target_tod = speed_target_smooth - 0.5 * deltaAngle_rad;

		stator_voltage = KE * rotorSpeed + AMP_OFFSET + statotVoltageReg;

		sineU_mod = floor(
				cos(2 * PI * sineU_ix / PWM_LEN) / (VBATT / 2)
						* (stator_voltage / sqrt(1)) * (PWM_LEN / 2));
		sineV_mod = floor(
				cos(2 * PI * sineV_ix / PWM_LEN) / (VBATT / 2)
						* (stator_voltage / sqrt(1)) * (PWM_LEN / 2));
		sineW_mod = floor(
				cos(2 * PI * sineW_ix / PWM_LEN) / (VBATT / 2)
						* (stator_voltage / sqrt(1)) * (PWM_LEN / 2));

		if (stator_voltage * sqrt(3) < VBATT) {
			minphase = MIN(sineU_mod, sineV_mod);
			minphase = MIN(minphase, sineW_mod);

			sineddpmU = sineU_mod - minphase;
			sineddpmV = sineV_mod - minphase;
			sineddpmW = sineW_mod - minphase;
		} else {
			sineddpmU = sineU_mod + PWM_LEN / 4;
			sineddpmV = sineV_mod + PWM_LEN / 4;
			sineddpmW = sineW_mod + PWM_LEN / 4;
		}
		if ((i % (PWM_LEN / 2)) < (sineddpmU - DEADTIME))
			sineU_bri = VBATT;
		else if ((i % (PWM_LEN / 2)) > (sineddpmU + DEADTIME))
			sineU_bri = 0;
		else if (iu < 0)
			sineU_bri = VBATT + DIODE;
		else
			sineU_bri = -DIODE;

		if ((i % (PWM_LEN / 2)) < (sineddpmV - DEADTIME))
			sineV_bri = VBATT;
		else if ((i % (PWM_LEN / 2)) > (sineddpmV + DEADTIME))
			sineV_bri = 0;
		else if (iv < 0)
			sineV_bri = VBATT + DIODE;
		else
			sineV_bri = -DIODE;

		if ((i % (PWM_LEN / 2)) < (sineddpmW - DEADTIME))
			sineW_bri = VBATT;
		else if ((i % (PWM_LEN / 2)) > (sineddpmW + DEADTIME))
			sineW_bri = 0;
		else if (iw < 0)
			sineW_bri = VBATT + DIODE;
		else
			sineW_bri = -DIODE;

		// Potenza assorbita. Diodi di ricircolo !!!
		powersunk = sineU_bri * iu + sineV_bri * iv + sineW_bri * iw;
		powersunkFilter += 30 * (powersunk - powersunkFilter) / FREQ;

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
		rotorAlpha = sin(U_VECT - rotorAngle) * iu * KE;
		rotorAlpha += sin(V_VECT - rotorAngle) * iv * KE;
		rotorAlpha += sin(W_VECT - rotorAngle) * iw * KE;
		rotorAlpha += (POWER_LOAD_AT_REF_SPEED / REF_SPEED / REF_SPEED
				/ REF_SPEED) * rotorSpeed * rotorSpeed;
		rotorAlpha += -0.001 * (rotorSpeed > 0 ? 1 : -1);
		rotorAlpha *= ((double) POLES * POLES / INERTIA);

		rotorSpeed += (rotorAlpha / FREQ);
		rotorAngle += (rotorSpeed / FREQ);

		if ((i % PWM_LEN) == 0) {
			dphase_inc = ((float) dphase * speed_target_tod);
			if (0xFFFFFFFF - phase < dphase_inc)
				phase_revs++;
			phase += dphase_inc;
		}

		deltaAngle_rad = 2 * PI
				* (phase_revs + (double) phase / (1 << 16) / (1 << 16))
				- rotorAngle;

		currentX = iu * cos(U_VECT) + iv * cos(V_VECT) + iw * cos(W_VECT);
		currentYold = currentY;
		currentY = iu * sin(U_VECT) + iv * sin(V_VECT) + iw * sin(W_VECT);

		if (currentX < 0) {
			if (currentYold > 0 && currentY < 0)
				currentAnglerevs++;
			if (currentYold < 0 && currentY > 0)
				currentAnglerevs--;
		}
		currentAngle = atan2(currentY, currentX) + 2 * PI * currentAnglerevs;

		if ((i % 40000) == 0) {
			printf("%d\n", i);
		}

		if ((i % 4000) == 0) {
			// printf("%d %d %d\n", sineU_mod, sineU_mod, sineU_mod);
			// fprintf(data_file, " %f %d %d %d\n", i / FREQ, sineddpmU, sineddpmV,
			// sineddpmW);
			// printf("%f %f %f\n", iu, iv, iw);
			fprintf(data_file, "%f %f %f %f %f %f %f\n", i / FREQ, iu,
					stator_voltage, speed_target - speed_target_smooth,
					rotorSpeed, currentAngle - rotorAngle, deltaAngle_rad);
		}
		fflush(stdout);
	}
	fclose(data_file);
	printf("%d\n", dphase);

	return EXIT_SUCCESS;
}
