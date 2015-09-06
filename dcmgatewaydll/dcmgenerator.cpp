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
				printf("时间到了");
				break;
			case WAIT_OBJECT_0 + 1:
				printf("要退出了");
				return;
			}
		}
	}
}
//用户负责连接、断开连接
DcmGenerator::DcmGenerator(DBHelper* dbhelper):m_dbHelper(dbhelper){
}
DcmGenerator::~DcmGenerator(){
	
}

bool DcmGenerator::StartServ(){
	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	hEventShutDown = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hTimer || !hEventShutDown){
		errInfo = "创建定时器失败！程序退出";
		gLog.LogIn(errInfo, FATAL_ERROR, __FILE__, __LINE__);
		return false;
	}
	if (!_beginthread(mthread, 0, this)){
		CloseHandle(hTimer);
		CloseHandle(hEventShutDown);
		hTimer = NULL;
		hEventShutDown = NULL;
		errInfo = "开启监控线程失败！程序退出";
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
		errInfo = "开启计时器失败！程序退出";
		gLog.LogIn(errInfo, FATAL_ERROR, __FILE__, __LINE__);
		return false;
	}
	gLog.LogIn("开启服务成功...",NORMAL_LOG);
	return true;
}
bool DcmGenerator::StopServ(){
	//取消定时器
	CancelWaitableTimer(hTimer);
	CloseHandle(hTimer);
	//结束定时器线程
	SetEvent(hEventShutDown);
	CloseHandle(hEventShutDown);
	hTimer = NULL;
	hEventShutDown = NULL;
	gLog.LogIn("服务结束...", NORMAL_LOG);
	return true;
}
//->m_dbHelper->Retrive:检索出还没有转换的记录
//->
bool DcmGenerator::Convert(){
	//->m_dbHelper->Retrive:检索出还没有转换的记录
	std::vector<InfoListType> records;
	if (!m_dbHelper->Retrive(records)){
		gLog.LogIn("数据库查询失败！", NORMAL_LOG);
		return false;
	}
	if (records.size() < 1){
		gLog.LogIn("当前数据库没有可用记录！", NORMAL_LOG);
		return true;
	}
	//->针对每一条记录进行处理
	for (auto iter = records.cbegin(); iter != records.cend(); ++iter){
		InfoListType record = *iter;
		if (!ProcessRecord(record)){
			gLog.LogIn("一条记录处理失败！", FIRST_ERROR);
		}
		gLog.LogIn("一条记录处理成功！", NORMAL_LOG);
		std::string pid = (record.find(GATE_PATIENT_ID))->second;
		if (!m_dbHelper->UpdateDcmFlag(pid)){
			errInfo = "更新记录标记失败，进入死循环，危险";
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
		errInfo = "该记录下没有可用图片,不进行处理";
		gLog.LogIn(errInfo, SECOND_ERROR);
		return false;
	}
	imagePath = iter->second;
	std::vector<std::string> files;
	M_PIC_TYPE picType;
	if (!GetDirFiles(imagePath, files, picType)){
		errInfo = imagePath + "目录下没有可用的转换图片";
		gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
		return false;
	}
	//直行到此处files一定不为空
	switch (picType)
	{
	case PIC_FORMAT_BMP:
		if (!ProcessBmpJpg2dcm(files, record,PIC_FORMAT_BMP)){
			errInfo = "bmp->dcm失败";
			gLog.LogIn(errInfo, SECOND_ERROR,__FILE__,__LINE__);
			return false;
		}
		break;
	case PIC_FORMAT_JPEG:
		if (!ProcessBmpJpg2dcm(files, record,PIC_FORMAT_JPEG)){
			errInfo = "jpg->dcm失败";
			gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
			return false;
		}
		break;
		//要指定宽高位
	case PIC_FORMAT_RAW:
		//*****************************************
		//--------->对raw信息进行设置
		//*****************************************
		RawAttrs rawAttrs;
		if (!ProcessRaw2dcm(files, record,rawAttrs)){
			errInfo = "raw->dcm失败";
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
		//取第一个文件的文件名为最终合成文件文件名
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
	//raw格式
	std::string file = path + "\\*.raw";
	HANDLE hfd = FindFirstFileA(file.c_str(), &wfd);
	if (hfd != INVALID_HANDLE_VALUE){
		picType = PIC_FORMAT_RAW;
	}
	//bmp格式
	else{
		file = path + "\\*.bmp";
		hfd = FindFirstFileA(file.c_str(), &wfd);
		if (hfd != INVALID_HANDLE_VALUE){
			picType = PIC_FORMAT_BMP;
		}
		//jpg格式
		else{
			file = path + "\\*.jpg";
			hfd = FindFirstFileA(file.c_str(), &wfd);
			if (hfd != INVALID_HANDLE_VALUE){
				picType = PIC_FORMAT_JPEG;
			}
			else{
				//其它格式暂不支持
				errInfo = "目前只提供对bmp/raw/jpg格式的支持";
				SetCurrentDirectoryA(cdir);
				gLog.LogIn(errInfo, THIRD_ERROR, __FILE__, __LINE__);
				return false;
			}
		}
	}
	do{
		files.push_back(path+"/"+std::string(wfd.cFileName));
	} while (FindNextFileA(hfd, &wfd) != NULL);

	//CloseHandle(hfd); 会奔溃，报错INVALID_HANDLE_VALUE
	SetCurrentDirectoryA(cdir);
	return true;
}
std::string DcmGenerator::GetErrInfo(){
	return errInfo;
}




