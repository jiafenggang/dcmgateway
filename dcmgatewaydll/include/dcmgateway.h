#pragma once

#ifdef GATE_WAY_LIBAPI
#else
#define GATE_WAY_LIBAPI __declspec(dllimport)
#endif
#include"dcmgatewayglobal.h"
//#include"dcmtk\dcmdata\dcxfer.h"  //解决extern  enum E_TransferSyntax;



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
	//功能：将输入转换为DCM格式图片
	//*inputFile->输入图片
	//*picType->输入图片类型，枚举M_PIC_TYPE
	//*outputFile->输出图片,默认为输入文件的名称，只改变后缀名
	//*picInfo->要给图片添加的信息(tag,值)列表,默认为空
	//返回：转化成功true，错误false，利用GetErrInfo查看错误原因
	bool Pic2Dcm(const std::string& inputFile, M_PIC_TYPE picType, const InfoListType& picInfo = InfoListType(), const std::string& outputFile = std::string(""));

	//功能：将多幅图像转换到一个dcm文件中
	//*inputFileList:要转换的文件名列表
	//*outputDcmFile:输出的dcm文件名
	//*picType->输入图片类型，枚举M_PIC_TYPE
	//*picInfo->要给图片添加的信息(tag,值)列表,默认为空
	//返回：转化成功true，错误false，利用GetErrInfo查看错误原因
	bool MulPic2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, M_PIC_TYPE picType, const  InfoListType& picInfo = InfoListType());
	//功能：客户首选是Mulpic2Dcm,但是如果在转换raw时失败，说明raw并非cximage支持的相机生成的。这时可能是完全的raw，一点文件头都没有，
	//		就根据用户设置的height/width/bitcount进行转换。
	//*inputFileList：
	//*height:
	//*width:
	//*bitcount:每像素位数,默认16，可选8或16
	//*highbit：每像素最高位位置，比如分配的是16位，但只使用了12位，最高位就是11。默认11
	//*info：给图片添加的额外信息
	//返回：转化成功true，错误false，利用GetErrInfo查看错误原因
	//bool MulRaw2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, unsigned short height, unsigned short width, unsigned char bitcount=16, unsigned char highbit=11, const  InfoListType& picInfo = InfoListType());
	bool MulRaw2Dcm(const std::vector<std::string>& inputFileList, const std::string& outputDcmFile, const RawAttrs& attrs, const  InfoListType& picInfo = InfoListType());
	//功能：将用户输入的信息添加到输入的文件中
	//*inputFile:要添加信息的dcm文件名
	//*picInfo:要添加的信息
	//*outputFilev:如果用户不指定就是使用旧的文件名，如果指定也会删除旧的文件
	//返回：转化成功true，错误false，利用GetErrInfo查看错误原因
	bool AddDcmFileInfo(const std::string& inputFile, const  InfoListType& picInfo, const std::string& outputFilev = "temp.dcm");
	//功能：往指定的DcmDataset中添加信息，如果picInfo中存在不支持的字段就跳过
	//*dataset->目标dataset
	//*picInfo->需要添加的信息
	//返回：如果全部添加成功返回true，错误返回false，利用GetErrInfo查看错误原因
	//注意：用户可以扩展支持的字符串
	bool AddDatasetInfo(DcmDataset* dataset, const  InfoListType& picInfo);
	//功能：将输入图像转换为bmp格式
	//*inputFile:输入文件名
	//*outputFile:输出文件名
	//*picType:原始文件类型
	//返回：转化成功true，错误false，利用GetErrInfo查看错误原因
	//注意：该函数用户可以扩展支持格式
	bool Pic2Bmp(const std::string& inputFile, const std::string& outputFile, M_PIC_TYPE picType);
	//获得执行错误的信息
	std::string GetErrInfo();
protected:
	//功能：向dataset中插入fileNum个"经过压缩的"大小为length的PixelData
	//*dataset:插入目标
	//*encapsulatedFrams:里面存放每一帧（pixData,length）
	//*ts：传输语法
	//返回：成功true，错误false，利用GetErrInfo查看错误原因
	//注意：该函数不会释放encapsulatedFrams内存,用户自己分配自己释放
	//bool InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, E_TransferSyntax ts);
	bool InsertEncapsulatedPixDatas(DcmDataset* dataset, const std::vector<std::pair<char*, Uint32>>& encapsulatedFrams, void* ts);
	std::string errInfo;
	typedef std::pair<char*, Uint32> EncapsulatedElement;
};
