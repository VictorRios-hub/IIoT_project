/*
 * Thread_Cligne_1_Hz.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
 * Le clignotement se fait à l'intérieur d'un thread.
 * L'arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>

/* Fonction clignote qui sera appelée par le thread "cligne"
 * 
 * Elle fait clignoter le DEL rouge à 1 Hertz.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *clignote()
{
	// Boucle infinie pour le clignotement du DEL
	while(1){
		bcm2835_gpio_write(20, HIGH);
		bcm2835_delay(5000);
		bcm2835_gpio_write(20, LOW);
		bcm2835_delay(5000);
	}
	pthread_exit(NULL);
}

void *clignote_o()
{
	// Boucle infinie pour le clignotement du DEL
	while(1){
		bcm2835_gpio_write(21, LOW);
		bcm2835_delay(5000);
		bcm2835_gpio_write(21, HIGH);
		bcm2835_delay(5000);
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
	pthread_t orange;  
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT);
	
	// Création du thread "cligne".
	// Lien avec la fonction clignote.
	// Cette dernière n'exige pas de paramètres.
	pthread_create(&cligne, NULL, &clignote, NULL);
	pthread_create(&orange, NULL, &clignote_o, NULL);
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while(bcm2835_gpio_lev(19) || bcm2835_gpio_lev(26)){
		// printf("hello");
		// bcm2835_delay(1);
		usleep(1000); // Délai de 1 ms !!!
	}

	// Si bouton-poussoir enfoncé, arrêt immédiat du thread 
    pthread_cancel(cligne);
    pthread_cancel(orange);
    // Attente de l'arrêt du thread
    pthread_join(cligne, NULL);
    pthread_join(orange, NULL);
    
    // Éteindre le DEL rouge
    bcm2835_gpio_write(20, LOW);
    bcm2835_gpio_write(21, LOW);
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
