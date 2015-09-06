#include"stdafx.h"
#include"DBHelper.h"
#include"util.h"
#include<sstream>
#include"log.h"
bool DBHelper::Connect(const std::string& connStr, const std::string& uid, const std::string& pwd){
	/*if (FAILED(::CoInitialize(NULL))){
		errInfo = "初始化com失败！";
		gLog.LogIn(errInfo, SECOND_ERROR,__FILE__,__LINE__);
		return false;
	}*/
	CoInitialize(NULL);
	try{
		HRESULT ret;
		ret=m_pConnect.CreateInstance(__uuidof(Connection));
		ret=m_pRecordset.CreateInstance(__uuidof(Recordset));
		ret=m_pConnect->Open(_bstr_t(connStr.c_str()), _bstr_t(uid.c_str()), _bstr_t(pwd.c_str()), adConnectUnspecified);
		return true;
	}
	catch (_com_error& e){
		errInfo = UTIL::unic2str(e.ErrorMessage()) + "连接失败，请检查连接字符串、账号、密码";
		gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
		return false;
	}
}

void DBHelper::DisConnect(){
	/*if (m_pRecordset != NULL){
		m_pRecordset->Close();
		m_pRecordset = NULL;
	}
	if (m_pConnect != NULL){
		m_pConnect->Close();
		m_pConnect = NULL;
	}*/
	m_pConnect->Close();
	::CoUninitialize();
}

bool DBHelper::Retrive(std::vector<InfoListType>& result){
	try{
		HRESULT ret;
		ret = m_pRecordset->Open("select * from [dbo].[gateway] where GenratedDcm=0",m_pConnect.GetInterfacePtr() ,adOpenDynamic, adLockOptimistic, adCmdText);
		if (!ParsRecordset(result)){
			if (m_pRecordset != NULL){
				m_pRecordset->Close();
				//m_pRecordset = NULL;
			}
			return false;
		}
		ret = m_pRecordset->Close();
		//m_pRecordset = NULL;
		return true;
	}
	catch(_com_error& e){
		if (m_pRecordset != NULL){
			m_pRecordset->Close();
			//m_pRecordset = NULL;
		}
		errInfo = UTIL::unic2str(e.ErrorMessage());
		return false;
	}
}

bool DBHelper::ParsRecordset(std::vector<InfoListType>& result){
	try{
		while (!m_pRecordset->adoEOF){
			std::map<M_DCM_GATE_TAG, std::string> oneRecord;
			oneRecord.insert(InfoElemType(GATE_PATIENT_ID,std::string(_bstr_t( m_pRecordset->GetCollect("PatientID")))));
			oneRecord.insert(InfoElemType(GATE_PATIENT_NAME, std::string(_bstr_t(m_pRecordset->GetCollect("PatientName")))));
			//对时间解析
			ParsElement(oneRecord, "PatientBirthDate", GATE_PATIENT_BIRTH_DATE);
			ParsElement(oneRecord, "PatientSex", GATE_PATENT_SEX);
			//oneRecord.insert(InfoElemType(GATE_STUDY_INSTANCE_UID, std::string(_bstr_t(m_pRecordset->GetCollect("StudyInstanceUID")))));
			ParsElement(oneRecord, "StudyInstanceUID", GATE_STUDY_INSTANCE_UID);
			//同时存储GATE_STUDY_DATE、GATE_STUDY_TIME,这里要对时间格式进行解析！
			ParsElement(oneRecord, "StudyDateTime", GATE_STUDY_DATE);
			ParsElement(oneRecord, "ReferringPhysicanName", GATE_REFERRING_PHYSICIAN_NAME);
			ParsElement(oneRecord, "StudyID", GATE_STUDY_ID);
			ParsElement(oneRecord, "Modality", GATE_MODALITY);
			ParsElement(oneRecord, "SeriesInstanceUID", GATE_SERIES_INSTANCE_UID);
			ParsElement(oneRecord, "SeriesNumber", GATE_ACCESSION_NUMBER);
			ParsElement(oneRecord, "InstanceNumber", GATE_INSTANCE_NUMBER);
			ParsElement(oneRecord, "PicPosition", GATE_IMAGE_POSITION);

			result.push_back(oneRecord);
			m_pRecordset->MoveNext();
		}
	}
	catch (_com_error& e){
		errInfo = UTIL::unic2str(e.ErrorMessage()) + "检索记录失败！";
		return false;
	}
}
//处理那些可以为空的，需要判断的元素,此处异常被上面捕获
void DBHelper::ParsElement(InfoListType& record, std::string key, M_DCM_GATE_TAG tag){
	_variant_t variant = m_pRecordset->GetCollect(key.c_str());
	if (variant.vt != VT_NULL){
		std::string str = _bstr_t(variant);
		//"1992/5/27 星期三"->"19920527"
		if (tag == GATE_PATIENT_BIRTH_DATE){
			record.insert(InfoElemType(GATE_PATIENT_BIRTH_DATE, ParsDate(str)));
			return;
		}
		//"2014/11/2 星期日 下午 12:01:23"->"20141102"、"120123"
		else if (tag == GATE_STUDY_DATE){
			record.insert(InfoElemType(GATE_STUDY_DATE, ParsDate(str)));
			record.insert(InfoElemType(GATE_STUDY_TIME, ParsTime(str)));
		}
		else{
			record.insert(InfoElemType(tag, str));
		}
	}
}
//"1992/5/27 星期三"->"19920527" "2014/11/2 星期日 下午 12:01:23"->"20141102" 日期4+2+2位
std::string DBHelper::ParsDate(std::string str){
	int y = 0, m = 0, d = 0;
	sscanf("1992/5/27 星期三", "%d/%d/%d", &y, &m, &d);
	char p[10];
	sprintf(p, "%4d%02d%02d", y, m, d);
	str = p;
	return str;
}
//"2014/11/2 星期日 下午 12:01:23"->"120123"
std::string DBHelper::ParsTime(std::string str){
	int index=str.find_last_of(" ");
	//12:01:23
	std::string time = str.substr(index + 1);
	int h=0, m=0, s=0;
	sscanf(time.c_str(), "%d:%d:%d", &h, &m, &s);
	char p[10];
	sprintf(p, "%02d%02d%02d", h, m, s);
	str = p;
	return str;
}
std::string DBHelper::GetErrInfo(){
	return errInfo;
}

bool DBHelper::UpdateDcmFlag(std::string PID){
	try{
		std::string sq = "update [dbo].[gateway] set GenratedDcm =1 where PatientID='" + PID + "'";
		m_pConnect->Execute(sq.c_str(), NULL, adCmdText);
		//m_pConnect->Execute("update [dbo].[gateway] set SendedDcm=1 where PatientID='PID000001'", NULL, adCmdText);
		return true;
	}
	catch (_com_error& e){
		errInfo = UTIL::unic2str(e.ErrorMessage()) + "更新GenratedDcm失败！";
		gLog.LogIn(errInfo, SECOND_ERROR, __FILE__, __LINE__);
		return false;
	}
	
}