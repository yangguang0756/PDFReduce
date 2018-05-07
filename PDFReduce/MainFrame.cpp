#include "MainFrame.h"
#include <atlstr.h>
#include <ShlObj.h>
#include <string>

#include "ximage.h"   

CMainFrame::CMainFrame()
{
}


CMainFrame::~CMainFrame()
{

}


CDuiString CMainFrame::GetSkinFolder()
{
	return _T("Resource");
}

CDuiString CMainFrame::GetSkinFile()
{
	return _T("xml\\MainFrame.xml");
}

LPCTSTR CMainFrame::GetWindowClassName(void) const
{
	return _T("MainFrameClass");
}

CControlUI* CMainFrame::CreateControl(LPCTSTR pstrClass)
{
	return NULL;
}

void CMainFrame::InitWindow()
{
	m_pTreeList = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("tab_tree_list")));

}

void CMainFrame::OnFinalMessage(HWND hWnd)
{

}

LRESULT CMainFrame::ResponseDefaultKeyEvent(WPARAM wParam)
{
	if (wParam == VK_RETURN)
	{
		return TRUE;
	}
	else if (wParam == VK_F10)
	{
		return TRUE;
	}


	return FALSE;
}

void CMainFrame::Notify(TNotifyUI& msg)
{
	if (msg.sType == DUI_MSGTYPE_CLICK){
		CDuiString szName = msg.pSender->GetName();
		if (szName == _T("btn_min"))
		{
			SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		else if (szName == _T("btn_close"))
		{
			ShowWindow(false);
			Close();
		}
		else if (szName == _T("opt_pdf_reduce"))  //select pdf reduce
		{
			m_pTreeList->SelectItem(0);
		}
		else if (szName == _T("opt_pdf_author"))  //select pdf author
		{
			m_pTreeList->SelectItem(1);
		}	
		else if (szName == _T("btn_open_pdf_compress"))  //启动PDF体积压缩
		{
			StartPDFCompress();
		}
		else if (szName == _T("btn_open_pdf_path"))  //选择打开PDF路径框
		{
			SelectPDFFolderDialog();
		}
	}
}

LRESULT CMainFrame::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT hr = 0;

	if (bHandled) return hr;
	return __super::HandleCustomMessage(uMsg, wParam, lParam, bHandled);
}


BOOL IsFileExist(const CString& csFile)
{
	DWORD dwAttrib = GetFileAttributes(csFile);
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

#include "UtilImage.h"

void CMainFrame::StartPDFCompress()
{

	CxImage image;
	CString  imageName = _T("C:\\Users\\C\\Desktop\\PDFReduce\\77777.tif");
	CString  imageSave = _T("C:\\Users\\C\\Desktop\\PDFReduce\\222222.jpg");
	Util::Image::ConvertType(imageName, imageSave);
	return;
	CEditUI* pEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edt_pdf_in_path")));
	ATL::CString strPDFPath = pEdit->GetText();
	if (strPDFPath.IsEmpty())
	{
		::MessageBox(m_hWnd, _T("输入PDF路径为空,请输入PDF路径!"), _T("提示"),MB_OK);
		return;
	}

	if (!IsFileExist(strPDFPath))
	{
		::MessageBox(m_hWnd, _T("输入PDF路径不存在，请重新输入!"), _T("提示"), MB_OK);
		return;
	}

	CString strExe = strPDFPath.Right(4);
	if ( 0 != strExe.CompareNoCase(_T(".pdf")))
	{
		::MessageBox(m_hWnd, _T("输入的不是PDF文件，请重新输入!"), _T("提示"), MB_OK);
		return;
	}
	
	//对PDF 压缩体积处理


}


void CMainFrame::SelectPDFFolderDialog()
{
	TCHAR szBuffer[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuffer;
	bi.lpszTitle = _T("从下面选择PDF文件:");
	bi.ulFlags = BIF_BROWSEINCLUDEFILES;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (NULL == idl)
	{
		return;
	}
	SHGetPathFromIDList(idl, szBuffer);
	
	CString strPDFPath(szBuffer);
	CEditUI* pEdit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edt_pdf_in_path")));
	if (pEdit && !strPDFPath.IsEmpty())pEdit->SetText(strPDFPath);
}


