#ifndef _INC_VIRTUALTWAINDEF_H_20170819
#define _INC_VIRTUALTWAINDEF_H_20170819

#include <windows.h>
#include "twain.h"

#ifdef	__cplusplus
extern	"C" {
#endif

#ifdef VIRTUALTWAIN_EXPORTS
#define VIRTUALTWAIN_API __declspec(dllexport)
#define VIRTUALTWAIN_VAR
#else
#define VIRTUALTWAIN_API __declspec(dllimport)
#define VIRTUALTWAIN_VAR __declspec(dllimport)
#endif
	
	extern VIRTUALTWAIN_API 
		TW_UINT16 _DSM_Entry(
			pTW_IDENTITY _pOrigin,
			pTW_IDENTITY _pDest,
			TW_UINT32    _DG,
			TW_UINT16    _DAT,
			TW_UINT16    _MSG,
			TW_MEMREF    _pData);

	//test
	extern VIRTUALTWAIN_API
		int test_main(HWND hParentWnd, LONG *pbCancel);

#ifdef	__cplusplus
}
#endif

#endif