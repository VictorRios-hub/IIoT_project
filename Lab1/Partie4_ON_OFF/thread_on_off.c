/*
 * Thread_Cligne_10_Hz_time.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 10 Hz.
 * Le clignotement se fait à l'intérieur d'un thread et le délai est
 * ajusté avec des fonctions de la librarie "time.h".
 * L'arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define MISO 23 // Master input Slave output : Rasp input MAX output
#define CS 24 // Chip select
#define SCK 25 // signal d'horloge

#define PWM 18 // PWM broche
// freauence de base du PWM a 19,2 MHz
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

double frequence = 1; // Hertz : fréquence de clignotement désirée

int commande_Thermo(double consigne, double mesure)
{
	int commande = 0 ;
	
	if(mesure < consigne)
		commande = range;
	
	else
		commande = 0;
	
	
	return commande;
}

void intensity(int commande)
{
		
	// Duty cycle desire
	bcm2835_pwm_set_data(0,commande);
	
}

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
	
	// Fréquence réglé a 1 Hz soit une periode de 1000 ms
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

/* Fonction clignote qui sera appelée par le thread "cligne"
 * 
 * Elle fait clignoter le DEL rouge à 1 Hertz.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *clignote()
{
	clock_t debut, fin; // Variables de temps
	double demiPer = 1/(2.0*frequence); // Calcul de la demi-période
	int etat = 0; // État du DEL
	
	debut = clock(); // Temps écoulé depuis le lancement du programme
	
	// Boucle infinie pour le clignotement du DEL
	while(1){
		if (etat==0){
			bcm2835_gpio_write(20, HIGH);
			etat = 1;
			intensity(commande_Thermo(30,lecture_SPI()));
		}
		
		else{
			bcm2835_gpio_write(20, LOW);
			etat = 0;
		}
		
		fin = clock(); // Temps écoulé depuis le lancement du programme
		
		// La différence (fin-debut) donne le temps d'exécution du code
		// On cherche une durée égale à demiPer. On compense avec un délai
		// CLOCK_PER_SEC est le nombre de coups d'horloges en 1 seconde
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		debut = fin;
	}
	pthread_exit(NULL);
}



/* Fonction main
 * 
 * Elle configure le GPIO et le thread.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
int main(int argc, char **argv)
{
	// Identificateur du thread
	pthread_t cligne;  
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
		// Configuration du PWM
	bcm2835_gpio_fsel(PWM, BCM2835_GPIO_FSEL_ALT5);

	// Inititialisation du PWM
	bcm2835_pwm_set_clock(diviseur);	// freq de base
	bcm2835_pwm_set_mode(0,1,1);			// mode du PWM
	bcm2835_pwm_set_range(0, range);	// plage ajustement
	bcm2835_pwm_set_data(0,0);			// PWM duty cycle == duree d'impulsion
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
	
	// Création du thread "cligne".
	// Lien avec la fonction clignote.
	// Cette dernière n'exige pas de paramètres.
	pthread_create(&cligne, NULL, &clignote, NULL);
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while(bcm2835_gpio_lev(19)){
		usleep(1000); // Délai de 1 ms !!!
	}

	// Si bouton-poussoir enfoncé, arrêt immédiat du thread 
    pthread_cancel(cligne);
    // Attente de l'arrêt du thread
    pthread_join(cligne, NULL);
    
    
    
    // Éteindre le DEL rouge
    bcm2835_gpio_write(20, LOW);
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
