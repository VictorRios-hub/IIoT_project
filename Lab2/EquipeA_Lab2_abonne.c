/*
/*
 * EquipeA_Lab2_abonne.c
 * 
 * Ce programme configure la connection de l'abonne avec le broker.
 * On s'abonne à chacun des topics voulues.
 * On va récupérer le message reçue par le broker, si le message correspond à
 * un topic subscribed, on affiche leur valeur dans le terminal.
 * On update la fenêtre GTK avec la nouvelle valeur de la température.
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
#define MQTT_QoS 2
#define MQTT_PORT 1883
#define MQTT_TOPIC1 "temp"
#define MQTT_TOPIC2 "consigne"
#define MQTT_TOPIC3 "PWM"
#define MQTT_TOPIC4 "DEL1"
#define MQTT_TOPIC5 "DEL2"

#define PWM 18 // PWM broche
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

char temp_str[100];
char cons_str[100];
char command_str[100];
char del1[100];
char del2[100];

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
 */
void intensity(int commande)
{
	// Duty cycle desire
	bcm2835_pwm_set_data(0,commande);
}

/*
 *  Fonction destroy qui sera connecté à l'évènement destroy de la fenêtre GTK appelé dans main
 * 
 *	Elle appelle la fonction gtk_main_quit() qui met fin à la boucle principale GTK+ et au programme
 *
 *	La fonction ne retourne aucune valeur.
 *
 *	La fonction prend en paramètre d'entrée le widget gtk. 
 *	
 */
static void destroy(GtkWidget *widget, gpointer data) 
{
	gtk_main_quit();
}

/*
 *  Fonction update_temp qui sera appelé périodiquement par g_timeout_add() dans la boucle GTK.
 * 
 *	Elle met à jour le texte de la fenêtre GTK grâce à gtk_label_set_text(),
 *  si le bouton orange est pressé, on sort du programme avec gtk_main_quit().
 *
 *	La fonction retourne un booléen.
 *
 *	La fonction prend en paramètre d'entrée un pointeur gtk sur la donnée à update
 *  dans la fenêtre. 
 *	
 */
static gboolean update_temp(gpointer label) 
{
	gtk_label_set_text(GTK_LABEL(label), temp_str);
	
	if(!(bcm2835_gpio_lev(BTN2)))
	{
		gtk_main_quit();
	}
	return TRUE;
}

/*
 *  Fonction my_message_callback qui sera appelé périodiquement par mosquitto_message_callback_set() dans main
 * 
 *	Elle lit le message reçu par le broker, on vérifie si le message contient le topic subsribed,
 *  dans ce cas, on affiche la valeur reçu dans le terminal.
 *
 *	La fonction ne retourne aucune valeur.
 *
 *	La fonction prend en paramètre d'entrée l'abonné mosquitto et le message reçu.
 *	
 */
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("Topic : %s  --> ", (char*) message->topic);
	printf("Message : %s\n", (char*) message->payload);

	
	if (!strcmp(message->topic,"temp"))
	{
		sprintf(temp_str,(char*)message->payload);
	
		printf("La temperature  est de %s.\n",temp_str);
	}
	if (!strcmp(message->topic,"consigne"))
	{
		sprintf(cons_str,(char*)message->payload);
	
		printf("La consigne est de %s.\n",cons_str);
	}
	if (!strcmp(message->topic,"PWM"))
	{
		sprintf(command_str,(char*)message->payload);
		// intensity((int)command_str);
	
		printf("La PWM est de %s.\n",command_str);
	}
	if (!strcmp(message->topic,"DEL1"))
	{
		sprintf(del1,(char*)message->payload);
	
		printf("La DEL1 est de %s.\n",del1);
	}
	if (!strcmp(message->topic,"DEL2"))
	{
		sprintf(del2,(char*)message->payload);
	
		printf("La DEL2 est de %s.\n",del2);
	}
	

}

/* Fonction main
 * 
 * Elle configure le GPIO, intitialse l'abonne et l'abonnement aux topics, la fenêtre GTK et sa boucle, la boucle fonctionne
 * jusqu'à l'appui sur le bouton orange.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
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
	
	// Configuration du PWM
	bcm2835_gpio_fsel(PWM, BCM2835_GPIO_FSEL_ALT5);
	
	// Inititialisation du PWM
	bcm2835_pwm_set_clock(diviseur);	// freq de base
	bcm2835_pwm_set_mode(0,1,1);		// mode du PWM
	bcm2835_pwm_set_range(0, range);	// plage ajustement
	bcm2835_pwm_set_data(0,0);			// PWM duty cycle == duree d'impulsion
	
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
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC1, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC2, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC3, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC4, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC5, MQTT_QoS);
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

