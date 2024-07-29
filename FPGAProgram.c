/* John Dolph
	12/14/22

	Final Program to read ADC and write values to Color LED
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

//Error message definitions
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE sysconf(_SC_PAGE_SIZE)
#define MAP_MASK (MAP_SIZE - 1)

//Component Register Offsets
#define LED_RED 0x0
#define LED_GREEN 0x4
#define LED_BLUE 0x8
#define POT_RED 0x8
#define POT_GREEN 0xC
#define POT_BLUE 0x10

//Period and Duty cycle values
#define F_PERIOD 6
#define F_DUTY_CYCLE 4

//variables for reading and writing
int file_descriptor; //file descriptor for /dev/mem
void *map_base, *virtual_address;
unsigned long read_value, write_value;

off_t hps_data_save_base_target = 0xff204100; //base address in physical memory to write to Registers
off_t hps_color_led_base_target = 0xff204120; //base address in physical memory to write to Registers
off_t adc_base_target = 0xff204140; //base address in physical memory to write to Registers

static volatile int looping = 1;

//handler function for ctrl+c break
void loopHandler(int dummy) {
    looping = 0;
}

// reads data from the register specified by 'target'
int readRegister(off_t target)
{
	//open file_descriptor
	if((file_descriptor = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;

	/* Map one page */
	//create physical address mapping (target & ~MAP_MASK makes sure a full page on a page boundary)
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, target & ~MAP_MASK);
	if(map_base == (void *) -1) FATAL;

	virtual_address = map_base + (target & MAP_MASK); // add offset back when accessing address using page boundary of mapped page
	read_value = *((unsigned long *) virtual_address);

	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(file_descriptor);

    return read_value;
}

// writes 'value' to the register specified by 'target'
void writeRegister(off_t target, off_t value)
{
	//open file_descriptor
	if((file_descriptor = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;

	/* Map one page */
	//create physical address mapping (target & ~MAP_MASK makes sure a full page on a page boundary)
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, target & ~MAP_MASK);
	if(map_base == (void *) -1) FATAL;

	virtual_address = map_base + (target & MAP_MASK); // add offset back when accessing address using page boundary of mapped page

	*((unsigned long *) virtual_address) = value;

	read_value = *((unsigned long *) virtual_address);

	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(file_descriptor);
}

// reads raw data from a specified potentiometer
int getPotentiometer(off_t target) {

    target += adc_base_target;
	return readRegister(target);
}

// sets a duty cycle for a specified LED color on hps_color_led
int setPWM(off_t target, off_t value) {

    target += hps_color_led_base_target;
	value = (int)value << F_DUTY_CYCLE;
	writeRegister(target, value);
}

// writes to the ADC update registers
int setUpdate(off_t target, off_t value) {

    target += adc_base_target;
	writeRegister(target, value);
}

// sets the hps_color_led PWM component period (to reduce flickering at low duty cycles)
void setPeriod(off_t value) {

    off_t target = hps_color_led_base_target + 0xC;
	value = value << F_PERIOD;
	writeRegister(target, value);
}

// limits the converted potentiometer duty cycles to 0-100% range
float limitDutyCycle(float dutyCycle)
{
	if(dutyCycle < 0.0)
		dutyCycle = 0.0;
	else if(dutyCycle > 100.0)
		dutyCycle = 100.0;

	return dutyCycle;
}

int main(int argc, char **argv) {

	//values for potentiometer conversion
    float potOne, potTwo, potThree;
	float minPotentiometerValue = 0.0;
	float maxPotentimeterValue = 4095.0;
	float scaleSlope = 100.0 / maxPotentimeterValue;
	float scaleIntercept = -minPotentiometerValue * scaleSlope;

	//set period
	int period = 20;
	setPeriod(period);

	//signal for ctrl+c break
	signal(SIGINT, loopHandler);
	printf("Update Color program running.. Press CTRL+C to exit.\n");

    // read potetiometer values from adc and perform conversions/scalings
    // write control words to associated color PWM
	while(looping)
	{	
		potOne = getPotentiometer(POT_RED);
		potTwo = getPotentiometer(POT_GREEN);
		potThree = getPotentiometer(POT_BLUE);

		//potentiometer conversions
		float redDutyCycle = potOne * scaleSlope + scaleIntercept;
		float greenDutyCycle = potTwo * scaleSlope + scaleIntercept;
		float blueDutyCycle = potThree * scaleSlope + scaleIntercept;
		
		//limit values to 0-100 in case calibration is off
		redDutyCycle = limitDutyCycle(redDutyCycle);
		greenDutyCycle = limitDutyCycle(greenDutyCycle);
		blueDutyCycle = limitDutyCycle(blueDutyCycle);

		//write values to PWM controller and shift by F_duty_cycle
        setPWM(LED_RED, redDutyCycle);
        setPWM(LED_GREEN, greenDutyCycle);
        setPWM(LED_BLUE, blueDutyCycle);
	}

	// turn off LED on program end
	setPWM(LED_RED, 0);
	setPWM(LED_GREEN, 0);
	setPWM(LED_BLUE, 0);

    return 0;
}
