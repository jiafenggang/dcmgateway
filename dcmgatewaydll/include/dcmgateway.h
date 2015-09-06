#pragma once

#ifdef GATE_WAY_LIBAPI
#else
#define GATE_WAY_LIBAPI __declspec(dllimport)
#endif
#include"dcmgatewayglobal.h"
//#include"dcmtk\dcmdata\dcxfer.h"  //���extern  enum E_TransferSyntax;



typedef unsigned long Uint32;
typedef unsigned short Uint16;
class DcmDataset;



struct RawAttrs{
	Uint16 rows = 2048;
	Uint16 cols = 2048;
	Uint16 samplesPerPixel = 1;
	std::string photoMetr = "MONOCHROME1";
	Uint16 bitsAlloc = 16;
	Uint16 bitStored = 16;
	Uint16 hightBit = 11;
	Uint16 pixelRepresent = 0;
	bool StoreAttr(DcmDataset* dataset) const;
};
class GATE_WAY_LIBAPI DcmGateway{
public:
	//���ܣ�������ת��ΪDCM��ʽͼƬ
	//*inputFile->����ͼƬ
	//*picType->����ͼƬ���ͣ�ö��M_PIC_TYPE
	//*outputFile->���ͼƬ,Ĭ��Ϊ�����ļ������ƣ�ֻ�ı��׺��
	//*picInfo->Ҫ��ͼƬ��ӵ���Ϣ(tag,ֵ)�б�,Ĭ��Ϊ��
	//���أ�ת���ɹ�true������false������GetErrInfo�鿴����ԭ��
	bool Pic2Dcm(const std::string& inputFile, M_PIC_TYPE picType, const InfoListType& picInfo = InfoListType(), const std::string& outputFile = std::string(""));

	//���ܣ������ͼ��ת����һ��dcm�ļ���
	//*inputFileList:Ҫת�����ļ����б�
	//*outputDcmFile:�����dcm�ļ���
	//*picType->����ͼƬ���ͣ�ö��M_PIC_TYPE
	//*picInfo->Ҫ��ͼƬ��ӵ���Ϣ(tag,ֵ)�б�,Ĭ��Ϊ��
	//���أ�ת���ɹ�true������false������GetErrInfo�鿴����ԭ��
	bool MulPic2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, M_PIC_TYPE picType, const  InfoListType& picInfo = InfoListType());
	//���ܣ��ͻ���ѡ��Mulpic2Dcm,���������ת��rawʱʧ�ܣ�˵��raw����cximage֧�ֵ�������ɵġ���ʱ��������ȫ��raw��һ���ļ�ͷ��û�У�
	//		�͸����û����õ�height/width/bitcount����ת����
	//*inputFileList��
	//*height:
	//*width:
	//*bitcount:ÿ����λ��,Ĭ��16����ѡ8��16
	//*highbit��ÿ�������λλ�ã�����������16λ����ֻʹ����12λ�����λ����11��Ĭ��11
	//*info����ͼƬ��ӵĶ�����Ϣ
	//���أ�ת���ɹ�true������false������GetErrInfo�鿴����ԭ��
	//bool MulRaw2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, unsigned short height, unsigned short width, unsigned char bitcount=16, unsigned char highbit=11, const  InfoListType& picInfo = InfoListType());
	bool MulRaw2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, const RawAttrs& attrs, const  InfoListType& picInfo = InfoListType());
	//���ܣ����û��������Ϣ��ӵ�������ļ���
	//*inputFile:Ҫ�����Ϣ��dcm�ļ���
	//*picInfo:Ҫ��ӵ���Ϣ
	//*outputFilev:����û���ָ������ʹ�þɵ��ļ��������ָ��Ҳ��ɾ���ɵ��ļ�
	//���أ�ת���ɹ�true������false������GetErrInfo�鿴����ԭ��
	bool AddDcmFileInfo(const std::string& inputFile, const  InfoListType& picInfo, const std::string& outputFilev = "temp.dcm");
	//���ܣ���ָ����DcmDataset�������Ϣ�����picInfo�д��ڲ�֧�ֵ��ֶξ�����
	//*dataset->Ŀ��dataset
	//*picInfo->��Ҫ��ӵ���Ϣ
	//���أ����ȫ����ӳɹ�����true�����󷵻�false������GetErrInfo�鿴����ԭ��
	//ע�⣺�û�������չ֧�ֵ��ַ���
	bool AddDatasetInfo(DcmDataset* dataset, const  InfoListType& picInfo);
	//���ܣ�������ͼ��ת��Ϊbmp��ʽ
	//*inputFile:�����ļ���
	//*outputFile:����ļ���
	//*picType:ԭʼ�ļ�����
	//���أ�ת���ɹ�true������false������GetErrInfo�鿴����ԭ��
	//ע�⣺�ú����û�������չ֧�ָ�ʽ
	bool Pic2Bmp(const std::string& inputFile, const std::string& outputFile, M_PIC_TYPE picType);
	//���ִ�д������Ϣ
	std::string GetErrInfo();
protected:
	//���ܣ���dataset�в���fileNum��"����ѹ����"��СΪlength��PixelData
	//*dataset:����Ŀ��
	//*encapsulatedFrams:������ÿһ֡��pixData,length��
	//*ts�������﷨
	//���أ��ɹ�true������false������GetErrInfo�鿴����ԭ��
	//ע�⣺�ú��������ͷ�encapsulatedFrams�ڴ�,�û��Լ������Լ��ͷ�
	//bool InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, E_TransferSyntax ts);
	bool InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, void* ts);
	std::string errInfo;
	typedef std::pair<char*, Uint32> EncapsulatedElement;
};
