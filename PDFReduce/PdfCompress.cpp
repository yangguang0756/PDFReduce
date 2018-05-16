#include "PdfCompress.h"
#include <sstream>  

#include <io.h>
#include <fstream>
#include<iostream>
#include <Windows.h>
#include "UtilImage.h"
#include "UtilPath.h"

#include <vector>
#include "ximage.h"



CPdfCompress::CPdfCompress():
m_ctx(nullptr),
m_doc(nullptr),
m_hWnd(nullptr)
{
	
}

CPdfCompress::~CPdfCompress()
{
}


UINT32 CPdfCompress::StartThread(ATL::CString strPdfPath, ATL::CString strPassword, ATL::CString strPdfOutPath, HWND hWnd)
{
	Util::Path::GetImageTempPath(TRUE);
	m_strPdfPath	= strPdfPath;
	m_strPassword	= strPassword;
	m_strPdfOutPath = strPdfOutPath;
	m_hWnd			= hWnd;

	UINT32 uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, (void*)this, 0, &uiThreadID);
	if (hThread == NULL)
		return 0;
	return uiThreadID;
}

// 线程回调
UINT WINAPI  CPdfCompress::ThreadProc(void* pVoid)
{
	CPdfCompress *pThreadCallback = static_cast<CPdfCompress *>(pVoid);
	if (pThreadCallback)
	{
		pThreadCallback->Run();
	}
	return 0;
}

void CPdfCompress::Run()
{
 	if (Init(m_strPdfPath, m_strPassword))
 	{
		PraseImageTypeObj();
 	}
}

//初始化操作
BOOL CPdfCompress::Init(ATL::CString strPdfPath, ATL::CString strPassword)
{
	if (strPdfPath.IsEmpty())
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	ATL::CW2A szPdfPath(strPdfPath.GetString(), CP_UTF8);
	ATL::CW2A szPassword(strPassword.GetString(), CP_UTF8);
	//创建上下文 包含全局信息
	m_ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!m_ctx)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	//打开文档
	m_doc = pdf_open_document_no_run(m_ctx, szPdfPath.m_psz);
	if (!m_doc)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	//PDF是否需要密码及密码鉴权
	if (pdf_needs_password(m_doc) && !pdf_authenticate_password(m_doc, szPassword.m_psz))
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	return TRUE;
}

//获取PDF页
UINT32 CPdfCompress::GetPdfPages(pdf_document* doc)
{
	return pdf_count_pages(doc);
}

//是否是图片
BOOL CPdfCompress::IsImageType(pdf_obj* dict)
{
	pdf_obj *type = pdf_dict_gets(dict, "Subtype");
	return pdf_is_name(type) && !strcmp(pdf_to_name(type), "Image");
}

//彩色空间是 RGB
BOOL CPdfCompress::IsRGBColorSpace(pdf_obj* dict)
{
	pdf_obj *colorspace = pdf_dict_gets(dict, "ColorSpace");
	return pdf_is_name(colorspace) && !strcmp(pdf_to_name(colorspace), "DeviceRGB");
}

//zlib压缩
BOOL CPdfCompress::IsFlateDecode(pdf_obj* dict)
{
	pdf_obj *filter;
	filter = pdf_dict_gets(dict, "Filter");
	if (!strcmp(pdf_to_name(filter), "FlateDecode")) 
		return TRUE;
	int n = 0;
	n = pdf_array_len(filter);  
	for (int i = 0; i < n; i++)
		if (!strcmp(pdf_to_name(pdf_array_get(filter, i)), "FlateDecode"))
			return TRUE;
	return FALSE;
}

BOOL  CPdfCompress::WritePixmap(fz_context *ctx, fz_pixmap *pix, ATL::CString strImagePath, int rgb)
{
	BOOL bRet = FALSE;

	fz_pixmap *converted = NULL;
	if (!pix)
	{
		return bRet;
	}
	if (rgb && pix->colorspace && pix->colorspace != fz_device_rgb(ctx))
	{
		fz_irect bbox;
		converted = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), fz_pixmap_bbox(ctx, pix, &bbox));
		fz_convert_pixmap(ctx, converted, pix);
		pix = converted;
	}
	if (pix->n <= 4)
	{
		ATL::CW2A szImagePath(strImagePath, CP_UTF8);
		fz_write_png(ctx, pix, (char*)szImagePath, 0);
		bRet = TRUE;
	}
	else
	{
		ATLASSERT(FALSE);
	}
	fz_drop_pixmap(ctx, converted);
	return bRet;
}

BOOL CPdfCompress::SaveImageAsPng(ATL::CString strImagePath, INT32 nNum)
{
	BOOL bRet = FALSE;
	if (strImagePath.IsEmpty())
	{
		ATLASSERT(FALSE);
		return bRet;
	}

	fz_image   *image = NULL;
	fz_pixmap  *pix = NULL;
	pdf_obj    *ref = NULL;

	ref = pdf_new_indirect(m_doc, nNum, 0);
	UINT32 u = 0;
	//保存图片到文件
	image = pdf_load_image(m_doc, ref);
	if (!image)
	{
		return bRet;
	}

#if 0
	std::string strBuffer((const char*)image->buffer->buffer->data,image->buffer->buffer->len);

	UINT8* bmpBuf = NULL;
	UINT64 u64BytesOut = 0;
	bmpBuf = Util::Image::CreateRenderedBitmap((const char*)strBuffer.c_str(), image->w, image->h, FALSE, &u64BytesOut);

	CxImage imag2e(bmpBuf, u64BytesOut, CXIMAGE_FORMAT_BMP);
	if (imag2e.IsValid())
	{
		imag2e.SetJpegQuality(50);
		imag2e.Save(_T("E:\\test\\convert\\bitmap.jpg"), CXIMAGE_FORMAT_JPG);
	}
#endif
#if 1
	fz_try(m_ctx)
	{
		pix = fz_new_pixmap_from_image(m_ctx, image, 0, 0);
		if (!pix)
		{
			return bRet;
		}
		fz_drop_image(m_ctx, image);
		//fz_free_image(ctx, &image->storable);
		//rgb mode
		bRet = WritePixmap(m_ctx, pix, strImagePath, 1);

		fz_drop_pixmap(m_ctx, pix);
		pdf_drop_obj(ref);
	}
	fz_catch(m_ctx)
	{
		u = GetLastError();
	}
#endif

	return TRUE;
}


BOOL CPdfCompress::SavaImageAsJpg(INT32 nNum)
{
	fz_image   *image = NULL;
	fz_pixmap  *pix = NULL;
	pdf_obj    *ref = NULL;

	ref = pdf_new_indirect(m_doc, nNum, 0);
	UINT32 u = 0;
	//保存图片到文件
	image = pdf_load_image(m_doc, ref);
	if (!image)
	{
		return FALSE;
	}

	fz_buffer* fz_buf = pdf_load_stream(m_doc, nNum, 0);
	if (nullptr == fz_buf)
	{
		return FALSE;
	}

	UINT32 fz_len = fz_buf->len;
	UINT64 uSizeOut = 0;
	FILE* fp;
	fp = fopen("E:\\test\\convert\\4bmpbin.txt", "wb");
	fwrite(fz_buf->data, fz_buf->len, 1, fp);
	fclose(fp);


	UINT8* bmpBuffer = Util::Image::AssembleBitmap(fz_buf->data, image->w, image->h, image->imagemask, &uSizeOut);
	if (nullptr == bmpBuffer){
		return FALSE;
	}


	FILE* fp2;
	fp2 = fopen("E:\\test\\convert\\111111.bmp", "wb");
	fwrite(bmpBuffer, uSizeOut, 1, fp2);
	fclose(fp);

	CxImage ximage((UINT8*)bmpBuffer, uSizeOut, CXIMAGE_FORMAT_BMP);
	if (!ximage.IsValid()){
		return FALSE;
	}

	ximage.SetJpegQuality(50);
	CString strDestImage;
	strDestImage.Format(_T("%s\\yueshu-img-%d.jpg"), Util::Path::GetImageTempPath(), nNum);
	ximage.Save(strDestImage, CXIMAGE_FORMAT_JPG);
	delete[] bmpBuffer;
	pdf_drop_obj(ref);
	fz_drop_buffer(m_ctx,fz_buf);
	return TRUE;
}

BOOL CPdfCompress::IsWriteStream(INT32 nNum)
{
	BOOL bRet = FALSE;
	//流中点阵图的大小
	fz_buffer* fz_buf = pdf_load_raw_stream(m_doc, nNum, 0);
	if (nullptr == fz_buf)
	{
		ATLASSERT(FALSE);
		return bRet;
	}
	UINT32 uLen = fz_buf->len;
	fz_drop_buffer(m_ctx, fz_buf);

	//获取jpg图片的大小
	CString strDestImage;
	strDestImage.Format(_T("%s\\yueshu-img-%d.jpg"), Util::Path::GetImageTempPath(), nNum);

	HANDLE hFile = CreateFile(strDestImage, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	LARGE_INTEGER size;
	bRet = GetFileSizeEx(hFile, &size);
	if (!bRet)
	{
		return bRet;
	}
	UINT32 uSize = size.QuadPart;
	if (uLen < uSize)
	{
		bRet = FALSE;
	}
	return bRet;
}


BOOL CPdfCompress::IsCompressImageStream(pdf_document* doc, INT32 num, INT32 gen)
{
	fz_compressed_buffer * fz_com_buffer = pdf_load_compressed_stream(doc, num, gen);
	if (!fz_com_buffer)
	{
		ATLASSERT(FALSE);
		return FALSE;
	}
	//Test 数据写本地
	if (FZ_IMAGE_JPEG  == fz_com_buffer->params.type)
	{
		CString strSrcImage;
		strSrcImage.Format(_T("%s\\yueshu-img-%d.jpg"), Util::Path::GetImageTempPath(), num);
		CString strDestImage;
		strDestImage.Format(_T("%s\\sec-yueshu-img-%d.jpg"), Util::Path::GetImageTempPath(), num);
		FILE* fp = NULL;
		fp = _wfopen(strSrcImage, _T("wb"));
		if (NULL != fp)
		{
			fwrite((char*)fz_com_buffer->buffer->data, fz_com_buffer->buffer->len, 1, fp);
			fclose(fp);

			if (Util::Image::ConvertType(strSrcImage, strDestImage))
			{
				pdf_obj  *obj;
				obj = pdf_load_object(doc, num, 0);
				WriteDataToStream(obj, strDestImage, num);
				pdf_drop_obj(obj);
			}

		}
		DeleteFile(strSrcImage);
		DeleteFile(strDestImage);
		return FALSE;
	}
	else if (FZ_IMAGE_FAX == fz_com_buffer->params.type)
	{
		return TRUE;
	}
	else if (FZ_IMAGE_RLD == fz_com_buffer->params.type)
	{
		return TRUE;
	}
	else if (FZ_IMAGE_FLATE == fz_com_buffer->params.type)
	{
		//1.对buffer 数据解码
		//2.位图+像素


		return TRUE;
	}
	else if (FZ_IMAGE_LZW == fz_com_buffer->params.type)
	{
		return TRUE;
	}
	return FALSE;
}


//解析资源信息
void CPdfCompress::PraseImageTypeObj()
{
	ATL::CString strTemp = _T("E:\\test\\convert\\Temp.pdf");
	
	std::vector<UINT32> vecObjNum;
	
	UINT32 uObjNums = pdf_count_objects(m_doc);
	for (UINT32 uIndex = 1; uIndex < uObjNums; uIndex++)
	{
		pdf_obj  *obj;
		obj = pdf_load_object(m_doc, uIndex, 0);
		
		if (IsImageType(obj) && IsFlateDecode(obj))
 		{
			vecObjNum.push_back(uIndex);
#if 0  //没有压缩的流 点阵像素图
			fz_buffer* fz_buf = pdf_load_stream(m_doc, uIndex, 0);
			FILE* fp;
			fp = fopen("E:\\test\\convert\\2222.txt", "wb");
			fwrite(fz_buf->data, fz_buf->len, 1, fp);
			fclose(fp);
			fz_drop_buffer(ctx, fz_buf);
#endif
 		}
		pdf_drop_obj(obj);
	}

	static UINT32 uCurIndex = 1;

	for (UINT32 uIndex = 0; uIndex < vecObjNum.size(); uIndex++)
	{	
		UINT32 uObjNum = vecObjNum[uIndex];
		//获取流数据
		/*if (IsCompressImageStream(doc, uNum, 0))*/
		{
#if 1
			CString strSrcImage, strDestImage;
			strSrcImage.Format(_T("%s\\yueshu-img-%d.png"), Util::Path::GetImageTempPath(), uObjNum);
			strDestImage.Format(_T("%s\\yueshu-img-%d.jpg"), Util::Path::GetImageTempPath(), uObjNum);
			if (/*SaveImageAsPng(strSrcImage, uObjNum)*/SavaImageAsJpg(uObjNum))
			{
				if (/*ImageConvert(strSrcImage, strDestImage, uObjNum) && */IsWriteStream(uObjNum))
				{
					pdf_obj  *obj = NULL;
					obj = pdf_load_object(m_doc, uObjNum, 0);
					WriteDataToStream(obj, strDestImage, uObjNum);
					pdf_drop_obj(obj);
				}
			}
			DeleteFile(strSrcImage);
			DeleteFile(strDestImage);
#endif
		}
		
		//跳线程设置进度
		JumpThreadSetProcess(uCurIndex++, vecObjNum.size());
	}

	fz_write_options ops;
	ops.do_clean = 1;
	ATL::CW2A szPdfOutPath(m_strPdfOutPath, CP_UTF8);
	pdf_write_document(m_doc, (char*)szPdfOutPath, &ops);
	if (m_doc)
	{
		pdf_close_document(m_doc);
		m_doc = nullptr;
	}
	if (m_ctx)
	{
		fz_free_context(m_ctx);
		m_ctx = nullptr;
	}

	//if (Util::Path::IsFileExist(strTemp))
	//{
	//	ATL::CString strTemp2 = _T("E:\\test\\convert\\TempA.pdf");
	//	ATL::CW2A szPdfOutPath(strTemp2, CP_UTF8);
	//	pdf_write_document(doc, (char*)szPdfOutPath, nullptr);
	//	if (doc)
	//	{
	//		pdf_close_document(doc);
	//		doc = NULL;
	//		fz_free_context(ctx);
	//		ctx = NULL;
	//	}
	//	::DeleteFile(strTemp);
	//	::CopyFile(strTemp2, strTemp, false);
	//	::DeleteFile(strTemp2);
	//}
	//else
	//{
	//	ATL::CW2A szPdfOutPath(strTemp, CP_UTF8);
	//	pdf_write_document(doc, (char*)szPdfOutPath, nullptr);
	//	if (doc)
	//	{
	//		pdf_close_document(doc);
	//		doc = NULL;
	//		fz_free_context(ctx);
	//		ctx = NULL;
	//	}
	//}
	
	//if (uIndex == vecNum.size())
	//{
	//	::CopyFile(strTemp, m_strPdfOutPath, false);
	//}

	uCurIndex = 1;
}

BOOL CPdfCompress::ImageConvert(ATL::CString strSrcImagePath, ATL::CString strDestImagePath, INT32 nNum)
{
	BOOL bRet = FALSE;
	if (strSrcImagePath.IsEmpty() || strDestImagePath.IsEmpty())
	{
		ATLASSERT(FALSE);
		return bRet;
	}
	ATL::CString strNum;
	strNum.Format(_T("yueshu-img-%d."), nNum);
	if (strSrcImagePath.Find(strNum) <  0 || strDestImagePath.Find(strNum) < 0)
	{
		return bRet;
	}
	bRet = Util::Image::ConvertType(strSrcImagePath, strDestImagePath);
	return bRet;
}


BOOL  CPdfCompress::WriteDataToStream(pdf_obj* dict, ATL::CString  strDestImagePath, INT32 nNum)
{
	ATL::CW2A szDestImagePath(strDestImagePath, CP_UTF8);

	std::ifstream fin(szDestImagePath, std::ios::binary);
	fin.seekg(0, std::ios::end);
	UINT32 uSize = fin.tellg();
	UINT8* szBuf = new (std::nothrow)UINT8[uSize];
	memset(szBuf, 0, sizeof(szBuf));
	fin.seekg(0, std::ios::beg);
	fin.read((char*)szBuf, sizeof(char) * uSize);
	fin.close();
#if 0
	UINT32 uDestLen = uSize;
	UINT8* buffer_dest = new (std::nothrow)UINT8[uSize];
	compress(buffer_dest, (uLong*)&uDestLen, szBuf, uSize);
#endif

	fz_buffer * stm_buf = nullptr;
	fz_try(m_ctx)
	{
		//对考jpeg图片格式数据到流中
		stm_buf = fz_new_buffer(m_ctx, (int)uSize);
		memcpy(stm_buf->data, szBuf, uSize);
		stm_buf->len = uSize;
	}
	fz_catch(m_ctx)
	{
		UINT32 u = 0;
		u = GetLastError();
	}
	delete[] szBuf;

#if 0
	CStringA strTest;
	for (UINT32 i = 0; i < uSize; i++)
	{
		strTest.AppendFormat("%02X", szBuf[i]);
	}
	FILE* fp;
	fp = fopen("E:\\1111.txt", "wb");
	fwrite(strTest, strTest.GetLength(), 1, fp);
	fclose(fp);
#endif

	//更新长度
	pdf_obj *length = pdf_dict_gets(dict, "Length");
	if (pdf_is_indirect(length))
	{
		int num = pdf_to_num(length);
		int gen = pdf_to_gen(length);
		pdf_obj* lenobj = pdf_load_object(m_doc, num, gen);
		pdf_set_int(lenobj, uSize);
		pdf_drop_obj(lenobj);
		pdf_update_object(m_doc, num, lenobj);
	}
	else if (pdf_is_int(length))
	{
		pdf_set_int(length, uSize);
	}
	pdf_dict_dels(dict, "Filter");
	pdf_dict_puts_drop(dict, "Filter", pdf_new_name(m_doc, "DCTDecode"));
	pdf_dict_dels(dict, "ColorSpace");
	pdf_dict_dels(dict, "DecodeParms");
	pdf_dict_puts_drop(dict, "ColorSpace", pdf_new_name(m_doc, "DeviceRGB"));
	pdf_dict_puts_drop(dict, "BitsPerComponent", pdf_new_int(m_doc, 8));

	//更新流
	pdf_update_stream(m_doc, nNum, stm_buf);
	fz_drop_buffer(m_ctx, stm_buf);
	//delete[] buffer_dest;
	return TRUE;
}

BOOL CPdfCompress::ImageSizeCompare(CString strSrcIamge, CString strDestImage)
{
	HANDLE h1 = CreateFile(strSrcIamge, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h1 == INVALID_HANDLE_VALUE)
		return FALSE;
	LARGE_INTEGER size1;
	BOOL ok = GetFileSizeEx(h1, &size1);
	if (!ok)
		return FALSE;
	UINT32 uSrcSize = size1.QuadPart;

	HANDLE h2 = CreateFile(strDestImage, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h2 == INVALID_HANDLE_VALUE)
		return FALSE;
	LARGE_INTEGER size2;
	BOOL ok2 = GetFileSizeEx(h2, &size2);
	if (!ok2)
		return FALSE;
	UINT32 uDestSize = size2.QuadPart;

	if (uDestSize > uSrcSize)
	{
		return FALSE;
	}
	return TRUE;
}

void CPdfCompress::JumpThreadSetProcess(INT32 nCurPos, INT32 nTotal)
{
	::PostMessage(m_hWnd, WM_UI_PROCESS, (WPARAM)nCurPos, (LPARAM)nTotal);
}


#if 0 //大纲目录 test

// 	if (Init(m_strPdfPath, m_strPassword))
// 	{
// 
// 		pdf_obj* root = pdf_dict_gets(pdf_trailer(doc), "Root");
//		pdf_obj* obj = pdf_dict_gets(root, "Outlines");
// 
// 		if (pdf_is_indirect(obj))
// 		{
// 			int num = pdf_to_num(obj);
// 			int gen = pdf_to_gen(obj);
// 			pdf_delete_object(doc, num);
// // 			pdf_obj* sub_obj = pdf_load_object(doc, num, gen);
// // 			pdf_drop_obj(sub_obj);
// // 			sub_obj = NULL;
// 		}
// 		pdf_obj* oa_obj = pdf_dict_gets(root, "OpenAction");
// 		if (pdf_is_indirect(oa_obj))
// 		{
// 			int num = pdf_to_num(oa_obj);
// 			int gen = pdf_to_gen(oa_obj);
// 			pdf_delete_object(doc, num);
// // 			pdf_obj* sub_obj2 = pdf_load_object(doc, num, gen);
// // 			pdf_drop_obj(sub_obj2);
// // 			sub_obj2 = NULL;
// 		}
// 		pdf_dict_dels(root, "Outlines");
// 		pdf_dict_dels(root, "OpenAction");
// 		pdf_dict_dels(root, "PageMode");
// 		
// 
// 		ATL::CW2A szPdfOutPath(m_strPdfOutPath, CP_UTF8);
// 		pdf_write_document(doc, (char*)szPdfOutPath, nullptr);
// 		if (doc)
// 		{
// 			pdf_close_document(doc);
// 			doc = NULL;
// 			fz_free_context(ctx);
// 			ctx = NULL;
// 		}
// 		//PraseResImage();
// 	}
// 	return 0;

#endif