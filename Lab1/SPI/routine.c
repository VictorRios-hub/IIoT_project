#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

/* Fonction lecture_SPI(void) qui sera appelée par le thread "cligne"
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

double frequence = 1; // Hertz : fréquence de refresh désirée
int data = 0;

double lecture_SPI(void)
{
	clock_t debut, fin; // Variables de temps
	double demiPer = 1/(2.0*frequence); // Calcul de la demi-période
	
	// Configuration du GPIO MISO input
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO CS output
	bcm2835_gpio_fsel(CS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CS, HIGH);
	// Configuration du GPIO SCK output
	bcm2835_gpio_fsel(SCK, BCM2835_GPIO_FSEL_OUTP);
	
	debut = clock(); // Temps écoulé depuis le lancement du programme
	
	// Initialisation de la lecture
	bcm2835_gpio_write(CS, LOW);
	bcm2835_delayMicroseconds(1000);
	
	// Fréquence réglé a 1 Hz soit une periode de 1000 ms
	for (int i = 0; i < 32; i++)
	{
		bcm2835_gpio_write(SCK, HIGH);
			
		fin = clock(); // Temps écoulé depuis le lancement du programme
			
		// La différence (fin-debut) donne le temps d'exécution du code
		// On cherche une durée égale à demiPer. On compense avec un délai
		// CLOCK_PER_SEC est le nombre de coups d'horloges en 1 seconde
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		debut = fin;
	}

	
	// Find de la lecture
	bcm2835_gpio_write(CS, HIGH);
	bcm2835_delayMicroseconds(1000);
}
