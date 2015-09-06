#include"stdafx.h"

#include"dcmtk\dcmdata\dctk.h"
#include "dcmtk/dcmdata/libi2d/i2d.h"
#include "dcmtk/dcmdata/libi2d/i2djpgs.h"
#include "dcmtk/dcmdata/libi2d/i2dbmps.h"
#include "dcmtk/dcmdata/libi2d/i2dplsc.h"
#include "dcmtk\dcmdata\dcpxitem.h"

#define GATE_WAY_LIBAPI __declspec(dllexport)
#include"dcmgateway.h"

#include"MXImage.h"
#include "util.h"
#include <sstream>
struct PicAttrs{
	Uint16 rows;
	Uint16 cols;
	Uint16 samplesPerPi;
	OFString photoMetrI;
	Uint16 bitsAlloc;
	Uint16 bitsStored;
	Uint16 highBit;
	Uint16 pixelRepr;
	Uint16 planConf;
	Uint16 pixAspectH;
	Uint16 pixAspectV;
	bool StoreAttr(DcmDataset* dataset)const;
};

//
bool DcmGateway::MulRaw2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, const RawAttrs& attrs, const  InfoListType& picInfo){
	if ((attrs.bitsAlloc != 16) &&( attrs.bitsAlloc != 8)){
		errInfo = "bitcount ����Ϊ16��8";
		return false;
	}
	UINT8 fileNum = inputFileList.size();
	DcmDataset* dataset = new DcmDataset();
	OFCondition cond;
	//һ��ͼƬ�Ĵ�С�����ֽ�Ϊ��λ
	UINT32 imageSize = attrs.rows *attrs.cols *attrs.bitsAlloc / 8;
	//������dcm�����ش�С�����ֽ�Ϊ��λ
	UINT32 pixSize = imageSize*fileNum;
	UINT8* pixData = new UINT8[pixSize];
	memset(pixData, 0, sizeof(UINT8)*pixSize);
	//�Ƿ����ָ��ͬһ���ڴ棿
	UINT8* mpixData = pixData;
	//->ѭ������ÿ��ͼƬ����������
	for (auto iter = inputFileList.cbegin(); iter != inputFileList.cend(); ++iter){
		std::string fileName = (*iter);
		FILE* pf = fopen(fileName.c_str(), "rb");
		//��ʧ�ܾ���������������ֻ�����˸��ļ�
		if (pf != NULL){
			UINT32 readcount=fread(pixData, sizeof(UINT8), imageSize, pf);
			pixData += readcount;
			fclose(pf);
		}
	}
	//->������������
	if (attrs.bitsAlloc == 16){
		//16λʱ�򣬽�UINT8->UINT16,�����С��ΪpixSize/2
		cond = dataset->putAndInsertUint16Array(DCM_PixelData, OFreinterpret_cast(Uint16*, mpixData), pixSize / 2);
	}
	else if (attrs.bitsAlloc == 8){
		cond = dataset->putAndInsertUint8Array(DCM_PixelData, mpixData, pixSize);
	}
	if (cond.bad()){
		errInfo = cond.text() + std::string(":")+ "������������ʧ�ܣ�";
		delete[] mpixData;
		delete dataset;
		return false;
	}
	//->����ͼ����Ϣ
	char p[10];
	sprintf_s(p, 10, "%d", fileNum);
	cond = dataset->putAndInsertString(DCM_NumberOfFrames, p);

	attrs.StoreAttr(dataset);

	//->�����û��Զ������Ϣ�����ʧ�ܣ�������
	if (picInfo.size() > 0){
		if (!AddDatasetInfo(dataset, picInfo)){
			
			//return false;
		}
	}
	//->����ϳɵ�ͼƬ
	DcmFileFormat fileFormat(dataset);
	cond = fileFormat.saveFile(outputDcmFile.c_str(), EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	if (cond.bad()){
		errInfo = cond.text() + std::string(":")+"����dcm�ļ�ʧ��";
		delete[] mpixData;
		delete dataset;
		return false;
	}
	//->�����ڴ�
	delete[] mpixData;
	delete dataset;
	return true;
}

//��ǰֻ��bmp/jpeg������֧��
bool DcmGateway::MulPic2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, M_PIC_TYPE picType, const  InfoListType& picInfo){
	//->��ʼ��Ҫ�õ�������
	char* pixData = NULL;
	char* mPixData = NULL;
	//���ѹ��ͼ���֡���ݡ�֡��С������
	std::vector<EncapsulatedElement> encapsulatedFrams;
	size_t fileNum = inputFileList.size();
	bool firstTime = true, bCompressed = false;
	OFCondition cond;
	DcmDataset* dataset = new DcmDataset;
	//���I2DImgSource.readPixelData��ȡ��ͼƬ����
	PicAttrs picAttrs;
	Uint32 length;
	E_TransferSyntax ts;
	//->���ļ��б�����̶���ÿ��ͼ������,���Һϲ�
	//������һ��ͼҪ�����ڴ�
	for (auto iter = inputFileList.begin(); iter != inputFileList.end(); ++iter){
		I2DImgSource* imgSource;
		if (picType == PIC_FORMAT_BMP){
			imgSource = new I2DBmpSource();
		}
		else if (picType == PIC_FORMAT_JPEG){
			imgSource = new I2DJpegSource();
		}
		else{
			errInfo = "ͼƬ��ʽ��֧�֣�ֻ֧��jpeg/bmp";
			return false;
		}
		imgSource->setImageFile((*iter).c_str());
		char* tempPix = NULL;
		cond = imgSource->readPixelData(picAttrs.rows, picAttrs.cols, picAttrs.samplesPerPi, picAttrs.photoMetrI, \
			picAttrs.bitsAlloc, picAttrs.bitsStored, picAttrs.highBit, picAttrs.pixelRepr, picAttrs.planConf, picAttrs.pixAspectH, picAttrs.pixAspectV, tempPix, length, ts);
		if (cond.bad()){
			errInfo = "";
			return false;
		}
		if (firstTime){
			//�ж������Ƿ������ѹ��
			DcmXfer transport(ts);
			if (transport.isEncapsulated()){
				bCompressed = true;
			}
			//��ѹ�����
			if (!bCompressed){
				pixData = new char[length*fileNum];
				memset(pixData, 0, length*fileNum);
				mPixData = pixData;
			}
			firstTime = false;
		}
		//*********************
		//����Ҫ�ֿ������ѹ���ˣ�ÿ��ͼ��Ȼrow colһ����length��һ��
		//*********************
		//����tempPix�����ͷ���
		//���������ѹ���ģ�Ҫ��������ÿһ֡���ݣ���Ϊ���Ȳ����
		if (bCompressed){
			char* encapsulatedData;
			encapsulatedData = new char[length];
			memset(encapsulatedData, 0, length);
			memcpy(encapsulatedData, tempPix, length);
			encapsulatedFrams.push_back(EncapsulatedElement(encapsulatedData, length));
		}
		else{
			memcpy_s(pixData, length, tempPix, length);
			pixData += length;
		}
		delete imgSource;
		delete tempPix;
	}
	//->��dcm�ļ��б�����Ϣ���ֿ���ѹ�����ǲ�ѹ�����������
	//ע��pixData�Ѿ���֪��֪������ȥ�ˣ�����ʹ����ǰ�����mPixData
	if (bCompressed){
		if (!InsertEncapsulatedPixDatas(dataset, encapsulatedFrams, &ts)){
			return false;
		}
	}
	else{
		cond = dataset->putAndInsertUint8Array(DCM_PixelData, OFreinterpret_cast(Uint8*, mPixData), length*fileNum);
		if (cond.bad()){
			errInfo = "�����ѹ������ʧ�ܣ�";
			return false;
		}
	}
	//->����ͼ������
	//����֡��
	char p[10];
	sprintf_s(p, 10, "%d", fileNum);
	cond = dataset->putAndInsertString(DCM_NumberOfFrames, p);
	//���������ļ�����
	if (!(picAttrs.StoreAttr(dataset) && cond.good())){
		errInfo = "����ͼ������ʧ�ܣ�";
		return false;
	}
	//->�����û���������
	if (picInfo.size() > 0){
		AddDatasetInfo(dataset, picInfo);
	}
	//->�����ļ�
	DcmFileFormat fileFormat(dataset);
	cond = fileFormat.saveFile(outputDcmFile.c_str(), ts, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	//->�ͷ��ڴ�
	if (bCompressed){
		for (auto iter = encapsulatedFrams.begin(); iter != encapsulatedFrams.end(); ++iter){
			delete[](*iter).first;
		}
		encapsulatedFrams.clear();
	}
	else{
		delete[] mPixData;//pixData;
	}
	delete dataset;
	return true;
}
//bug��ÿ��jpg��length��һ����
bool DcmGateway::InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, void* ts){
	E_TransferSyntax* pts = (E_TransferSyntax*)ts;
	DcmPixelSequence* pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
	OFCondition cond;
	//����һ���յ�item
	DcmPixelItem* offsetTable = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
	pixelSequence->insert(offsetTable);
	for (auto iter = encapsulatedFrams.cbegin(); iter != encapsulatedFrams.cend(); ++iter){
		DcmOffsetList dummyList;
		cond = pixelSequence->storeCompressedFrame(dummyList, OFreinterpret_cast(Uint8*, (*iter).first), (*iter).second, 0);
		if (cond.bad()){
			delete pixelSequence;
			errInfo = "����һ֡ʧ�ܣ�";
			return false;
		}
	}
	DcmPixelData *pixelData = new DcmPixelData(DCM_PixelData);
	pixelData->putOriginalRepresentation(*pts, NULL, pixelSequence);
	cond = dataset->insert(pixelData);
	if (cond.bad())
	{
		errInfo = "��������ܵ�֡ʧ�ܣ�";
		return false;
	}
	return true;
}


//*ת��
//*�����û��Զ�����Ϣ
//*����
bool DcmGateway::Pic2Dcm(const std::string& inputFile, M_PIC_TYPE picType, const InfoListType& picInfo, const std::string& outputFile){
	//->�õ����롢����ļ���
	std::string inputFileName(inputFile), outputFileName(outputFile);
	std::string outputFileWithoutSufix;
	bool bCach = false;
	//�û�û����������ļ���
	if ("" == outputFile){
		size_t sufixBeginIndex = inputFileName.find_last_of(".");
		outputFileWithoutSufix = inputFileName.substr(0, sufixBeginIndex);
		outputFileName = outputFileWithoutSufix + ".dcm";
	}
	//->ͼƬ��ʽͳһ��
	if (picType != PIC_FORMAT_BMP&&picType != PIC_FORMAT_JPEG){
		inputFileName = outputFileWithoutSufix + ".bmp";
		if (!Pic2Bmp(inputFile, inputFileName, picType)){
			return false;
		}
		picType = PIC_FORMAT_BMP;
		bCach = true;
	}
	//->ʵ����Ҫ�õ��Ķ���Image2Dcm��I2DImgSource��I2DOutputPlug��DcmDataset
	//DcmDataset�õ�����dcmtk�з����ڴ�����ݣ����Ҫ�ͷ�
	Image2Dcm i2d;
	I2DImgSource* inputPlug;
	I2DOutputPlug* outputPlug;
	DcmDataset* resultDataset;
	switch (picType)
	{
	case PIC_FORMAT_BMP:
		inputPlug = new I2DBmpSource();
		break;
	case PIC_FORMAT_JPEG:
		inputPlug = new I2DJpegSource();
		break;
	default:
		break;
	}
	outputPlug = new I2DOutputPlugSC();
	//->������ʵ������Ķ�������
	inputPlug->setImageFile(OFString(inputFileName.c_str()));
	outputPlug->setValidityChecking(true);
	i2d.setISOLatin1(false);
	i2d.setValidityChecking(true);
	//->ִ��ת��
	OFCondition cond;
	E_TransferSyntax writeXfer;
	cond = i2d.convert(inputPlug, outputPlug, resultDataset, writeXfer);
	if (cond.bad()){
		errInfo = "Image2Dcm.convertִ��ת��ʧ�ܣ�";
		return false;
	}
	//->AddDatasetInfo����û�¼����Ϣ
	if (picInfo.size() > 0){
		if (!AddDatasetInfo(resultDataset, picInfo)){
			return false;
		}
	}
	//->�洢
	//DcmFileFormat fileFormat(dataset);
	//cond = fileFormat.saveFile(outputDcmFile.c_str(), ts, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	DcmFileFormat fileFormat(resultDataset);
	cond = fileFormat.saveFile(outputFileName.c_str(), writeXfer, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	//cond = fileFormat.saveFile(outputFileName.c_str(), writeXfer);// , EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	if (cond.bad()){
		errInfo = "�����ļ�ʧ��" + std::string(cond.text());
		return false;
	}
	//->�ͷ��ڴ�	
	delete resultDataset;   //��ָ������out�ͻ��ͷ�
	delete outputPlug;
	delete inputPlug;

	//���ʹ�õ�������ʽ->bmpɾ���м仺��
	if (bCach){
		BOOL dele = DeleteFile(UTIL::str2unic(inputFileName));
#ifdef _DEBUG
		if (!dele){
			std::ostringstream os;
			os << GetLastError();
			errInfo = "�޷�ɾ���ļ�:" + inputFileName + "������ţ�" + os.str();
			return false;
		}
#endif
	}
	return true;

}

bool DcmGateway::Pic2Bmp(const std::string& inputFile, const std::string& outputFile, M_PIC_TYPE picType){
	size_t cximageType = 0;
	switch (picType){
	case PIC_FORMAT_GIF:
		cximageType = CXIMAGE_FORMAT_GIF;
		break;
	case PIC_FORMAT_PNG:
		cximageType = CXIMAGE_FORMAT_PNG;
		break;
	case PIC_FORMAT_TIFF:
		cximageType = CXIMAGE_FORMAT_TIF;
		break;
	case PIC_FORMAT_RAW:
		//errInfo = "��ʱ���ṩ��raw�ļ���֧�֣�";
		cximageType = CXIMAGE_FORMAT_RAW;
		break;
	default:
		errInfo = "��ʱ���ṩ�Ը��ļ���ʽ��֧�֣�";
		break;
	}
	//CxImage image;
	MXImage image;
	wchar_t* wtemp = UTIL::str2unic(inputFile);
	if (!image.Load(wtemp, cximageType)){
		errInfo = "�����ļ�" + inputFile + "ʧ��";
		delete[] wtemp;
		return false;
	}
	delete[] wtemp;

	wtemp = UTIL::str2unic(outputFile);
	if (!image.Save(wtemp, CXIMAGE_FORMAT_BMP)){
		errInfo = "ת��λͼ��ʽʧ�ܣ�";
		delete[] wtemp;
		return false;
	}
	delete[] wtemp;
	return true;
}
//*��ȡdcm�ļ�
//*����AddDatasetInfo
bool DcmGateway::AddDcmFileInfo(const std::string& inputFile, const InfoListType& picInfo, const std::string& outputFilev){
	std::string outputFile(outputFilev);
	DcmFileFormat fileformat;
	OFCondition cond;
	cond = fileformat.loadFile(inputFile.c_str());
	DcmDataset* dataset = fileformat.getDataset();
	if (!AddDatasetInfo(dataset, picInfo)){
		//������ϢerrInfo��AddDatasetInfo���
		return false;
	}
	cond = fileformat.saveFile(outputFile.c_str());
	DeleteFileA(inputFile.c_str());

	if (outputFile == "temp.dcm"){
		rename("temp.dcm", inputFile.c_str());
	}
	return true;
}
std::string DcmGateway::GetErrInfo(){
	return errInfo;
}
bool DcmGateway::AddDatasetInfo(DcmDataset* dataset, const InfoListType& picInfo){
	OFCondition ret;
	for (auto iter = picInfo.cbegin(); iter != picInfo.cend(); ++iter){
		switch (iter->first)
		{
		case GATE_PATIENT_NAME:
			ret = dataset->putAndInsertString(DCM_PatientName, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_PatientName:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATIENT_ID:
			ret = dataset->putAndInsertString(DCM_PatientID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_PatientID:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATENT_SEX:
			ret = dataset->putAndInsertString(DCM_PatientSex, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_PatientSex:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATIENT_BIRTH_DATE:
			ret = dataset->putAndInsertString(DCM_PatientBirthDate, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_PatientBirthDate:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_INSTANCE_UID:
			ret = dataset->putAndInsertString(DCM_StudyInstanceUID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_StudyInstanceUID:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_DATE:
			ret = dataset->putAndInsertString(DCM_StudyDate, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_StudyDate:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_TIME:
			ret = dataset->putAndInsertString(DCM_StudyTime, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_StudyTime:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_REFERRING_PHYSICIAN_NAME:
			ret = dataset->putAndInsertString(DCM_ReferringPhysicianName, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_ReferringPhysicianName:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_ID:
			ret = dataset->putAndInsertString(DCM_StudyID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_StudyID:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_ACCESSION_NUMBER:
			ret = dataset->putAndInsertString(DCM_AccessionNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_AccessionNumber:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_MODALITY:
			ret = dataset->putAndInsertString(DCM_Modality, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_Modality:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_SERIES_INSTANCE_UID:
			ret = dataset->putAndInsertString(DCM_SeriesInstanceUID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_SeriesInstanceUID:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_SERIES_NUMBER:
			ret = dataset->putAndInsertString(DCM_SeriesNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_SeriesNumber:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_INSTANCE_NUMBER:
			ret = dataset->putAndInsertString(DCM_InstanceNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "����DCM_InstanceNumber:" + iter->second + " ����" + std::string(ret.text());
				return false;
			}
			break;
			//�������������ֶΣ����Ƿ��������󣬿�������
		default:
			break;
		}
	}
	return true;
}

bool PicAttrs::StoreAttr(DcmDataset* dset)const{
	OFCondition cond;
	cond = dset->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPi);
	if (cond.bad())
		return false;

	cond = dset->putAndInsertOFStringArray(DCM_PhotometricInterpretation, photoMetrI.c_str());
	if (cond.bad())
		return false;

	// Should only be written if Samples per Pixel > 1
	if (samplesPerPi > 1)
	{
		cond = dset->putAndInsertUint16(DCM_PlanarConfiguration, planConf);
		if (cond.bad())
			return false;
	}

	cond = dset->putAndInsertUint16(DCM_Rows, rows);
	if (cond.bad())
		return false;

	cond = dset->putAndInsertUint16(DCM_Columns, cols);
	if (cond.bad())
		return false;

	cond = dset->putAndInsertUint16(DCM_BitsAllocated, bitsAlloc);
	if (cond.bad())
		return false;

	cond = dset->putAndInsertUint16(DCM_BitsStored, bitsStored);
	if (cond.bad())
		return false;

	cond = dset->putAndInsertUint16(DCM_HighBit, highBit);
	if (cond.bad())
		return false;

	if (pixAspectH != pixAspectV)
	{
		char buf[200];
		int err = sprintf(buf, "%u\\%u", pixAspectV, pixAspectH);
		if (err == -1) return false;
		cond = dset->putAndInsertOFStringArray(DCM_PixelAspectRatio, buf);
		if (cond.bad())
			return false;
	}
	cond = dset->putAndInsertUint16(DCM_PixelRepresentation, pixelRepr);
	if (cond.bad())
		return false;
	return true;
}

bool RawAttrs::StoreAttr(DcmDataset* dataset)const{
	OFCondition cond;
	cond = dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1);
	cond = dataset->putAndInsertOFStringArray(DCM_PhotometricInterpretation, OFString(photoMetr.c_str()));
	cond = dataset->putAndInsertUint16(DCM_Rows, rows);
	cond = dataset->putAndInsertUint16(DCM_Columns, cols);
	cond = dataset->putAndInsertUint16(DCM_BitsAllocated, bitsAlloc);
	cond = dataset->putAndInsertUint16(DCM_BitsStored, bitStored);
	cond = dataset->putAndInsertUint16(DCM_HighBit, hightBit);
	cond = dataset->putAndInsertUint16(DCM_PixelRepresentation, pixelRepresent);
	return true;
}