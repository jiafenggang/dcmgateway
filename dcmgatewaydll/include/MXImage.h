#pragma once 

#ifdef XIMAGE_LIBAPI
#else
#define XIMAGE_LIBAPI __declspec(dllimport)
#endif
#include<string>
#include"ximage.h"
//自定义图像转换类，定义原因：去除原有CxImage的，如果一个格式不识别群举所有格式，去掉这个功能
class XIMAGE_LIBAPI MXImage :public CxImage{
public:
	//功能：覆盖原有类的该功能
	//*filename->文件名
	//*imagetype->文件类型，CXIMAGE_FORMAT_X样式
	//返回：成功返回true，失败返回false,具体信息查看GetErrInfo
	bool Load(const TCHAR * filename, uint32_t imagetype);
	std::string GetErrInfo(){ return errInfo; }
private:
	std::string errInfo;
};
