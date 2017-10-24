//ui_ipc.h
#ifndef _MSG_SHM_UI_H_
#define _MSG_SHM_UI_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "ui_ipc.h"


/*init The ipc object : IPC Queue And IPC Shmory
*@param : keyShm	The Key for IPC Shmory
*@param : shared_memory		The handle for shmory link process addr
*@retval: success 0;  fail -1;
*/
int open_ipc_shm(key_t keyShm, int *shmid);



/*Link The shmid in process
*@param : shmid  The handle for IPC shmory
*@param : shared_memory  The handle for link The shmid in process 
*@retval: success 0;  fail -1;
*/
int link_shmid_process(uiIpcHandle *handle, void **shared_memory);


/*destroy The shmory  By shmid point
*@param : shmid  The handle for IPC shmory
*@retval: success 0;  fail -1;
*/
int destory_shmory(int shmid);

#if defined(__cplusplus)
}
#endif


#endif
