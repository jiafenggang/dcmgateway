#pragma once
#include"dcmgatewayglobal.h"

#import "lib/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

class __declspec(dllexport) DBHelper{
public:
	//功能：使用传递的参数连接到数据库
	//*connStr:连接字符串
	//*uid:用户名
	//*pwd:密码
	//返回：成功返回true，失败返回false,具体失败信息查看errInfo
	bool Connect(const std::string& connStr, const std::string& uid, const std::string& pwd);
	//功能：检索还没有转换成dcm的用户信息和图片位置
	//*result(out):存放多个检索到的用户的<键，值>信息，以及图像位置
	//返回：成功返回true，失败返回false,具体失败信息查看errInfo
	bool Retrive(std::vector<InfoListType>& result);
	//功能：更新数据库中已经成功转换为dcm文件的记录标记为1
	//*PID:要更新的用户ID
	//返回：成功返回true，失败返回false,具体失败信息查看errInfo
	bool UpdateDcmFlag(std::string PID);
	//功能：断开连接
	void DisConnect();
	std::string GetErrInfo();
private:
	bool ParsRecordset(std::vector<InfoListType>& result);
	void ParsElement(InfoListType& record, std::string key,M_DCM_GATE_TAG tag);
	//"2014/11/2 星期日 下午 12:01:23"->"20141102"
	std::string ParsDate(std::string date);
	//"2014/11/2 星期日 下午 12:01:23"->"120123"
	std::string ParsTime(std::string time);
	std::string errInfo;
	_ConnectionPtr m_pConnect;
	_RecordsetPtr m_pRecordset;
};

