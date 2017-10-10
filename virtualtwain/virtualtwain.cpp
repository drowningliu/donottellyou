#include "virtualtwain.h"

#include <assert.h>
#include <unordered_map>
#include <functional>

TW_FIX32 FloatToFix32(float floater)
{
	TW_FIX32 Fix32_value;
	TW_BOOL sign = (floater < 0) ? TRUE : FALSE;
	TW_INT32 value = (TW_INT32)(floater * 65536.0 + (sign ? (-0.5) : 0.5));
	Fix32_value.Whole = (TW_INT16)(value >> 16);
	Fix32_value.Frac = (TW_UINT16)(value & 0x0000ffffL);
	return (Fix32_value);
	// 
	// 
	// #if 0
	// 	TW_FIX32 Fix32_value;
	// 	TW_BOOL sign = (floater < 0) ? TRUE : FALSE;
	// 	TW_INT32 value = (TW_INT32) (floater * 65536.0 + (sign ? (-0.5) : 0.5));
	// 
	// 	memset(&Fix32_value, 0, sizeof(TW_FIX32));
	// 
	// 	Fix32_value.Whole = LOWORD(value >> 16);
	// 	Fix32_value.Frac = LOWORD(value & 0x0000ffffL);
	// 
	// 	return (Fix32_value);
	// #else
	// 
	// 	TW_FIX32 fix32;
	// 	TW_INT32 value = (TW_INT32) (floater*65536.0 + 0.5);
	// 	fix32.Whole = (TW_INT16) (value >> 16);
	// 	fix32.Frac = (TW_UINT16) (value & 0x0000ffffL);
	// 	return fix32;
	// #endif
}

float FIX32ToFloat(TW_FIX32 fix32)
{
	float floater;
	floater = (float)fix32.Whole + (float)(fix32.Frac / 65536.0);
	return floater;
}

class Param
{
public:
	//!< 扫描属性
	enum { ATTI_DEFPPI = 500, ATTI_DEFBPP = 8, ATTI_ALL };

	TW_IDENTITY		_twSourceID = { 0 };
	TW_IMAGELAYOUT  _tw_LayOut = { 0 };
	float			_dpiX = ATTI_DEFPPI;
	float			_dpiY = ATTI_DEFPPI;
	RECT			_rtFrame = { 0 };
	TW_IDENTITY		_twAppID = { 0 };
	HWND			_hParentWnd = NULL;
	TW_UINT32		_twImgBPP = ATTI_DEFBPP;			//!< 扫描图像的BPP，缺省为8位
	TW_UINT32		_twImgUnitType = TWUN_INCHES;		//图像大小的计量单位，缺省为Inch
	TW_UINT32		_twImgPixelType = TWPT_GRAY;		//图像像素模式，缺省为 Gray 

	TW_UINT32		_twTransferMode = TWSX_MEMORY;
	TW_USERINTERFACE	_twShowUI = { 0 };			//扫描仪用户界面
	SIZE			_sizeImageData = { 0 };

	typedef std::tuple<int, int, int> dg_dat_msg_t;

	struct key_hash : public std::unary_function<dg_dat_msg_t, std::size_t>
	{
		std::size_t operator()(const dg_dat_msg_t& k) const
		{
			return std::get<0>(k) ^ std::get<1>(k) ^ std::get<2>(k);
		}
	};

	struct key_equal : public std::binary_function<dg_dat_msg_t, dg_dat_msg_t, bool>
	{
		bool operator()(const dg_dat_msg_t& v0, const dg_dat_msg_t& v1) const
		{
			return (
				std::get<0>(v0) == std::get<0>(v1) &&
				std::get<1>(v0) == std::get<1>(v1) &&
				std::get<2>(v0) == std::get<2>(v1)
				);
		}
	};

	std::unordered_map<dg_dat_msg_t, std::function<int()>, key_hash, key_equal> map_msg_func;

	Param()
	{
		
	}
};

Param g_param;

int test_main(HWND hParentWnd, LONG *pbCancel)
{
	//init
	TW_IDENTITY		m_twAppID;				//应用程序标识
	memset(&m_twAppID, 0, sizeof(TW_IDENTITY));

	//初始化应用程序信息
	m_twAppID.Id = 0;			// init to 0, but Source Manager will assign real value
	m_twAppID.Version.MajorNum = 5;
	m_twAppID.Version.MinorNum = 1;
	m_twAppID.Version.Language = TWLG_CHINESE_PRC;	// TWLG_USA;
	m_twAppID.Version.Country = TWCY_CHINA;		// TWCY_USA;
	strncpy(m_twAppID.Version.Info, "Scanner!", sizeof(m_twAppID.Version.Info));
	m_twAppID.ProtocolMajor = TWON_PROTOCOLMAJOR;
	m_twAppID.ProtocolMinor = TWON_PROTOCOLMINOR;
	m_twAppID.SupportedGroups = DG_IMAGE | DG_CONTROL;
	strncpy(m_twAppID.Manufacturer, "DrowningLiu", sizeof(m_twAppID.Manufacturer));
	strncpy(m_twAppID.ProductFamily, "DL System", sizeof(m_twAppID.ProductFamily));
	strncpy(m_twAppID.ProductName, "DLScanner!", sizeof(m_twAppID.ProductName));

	//初始化image attribute
	//!< 扫描属性
	enum { ATTI_DEFPPI = 500, ATTI_DEFBPP = 8, ATTI_ALL };

	TW_UINT32	m_twImgDpi = ATTI_DEFPPI;
	TW_UINT32	m_twImgBPP = ATTI_DEFBPP;
	TW_UINT32	m_twImgUnitType = TWUN_INCHES;
	TW_UINT32	m_twImgPixelType = TWPT_GRAY;

	HWND		m_hParentWnd = hParentWnd;           //调用CScanner类的父窗口
	//m_hParentWnd = GetSafeHwnd();

	RECT	m_rtFrame;						//扫描区域
	memset(&m_rtFrame, 0, sizeof(m_rtFrame));

	//OpenSourceManager
	{
		TW_UINT16  tw_retval;
		tw_retval = _DSM_Entry(&m_twAppID, NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF)&m_hParentWnd);
		if (tw_retval != TWRC_SUCCESS)
			return -1;
	}
	
	//SelectSource
	TW_IDENTITY	m_twSourceID;
	memset(&m_twSourceID, 0, sizeof(TW_IDENTITY));

	{
		BOOL bIsDefSource = FALSE;
		TW_UINT16  tw_retval;

		if (!bIsDefSource) //user select the source.
		{
			tw_retval = _DSM_Entry(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, (TW_MEMREF)&m_twSourceID);
			if (TWRC_CANCEL == tw_retval) bIsDefSource = TRUE; //如果用户取消选择的数据源，则采用默认的数据源
			else if (TWRC_SUCCESS != tw_retval)
			{
				//SetErrorInfoByReturnCode(tw_retval);
				return -1;
			}
		}
		
		if (bIsDefSource)             //select the default data source;
		{
			tw_retval = _DSM_Entry(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, (TW_MEMREF)&m_twSourceID);
			if (tw_retval != TWRC_SUCCESS)
			{
				//SetErrorInfoByReturnCode(tw_retval);
				return -1;
			}
		}
	}
	
	//OpenSource
	{
		TW_UINT16 tw_retval;
		tw_retval = _DSM_Entry(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, (TW_MEMREF)&m_twSourceID);
		if (tw_retval != TWRC_SUCCESS)
		{
			//SetErrorInfoByReturnCode(tw_retval);
			return -1;
		}
	}

	//SetDefaultAttribute

	//SetImagePixelType
	{
		TW_UINT16		tw_retval;
		TW_CAPABILITY	tw_Cap;
		pTW_ONEVALUE	pval;

		memset(&tw_Cap, 0, sizeof(TW_CAPABILITY));
		tw_Cap.Cap = ICAP_PIXELTYPE;
		tw_Cap.ConType = TWON_ONEVALUE;
		tw_Cap.hContainer = ::GlobalAlloc(GHND, sizeof(TW_ONEVALUE));

		if (tw_Cap.hContainer != NULL)
		{
			pval = (pTW_ONEVALUE)(::GlobalLock(tw_Cap.hContainer));
			pval->ItemType = TWTY_UINT16;
			pval->Item = m_twImgPixelType;	//TWPT_GRAY;
			::GlobalUnlock(tw_Cap.hContainer);
			pval = NULL;

			tw_retval = _DSM_Entry(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_CAPABILITY, MSG_SET, (TW_MEMREF)&tw_Cap);

			::GlobalFree((HANDLE)tw_Cap.hContainer);
			tw_Cap.hContainer = NULL;
			if (tw_retval != TWRC_SUCCESS)
				return -1;
		}
	}
	
	//SetImageUnitType
	{
		TW_UINT16  tw_retval;
		TW_CAPABILITY  tw_Cap;
		pTW_ONEVALUE  pval;

		memset(&tw_Cap, 0, sizeof(TW_CAPABILITY));
		tw_Cap.Cap = ICAP_UNITS;
		tw_Cap.ConType = TWON_ONEVALUE;
		tw_Cap.hContainer = ::GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
		if (tw_Cap.hContainer != NULL)
		{
			pval = (pTW_ONEVALUE)(::GlobalLock(tw_Cap.hContainer));
			pval->ItemType = TWTY_UINT16;
			pval->Item = m_twImgUnitType; //TWUN_INCHES;
			::GlobalUnlock(tw_Cap.hContainer);
			pval = NULL;
			tw_retval = _DSM_Entry(&m_twAppID,
				&m_twSourceID,
				DG_CONTROL,
				DAT_CAPABILITY,
				MSG_SET,
				(TW_MEMREF)&tw_Cap);
			::GlobalFree((HANDLE)tw_Cap.hContainer);
			tw_Cap.hContainer = NULL;
			if (tw_retval != TWRC_SUCCESS)
				return -1;
		}
	}

	//SetImageDPI
	{
		TW_UINT16  tw_retval;
		TW_CAPABILITY tw_Cap;
		pTW_ONEVALUE   pval;

		// set x DPI
		memset(&tw_Cap, 0, sizeof(TW_CAPABILITY));
		tw_Cap.Cap = ICAP_XRESOLUTION;
		tw_Cap.ConType = TWON_ONEVALUE;
		tw_Cap.hContainer = ::GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
		if (tw_Cap.hContainer != NULL)
		{
			pval = (pTW_ONEVALUE) ::GlobalLock(tw_Cap.hContainer);
			pval->ItemType = TWTY_FIX32;
			TW_FIX32 fix32 = FloatToFix32((float)m_twImgDpi);
			pval->Item = *((pTW_UINT32)&fix32);

			::GlobalUnlock(tw_Cap.hContainer);
			pval = NULL;
			tw_retval = _DSM_Entry(&m_twAppID,
				&m_twSourceID,
				DG_CONTROL,
				DAT_CAPABILITY,
				MSG_SET,
				(TW_MEMREF)&tw_Cap);
			::GlobalFree((HANDLE)tw_Cap.hContainer);
			tw_Cap.hContainer = NULL;
			if (tw_retval != TWRC_SUCCESS)
				return -1;
		}

		//set y dpi
		memset(&tw_Cap, 0, sizeof(TW_CAPABILITY));
		tw_Cap.Cap = ICAP_YRESOLUTION;
		tw_Cap.ConType = TWON_ONEVALUE;
		tw_Cap.hContainer = ::GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
		if (tw_Cap.hContainer != NULL)
		{
			pval = (pTW_ONEVALUE)(::GlobalLock(tw_Cap.hContainer));
			pval->ItemType = TWTY_FIX32;
			TW_FIX32 fix32 = FloatToFix32((float)m_twImgDpi);
			pval->Item = *((pTW_UINT32)&fix32);

			::GlobalUnlock(tw_Cap.hContainer);
			pval = NULL;
			tw_retval = _DSM_Entry(&m_twAppID,
				&m_twSourceID,
				DG_CONTROL,
				DAT_CAPABILITY,
				MSG_SET,
				(TW_MEMREF)&tw_Cap);
			::GlobalFree((HANDLE)tw_Cap.hContainer);
			tw_Cap.hContainer = NULL;
			if (tw_retval != TWRC_SUCCESS)
				return -1;
		}
	}

	//SetImageFrame
	{
		TW_UINT16  tw_retval;
		TW_IMAGELAYOUT  tw_LayOut;

		TW_FRAME tw_Frame;
		float fdpi = (float)m_twImgDpi;

		memset(&tw_Frame, 0, sizeof(TW_FRAME));
		memset(&tw_LayOut, 0, sizeof(TW_IMAGELAYOUT));
		//设置默认的指纹框大小(inch)*(inch),每英寸的线数是m_dwImgDpi
		//所以指纹框的大小为, 同时加大范围

		tw_Frame.Left = FloatToFix32(((float)(m_rtFrame.left)) / fdpi);
		tw_Frame.Top = FloatToFix32(((float)(m_rtFrame.top)) / fdpi);
		tw_Frame.Right = FloatToFix32(((float)(m_rtFrame.right)) / fdpi);
		tw_Frame.Bottom = FloatToFix32(((float)(m_rtFrame.bottom)) / fdpi);

		tw_LayOut.Frame = tw_Frame;
		tw_LayOut.DocumentNumber = 0x00000001;
		tw_LayOut.PageNumber = 0x00000001;
		tw_LayOut.FrameNumber = 0x00000001;
		tw_retval = _DSM_Entry(&m_twAppID,
			&m_twSourceID,
			DG_IMAGE,
			DAT_IMAGELAYOUT,
			MSG_SET,
			(TW_MEMREF)&tw_LayOut);
	}

	//SetTransferMode
	{
		TW_UINT16      tw_retval;
		TW_CAPABILITY  tw_Cap;
		pTW_ONEVALUE   pval;

		tw_Cap.Cap = ICAP_XFERMECH;
		tw_Cap.ConType = TWON_ONEVALUE;
		tw_Cap.hContainer = ::GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
		if (tw_Cap.hContainer == NULL)
			return -1;

		pval = (pTW_ONEVALUE)::GlobalLock(tw_Cap.hContainer);
		pval->ItemType = TWTY_UINT16;
		pval->Item = TWSX_MEMORY;
		::GlobalUnlock(tw_Cap.hContainer);
		tw_retval = _DSM_Entry(&m_twAppID,
			&m_twSourceID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&tw_Cap);

		::GlobalFree(tw_Cap.hContainer);
		tw_Cap.hContainer = NULL;
	}

	//EnableScanner
	{
		TW_UINT16 tw_retval;

		//记录下用户打开数据源用户界面是的设置，用于结束用户界面
		TW_USERINTERFACE	m_twShowUI;			//扫描仪用户界面
		memset(&m_twShowUI, 0, sizeof(TW_USERINTERFACE));

		m_twShowUI.ShowUI = FALSE;
		m_twShowUI.hParent = (HANDLE)m_hParentWnd;

		tw_retval = _DSM_Entry(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_USERINTERFACE,
			MSG_ENABLEDS, (TW_MEMREF)&m_twShowUI);
	}

	auto do_memory_transfer_imagedata = [&](TW_UINT16 &twRC)->int
	{
		TW_UINT16            rc = TWRC_FAILURE;
		TW_SETUPMEMXFER      tw_SetupMem;
		TW_IMAGEMEMXFER      tw_XferMem;
		unsigned char		 *pXferData = NULL;
		TW_UINT32            tw_uWrittenBytes = 0, tw_uSize = 0;
		TW_BOOL				 bPendingXfers = TRUE;

		memset(&tw_SetupMem, 0, sizeof(TW_SETUPMEMXFER));
		memset(&tw_XferMem, 0, sizeof(TW_IMAGEMEMXFER));
		memset(&g_param._sizeImageData, 0, sizeof(SIZE));
		m_twImgBPP = 0;
		int m_bIsBmpFormat = 1;
		int m_twCurImgSize = 0;

//		CWaitCursor waitCur;

		while (bPendingXfers)
		{
			if (!GetAndCheckImageAttribute()) return -1;

			DeleteImageData();

			//获得每次传送中，传送的图片块大小
			rc = (*m_pDSM_Entry)(&m_twAppID,
				&m_twSourceID,
				DG_CONTROL,
				DAT_SETUPMEMXFER,
				MSG_GET,
				(TW_MEMREF)&tw_SetupMem);
			twRC = rc;
			if (rc != TWRC_SUCCESS)
			{
				SetErrorInfoByReturnCode(rc);
				return -1;
			}

			tw_uSize = (tw_SetupMem.Preferred / 512 + 1) * 512;
			pXferData = (unsigned char*)malloc(tw_uSize);
			if (NULL == pXferData)
			{
				SetErrorInfo(ENO_TWCCBASE + TWCC_LOWMEMORY);
				return -1;
			}

			tw_XferMem.Memory.Flags = TWMF_APPOWNS | TWMF_POINTER;
			tw_XferMem.Memory.Length = tw_SetupMem.Preferred;
			tw_XferMem.Memory.TheMem = pXferData;

			tw_uWrittenBytes = 0;

			//循环接受数据
			do {
				rc = (*m_pDSM_Entry)(&m_twAppID, &m_twSourceID, DG_IMAGE,
					DAT_IMAGEMEMXFER, MSG_GET, (TW_MEMREF)&tw_XferMem);
				twRC = rc;

				if (!ModifyImageBufAndSizeByXfer(&tw_XferMem)) return -1;

				switch (rc)
				{
				case TWRC_SUCCESS:
				{
					if (tw_uWrittenBytes + tw_XferMem.BytesWritten <= m_twCurImgSize)
					{
						::memcpy((unsigned char *)(m_ptwImageData + tw_uWrittenBytes), pXferData, tw_XferMem.BytesWritten);
						tw_uWrittenBytes += tw_XferMem.BytesWritten;
					}
				}
				break;

				case TWRC_XFERDONE:
				{
					//Successful Transfer all data.
					if (tw_uWrittenBytes + tw_XferMem.BytesWritten <= m_twCurImgSize)
					{
						::memcpy((unsigned char *)(m_ptwImageData + tw_uWrittenBytes), pXferData, tw_XferMem.BytesWritten);
						tw_uWrittenBytes += tw_XferMem.BytesWritten;
					}
					free(pXferData);
					m_twStatus.bIsAcquire = 1;
					bPendingXfers = DoEndTransfer();
					break;
				}

				case TWRC_CANCEL:
				{
					m_twStatus.bIsCancel = 1;
					bPendingXfers = DoEndTransfer();
					DeleteImageData();
					free(pXferData);
					break;
				}

				case TWRC_FAILURE:
				{
					SetErrorInfoByReturnCode(twRC);
					DoAbortTransfer(MSG_RESET);
					bPendingXfers = FALSE;
					DeleteImageData();
					free(pXferData);
					break;;
				}
				}
			} while (rc == TWRC_SUCCESS);
		}

		return 1;
	};

	//扫描仪消息处理,截获所有发送到窗口的消息，分析、处理属于扫描仪的消息
	auto pre_trans_message = [&](MSG *pMsg, TW_UINT16 &TWMessage)->int
	{
		TW_EVENT tw_Event;
		TW_UINT16 tw_retval, twRCXfReady;

		TWMessage = MSG_NULL;
	/*	m_nErrorCode = ENO_SUCCESS;

		m_bScannerBusy = FALSE;*/

		memset(&tw_Event, 0, sizeof(TW_EVENT));
		tw_retval = TWRC_NOTDSEVENT;
		// See if source has a message for us.

		tw_Event.pEvent = (TW_MEMREF)pMsg;
		tw_Event.TWMessage = MSG_NULL;
		tw_retval = _DSM_Entry(&m_twAppID,
			&m_twSourceID,
			DG_CONTROL,
			DAT_EVENT,
			MSG_PROCESSEVENT,
			(TW_MEMREF)&tw_Event);

		switch (tw_Event.TWMessage)
		{
		case MSG_XFERREADY:
		{              
			//scanner is ready,begin transfer data.
			//m_twStatus.nCurrentState = 6;
			do_memory_transfer_imagedata(twRCXfReady);
			//ResetSource();
			break;
		}
		case MSG_CLOSEDSREQ:
		{
			//ResetSource();
			break;
		}
		case MSG_CLOSEDSOK:   //NOT in 1.5
		{
			//ResetSource();
			break;
		}
		case MSG_NULL:
		default:
			break;
		}

		//if ( tw_retval == TWRC_NOTDSEVENT ) return -1;
		if (tw_retval != TWRC_DSEVENT) return -1;

		TWMessage = tw_Event.TWMessage;
		return 1;
	};

	//Scanner_ProcessMessage
	{
		MSG		msgMsg;
		TW_UINT16 TWMessage;

		int bCancel = 0;
		while (GetMessage(&msgMsg, NULL, 0, 0))
		{
			if (pre_trans_message(&msgMsg, TWMessage) < 0)
			{
				// 非 TWAIN 消息
				TranslateMessage(&msgMsg);
				DispatchMessage(&msgMsg);
				continue;
			}

			/*if (m_Scanner.m_nErrorCode == CScanner::ENO_SUCCESS)
			{
				if (m_Scanner.IsFinishedSuccess()) Scanner_ScanFinish();
				else if (m_Scanner.ISCancelScan()) bCancel = 1;
			}

			if ((TWMessage == MSG_CLOSEDSREQ) || (TWMessage == MSG_CLOSEDSOK) || (TWMessage == MSG_XFERREADY))
			{
				return m_Scanner.m_nErrorCode;
			}*/

			continue;
		}
	}

	return 0;
}

TW_UINT16 _DSM_Entry(
	pTW_IDENTITY _pOrigin,
	pTW_IDENTITY _pDest,
	TW_UINT32    _DG,
	TW_UINT16    _DAT,
	TW_UINT16    _MSG,
	TW_MEMREF    _pData)
{
	if (_pOrigin == NULL)
		return -1;

	/*
	//OpenSourceManager
	(&m_twAppID, NULL,			DG_CONTROL, DAT_PARENT,			MSG_OPENDSM,	(TW_MEMREF)&m_hParentWnd);
	
	//SelectSource
	(&m_twAppID, NULL,			DG_CONTROL, DAT_IDENTITY,		MSG_USERSELECT, (TW_MEMREF)&m_twSourceID);
	(&m_twAppID, NULL,			DG_CONTROL, DAT_IDENTITY,		MSG_GETDEFAULT, (TW_MEMREF)&m_twSourceID);
	
	//OpenSource
	(&m_twAppID, NULL,			DG_CONTROL, DAT_IDENTITY,		MSG_OPENDS,		(TW_MEMREF)&m_twSourceID);
	
	//SetDefaultAttribute
	//：
		//SetImagePixelType
		//SetImageUnitType
		//SetImageDPI
		//SetTransferMode
		(&m_twAppID, &m_twSourceID, DG_CONTROL,	DAT_CAPABILITY,		MSG_SET,		(TW_MEMREF)&tw_Cap);
	
		//SetImageFrame
		(&m_twAppID, &m_twSourceID, DG_IMAGE,	DAT_IMAGELAYOUT,	MSG_SET,		(TW_MEMREF)&tw_LayOut);

	//EnableScanner
	(&m_twAppID, &m_twSourceID, DG_CONTROL,	DAT_USERINTERFACE,	MSG_ENABLEDS,	(TW_MEMREF)&m_twShowUI);
	
	//PreTransMessage
	(&m_twAppID,&m_twSourceID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, (TW_MEMREF)&tw_Event);

	//get error
	(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_STATUS, MSG_GET, (TW_MEMREF)&m_twStatus.status);

	//CloseSource
	(&m_twAppID,	NULL,	DG_CONTROL,	DAT_IDENTITY,	MSG_CLOSEDS,	(TW_MEMREF) &m_twSourceID );

	//CloseDSM
	(&m_twAppID,	NULL,	DG_CONTROL,	DAT_PARENT,	MSG_CLOSEDSM,	(TW_MEMREF) &m_hParentWnd);
	*/

	auto open_source_manager = [&]()
	{
		//(&m_twAppID, NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF)&m_hParentWnd);

		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_PARENT);
		assert(_MSG == MSG_OPENDSM);

		g_param._twAppID = *_pOrigin;
		g_param._twAppID.Id = 1;
		
		g_param._hParentWnd = *(HWND *)_pData;
		
		return TWRC_SUCCESS;
	};

	auto select_source = [&]()
	{
		//(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, (TW_MEMREF)&m_twSourceID);
		//(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, (TW_MEMREF)&m_twSourceID);
		
		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_IDENTITY);
		assert(_MSG == MSG_USERSELECT || _MSG == MSG_GETDEFAULT);

		TW_IDENTITY	twAppID = *_pOrigin;
		TW_IDENTITY	&twSourceID = *(TW_IDENTITY *)_pData;
		
		if (twAppID.Id != g_param._twAppID.Id)
			return TWRC_FAILURE;

		twSourceID.Id = 1;

		g_param._twSourceID = twSourceID;

		return TWRC_SUCCESS;
	};

	auto open_source = [&]()
	{
		//(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, (TW_MEMREF)&m_twSourceID);

		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_IDENTITY);
		assert(_MSG == MSG_OPENDS);

		TW_IDENTITY	&twAppID = *_pOrigin;
		TW_IDENTITY	&twSourceID = *(TW_IDENTITY *)_pData;

		if (twAppID.Id != g_param._twAppID.Id)
			return TWRC_FAILURE;

		if (twSourceID.Id != 0)
		{
			//already selected
			if (twSourceID.Id != g_param._twSourceID.Id)
				return TWRC_FAILURE;
		}
		else
		{
			//select default
			twSourceID.Id = 1;
			g_param._twSourceID = twSourceID;
		}

		return TWRC_SUCCESS;
	};

	auto set_default_attribute = [&]()
	{
		//(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_CAPABILITY, MSG_SET, (TW_MEMREF)&tw_Cap);

		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_CAPABILITY);
		assert(_MSG == MSG_SET);

		TW_IDENTITY	&twAppID = *_pOrigin;
		TW_IDENTITY	&twSourceID = *(TW_IDENTITY *)_pDest;
		TW_CAPABILITY &tw_Cap = *(TW_CAPABILITY *)_pData;

		if (twAppID.Id != g_param._twAppID.Id || twSourceID.Id != g_param._twSourceID.Id)
			return TWRC_FAILURE;

		if (tw_Cap.hContainer == NULL)
			return TWRC_FAILURE;

		auto get_cap_value = [&]()
		{
			pTW_ONEVALUE pval = NULL;
			pval = (pTW_ONEVALUE)(::GlobalLock(tw_Cap.hContainer));
			
			//pval->ItemType = TWTY_UINT16;
			//pval->Item = m_twImgPixelType;	//TWPT_GRAY;

			/*pval->ItemType = TWTY_UINT16;
			pval->Item = TWSX_MEMORY;*/

			/* ICAP_XFERMECH values (SX_ means Setup XFer) */
//#define TWSX_NATIVE      0
//#define TWSX_FILE        1
//#define TWSX_MEMORY      2
//#define TWSX_FILE2       3    /* added 1.9 */

			int val = 0;
			if (pval->ItemType == TWTY_UINT16)
				val = pval->Item;
			else if (pval->ItemType == TWTY_FIX32)
			{
				val = FIX32ToFloat(*(pTW_FIX32)&pval->Item);
			}
			
			/*pval->ItemType = TWTY_FIX32;
			TW_FIX32 fix32 = FloatToFix32((float)m_twImgDpi);
			pval->Item = *((pTW_UINT32)&fix32);*/

			::GlobalUnlock(tw_Cap.hContainer);
			pval = NULL;

			return val;
		};

		/*tw_Cap.Cap = ICAP_PIXELTYPE;
		tw_Cap.ConType = TWON_ONEVALUE;*/

		switch (tw_Cap.Cap)
		{
		case ICAP_PIXELTYPE:
			g_param._twImgPixelType = get_cap_value();
			break;
		case ICAP_UNITS:
			g_param._twImgUnitType = get_cap_value();
			break;
		case ICAP_XRESOLUTION:
			g_param._dpiX = get_cap_value();
			break;
		case ICAP_YRESOLUTION:
			g_param._dpiY = get_cap_value();
			break;
		case ICAP_XFERMECH:
			g_param._twTransferMode = get_cap_value();
			break;
		default:
			return TWRC_FAILURE;
		}

		return TWRC_SUCCESS;
	};

	auto set_image_frame = [&]()
	{
		//(&m_twAppID, &m_twSourceID, DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET, (TW_MEMREF)&tw_LayOut);

		assert(_DG == DG_IMAGE);
		assert(_DAT == DAT_IMAGELAYOUT);
		assert(_MSG == MSG_SET);

		TW_IDENTITY		twAppID = *_pOrigin;
		TW_IDENTITY		twSourceID = *_pDest;
		TW_IMAGELAYOUT	&tw_LayOut = *(TW_IMAGELAYOUT *)_pData;
		TW_FRAME		tw_Frame = tw_LayOut.Frame;
		RECT			rtFrame;	//扫描区域

		if (twAppID.Id != g_param._twAppID.Id || twSourceID.Id != g_param._twSourceID.Id)
			return TWRC_FAILURE;

		float fdpi = g_param._dpiX;
		rtFrame.left = FIX32ToFloat(tw_Frame.Left) * fdpi;
		rtFrame.top = FIX32ToFloat(tw_Frame.Top) * fdpi;
		rtFrame.right = FIX32ToFloat(tw_Frame.Right) * fdpi;
		rtFrame.bottom = FIX32ToFloat(tw_Frame.Bottom) * fdpi;

		g_param._rtFrame = rtFrame;
		g_param._tw_LayOut = tw_LayOut;

		return TWRC_SUCCESS;
	};

	auto enable_scanner = [&]()
	{
		//(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, (TW_MEMREF)&m_twShowUI);

		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_USERINTERFACE);
		assert(_MSG == MSG_ENABLEDS);

		TW_IDENTITY	&twAppID = *_pOrigin;
		TW_IDENTITY	&twSourceID = *(TW_IDENTITY *)_pDest;
		TW_USERINTERFACE &twShowUI = *(TW_USERINTERFACE *)_pData;

		if (twAppID.Id != g_param._twAppID.Id || twSourceID.Id != g_param._twSourceID.Id)
			return TWRC_FAILURE;

		/*m_twShowUI.ShowUI = m_twStatus.bShowUI;
		m_twShowUI.hParent = (HANDLE)m_hParentWnd;*/
		g_param._twShowUI.ShowUI = 0;
		g_param._twShowUI.hParent = (HANDLE)twShowUI.hParent;

		return TWRC_SUCCESS;
	};

	auto pre_trans_msg = [&]()
	{
		//(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, (TW_MEMREF)&tw_Event);

		assert(_DG == DG_CONTROL);
		assert(_DAT == DAT_EVENT);
		assert(_MSG == MSG_PROCESSEVENT);

		TW_IDENTITY	&twAppID = *_pOrigin;
		TW_IDENTITY	&twSourceID = *(TW_IDENTITY *)_pDest;
		TW_EVENT &tw_Event = *(TW_EVENT *)_pData;

		if (twAppID.Id != g_param._twAppID.Id || twSourceID.Id != g_param._twSourceID.Id)
			return TWRC_FAILURE;

		/*tw_Event.pEvent = (TW_MEMREF)pMsg;
		tw_Event.TWMessage = MSG_NULL;*/

		//switch (tw_Event.TWMessage)
		//{
		//case MSG_XFERREADY:
		//{
		//	//scanner is ready,begin transfer data.
		//	//m_twStatus.nCurrentState = 6;
		//	//DoMemoryTransferImageData(twRCXfReady);
		//	//ResetSource();
		//	break;
		//}
		//case MSG_CLOSEDSREQ:
		//{
		//	//ResetSource();
		//	break;
		//}
		//case MSG_CLOSEDSOK:   //NOT in 1.5
		//{
		//	//ResetSource();
		//	break;
		//}
		//case MSG_NULL:
		//default:
		//	break;
		//}

		tw_Event.TWMessage = MSG_XFERREADY;

		return TWRC_SUCCESS;
	};

	auto process_msg = [&]()
	{
		switch (_MSG)
		{
		case MSG_OPENDSM:

		case MSG_CLOSEDSM:

		case MSG_USERSELECT:

		case MSG_GETDEFAULT:

		case MSG_OPENDS:

		case MSG_CLOSEDS:

		case MSG_SET:

		case MSG_ENABLEDS:

			break;
		default:
			return -1;
		}

		return 0;
	};

	auto process_dat = [&](auto msg)
	{
		switch (_DAT)
		{
		case DAT_PARENT:

			//OpenSourceManager
			//CloseDSM
		case DAT_IDENTITY:

			//SelectSource
			//OpenSource
			//CloseSource
		case DAT_CAPABILITY:
		case DAT_IMAGELAYOUT:
		case DAT_USERINTERFACE:

			break;
		default:

			return -1;
		}
		
		return msg();
	};

	auto process_dg = [&](auto dat)
	{
		//#define DG_CONTROL          0x0001L /* data pertaining to control       */
		//#define DG_IMAGE            0x0002L /* data pertaining to raster images */
		//	/* Added 1.8 */
		//#define DG_AUDIO            0x0004L /* data pertaining to audio */

		switch (_DG)
		{
		case DG_CONTROL:
		case DG_IMAGE:

			break;
		default:
			return -1;
		}

		return dat(process_msg);
	};

	auto param_init = [&]()
	{
		g_param.map_msg_func.clear();

		//OpenSourceManager
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_PARENT, MSG_OPENDSM)] = open_source_manager;
		
		//SelectSource
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT)] = select_source;
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT)] = select_source;

		//OpenSource
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_IDENTITY, MSG_OPENDS)] = open_source;

		//SetDefaultAttribute
		//：
		//SetImagePixelType
		//SetImageUnitType
		//SetImageDPI
		//SetTransferMode
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_CAPABILITY, MSG_SET)] = set_default_attribute;

		//SetImageFrame
		g_param.map_msg_func[std::make_tuple(DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET)] = set_image_frame;

		//EnableScanner
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS)] = enable_scanner;

		//PreTransMessage
		g_param.map_msg_func[std::make_tuple(DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT)] = pre_trans_msg;

		//get error
		//(&m_twAppID, &m_twSourceID, DG_CONTROL, DAT_STATUS, MSG_GET, (TW_MEMREF)&m_twStatus.status);

		//CloseSource
		//(&m_twAppID, NULL, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, (TW_MEMREF)&m_twSourceID);

		//CloseDSM
		//(&m_twAppID, NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, (TW_MEMREF)&m_hParentWnd);
	};

	static bool g_bInit = false;

	if (!g_bInit)
	{
		param_init();
		g_bInit = true;
	}

	//check parameter
	if(0 > process_dg(process_dat))
		return TWRC_FAILURE;

	//call function
	Param::dg_dat_msg_t tp = std::make_tuple(_DG, _DAT, _MSG);
	auto itr = g_param.map_msg_func.find(tp);
	if (itr == g_param.map_msg_func.end())
	{
		return TWRC_FAILURE;
	}

	auto func = itr->second;
	if(std::is_function<decltype(func)>::value)
		func();
	
	return TWRC_SUCCESS;
}