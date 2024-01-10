/*
 * Cligne_1_Hz.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
 * Aucun paramètre en entrée.
 *
 */

#include <stdio.h>
#include <bcm2835.h>

#define DEL_Rouge 20 // Définition du GPIO associé au DEL rouge (broche 38)
#define DEL_Orange 21 // Définition du GPIO associé au DEL rouge (broche 40)
#define BTN1 19 // Définition du GPIO associé au BTN1
#define BTN2 26 // Définition du GPIO associé au BTN1

int main(void)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Orange, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(BTN1, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour bouton-poussoir 2
	bcm2835_gpio_fsel(BTN2, BCM2835_GPIO_FSEL_INPT);
	
	// Clignoter DEL:
	// DEL allumée 500 ms et éteinte 500 ms
	while(bcm2835_gpio_lev(BTN1) || bcm2835_gpio_lev(BTN2)){
		bcm2835_gpio_write(DEL_Rouge, HIGH);
		bcm2835_gpio_write(DEL_Orange, LOW);
		bcm2835_delay(5000);
		bcm2835_gpio_write(DEL_Rouge, LOW);
		bcm2835_gpio_write(DEL_Orange, HIGH);
		bcm2835_delay(5000);
	}
	// Éteindre les deux LEDs a la sortie du programme
	bcm2835_gpio_write(DEL_Rouge, LOW);
	bcm2835_gpio_write(DEL_Orange, LOW);
	
	// Libérer les broches du GPIO
	bcm2835_close();
	return 0;
}
