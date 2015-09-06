#pragma once


#include<map>
#include<string>
#include<vector>
//�û���һ����dcmtk������ʹ�������װ������dcmtk��tag�����Ҫ���䣺�ı�˴���AddDatasetInfo
//���䣺a.�ڴ˴����֧�ֵ���Ϣ����;b.��дDcmGateway::AddDatasetInfo
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
	//GATE_CONVERSION_TYPE, dcmtk�Զ�����
	GATE_INSTANCE_NUMBER,
	GATE_IMAGE_POSITION
};
//����֧�ֵ�ͼ������
//���䣺a.�˴����֧�ֵ�ͼƬ����;b.DcmGateway::Pic2Bmp���ʵ��
enum M_PIC_TYPE{
	PIC_FORMAT_BMP,
	PIC_FORMAT_JPEG,
	PIC_FORMAT_GIF,
	PIC_FORMAT_TIFF,
	PIC_FORMAT_PNG,
	PIC_FORMAT_RAW
};

//InfoList->Ҫ��ӵ���Ϣ���ͣ���tag��ֵ���б�
typedef std::map<M_DCM_GATE_TAG, std::string> InfoListType;
typedef InfoListType::value_type InfoElemType;
