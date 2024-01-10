#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

/* Fonction lecture_SPI(void) qui sera appelée par le thread ""
 * 
 * Elle lit la température du thermocouple
 *
 * La fonction retournera une valeur de type double 
 * correspondant a la température du thermocouple en celsius.
 * 
 * La fonction n'exige aucun paramètre en entrée.
 */

#define MISO 23 // Master input Slave output : Rasp input MAX output
#define CS 24 // Chip select
#define SCK 25 // signal d'horloge

double lecture_SPI(void)
{
	
	// Configuration du GPIO MISO input
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO CS output
	bcm2835_gpio_fsel(CS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CS, HIGH);
	// Configuration du GPIO SCK output
	bcm2835_gpio_fsel(SCK, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(SCK, LOW);
	
	
	// Initialisation de la lecture
	bcm2835_gpio_write(CS, LOW);
	bcm2835_delayMicroseconds(1000);
	bcm2835_gpio_write(SCK, HIGH);
	// short int flag = 1;
	int buffer = 0x00;
	
	// Récupération de la trame 32 bits
	for (int i = 0; i < 32; i++)
	{
		// Signaux d'horloge

		bcm2835_gpio_write(SCK, HIGH);	
		buffer = (buffer << 1) + bcm2835_gpio_lev(MISO);
		// printf("%d",bcm2835_gpio_lev(MISO));
		bcm2835_gpio_write(SCK, LOW);
		bcm2835_delayMicroseconds(10000); // 10 ms
		
	}
	//Recuperation de la temperature
	
	// Temperature
	buffer = (buffer >> 18) ;
	double temp = (float)buffer/4;
	
	printf("%.2f \n",temp);
	
	// Fin de la lecture
	bcm2835_gpio_write(CS, HIGH);
	bcm2835_delayMicroseconds(10000); // 10 ms
	
	return temp;
}

int main(int argc, char **argv)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	while(1)
	{
		lecture_SPI();
	}
	// Libérer le GPIO
    bcm2835_close();
    return 0;
}
