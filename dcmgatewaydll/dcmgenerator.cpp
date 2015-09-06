#include"stdafx.h"

#define GATE_GENERATOR_LIBAPI __declspec(dllexport)

#include"dcmgenerator.h"
#include"log.h"


extern HANDLE hTimer = NULL;
HANDLE hEventShutDown=NULL;
JLog gLog("gatewaylog.log");

void mthread(void * v){
	DcmGenerator* dcm = (DcmGenerator*)v;
	HANDLE hand[2];
	hand[0] = hTimer;
	hand[1] = hEventShutDown;
	while (true){
		DWORD dw = WaitForMultipleObjects(2, hand, FALSE, INFINITE); {
			switch (dw)
			{
			case WAIT_OBJECT_0 + 0:
				dcm->Convert();
				printf("ʱ�䵽��");
				break;
			case WAIT_OBJECT_0 + 1:
				printf("Ҫ�˳���");
				return;
			}
		}
	}
}
//�û��������ӡ��Ͽ�����
DcmGenerator::DcmGenerator(DBHelper* dbhelper):m_dbHelper(dbhelper){
}
DcmGenerator::~DcmGenerator(){
	
}

bool DcmGenerator::StartServ(){
	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	hEventShutDown = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hTimer || !hEventShutDown){
		errInfo = "������ʱ��ʧ�ܣ������˳�";
		gLog.LogIn(errInfo, FATAL_ERROR, __FILE__, __LINE__);
		return false;
	}
	if (!_beginthread(mthread, 0, this)){
		CloseHandle(hTimer);
		CloseHandle(hEventShutDown);
		hTimer = NULL;
		hEventShutDown = NULL;
		errInfo = "��������߳�ʧ�ܣ������˳�";
		gLog.LogIn(errInfo, FATAL_ERROR, __FILE__, __LINE__);
		return false;
	}

	LARGE_INTEGER li;
	li.QuadPart = 0;
	if (!SetWaitableTimer(hTimer, &li, 5000, NULL, NULL, FALSE)){
		SetEvent(hEventShutDown);
		CloseHandle(hTimer);
		CloseHandle(hEventShutDown);
		hTimer = NULL;
		hEventShutDown = NULL;
		errInfo = "������ʱ��ʧ�ܣ������˳�";
		gLog.LogIn(errInfo, FATAL_ERROR, __FILE__, __LINE__);
		return false;
	}
	gLog.LogIn("��������ɹ�...",NORMAL_LOG);
	return true;
}
bool DcmGenerator::StopServ(){
	//ȡ����ʱ��
	CancelWaitableTimer(hTimer);
	CloseHandle(hTimer);
	//������ʱ���߳�
	SetEvent(hEventShutDown);
	CloseHandle(hEventShutDown);
	hTimer = NULL;
	hEventShutDown = NULL;
	gLog.LogIn("�������...", NORMAL_LOG);
	return true;
}
//->m_dbHelper->Retrive:��������û��ת���ļ�¼
//->
bool DcmGenerator::Convert(){
	//->m_dbHelper->Retrive:��������û��ת���ļ�¼
	std::vector<InfoListType> records;
	if (!m_dbHelper->Retrive(records)){
		gLog.LogIn("���ݿ��ѯʧ�ܣ�", NORMAL_LOG);
		return false;
	}
	if (records.size() < 1){
		gLog.LogIn("��ǰ���ݿ�û�п��ü�¼��", NORMAL_LOG);
		return true;
	}
	//->���ÿһ����¼���д���
	for (auto iter = records.cbegin(); iter != records.cend(); ++iter){
		InfoListType record = *iter;
		if (!ProcessRecord(record)){
			gLog.LogIn("һ����¼����ʧ�ܣ�", FIRST_ERROR);
		}
		gLog.LogIn("һ����¼����ɹ���", NORMAL_LOG);
		std::string pid = (record.find(GATE_PATIENT_ID))->second;
		if (!m_dbHelper->UpdateDcmFlag(pid)){
			errInfo = "���¼�¼���ʧ�ܣ�������ѭ����Σ��";
			gLog.LogIn(errInfo, FATAL_ERROR);
			return false;
		}
	}
	return true;
}
bool DcmGenerator::ProcessRecord(const InfoListType& record){
	std::string imagePath;
	auto iter = record.find(GATE_IMAGE_POSITION);
	if (iter == record.end()){
		errInfo = "�ü�¼��û�п���ͼƬ,�����д���";
		gLog.LogIn(errInfo, SECOND_ERROR);
		return false;
	}
	imagePath = iter->second;
	std::vector<std::string> files;
	M_PIC_TYPE picType;
	if (!GetDirFiles(imagePath, files, picType)){
		errInfo = imagePath + "Ŀ¼��û�п��õ�ת��ͼƬ";
		gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
		return false;
	}
	//ֱ�е��˴�filesһ����Ϊ��
	switch (picType)
	{
	case PIC_FORMAT_BMP:
		if (!ProcessBmpJpg2dcm(files, record,PIC_FORMAT_BMP)){
			errInfo = "bmp->dcmʧ��";
			gLog.LogIn(errInfo, SECOND_ERROR,__FILE__,__LINE__);
			return false;
		}
		break;
	case PIC_FORMAT_JPEG:
		if (!ProcessBmpJpg2dcm(files, record,PIC_FORMAT_JPEG)){
			errInfo = "jpg->dcmʧ��";
			gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
			return false;
		}
		break;
		//Ҫָ�����λ
	case PIC_FORMAT_RAW:
		//*****************************************
		//--------->��raw��Ϣ��������
		//*****************************************
		RawAttrs rawAttrs;
		if (!ProcessRaw2dcm(files, record,rawAttrs)){
			errInfo = "raw->dcmʧ��";
			gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
			return false;
		}
		break;
	}
	return true;
}
bool DcmGenerator::ProcessRaw2dcm(const std::vector<std::string> files, const InfoListType& record, const RawAttrs& attrs){
	
	std::string outputfile = files[0].substr(0, files[0].find_last_of("."));
	outputfile += ".dcm";
	if (!dcmGateway.MulRaw2Dcm(files, outputfile, attrs, record)){
		errInfo = dcmGateway.GetErrInfo();
		gLog.LogIn(errInfo, THIRD_ERROR, __FILE__, __LINE__);
		return false;
	}
	return true;
}
bool DcmGenerator::ProcessBmpJpg2dcm(const std::vector<std::string> files, const InfoListType& record,M_PIC_TYPE picType){
	if (files.size() == 1){
		if (!dcmGateway.Pic2Dcm(files[0].c_str(), PIC_FORMAT_BMP, record)){
			errInfo = dcmGateway.GetErrInfo();
			gLog.LogIn(errInfo, THIRD_ERROR, __FILE__, __LINE__);
			return false;
		}
	}
	else{
		//ȡ��һ���ļ����ļ���Ϊ���պϳ��ļ��ļ���
		std::string outputfile = files[0].substr(0, files[0].find("."));
		outputfile += ".dcm";
		if (!dcmGateway.MulPic2Dcm(files, outputfile, picType, record)){
			errInfo = dcmGateway.GetErrInfo();
			gLog.LogIn(errInfo, THIRD_ERROR, __FILE__, __LINE__);
			return false;
		}
	}
	return true;
}
bool DcmGenerator::GetDirFiles(const std::string& path,std::vector<std::string>& files, M_PIC_TYPE& picType){
	char cdir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, cdir);
	SetCurrentDirectoryA(path.c_str());

	WIN32_FIND_DATAA wfd;
	//raw��ʽ
	std::string file = path + "\\*.raw";
	HANDLE hfd = FindFirstFileA(file.c_str(), &wfd);
	if (hfd != INVALID_HANDLE_VALUE){
		picType = PIC_FORMAT_RAW;
	}
	//bmp��ʽ
	else{
		file = path + "\\*.bmp";
		hfd = FindFirstFileA(file.c_str(), &wfd);
		if (hfd != INVALID_HANDLE_VALUE){
			picType = PIC_FORMAT_BMP;
		}
		//jpg��ʽ
		else{
			file = path + "\\*.jpg";
			hfd = FindFirstFileA(file.c_str(), &wfd);
			if (hfd != INVALID_HANDLE_VALUE){
				picType = PIC_FORMAT_JPEG;
			}
			else{
				//������ʽ�ݲ�֧��
				errInfo = "Ŀǰֻ�ṩ��bmp/raw/jpg��ʽ��֧��";
				SetCurrentDirectoryA(cdir);
				gLog.LogIn(errInfo, THIRD_ERROR, __FILE__, __LINE__);
				return false;
			}
		}
	}
	do{
		files.push_back(path+"/"+std::string(wfd.cFileName));
	} while (FindNextFileA(hfd, &wfd) != NULL);

	//CloseHandle(hfd); �ᱼ��������INVALID_HANDLE_VALUE
	SetCurrentDirectoryA(cdir);
	return true;
}
std::string DcmGenerator::GetErrInfo(){
	return errInfo;
}




