#pragma once
#include"dcmgatewayglobal.h"

#import "lib/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

class __declspec(dllexport) DBHelper{
public:
	//���ܣ�ʹ�ô��ݵĲ������ӵ����ݿ�
	//*connStr:�����ַ���
	//*uid:�û���
	//*pwd:����
	//���أ��ɹ�����true��ʧ�ܷ���false,����ʧ����Ϣ�鿴errInfo
	bool Connect(const std::string& connStr, const std::string& uid, const std::string& pwd);
	//���ܣ�������û��ת����dcm���û���Ϣ��ͼƬλ��
	//*result(out):��Ŷ�����������û���<����ֵ>��Ϣ���Լ�ͼ��λ��
	//���أ��ɹ�����true��ʧ�ܷ���false,����ʧ����Ϣ�鿴errInfo
	bool Retrive(std::vector<InfoListType>& result);
	//���ܣ��������ݿ����Ѿ��ɹ�ת��Ϊdcm�ļ��ļ�¼���Ϊ1
	//*PID:Ҫ���µ��û�ID
	//���أ��ɹ�����true��ʧ�ܷ���false,����ʧ����Ϣ�鿴errInfo
	bool UpdateDcmFlag(std::string PID);
	//���ܣ��Ͽ�����
	void DisConnect();
	std::string GetErrInfo();
private:
	bool ParsRecordset(std::vector<InfoListType>& result);
	void ParsElement(InfoListType& record, std::string key,M_DCM_GATE_TAG tag);
	//"2014/11/2 ������ ���� 12:01:23"->"20141102"
	std::string ParsDate(std::string date);
	//"2014/11/2 ������ ���� 12:01:23"->"120123"
	std::string ParsTime(std::string time);
	std::string errInfo;
	_ConnectionPtr m_pConnect;
	_RecordsetPtr m_pRecordset;
};

