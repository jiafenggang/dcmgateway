#pragma once
#include<string>
#include<Windows.h>
enum ErrLevel{
	//致命错误，要退出://2014-02-03 12:02:06->××××
	FATAL_ERROR,
	//一级错误，该次任务处理失败，但还能继续处理后续任务：2014-02-03 12:02:06->×××
	FIRST_ERROR,
	//二级错误，该次任务中细节处理失败，但还能处理其余细节：2014-02-03 12:02:06->××
	SECOND_ERROR,
	//三级错误，细微错误，不影响程序执行：2014-02-03 12:02:06->×
	THIRD_ERROR,
	//程序执行信息:2014-02-03 12:02:06->√
	NORMAL_LOG
};
class JLog{
public:
	JLog(const std::string& fileName = "jialog.log");
	~JLog();
	void LogIn(const std::string& content, ErrLevel errLevel, const std::string& file = "",int line = 0);
	
private:
	CRITICAL_SECTION m_hCriticalSection;
	HANDLE m_hFile;
	void LockSection();
	void UnLockSection();
	std::string GetStrTime();
	std::string GetLogHeader(ErrLevel);
	bool IsOpen();
	bool OpenFile();
	void Close();
	void WriteLog(const std::string& content, ErrLevel errLevel, const std::string& file, int line);
	std::string m_fileName;
};