#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/**
 *Description: this function used to handle the situation when a signal occurs
 *@param sig: signal
 *no return value
 */ 
void sigHandler(int sig);

/**
 *Description: this function used to initiate the siganl handling process
 */
void signalSetup();

#endif
