/***************************************************************************
 * Author     : yuanliixjtu@gmail.com			_________________  *
 * Date       : 06/30/2006 14:26:11			|    | This file | *
 * File name  : netstatus.h	   			| vi |  powered  | *
 * Description: the header file for get the net status	|____|___________| *
 *									   *
 ***************************************************************************/

#ifndef	_NETSTATUS_H_
#define _NETSTATUS_H_

/**
 * IsWiredAvailable - check whether wired is linked
 * @return: 1 when wired is linked
 * 	    0 when not linked
 * 	    -1 when any error occured
 */
int IsWiredAvailable(void);

/**
 * IsWirelessAvailable - check whether wireless is linked
 * @return: 1 when wireless is linked
 * 	    0 when not linked
 * 	    -1 when any error occured
 */
int IsWirelessAvailable(void);

/**
 * GetWiredIfname - get a wired interface name
 * @return: the name of any wired interface, linked interface has the higher
 * 		priority
 * 	    NULL when no wired If available
 */
char *GetWiredIfname(void);

/**
 * GetWirelessIfname - get a wireless interface name
 * @return: the name of any wireless interface, linked interface has the
 * 		higher priority
 * 	    NULL when no wireless If available
 */
char *GetWirelessIfname(void);

#endif/*_NETSTATUS_H_*/
