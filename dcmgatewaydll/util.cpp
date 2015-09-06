#include"stdafx.h"
#include"util.h"

PWSTR UTIL::ansi2unic(PCSTR ansiStr){
	PWSTR unicStr=NULL;
	DWORD len = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, unicStr, 0);
	unicStr = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, unicStr, len);
	return unicStr;
}

PSTR UTIL::unic2ansi(PCWSTR unicStr){
	PSTR ansiStr=NULL;
	DWORD len = WideCharToMultiByte(CP_ACP, 0, unicStr, -1, ansiStr, 0,NULL,NULL);
	ansiStr = new char[len];
	WideCharToMultiByte(CP_ACP, 0, unicStr, -1, ansiStr, len, NULL, NULL);
	return ansiStr;
}

std::string UTIL::unic2str(const wchar_t* unicStr){
	const char* p = unic2ansi(unicStr);
	std::string str(p);
	delete p;
	return str;
}

wchar_t* UTIL::str2unic(const std::string& str){
	return ansi2unic(str.c_str());
}