/*
 * EquipeA_Lab2_publieur.c
 * 
 *
 * Ce programme configure la connection du publieur avec le broker. On récupère
 * l'ensemble des informations des topics et on les publie sur les différents topics
 * les informations correspondantes.
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

#define DEL_Rouge 20 // Définition du GPIO associé au DEL rouge (broche 38)
#define DEL_Orange 21 // Définition du GPIO associé au DEL rouge (broche 40)
#define BTN1 19 // Définition du GPIO associé au BTN1
#define BTN2 26 // Définition du GPIO associé au BTN1

#define MQTT_HOSTNAME "localhost"
#define MQTT_PORT 1883
#define MQTT_TOPIC1 "temp"
#define MQTT_TOPIC2 "consigne"
#define MQTT_TOPIC3 "PWM"
#define MQTT_TOPIC4 "DEL1"
#define MQTT_TOPIC5 "DEL2"

#define period 1
#define CONSIGNE 30
#define gainKP 10
#define gainKI 10
#define gainKD 0.2

double err_old = 0;
double err_last= 0;
double err = 0;
double u = 0;

#define range 150

double frequence = 0.5; // Hertz : fréquence de clignotement désirée T = 2s

/*
 *  Fonction commande_PID qui sera appelée dans la fonction clignote()
 * 
 *	Elle sert à réguler la commande à un niveau désiré (boucle d'asservissement). 
 *
 *	La fonction retourne la commande sous la forme d’un entier dont
 *	la valeur minimale est 0 et la valeur maximale celle du "range" définit
 *
 *	La fonction prend en paramètre 5 entrées de type double : une consigne de température, 
 *	une mesure faite par le thermocouple et les trois gains du PID : Kp, Ki et Kd.
 *
 */
 
int commande_PID(double consigne, double mesure, double gainKp, double gainKi, double gainKd)
{
	// recuperation de l erreur 
	err_old = err_last;
	err_last = err;
	err = consigne - mesure;
	
	//formule PID
	int u_next = gainKp*(err - err_last) + (gainKi/2)*(err + err_last) + (gainKd/(2*period))*(err - (2*err_last) + err_old) + u;
	
	//formule commande saturation
	if(u_next < 0)
		u_next = 0;
	
	if(u_next > range)
		u_next = range;
		
	u = u_next;	
	return u_next;
}

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
 * Cette fonction va initialiser la connexion au broker MQTT, récupérer l'ensemble
 * des informations des capteurs et publier sur les différents topics les
 * informations correspondantes
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *publish()
{
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour DEL 2 (orange)
	bcm2835_gpio_fsel(DEL_Orange, BCM2835_GPIO_FSEL_OUTP);	
	
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
	
	// Boucle infinie pour le publish
	while(1){
		if (etat==0){

			// Recuperation de la temperature
			
			double temp = lecture_SPI();
			sprintf(text, "%5.2f C",temp);
		
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC1, strlen(text), text, 0, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
			
			// Recuperation de la consigne
			
			sprintf(text, "%d C",CONSIGNE);
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC2, strlen(text), text, 0, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
			
			// Recuperation du pourcentage PWM
			
			int command = commande_PID(CONSIGNE, temp, gainKP, gainKI, gainKD);
			sprintf(text, "%d pourcent",command*100/range);
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC3, strlen(text), text, 0, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
			
			// Recuperation de DEL1
			
			sprintf(text, "%d ",bcm2835_gpio_lev(DEL_Rouge));
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC4, strlen(text), text, 2, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
			
			// Recuperation de DEL2
			
			sprintf(text, "%d ",bcm2835_gpio_lev(DEL_Orange));
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC5, strlen(text), text, 2, false);
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

/* Fonction main
 * 
 * Elle configure le GPIO et le thread, le thread fonctionne
 * jusqu'à l'appui sur le bouton orange.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
int main(int argc, char **argv)
{
	// Identificateur du thread
	pthread_t pub;  
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(BTN1, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour bouton-poussoir 2
	bcm2835_gpio_fsel(BTN2, BCM2835_GPIO_FSEL_INPT);
	
	// Création du thread "pub".
	pthread_create(&pub, NULL, &publish, NULL);
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while(bcm2835_gpio_lev(BTN2))
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

