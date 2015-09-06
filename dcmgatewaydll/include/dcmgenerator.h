#pragma once

#ifdef GATE_GENERATOR_LIBAPI
#else
#define GATE_GENERATOR_LIBAPI __declspec(dllimport)
#endif

#include"DBHelper.h"
#include"dcmgateway.h"
#include<Windows.h>
#include<process.h>
//目前支持jpeg/bmp/raw，如果要扩充，扩充ProcessRecord和GetDirFiles
//ProcessRaw2dcm:文件名、宽、高、位
class GATE_GENERATOR_LIBAPI DcmGenerator{
public:
	//构造时候必须要用DBHelper
	DcmGenerator(DBHelper*);
	~DcmGenerator();
	//功能：检索数据库中表[dbo].[gateway]，将还没有转换为dcm的记录进行转换，将生成的图片存储到相应位置，并修改生成标记
	//返回：成功返回true，失败返回false,具体信息查看errInfo
	//前提：数据库已经连接成功
	bool Convert();
	//功能：开启服务
	//返回：成功返回true，失败返回false,具体信息查看errInfo
	bool StartServ();
	//功能：结束服务 
	bool StopServ();
	std::string GetErrInfo();
private:
	bool ProcessRecord(const InfoListType&);
	//*files(out):
	//picType(out):
	bool GetDirFiles(const std::string& path, std::vector<std::string>& files, M_PIC_TYPE& picType);
	bool ProcessBmpJpg2dcm(const std::vector<std::string> files, const InfoListType& record,M_PIC_TYPE picType);
	//bool ProcessRaw2dcm(const std::vector<std::string> files, const InfoListType& record, unsigned short height = 2048, unsigned short width = 2048, unsigned char bitcount = 16, unsigned char highbit = 11);
	bool ProcessRaw2dcm(const std::vector<std::string> files, const InfoListType& record, const RawAttrs& attr);
	DBHelper* m_dbHelper;
	std::string errInfo;
	std::string log;
	DcmGateway dcmGateway;
};
extern  HANDLE hTimer;
extern "C" GATE_GENERATOR_LIBAPI extern HANDLE hEventShutDown;
//void  ConvertThread(void* pvParam);

