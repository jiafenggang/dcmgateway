#pragma once 

#ifdef XIMAGE_LIBAPI
#else
#define XIMAGE_LIBAPI __declspec(dllimport)
#endif
#include<string>
#include"ximage.h"
//�Զ���ͼ��ת���࣬����ԭ��ȥ��ԭ��CxImage�ģ����һ����ʽ��ʶ��Ⱥ�����и�ʽ��ȥ���������
class XIMAGE_LIBAPI MXImage :public CxImage{
public:
	//���ܣ�����ԭ����ĸù���
	//*filename->�ļ���
	//*imagetype->�ļ����ͣ�CXIMAGE_FORMAT_X��ʽ
	//���أ��ɹ�����true��ʧ�ܷ���false,������Ϣ�鿴GetErrInfo
	bool Load(const TCHAR * filename, uint32_t imagetype);
	std::string GetErrInfo(){ return errInfo; }
private:
	std::string errInfo;
};
