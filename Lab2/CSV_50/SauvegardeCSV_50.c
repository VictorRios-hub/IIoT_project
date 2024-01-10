/*
 * SauvegardeCSV.c
 * 
 * Exemple de programme pour céer un fichier CSV
 * avec l'heure et la date actuelle
 *  
 * 
 */

#include <stdio.h> 
#include <time.h>
#include <unistd.h>
#include <bcm2835.h>
#include <pthread.h>


#define MISO 23 // Master input Slave output : Rasp input MAX output
#define CS 24 // Chip select
#define SCK 25 // signal d'horloge

#define PWM 18 // PWM broche
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

char date_et_heure[20];

void intensity(void)
{
	// Configuration du PWM
	bcm2835_gpio_fsel(PWM, BCM2835_GPIO_FSEL_ALT5);

	// Inititialisation du PWM
	bcm2835_pwm_set_clock(diviseur);	// freq de base
	bcm2835_pwm_set_mode(0,1,1);			// mode du PWM
	bcm2835_pwm_set_range(0, range);	// plage ajustement
	bcm2835_pwm_set_data(0,0);			// PWM duty cycle == duree d'impulsion
	
	float commande = 0.0;
	
	commande = 75 ;
	
	// Duty cycle desire
	bcm2835_pwm_set_data(0,commande);
	
	// Wait 10 ms
	bcm2835_delayMicroseconds(10000); // 10 ms
		
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

int main() {

	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	// PWM 50% puissance
	intensity();
	
	// Ecriture entete
	FILE *fp = fopen("EQUIPEA_Temp_50_5min.csv", "a");
	fprintf(fp, "Date et heure,Adresse MAC,Emplacement,Temperature\n");
	fclose(fp);
	
	for (int cpt = 0; cpt < 150; cpt ++)
	{
		// Capturer l'heure et la date actuelle
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		strftime(date_et_heure, 20, "%Y-%m-%d %H:%M:%S",tm);

		// Écrire les données dans un fichier CSV 
		
		// Open file
		FILE *fp = fopen("EQUIPEA_Temp_50_5min.csv", "a");
		fprintf(fp, "%s,%s,%s,%.1f\n", date_et_heure, "b8:27:eb:ec:58:7b", "Poste 11", lecture_SPI());
		
		// Close file
		fclose(fp);
		
		bcm2835_delayMicroseconds(1670000); // 1,67s car lecture_SPI dure 330ms
	}

	
	// Libérer le GPIO
    bcm2835_close();
    return 0;
}

