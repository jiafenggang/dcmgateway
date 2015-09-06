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
		errInfo = "bitcount 必须为16或8";
		return false;
	}
	UINT8 fileNum = inputFileList.size();
	DcmDataset* dataset = new DcmDataset();
	OFCondition cond;
	//一幅图片的大小，以字节为单位
	UINT32 imageSize = attrs.rows *attrs.cols *attrs.bitsAlloc / 8;
	//最后存入dcm的像素大小，以字节为单位
	UINT32 pixSize = imageSize*fileNum;
	UINT8* pixData = new UINT8[pixSize];
	memset(pixData, 0, sizeof(UINT8)*pixSize);
	//是否真的指向同一块内存？
	UINT8* mpixData = pixData;
	//->循环读入每张图片的像素数据
	for (auto iter = inputFileList.cbegin(); iter != inputFileList.cend(); ++iter){
		std::string fileName = (*iter);
		FILE* pf = fopen(fileName.c_str(), "rb");
		//打开失败就跳过，并不报错！只是少了个文件
		if (pf != NULL){
			UINT32 readcount=fread(pixData, sizeof(UINT8), imageSize, pf);
			pixData += readcount;
			fclose(pf);
		}
	}
	//->插入像素数据
	if (attrs.bitsAlloc == 16){
		//16位时候，将UINT8->UINT16,数组大小变为pixSize/2
		cond = dataset->putAndInsertUint16Array(DCM_PixelData, OFreinterpret_cast(Uint16*, mpixData), pixSize / 2);
	}
	else if (attrs.bitsAlloc == 8){
		cond = dataset->putAndInsertUint8Array(DCM_PixelData, mpixData, pixSize);
	}
	if (cond.bad()){
		errInfo = cond.text() + std::string(":")+ "插入像素数据失败！";
		delete[] mpixData;
		delete dataset;
		return false;
	}
	//->插入图像信息
	char p[10];
	sprintf_s(p, 10, "%d", fileNum);
	cond = dataset->putAndInsertString(DCM_NumberOfFrames, p);

	attrs.StoreAttr(dataset);

	//->插入用户自定义的信息，如果失败，不返回
	if (picInfo.size() > 0){
		if (!AddDatasetInfo(dataset, picInfo)){
			
			//return false;
		}
	}
	//->保存合成的图片
	DcmFileFormat fileFormat(dataset);
	cond = fileFormat.saveFile(outputDcmFile.c_str(), EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	if (cond.bad()){
		errInfo = cond.text() + std::string(":")+"保存dcm文件失败";
		delete[] mpixData;
		delete dataset;
		return false;
	}
	//->清理内存
	delete[] mpixData;
	delete dataset;
	return true;
}

//当前只对bmp/jpeg进行了支持
bool DcmGateway::MulPic2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, M_PIC_TYPE picType, const  InfoListType& picInfo){
	//->初始化要用到的数据
	char* pixData = NULL;
	char* mPixData = NULL;
	//存放压缩图像的帧内容、帧大小的容器
	std::vector<EncapsulatedElement> encapsulatedFrams;
	size_t fileNum = inputFileList.size();
	bool firstTime = true, bCompressed = false;
	OFCondition cond;
	DcmDataset* dataset = new DcmDataset;
	//存放I2DImgSource.readPixelData读取的图片属性
	PicAttrs picAttrs;
	Uint32 length;
	E_TransferSyntax ts;
	//->从文件列表中相继读出每幅图的数据,并且合并
	//碰到第一副图要分配内存
	for (auto iter = inputFileList.begin(); iter != inputFileList.end(); ++iter){
		I2DImgSource* imgSource;
		if (picType == PIC_FORMAT_BMP){
			imgSource = new I2DBmpSource();
		}
		else if (picType == PIC_FORMAT_JPEG){
			imgSource = new I2DJpegSource();
		}
		else{
			errInfo = "图片格式不支持，只支持jpeg/bmp";
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
			//判断数据是否进行了压缩
			DcmXfer transport(ts);
			if (transport.isEncapsulated()){
				bCompressed = true;
			}
			//非压缩情况
			if (!bCompressed){
				pixData = new char[length*fileNum];
				memset(pixData, 0, length*fileNum);
				mPixData = pixData;
			}
			firstTime = false;
		}
		//*********************
		//这里要分开，如果压缩了，每张图虽然row col一样但length不一样
		//*********************
		//拷贝tempPix，并释放它
		//如果数据是压缩的，要单独保存每一帧数据，因为长度不相等
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
	//->向dcm文件中保存信息，分开是压缩还是不压缩的两种情况
	//注意pixData已经不知道知道哪里去了，这里使用提前保存的mPixData
	if (bCompressed){
		if (!InsertEncapsulatedPixDatas(dataset, encapsulatedFrams, &ts)){
			return false;
		}
	}
	else{
		cond = dataset->putAndInsertUint8Array(DCM_PixelData, OFreinterpret_cast(Uint8*, mPixData), length*fileNum);
		if (cond.bad()){
			errInfo = "插入非压缩数据失败！";
			return false;
		}
	}
	//->保存图像属性
	//保存帧数
	char p[10];
	sprintf_s(p, 10, "%d", fileNum);
	cond = dataset->putAndInsertString(DCM_NumberOfFrames, p);
	//保存其它文件属性
	if (!(picAttrs.StoreAttr(dataset) && cond.good())){
		errInfo = "保存图像属性失败！";
		return false;
	}
	//->保存用户定义属性
	if (picInfo.size() > 0){
		AddDatasetInfo(dataset, picInfo);
	}
	//->保存文件
	DcmFileFormat fileFormat(dataset);
	cond = fileFormat.saveFile(outputDcmFile.c_str(), ts, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	//->释放内存
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
//bug，每张jpg的length不一样大
bool DcmGateway::InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, void* ts){
	E_TransferSyntax* pts = (E_TransferSyntax*)ts;
	DcmPixelSequence* pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
	OFCondition cond;
	//插入一个空的item
	DcmPixelItem* offsetTable = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
	pixelSequence->insert(offsetTable);
	for (auto iter = encapsulatedFrams.cbegin(); iter != encapsulatedFrams.cend(); ++iter){
		DcmOffsetList dummyList;
		cond = pixelSequence->storeCompressedFrame(dummyList, OFreinterpret_cast(Uint8*, (*iter).first), (*iter).second, 0);
		if (cond.bad()){
			delete pixelSequence;
			errInfo = "插入一帧失败！";
			return false;
		}
	}
	DcmPixelData *pixelData = new DcmPixelData(DCM_PixelData);
	pixelData->putOriginalRepresentation(*pts, NULL, pixelSequence);
	cond = dataset->insert(pixelData);
	if (cond.bad())
	{
		errInfo = "插入最后总的帧失败！";
		return false;
	}
	return true;
}


//*转换
//*增加用户自定义信息
//*保存
bool DcmGateway::Pic2Dcm(const std::string& inputFile, M_PIC_TYPE picType, const InfoListType& picInfo, const std::string& outputFile){
	//->得到输入、输出文件名
	std::string inputFileName(inputFile), outputFileName(outputFile);
	std::string outputFileWithoutSufix;
	bool bCach = false;
	//用户没有设置输出文件名
	if ("" == outputFile){
		size_t sufixBeginIndex = inputFileName.find_last_of(".");
		outputFileWithoutSufix = inputFileName.substr(0, sufixBeginIndex);
		outputFileName = outputFileWithoutSufix + ".dcm";
	}
	//->图片格式统一化
	if (picType != PIC_FORMAT_BMP&&picType != PIC_FORMAT_JPEG){
		inputFileName = outputFileWithoutSufix + ".bmp";
		if (!Pic2Bmp(inputFile, inputFileName, picType)){
			return false;
		}
		picType = PIC_FORMAT_BMP;
		bCach = true;
	}
	//->实例化要用到的对象Image2Dcm、I2DImgSource、I2DOutputPlug、DcmDataset
	//DcmDataset得到的是dcmtk中分配内存的数据，最后要释放
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
	//->对上面实例化后的对象设置
	inputPlug->setImageFile(OFString(inputFileName.c_str()));
	outputPlug->setValidityChecking(true);
	i2d.setISOLatin1(false);
	i2d.setValidityChecking(true);
	//->执行转换
	OFCondition cond;
	E_TransferSyntax writeXfer;
	cond = i2d.convert(inputPlug, outputPlug, resultDataset, writeXfer);
	if (cond.bad()){
		errInfo = "Image2Dcm.convert执行转换失败！";
		return false;
	}
	//->AddDatasetInfo添加用户录入信息
	if (picInfo.size() > 0){
		if (!AddDatasetInfo(resultDataset, picInfo)){
			return false;
		}
	}
	//->存储
	//DcmFileFormat fileFormat(dataset);
	//cond = fileFormat.saveFile(outputDcmFile.c_str(), ts, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	DcmFileFormat fileFormat(resultDataset);
	cond = fileFormat.saveFile(outputFileName.c_str(), writeXfer, EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	//cond = fileFormat.saveFile(outputFileName.c_str(), writeXfer);// , EET_ExplicitLength, EGL_recalcGL, EPD_noChange, 0, 0, EWM_fileformat);
	if (cond.bad()){
		errInfo = "保存文件失败" + std::string(cond.text());
		return false;
	}
	//->释放内存	
	delete resultDataset;   //该指针属于out客户释放
	delete outputPlug;
	delete inputPlug;

	//如果使用的其它格式->bmp删除中间缓存
	if (bCach){
		BOOL dele = DeleteFile(UTIL::str2unic(inputFileName));
#ifdef _DEBUG
		if (!dele){
			std::ostringstream os;
			os << GetLastError();
			errInfo = "无法删除文件:" + inputFileName + "错误代号：" + os.str();
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
		//errInfo = "暂时不提供对raw文件的支持！";
		cximageType = CXIMAGE_FORMAT_RAW;
		break;
	default:
		errInfo = "暂时不提供对该文件格式的支持！";
		break;
	}
	//CxImage image;
	MXImage image;
	wchar_t* wtemp = UTIL::str2unic(inputFile);
	if (!image.Load(wtemp, cximageType)){
		errInfo = "加载文件" + inputFile + "失败";
		delete[] wtemp;
		return false;
	}
	delete[] wtemp;

	wtemp = UTIL::str2unic(outputFile);
	if (!image.Save(wtemp, CXIMAGE_FORMAT_BMP)){
		errInfo = "转换位图格式失败！";
		delete[] wtemp;
		return false;
	}
	delete[] wtemp;
	return true;
}
//*读取dcm文件
//*调用AddDatasetInfo
bool DcmGateway::AddDcmFileInfo(const std::string& inputFile, const InfoListType& picInfo, const std::string& outputFilev){
	std::string outputFile(outputFilev);
	DcmFileFormat fileformat;
	OFCondition cond;
	cond = fileformat.loadFile(inputFile.c_str());
	DcmDataset* dataset = fileformat.getDataset();
	if (!AddDatasetInfo(dataset, picInfo)){
		//错误信息errInfo是AddDatasetInfo添加
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
				errInfo = "插入DCM_PatientName:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATIENT_ID:
			ret = dataset->putAndInsertString(DCM_PatientID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_PatientID:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATENT_SEX:
			ret = dataset->putAndInsertString(DCM_PatientSex, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_PatientSex:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_PATIENT_BIRTH_DATE:
			ret = dataset->putAndInsertString(DCM_PatientBirthDate, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_PatientBirthDate:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_INSTANCE_UID:
			ret = dataset->putAndInsertString(DCM_StudyInstanceUID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_StudyInstanceUID:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_DATE:
			ret = dataset->putAndInsertString(DCM_StudyDate, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_StudyDate:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_TIME:
			ret = dataset->putAndInsertString(DCM_StudyTime, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_StudyTime:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_REFERRING_PHYSICIAN_NAME:
			ret = dataset->putAndInsertString(DCM_ReferringPhysicianName, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_ReferringPhysicianName:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_STUDY_ID:
			ret = dataset->putAndInsertString(DCM_StudyID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_StudyID:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_ACCESSION_NUMBER:
			ret = dataset->putAndInsertString(DCM_AccessionNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_AccessionNumber:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_MODALITY:
			ret = dataset->putAndInsertString(DCM_Modality, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_Modality:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_SERIES_INSTANCE_UID:
			ret = dataset->putAndInsertString(DCM_SeriesInstanceUID, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_SeriesInstanceUID:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_SERIES_NUMBER:
			ret = dataset->putAndInsertString(DCM_SeriesNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_SeriesNumber:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
		case GATE_INSTANCE_NUMBER:
			ret = dataset->putAndInsertString(DCM_InstanceNumber, iter->second.c_str());
			if (ret.bad()){
				errInfo = "插入DCM_InstanceNumber:" + iter->second + " 错误" + std::string(ret.text());
				return false;
			}
			break;
			//还有其他类型字段，这是非致命错误，可以容忍
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