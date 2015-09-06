#pragma once


#include<map>
#include<string>
#include<vector>
//用户不一定有dcmtk，所以使用这个包装，不用dcmtk中tag。如果要扩充：改变此处和AddDatasetInfo
//扩充：a.在此处添加支持的信息类型;b.重写DcmGateway::AddDatasetInfo
enum M_DCM_GATE_TAG
{
	GATE_PATIENT_NAME,
	GATE_PATIENT_ID,
	GATE_PATIENT_BIRTH_DATE,
	GATE_PATENT_SEX,
	GATE_STUDY_INSTANCE_UID,
	GATE_STUDY_DATE,
	GATE_STUDY_TIME,
	GATE_REFERRING_PHYSICIAN_NAME,
	GATE_STUDY_ID,
	GATE_ACCESSION_NUMBER,
	GATE_MODALITY,
	GATE_SERIES_INSTANCE_UID,
	GATE_SERIES_NUMBER,
	//GATE_CONVERSION_TYPE, dcmtk自动生成
	GATE_INSTANCE_NUMBER,
	GATE_IMAGE_POSITION
};
//扩充支持的图像类型
//扩充：a.此处添加支持的图片类型;b.DcmGateway::Pic2Bmp添加实现
enum M_PIC_TYPE{
	PIC_FORMAT_BMP,
	PIC_FORMAT_JPEG,
	PIC_FORMAT_GIF,
	PIC_FORMAT_TIFF,
	PIC_FORMAT_PNG,
	PIC_FORMAT_RAW
};

//InfoList->要添加的信息类型，（tag，值）列表
typedef std::map<M_DCM_GATE_TAG, std::string> InfoListType;
typedef InfoListType::value_type InfoElemType;
