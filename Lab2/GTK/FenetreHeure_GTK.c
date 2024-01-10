/*
 * FenetreHeure.c
 *
 * Exemple de programme pour Afficher l'heure dans une fenêtre 
 *
 *
 */
 
#include <gtk/gtk.h>
#include <time.h>
#include <stdio.h> 
#include <unistd.h>
#include <bcm2835.h>
#include <pthread.h>


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
	
	//printf("%.2f \n",temp);
	
	//print with gtk
	gtk_label_set_text(GTK_LABEL(label), time_str);
	
	// Fin de la lecture
	bcm2835_gpio_write(CS, HIGH);
	bcm2835_delayMicroseconds(10000); // 10 ms
	
	return temp;
}

static gboolean update_time(gpointer label) 
{
	time_t now = time(0);
	char time_str[100];
	strftime(time_str, 100, "%H:%M:%S", localtime(&now));
	gtk_label_set_text(GTK_LABEL(label), time_str);
	return TRUE;
}

static void destroy(GtkWidget *widget, gpointer data) 
{
	gtk_main_quit();
}

int main(int argc, char *argv[]) 
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Heure");
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_widget_set_size_request(window, 100, 75);

	GtkWidget *label = gtk_label_new("----");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_container_add(GTK_CONTAINER(window), label);

	//g_timeout_add(1000, update_time, label);
	g_timeout_add(500, lecture_SPI(), label);

	gtk_widget_show_all(window);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
	

	gtk_main();
	//close
	bcm2835_close();
	
	return 0;
}
