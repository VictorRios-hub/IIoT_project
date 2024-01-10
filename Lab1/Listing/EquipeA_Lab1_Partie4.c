/*
 * EquipeA_Lab1_Partie4.c
 * 
 * Ce programme fait clignoter un DEL toute les secondes et 
 * applique une commande PID à la lampe pour réguler la température 
 * en fonction de la valeur consigne tout en affichant consigne et commande.
 * Le clignotement et la régulation se fait à l'intérieur d'un thread et le délai est
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

#define period 1
#define CONSIGNE 42

double err_old = 0;
double err_last= 0;
double err = 0;
double u = 0;

#define MISO 23 // Master input Slave output : Rasp input MAX output
#define CS 24 // Chip select
#define SCK 25 // signal d'horloge

#define PWM 18 // PWM broche
// freauence de base du PWM a 19,2 MHz
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

double frequence = 1; // Hertz : fréquence de clignotement désirée

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

/*
 *  Fonction intensity qui sera appelée dans la fonction clignote()
 * 
 *	Elle actualise la valeur du duty cycle avec bcm2835_pwm_set_data(0,commande) où
 *	commande est le paramètre ajusté par le PID. 
 *
 *	La fonction ne retourne aucune valeur.
 *
 *	La fonction prend en paramètre d'entrée la variable int commande. 
 *
 *	
 */
 
void intensity(int commande)
{
		
	// Duty cycle desire
	bcm2835_pwm_set_data(0,commande);
	
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
	
	printf("temperature mesuree : %.2f \n",temp);
	
	// Fin de la lecture
	bcm2835_gpio_write(CS, HIGH);
	bcm2835_delayMicroseconds(10000); // 10 ms
	
	return temp;
}

/* Fonction clignote qui sera appelée par le thread "cligne"
 * 
 * Elle fait clignoter le DEL rouge à chaque seconde et
 * récupère dans un tampon (tmp) la valeur de issue de la boucle PID
 * et l'envoie dans intensity() pour modifier le PWM
 * La fonction lecture_SPI est passé en paramètre de commande_PID() pour
 * donner la valeur de température du thermocouple à la fonction
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
			int tmp = commande_PID(CONSIGNE, lecture_SPI(), 20, 20, 1); // tmp est une variable qui veut dire tampon
			printf("commande : %d \n",(tmp*100)/range);
			printf("valeur de consigne : %d \n", CONSIGNE);
			intensity(tmp);
			
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
 * Elle configure le GPIO, le PWM et le thread.
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
