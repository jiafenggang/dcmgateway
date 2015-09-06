#pragma once
#include<Windows.h>
#include<string>

class UTIL
{
public:
	//功能：将char数组 转换为wchar_t数组
	//*ansi->char数组
	//返回：生成的wchar_t数组
	//注意：用户负责释放返回数组内存
	//约定：不会改变用户传入的参数，但用户可以改变返回值
	static PWSTR ansi2unic(PCSTR ansi);
	//功能：将wchar_t数组 转换为char数组
	//*unic->wchar_t数组
	//返回：生成的char数组
	//注意：用户负责释放返回数组内存
	//约定：不会改变用户传入的参数，但用户可以改变返回值
	static PSTR	unic2ansi(PCWSTR unic);
	//功能：将wchar_t数组 转换为string
	//*unic->wchar_t数组
	//返回：生成的string
	//约定：不会改变用户传入的参数，但用户可以改变返回值
	static std::string unic2str(const wchar_t* unic);
	//功能：将string 转换为wchar_t数组
	//*str->要转换的string
	//返回：生成的wchar_t数组
	//注意：用户负责释放返回数组内存
	//约定：不会改变用户传入的参数，但用户可以改变返回值
	static wchar_t* str2unic(const std::string& str);
private:

};
