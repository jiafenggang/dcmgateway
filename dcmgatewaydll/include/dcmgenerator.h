#pragma once

#ifdef GATE_GENERATOR_LIBAPI
#else
#define GATE_GENERATOR_LIBAPI __declspec(dllimport)
#endif

#include"DBHelper.h"
#include"dcmgateway.h"
#include<Windows.h>
#include<process.h>
//Ŀǰ֧��jpeg/bmp/raw�����Ҫ���䣬����ProcessRecord��GetDirFiles
//ProcessRaw2dcm:�ļ��������ߡ�λ
class GATE_GENERATOR_LIBAPI DcmGenerator{
public:
	//����ʱ�����Ҫ��DBHelper
	DcmGenerator(DBHelper*);
	~DcmGenerator();
	//���ܣ��������ݿ��б�[dbo].[gateway]������û��ת��Ϊdcm�ļ�¼����ת���������ɵ�ͼƬ�洢����Ӧλ�ã����޸����ɱ��
	//���أ��ɹ�����true��ʧ�ܷ���false,������Ϣ�鿴errInfo
	//ǰ�᣺���ݿ��Ѿ����ӳɹ�
	bool Convert();
	//���ܣ���������
	//���أ��ɹ�����true��ʧ�ܷ���false,������Ϣ�鿴errInfo
	bool StartServ();
	//���ܣ��������� 
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

