#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define PWM 18 // PWM broche
// freauence de base du PWM a 19,2 MHz
#define diviseur 128 // pour une frequence de fct a 1kHz
#define range 150

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
	float increment = 1.5;
	
	while (commande < 150)
	{
		commande += increment;
		
		// Duty cycle desire
		bcm2835_pwm_set_data(0,commande);
		
		// Wait 10 ms
		bcm2835_delayMicroseconds(10000); // 10 ms
	}
	
	while (commande > 0)
	{
		commande -= increment;
		
		// Duty cycle desire
		bcm2835_pwm_set_data(0,commande);
		
		// Wait 10 ms
		bcm2835_delayMicroseconds(10000); // 10 ms
	}

	// Commande nul == lumiere eteinte
	bcm2835_pwm_set_data(0,0);
}

int main(int argc, char **argv)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	while(1)
	{
		intensity();
	}
	// Libérer le GPIO
    bcm2835_close();
    return 0;
}
