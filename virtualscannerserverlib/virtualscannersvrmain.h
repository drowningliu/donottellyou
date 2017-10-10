#pragma once
namespace DROWNINGLIU
{
	namespace VSCANNERSVRLIB
	{
#ifdef VIRTUALSCANNERSERVERLIB_EXPORTS
#define GA2NDIDSVRLIB_API __declspec(dllexport)
#else
#define GA2NDIDSVRLIB_API __declspec(dllimport)
#endif

		extern GA2NDIDSVRLIB_API
			int main1(int argc, char* argv[]);

		extern GA2NDIDSVRLIB_API
			int main_client(int argc, char* argv[]);
	};
};