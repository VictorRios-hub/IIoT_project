#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define period 1
double err_old = 0;
double err_last= 0;
double err = 0;

int commande_PID(double consigne, double mesure, double gainKp, double gainKi, double gainKd)
{
	// recuperation de l erreur 
	err_old = err_last;
	err_last = err;
	err = consigne - mesure;
	
	//formule PID
	u_next = gainKp*(err - err_last) + (gainKi/2)*(err + err_last) + (gainKd/(2*period))*(err - (2*err_last) + err_old) + u;
	
	//formule commande saturation
	if(u_next < 0)
		u_next = 0;
	
	if(u_next > range)
		u_next = range;
		
		
	return u_next;
}
