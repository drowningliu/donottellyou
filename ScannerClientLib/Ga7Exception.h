#pragma once

#include <exception>
#include <string>
#include <iostream>
#include <atomic>
#include "time.h"

namespace GAFIS7{
	namespace GA7BASE{

//#define ERRLOG_PATH	"c:\\temp\\errlog.dat"

		class Ga7Exception : public std::exception
		{
		private:
			std::string	_strFileName;
			int			_nLine;
			std::string	_strLine;
			int			_nErrNum;

			static std::atomic<__int64>	_sllErrCount;
		public:
			Ga7Exception() : exception()
			{
				_nLine = 0;
				_sllErrCount++;
				_nErrNum = 0;
			}

			explicit Ga7Exception(const char * const & pszErr) : exception(pszErr)
			{
				_nLine = 0;
				_sllErrCount++;
				_nErrNum = 0;
			}

			Ga7Exception(const char * const & pszFileName, int nLine) :
				_strFileName(pszFileName), _nLine(nLine)
			{
				char	szBuf[32] = { 0 };
				sprintf(szBuf, " line:%d", _nLine);
				_strLine = szBuf;
				_sllErrCount++;
				_nErrNum = 0;
			}

			Ga7Exception(const char * const & pszFileName, int nLine, const char * const & pszErr) :
				exception(pszErr), _strFileName(pszFileName), _nLine(nLine)
			{
				char	szBuf[32] = { 0 };
				sprintf(szBuf, " line:%d", _nLine);
				_strLine = szBuf;
				_sllErrCount++;
				_nErrNum = 0;
			}

			Ga7Exception(const char * const & pszFileName, int nLine, int nErrNum) :
				_strFileName(pszFileName), _nLine(nLine), _nErrNum(nErrNum)
			{
				char	szBuf[32] = { 0 };
				sprintf(szBuf, " line:%d", _nLine);
				_strLine = szBuf;
				_sllErrCount++;
			}

			int getLine()
			{
				return _nLine;
			}

			std::string getLineName()
			{
				return _strLine;
			}

			std::string getFileName()
			{
				return _strFileName;
			}

			__int64 getErrCount()
			{
				return _sllErrCount;
			}

			int getErrNum()
			{
				return _nErrNum;
			}

			int print(std::string strErrAppend = "", std::ostream &out = std::cout) const
			{
				std::string	strPrint;
				strPrint.append("\r\n");

				char	szBuf[32] = { 0 };
				if (_nErrNum != 0)
				{
					sprintf(szBuf, "err num:%d ", _nErrNum);
					strPrint.append(szBuf);
				}

				strPrint.append("file name:");
				strPrint.append(_strFileName);
				//strPrint.append(" line:");
				strPrint.append(_strLine);
				strPrint.append("\r\n");
				strPrint.append(this->what());
				strPrint.append("\r\n");

				if (!strErrAppend.empty())
				{
					strPrint.append(strErrAppend);
					strPrint.append("\r\n");
				}

				//std::cout << strPrint;
				out << strPrint;

				return 0;
			}

			int print2File(FILE *fp, std::string strErrAppend = "") const
			{
				if (fp == NULL)
					return -1;

				std::string	strPrint;
				strPrint.append("\r\n");

				char	szBuf[32] = { 0 };
				if (_nErrNum != 0)
				{
					sprintf(szBuf, "err num:%d ", _nErrNum);
					strPrint.append(szBuf);
				}

				strPrint.append("file name:");
				strPrint.append(_strFileName);
				//strPrint.append(" line:");
				strPrint.append(_strLine);
				strPrint.append("\r\n");
				strPrint.append(this->what());
				strPrint.append("\r\n");

				if (!strErrAppend.empty())
				{
					strPrint.append(strErrAppend);
					strPrint.append("\r\n");
				}

				struct tm	*t = NULL;
				time_t		tt;

				time(&tt);
				t = localtime(&tt);

				char	szBuf2[128] = { 0 };
				snprintf(szBuf2, sizeof(szBuf), "%04d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
				strPrint.append(szBuf2);
				strPrint.append("\r\n");

				//std::cout << strPrint;
				//out << strPrint;
				if (0 >= fwrite(strPrint.c_str(), strPrint.size(), 1, fp))
					return -1;

				return 0;
			}
		};


	}
}
