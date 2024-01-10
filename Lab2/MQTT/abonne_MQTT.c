/*
 * abonne_MQTT.c
 * 
 * 
 * Lokman Sboui et Guy Gauthier
 * 
 */
 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>
#include <bcm2835.h>
#include <gtk/gtk.h>

#define DEL_Rouge 20 // Définition du GPIO associé au DEL rouge (broche 38)
#define DEL_Orange 21 // Définition du GPIO associé au DEL rouge (broche 40)
#define BTN1 19 // Définition du GPIO associé au BTN1
#define BTN2 26 // Définition du GPIO associé au BTN1

#define MQTT_HOSTNAME "localhost"
#define MQTT_QoS 1
#define MQTT_PORT 1883
#define MQTT_TOPIC "sujet"

char temp_str[100];

static void destroy(GtkWidget *widget, gpointer data) 
{
	gtk_main_quit();
}

static gboolean update_temp(gpointer label) 
{
	gtk_label_set_text(GTK_LABEL(label), temp_str);
	
	if(!(bcm2835_gpio_lev(BTN2)))
	{
		gtk_main_quit();
	}
	return TRUE;
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("Topic : %s  --> ", (char*) message->topic);
	printf("Message : %s\n", (char*) message->payload);
	
	sprintf(temp_str,(char*)message->payload);
	
	if (!strcmp(message->topic,"sujet"))
	{
		// sprintf(temp_str,"%.2f",valeur);
	
		printf("La temperature dans le salon est de %s.\n",temp_str);
	}
}

int main(int argc, char **argv)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour DEL 2 (orange)
	bcm2835_gpio_fsel(DEL_Orange, BCM2835_GPIO_FSEL_OUTP);	
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(BTN1, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour bouton-poussoir 2
	bcm2835_gpio_fsel(BTN2, BCM2835_GPIO_FSEL_INPT);
	
	// GTK init
	gtk_init(&argc, &argv);
	
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Temperature");
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_widget_set_size_request(window, 100, 75);

	GtkWidget *label = gtk_label_new("----");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_container_add(GTK_CONTAINER(window), label);
	
	g_timeout_add(500, update_temp, label);

	gtk_widget_show_all(window);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
	
	int ret;
	struct mosquitto *mosq = NULL;
	
	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq)
	{
		fprintf(stderr,"Ne peut initialiser la librairie de Mosquitto\n");
		exit(-1);
	}
	mosquitto_message_callback_set(mosq, my_message_callback);
	
	ret = mosquitto_connect(mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
	if (ret)
	{
		fprintf(stderr,"Ne peut se connecter au serveur Mosquitto\n");
		exit(-1);
	}
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	
	mosquitto_loop_start(mosq);
	gtk_main();
	
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	// Libérer le GPIO
    bcm2835_close();
	
	return 0;
}

