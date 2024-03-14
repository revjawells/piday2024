#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pigpio.h>

/* default number of pi digits to calculate */
#define N 100

/* speed for blinking the LEDs */
#define DELAY 0.002

/* segment and digit arrays are for the GPIO pins on the raspberry pi */
#define NSEGMENTS 7
const int segment[] = {
	26, 19, 13, 6, 5, 0, 11
};

#define NDIGITS 4
const int digit[] = {
	25, 8, 7, 1
};

/* led array contains the patterns of lights to toggle for each digit */
const int leds[10][7] = {
	{1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 0, 0, 0, 0},
	{1, 1, 0, 1, 1, 0, 1},
	{1, 1, 1, 1, 0, 0, 1},
	{0, 1, 1, 0, 0, 1, 1},
	{1, 0, 1, 1, 0, 1, 1},
	{1, 0, 1, 1, 1, 1, 1},
	{1, 1, 1, 0, 0, 0, 0},
	{1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 0, 1, 1}
};

/* array to store digits of pi as they are calculated */
int *pi = NULL;
int max = 0;

/* setup - initialize GPIO library and set up the pins for use */
void setup(void)
{
	int i;

	if (gpioInitialise() == PI_INIT_FAILED) {
		printf("ERROR: Failed to initialize the GPIO interface.\n");
		exit(1);
	}

	for (i = 0; i < NSEGMENTS; i++) {
		gpioSetMode(segment[i], PI_OUTPUT);
		gpioWrite(segment[i], PI_HIGH);
	}

	for (i = 0; i < NDIGITS; i++) {
		gpioSetMode(digit[i], PI_OUTPUT);
		gpioWrite(digit[i], PI_LOW);
	}
}

/* cleanup - shut down GPIO library and pins when complete */
void cleanup(void)
{
	int i;

	for (i = 0; i < NSEGMENTS; i++)  {
		gpioWrite(segment[i], PI_HIGH);
		gpioSetMode(segment[i], PI_INPUT);
	}
	
	for (i = 0; i < NDIGITS; i++) {
		gpioWrite(digit[i], PI_LOW);
		gpioSetMode(digit[i], PI_INPUT);
	}

	gpioTerminate();
}

/* drip - store digits in the pi array */
void drip(int n)
{
	static int i = 0;

	if (i < max) {
		pi[i] = n;
		i = i + 1;
	}
}

/* show - display the marquee of four digits */
void show(int n)
{
	int i, j;

	for (j = 0; j < NDIGITS; j++) {
		/* turn on digit */
		gpioWrite(digit[j], PI_HIGH);

		/* error check - turn off all leds */
		if (pi[n + j] < 0 || pi[n + j] > 9) {
			for (i = 0; i < NSEGMENTS; i++) 
				gpioWrite(segment[i], PI_HIGH);
		} else {
			for (i = 0; i < NSEGMENTS; i++)
				if (leds[pi[n + j]][i])
					gpioWrite(segment[i], PI_LOW);
		}

		time_sleep(DELAY);

		/* turn off digit */
		gpioWrite(digit[j], PI_LOW);

		/* turn off segments */
		for (i = 0; i < NSEGMENTS; i++)
			gpioWrite(segment[i], PI_HIGH);
	}
}

/* spigot - calculate n digits of pi */
void spigot(int n)
{
	int *a;

	int i, j, k, q, x;

	int len = floor(10 * n / 3);
	int predigit = 0;
	int nines = 0;

	int trim = 2;

	a = (int *) malloc(len * sizeof(int));

	for (j = 0; j < len; j++)
		a[j] = 2;

	for (j = 0; j < n; j++) {
		q = 0;
		for(i = len; i > 0; i--) {
			x = 10 * a[i - 1] + q * i;	
			a[i - 1] = x % (2 * i - 1);
			q = floor(x / (2 * i - 1));
		}

		a[0] = q % 10;
		q = floor(q / 10);

		if (q == 9) {
			nines += 1;
		} else if (q == 10) {
			drip(predigit + 1);
			
			for (k = 0; k < nines; k++)
				drip(0);

			predigit = 0;
			nines = 0;
		} else {
			drip(predigit);
	
			predigit = q;
			if (nines != 0) {
				for (k = 0; k < nines; k++)
					drip(9);

				nines = 0;
			}
		}
	}

	drip(predigit);
}

int main(int argc, char **argv)
{
	int i, cycle;

	/* input how many digits to compute */
	if (argc < 2) 
		max = N;
	else
		max = atoi(argv[1]);

	/* generate list of digits */
	pi = (int *) malloc (max * sizeof (int));
	if (pi == NULL) {
		printf("malloc error\n");
		return 1;
	}

	for (i = 0; i < max; i++)
		pi[i] = -1;

	spigot(max);

	/* display as marquee */
	setup();
	for (i = 0; i < max - NDIGITS; i++) {
		cycle = 40;
		while(cycle--) {
			show(i);
			time_sleep(DELAY);
		}
	}

	cleanup();
	return 0;
}
