/*
 * publieur_MQTT.c
 * 
 * 
 * Lokman Sboui et Guy Gauthier
 * 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include <bcm2835.h>
#include <time.h>
#include <pthread.h>

#define MISO 23 // Master input Slave output : Rasp input MAX output
#define CS 24 // Chip select
#define SCK 25 // signal d'horloge

#define MQTT_HOSTNAME "localhost"
#define MQTT_PORT 1883
#define MQTT_TOPIC "sujet"

double frequence = 0.5; // Hertz : fréquence de clignotement désirée T = 2s

/* Fonction lecture_SPI(void) qui sera appelée dans la fonction *clignote() par le thread "clignote"
 * 
 * Elle lit la température du thermocouple à travers le protocole SPI 
 * et l'affiche sur le terminal
 *
 * La fonction retournera une valeur de type double 
 * correspondant a la température du thermocouple en celsius.
 * 
 * La fonction n'exige aucun paramètre en entrée.
 */
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
	
	// printf("temperature mesuree : %.2f \n",temp);
	
	// Fin de la lecture
	bcm2835_gpio_write(CS, HIGH);
	bcm2835_delayMicroseconds(10000); // 10 ms
	
	return temp;
}

/* Fonction publish qui sera appelée par le thread "pub"
 * 
 * 
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *publish()
{
	clock_t debut, fin; // Variables de temps
	double demiPer = 1/(2.0*frequence); // Calcul de la demi-période
	int etat = 0; // État du DEL
	
	debut = clock(); // Temps écoulé depuis le lancement du programme
	
	char text[40];
	int ret;
	struct mosquitto *mosq = NULL;
	
	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq)
	{
		fprintf(stderr,"Ne peut initialiser la librairie de Mosquitto\n");
		exit(-1);
	}
	
	ret = mosquitto_connect(mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
	if (ret)
	{
		fprintf(stderr,"Ne peut se connecter au serveur Mosquitto\n");
		exit(-1);
	}
	
	// Boucle infinie pour le clignotement du DEL
	while(1){
		if (etat==0){

			// Recuperation de la temperature
			double temp = lecture_SPI();
			sprintf(text, "%5.2f C",temp);
		
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(text), text, 0, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
		}
		else{

			etat = 0;
		}
		
		fin = clock(); // Temps écoulé depuis le lancement du programme
		
		// La différence (fin-debut) donne le temps d'exécution du code
		// On cherche une durée égale à demiPer. On compense avec un délai
		// CLOCK_PER_SEC est le nombre de coups d'horloges en 1 seconde
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		debut = fin;
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	// Identificateur du thread
	pthread_t pub;  
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour bouton-poussoir 2
	bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT);
	
	// Création du thread "pub".
	pthread_create(&pub, NULL, &publish, NULL);
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while(bcm2835_gpio_lev(26))
	{
		usleep(1000); // Délai de 1 ms !!!
	}
	
	// Si bouton-poussoir enfoncé, arrêt immédiat du thread 
    pthread_cancel(pub);
    // Attente de l'arrêt du thread
    pthread_join(pub, NULL);
	
	// Libérer le GPIO
    bcm2835_close();
    
	return 0;
}

