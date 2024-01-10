#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define PWM 18 // PWM broche
// freauence de base du PWM a 19,2 MHz
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

int commande_Thermo(double consigne, double mesure)
{
	int commande = 0 ;
	
	if(mesure < consigne)
		commande = range;
	
	else
		commande = 0;
	
	
	return commande;
}


int main(int argc, char **argv)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	printf("%d \n", commande_Thermo(30,15));
	printf("%d \n", commande_Thermo(15,30));

	
	// Libérer le GPIO
    bcm2835_close();
    return 0;
}
