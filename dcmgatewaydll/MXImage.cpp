#include"stdafx.h"
#define XIMAGE_LIBAPI __declspec(dllexport)

#include"MXImage.h"

bool MXImage::Load(const TCHAR * filename, uint32_t imagetype)
{
	bool bOK = false;
	if (GetTypeIndexFromId(imagetype)){
		FILE* hFile;	//file handle to read the image
		if ((hFile = _tfopen(filename, _T("rb"))) == NULL)  return false;	// For UNICODE support
		bOK = Decode(hFile, imagetype);
		fclose(hFile);
		if (bOK) return bOK;
	}
	errInfo = "请查看图片格式是否正确，或者文件是否损坏！";
	return false;
}