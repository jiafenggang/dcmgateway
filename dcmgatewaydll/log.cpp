#include"stdafx.h"
#include"log.h"
#include<time.h>

JLog::JLog(const std::string& fileName):m_hFile(INVALID_HANDLE_VALUE),m_fileName(fileName){
	::InitializeCriticalSection(&m_hCriticalSection);
}
JLog::~JLog(){
	Close();
}
bool JLog::OpenFile(){
	if (IsOpen()){
		return true;
	}
	//OPEN_ALWAYS->打开一个已有文件，如果文件存在直接打开，不存在就创建.
	m_hFile = CreateFileA(m_fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE){
		Close();
		char p[20];
		sprintf(p, "错误代码：%d", GetLastError());
		std::string str = "创建日志文件失败,日志功能不在可用！请重启应用或检查应用权限是否可以创建文件，";
		str += p;
		MessageBoxA(NULL,str.c_str(), "log", MB_OK);
		return false;
	}
	//定位到末尾
	SetFilePointer(m_hFile, 0, NULL, FILE_END);
	return true;
}
void JLog::Close(){
	if (m_hFile != INVALID_HANDLE_VALUE){
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	::DeleteCriticalSection(&m_hCriticalSection);
}
bool JLog::IsOpen(){
	return m_hFile != INVALID_HANDLE_VALUE;
}
std::string JLog::GetStrTime(){
	std::string str;
	time_t now;
	time(&now);
	char p[30];
	strftime(p, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
	str = p;
	return str;
}
std::string JLog::GetLogHeader(ErrLevel errLevel){
	std::string str;
	switch (errLevel)
	{
	case FATAL_ERROR:
		str = GetStrTime() + "->×<FATAL_ERROR>:";
		break;
	case FIRST_ERROR:
		str = GetStrTime() + "->×  <FIRST_ERROR>:";
		break;
	case SECOND_ERROR:
		str = GetStrTime() + "->×    <SECOND_ERROR>:";
		break;
	case THIRD_ERROR:
		str = GetStrTime() + "->×      <THIRD_ERROR>:";
		break;
	case NORMAL_LOG:
		str = GetStrTime() + "->√:";
		break;
	}
	return str;
}
void JLog::LockSection(){
	::EnterCriticalSection(&m_hCriticalSection);
}
void JLog::UnLockSection(){
	::LeaveCriticalSection(&m_hCriticalSection);
}

void JLog::LogIn(const std::string& content, ErrLevel errLevel, const std::string& file, int line){
	__try{
		LockSection();
		WriteLog(content, errLevel,file,line);
	}
	__finally{
		UnLockSection();
	}
}
void JLog::WriteLog(const std::string& content, ErrLevel errLevel, const std::string& file, int line){
	OpenFile();
	std::string str(GetLogHeader(errLevel));
	str += content;
	str += file;
	//如果line不是默认的
	if (line != 0){
		char p[20];
		sprintf(p, "line:%d", line);
		str += p;
	}
	str+="\n";
	DWORD dw1;
	WriteFile(m_hFile, str.c_str(), str.length(), &dw1, NULL);
}