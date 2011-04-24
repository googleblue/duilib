#include "stdafx.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "UIDesigner.h"
#include "UIDesignerView.h"
#include "ImageDialog.h"

using DuiLib::TRelativePosUI;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
//CMFCPropertyGridColor32Property

CMFCPropertyGridColor32Property::CMFCPropertyGridColor32Property(const CString& strName, const COLORREF& color,CPalette* pPalette/*=NULL*/,LPCTSTR lpszDescr/*=NULL*/,DWORD_PTR dwData/*=0*/)
:CMFCPropertyGridColorProperty(strName,color,pPalette,lpszDescr,dwData)
{
}

CMFCPropertyGridColor32Property::~CMFCPropertyGridColor32Property()
{
}

BOOL CMFCPropertyGridColor32Property::OnUpdateValue()
{
	ASSERT_VALID(this);

	if (m_pWndInPlace == NULL)
	{
		return FALSE;
	}

	ASSERT_VALID(m_pWndInPlace);
	ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));

	CString strText;
	m_pWndInPlace->GetWindowText(strText);

	COLORREF colorCurr = m_Color;
	int nA = 0,nR = 0, nG = 0, nB = 0;
	_stscanf_s(strText, _T("%2x%2x%2x%2x"), &nA, &nR, &nG, &nB);
	m_Color = ARGB(nA, nR, nG, nB);

	if (colorCurr != m_Color)
	{
		m_pWndList->OnPropertyChanged(this);
	}

	return TRUE;
}

void CMFCPropertyGridColor32Property::OnDrawValue(CDC* pDC, CRect rect)
{
	CRect rectColor = rect;

	rect.left += rect.Height();
	CMFCPropertyGridProperty::OnDrawValue(pDC, rect);

	rectColor.right = rectColor.left + rectColor.Height();
	rectColor.DeflateRect(1, 1);
	rectColor.top++;
	rectColor.left++;

	CBrush br(m_Color == (COLORREF)-1 ? m_ColorAutomatic : (m_Color&0x00FFFFFF));
	pDC->FillRect(rectColor, &br);
	pDC->Draw3dRect(rectColor, 0, 0);
}

CString CMFCPropertyGridColor32Property::FormatProperty()
{
	ASSERT_VALID(this);

	CString str;
	str.Format(_T("%02x%02x%02x%02x"),GetAValue(m_Color),GetRValue(m_Color),GetGValue(m_Color),GetBValue(m_Color));

	return str;
}

//////////////////////////////////////////////////////////////////////////
//CMFCPropertyGridImageProperty

IMPLEMENT_DYNAMIC(CMFCPropertyGridImageProperty, CMFCPropertyGridProperty)

CMFCPropertyGridImageProperty::CMFCPropertyGridImageProperty(const CString& strName, const CString& strImage, LPCTSTR lpszDescr/* = NULL*/, DWORD_PTR dwData/* = 0*/)
:CMFCPropertyGridProperty(strName, COleVariant((LPCTSTR)strImage), lpszDescr, dwData)
{
	m_dwFlags = AFX_PROP_HAS_BUTTON;
}

CMFCPropertyGridImageProperty::~CMFCPropertyGridImageProperty()
{

}

void CMFCPropertyGridImageProperty::OnClickButton(CPoint point)
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pWndList);
	ASSERT_VALID(m_pWndInPlace);
	ASSERT(::IsWindow(m_pWndInPlace->GetSafeHwnd()));

	m_bButtonIsDown = TRUE;
	Redraw();

	CString strImage = m_varValue.bstrVal;
	CImageDialog dlg(strImage);
	if(dlg.DoModal()==IDOK)
	{
		strImage=dlg.GetImage();
	}

	if (m_pWndInPlace != NULL)
	{
		m_pWndInPlace->SetWindowText(strImage);
	}
	m_varValue = (LPCTSTR) strImage;

	m_bButtonIsDown = FALSE;
	Redraw();

	if (m_pWndInPlace != NULL)
	{
		m_pWndInPlace->SetFocus();
	}
	else
	{
		m_pWndList->SetFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
// CPropertiesWnd

CPropertiesWnd::CPropertiesWnd():m_pControl(NULL)
{
	g_pPropertiesWnd=this;
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_PROPERTIES_TOOLBAR_SORT, OnPropertiesSort)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES_TOOLBAR_SORT, OnUpdatePropertiesSort)
	ON_COMMAND(ID_PROPERTIES_TOOLBAR_ALPHABETIC, OnPropertiesAlphaBetic)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES_TOOLBAR_ALPHABETIC, OnUpdatePropertiesAlphaBetic)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd 消息处理程序

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() -cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// 创建组合:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("未能创建属性网格\n");
		return -1;      // 未能创建
	}

	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* 已锁定*/);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0, TRUE /* 锁定*/);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// 所有命令将通过此控件路由，而不是通过主框架路由:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnPropertiesSort()
{
	m_wndPropList.SetAlphabeticMode(FALSE);
}

void CPropertiesWnd::OnUpdatePropertiesSort(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnPropertiesAlphaBetic()
{
	m_wndPropList.SetAlphabeticMode(TRUE);
}

void CPropertiesWnd::OnUpdatePropertiesAlphaBetic(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();

	CMFCPropertyGridProperty* pPropUI=NULL;
	CMFCPropertyGridProperty* pValueList=NULL;
	CMFCPropertyGridProperty* pProp=NULL;
	CMFCPropertyGridColorProperty* pPropColor=NULL;
	CMFCPropertyGridImageProperty* pPropImage=NULL;

	//Form
	pPropUI=new CMFCPropertyGridProperty(_T("Form"),classForm);

	pValueList=new CMFCPropertyGridProperty(_T("Size"),tagFormSize,TRUE);//size
	pProp=new CMFCPropertyGridProperty(_T("Width"),(_variant_t)(LONG)0,_T("窗体的宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Height"),(_variant_t)(LONG)0,_T("窗体的高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("Caption"),tagCaption,TRUE);//caption
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("标题的Left位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("标题的Top位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("标题的Right位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("标题的Bottom位置"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("SizeBox"),tagSizeBox,TRUE);//sizebox
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("尺寸盒的Left位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("尺寸盒的Top位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("尺寸盒的Right位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("尺寸盒的Bottom位置"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("RoundCorner"),tagRoundCorner,TRUE);//roundcorner
	pProp=new CMFCPropertyGridProperty(_T("Width"),(_variant_t)(LONG)0,_T("圆角的宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Height"),(_variant_t)(LONG)0,_T("圆角的高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("MinInfo"),tagMinInfo,TRUE);//mininfo
	pProp=new CMFCPropertyGridProperty(_T("MinWidth"),(_variant_t)(LONG)0,_T("窗口的最小跟踪宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("MinHeight"),(_variant_t)(LONG)0,_T("窗口的最小跟踪高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pProp=new CMFCPropertyGridProperty(_T("ShowDirty"),(_variant_t)false,_T("指示是否显示更新区域"),tagShowDirty);//showdirty
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Control
	pPropUI=new CMFCPropertyGridProperty(_T("Control"),classControl);

	pProp=new CMFCPropertyGridProperty(_T("Name"),(_variant_t)_T("Control"),_T("控件的名称"),tagName);//name
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Text"),(_variant_t)_T("Control"),_T("控件的显示文本"),tagText);//text
	pPropUI->AddSubItem(pProp);

	pValueList=new CMFCPropertyGridProperty(_T("Pos"),tagPos,TRUE);//pos
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("控件的Left位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("控件的Top位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("控件的Right位置"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("控件的Bottom位置"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("RelativePos"),tagRelativePos,TRUE);//relativepos
	pProp=new CMFCPropertyGridProperty(_T("MoveX"),(_variant_t)(LONG)0,_T("控件的水平位移"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("MoveY"),(_variant_t)(LONG)0,_T("控件的垂直位移"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("ZoomX"),(_variant_t)(LONG)0,_T("控件的水平比例"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("ZoomY"),(_variant_t)(LONG)0,_T("控件的垂直比例"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("Size"),tagSize,TRUE);//size
	pProp=new CMFCPropertyGridProperty(_T("Width"),(_variant_t)(LONG)0,_T("控件的宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Height"),(_variant_t)(LONG)0,_T("控件的高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("MinSize"),tagMinSize,TRUE);//minsize
	pProp=new CMFCPropertyGridProperty(_T("MinWidth"),(_variant_t)(LONG)0,_T("指定控件的最小宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("MinHeight"),(_variant_t)(LONG)0,_T("指定控件的最小高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("MaxSize"),tagMaxSize,TRUE);
	pProp=new CMFCPropertyGridProperty(_T("MaxWidth"),(_variant_t)(LONG)0,_T("指定控件的最大宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("MaxHeight"),(_variant_t)(LONG)0,_T("指定控件的最大高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("Padding"),tagPadding,TRUE);//padding
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("指定控件内部的左边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("指定控件内部的上边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("指定控件内部的右边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("指定控件内部的下边距"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pPropImage=new CMFCPropertyGridImageProperty(_T("BkImage"),_T(""),_T("指定控件的背景图片"),tagBkImage);//bkimage
	pPropUI->AddSubItem(pPropImage);

	pPropColor=new CMFCPropertyGridColor32Property(_T("BkColor"),(LONG)ARGB(0,0,0,0),NULL,_T("指定控件的背景颜色"),tagBkColor);//bkcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("BkColor2"),(LONG)RGB(0,0,0),NULL,_T("指定控件的背景颜色2"),tagBkColor2);//bkcolor2
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("BorderColor"),(LONG)RGB(0,0,0),NULL,_T("指定控件的边框颜色"),tagBorderColor);//bordercolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pProp=new CMFCPropertyGridProperty(_T("BorderSize"),(_variant_t)(LONG)0,_T("指定控件的边框线宽"),tagBorderSize);//bordersize
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Enabled"),(_variant_t)true,_T("指示是否已启用该控件"),tagEnabled);//enabled
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Float"),(_variant_t)false,_T("确定该控件是固定的，还是浮动的"),tagFloat);//float
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Visible"),(_variant_t)true,_T("确定该控件是可见的，还是隐藏的"),tagVisible);//visible
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Label
	pPropUI=new CMFCPropertyGridProperty(_T("Label"),classLabel);

	pProp=new CMFCPropertyGridProperty(_T("Align"),_T("Center"),_T("指示文本的对齐方式"),tagAlign);//align
	pProp->AddOption(_T("Center"));
	pProp->AddOption(_T("Left"));
	pProp->AddOption(_T("Right"));
	pProp->AddOption(_T("Top"));
	pProp->AddOption(_T("Bottom"));
	pProp->AllowEdit(FALSE);
	pPropUI->AddSubItem(pProp);

	pPropColor=new CMFCPropertyGridColorProperty(_T("TextColor"),(LONG)RGB(0,0,0),NULL,_T("指定文本的颜色"),tagTextColor);//textcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("DisabledTextColor"),(LONG)RGB(0,0,0),NULL,_T("指定文本的颜色"),tagDisabledTextColor);//disabledtextcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	LOGFONT lf;
	CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);
	pProp=new CMFCPropertyGridFontProperty(_T("Font"),lf,CF_EFFECTS|CF_SCREENFONTS,_T("指定文本的字体"),tagFont);//font
	pPropUI->AddSubItem(pProp);

	pValueList=new CMFCPropertyGridProperty(_T("TextPadding"),tagTextPadding,TRUE);//textpadding
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("指定文本区域的左边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("指定文本区域的上边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("指定文本区域的右边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("指定文本区域的下边距"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pProp=new CMFCPropertyGridProperty(_T("ShowHtml"),(_variant_t)false,_T("指示是否使用HTML格式的文本"),tagShowHtml);//showhtml
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Button
	pPropUI=new CMFCPropertyGridProperty(_T("Button"),classButton);

	pPropImage=new CMFCPropertyGridImageProperty(_T("NormalImage"),_T(""),_T("指定按钮正常显示时的图片"),tagNormalImage);//normalimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("HotImage"),_T(""),_T("指定按钮获得热点时的图片"),tagHotImage);//hotimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("PushedImage"),_T(""),_T("指定按钮被按压下时的图片"),tagPushedImage);//pushedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("FocusedImage"),_T(""),_T("指定按钮获得焦点后的图片"),tagFocusedImage);//focusedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("DisabledImage"),_T(""),_T("指定按钮被禁用后的图片"),tagDisabledImage);//disabledimage
	pPropUI->AddSubItem(pPropImage);

	m_wndPropList.AddProperty(pPropUI);

	//Edit
	pPropUI=new CMFCPropertyGridProperty(_T("Edit"),classEdit);

	pPropImage=new CMFCPropertyGridImageProperty(_T("NormalImage"),_T(""),_T("指定编辑框正常显示时的图片"),tagEditNormalImage);//normalimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("HotImage"),_T(""),_T("指定编辑框获得热点时的图片"),tagEditHotImage);//hotimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("FocusedImage"),_T(""),_T("指定编辑框获得焦点后的图片"),tagEditFocusedImage);//focusedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("DisabledImage"),_T(""),_T("指定编辑框被禁用后的图片"),tagEditDisabledImage);//disabledimage
	pPropUI->AddSubItem(pPropImage);

	pProp=new CMFCPropertyGridProperty(_T("ReadOnly"),(_variant_t)false,_T("指示是否只读"),tagReadOnly);//readonly
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Password"),(_variant_t)false,_T("指示是否使用密码框"),tagPassword);//password
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Option
	pPropUI=new CMFCPropertyGridProperty(_T("Option"),classOption);

	pPropImage=new CMFCPropertyGridImageProperty(_T("SelectedImage"),_T(""),_T("指定复选框被选择后的图片"),tagSelectedImage);//selectedimage
	pPropUI->AddSubItem(pPropImage);

	pProp=new CMFCPropertyGridProperty(_T("Selected"),(_variant_t)false,_T("指示是否已被选中"),tagSelected);//selected
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Group"),(_variant_t)false,_T("指示是否参与组合"),tagGroup);//group
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Progress
	pPropUI=new CMFCPropertyGridProperty(_T("Progress"),classProgress);

	pPropImage=new CMFCPropertyGridImageProperty(_T("FgImage"),_T(""),_T("指定进度条的前景图片"),tagFgImage);//fgimage
	pPropUI->AddSubItem(pPropImage);

	pValueList=new CMFCPropertyGridProperty(_T("MinMax"),tagMinMax,TRUE);//minmax
	pProp=new CMFCPropertyGridProperty(_T("Min"),(_variant_t)(LONG)0,_T("指定进度条的最小值"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Max"),(_variant_t)(LONG)0,_T("指定进度条的最大值"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pProp=new CMFCPropertyGridProperty(_T("Value"),(_variant_t)(LONG)0,_T("指定进度条的值"),tagValue);//value
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("Hor"),(_variant_t)false,_T("指示进度条是否水平"),tagHor);//hor
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Slider
	pPropUI=new CMFCPropertyGridProperty(_T("Slider"),classSlider);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ThumbImage"),_T(""),_T("指定滑块的滑条图片"),tagThumbImage);//thumbimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ThumbHotImage"),_T(""),_T("指定滑条获得热点时的图片"),tagThumbHotImage);//thumbhotimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ThumbPushedImage"),_T(""),_T("指定滑条被按压后的图片"),tagThumbPushedImage);//thumbpushedimage
	pPropUI->AddSubItem(pPropImage);

	pValueList=new CMFCPropertyGridProperty(_T("ThumbSize"),tagThumbSize,TRUE);//thumbsize
	pProp=new CMFCPropertyGridProperty(_T("Width"),(_variant_t)(LONG)0,_T("指定滑条的宽度"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Height"),(_variant_t)(LONG)0,_T("指定滑条的高度"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	m_wndPropList.AddProperty(pPropUI);

	//ActiveX
	pPropUI=new CMFCPropertyGridProperty(_T("ActiveX"),classActiveX);

	pProp=new CMFCPropertyGridProperty(_T("Clsid"),(_variant_t)_T(""),_T("指定ActiveX控件的Clsid值"),tagClsid);//clsid
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Container
	pPropUI=new CMFCPropertyGridProperty(_T("Container"),classContainer);

	pValueList=new CMFCPropertyGridProperty(_T("Inset"),tagInset,TRUE);//inset
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("指定容器客户区域的左边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("指定容器客户区域的上边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("指定容器客户区域的右边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("指定容器客户区域的下边距"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pProp=new CMFCPropertyGridProperty(_T("ChildPadding"),(_variant_t)(LONG)0,_T("指定子控件之间的间距"),tagChildPadding);//childpadding
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("MouseChild"),(_variant_t)false,_T("-----"),tagMouseChild);//mousechild
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("HScrollBar"),(_variant_t)false,_T("指示是否启用水平滚动条"),tagHScrollBar);//hscrollbar
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("VScrollBar"),(_variant_t)false,_T("指示是否启用垂直滚动条"),tagVScrollBar);//vscrollbar
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//Combo
	pPropUI=new CMFCPropertyGridProperty(_T("Combo"),classCombo);

	pPropImage=new CMFCPropertyGridImageProperty(_T("NormalImage"),_T(""),_T("指定组合框正常显示时的图片"),tagComboNormalImage);//normalimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("HotImage"),_T(""),_T("指定组合框获得热点时的图片"),tagComboHotImage);//hotimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("PushedImage"),_T(""),_T("指定组合框被按压下时的图片"),tagComboPushedImage);//pushedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("FocusedImage"),_T(""),_T("指定组合框获得焦点后的图片"),tagComboFocusedImage);//focusedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("DisabledImage"),_T(""),_T("指定组合框被禁用后的图片"),tagComboDisabledImage);//disabledimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ItemImage"),_T(""),_T("指定组项正常显示时的图片"),tagItemImage);//itemimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ItemHotImage"),_T(""),_T("指定组项获得热点时的图片"),tagItemHotImage);//itemhotimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ItemSelectedImage"),_T(""),_T("指定组项被选择时的图片"),tagItemSelectedImage);//itemselectedimage
	pPropUI->AddSubItem(pPropImage);

	pPropImage=new CMFCPropertyGridImageProperty(_T("ItemDisabledImage"),_T(""),_T("指定组项被禁用后的图片"),tagItemDisabledImage);//itemdisabledimage
	pPropUI->AddSubItem(pPropImage);

	pValueList=new CMFCPropertyGridProperty(_T("TextPadding"),tagComboTextPadding,TRUE);//textpadding
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("指定文本区域的左边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("指定文本区域的上边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("指定文本区域的右边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("指定文本区域的下边距"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pValueList=new CMFCPropertyGridProperty(_T("ItemTextPadding"),tagItemTextPadding,TRUE);//itemtextpadding
	pProp=new CMFCPropertyGridProperty(_T("Left"),(_variant_t)(LONG)0,_T("指定组项文本区域的左边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Top"),(_variant_t)(LONG)0,_T("指定组项文本区域的上边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Right"),(_variant_t)(LONG)0,_T("指定组项文本区域的右边距"));
	pValueList->AddSubItem(pProp);
	pProp=new CMFCPropertyGridProperty(_T("Bottom"),(_variant_t)(LONG)0,_T("指定组项文本区域的下边距"));
	pValueList->AddSubItem(pProp);
	pPropUI->AddSubItem(pValueList);

	pProp=new CMFCPropertyGridProperty(_T("ItemAlign"),_T("Center"),_T("指示组项文本的对齐方式"),tagItemAlign);//itemalign
	pProp->AddOption(_T("Center"));
	pProp->AddOption(_T("Left"));
	pProp->AddOption(_T("Right"));
	pProp->AllowEdit(FALSE);
	pPropUI->AddSubItem(pProp);

	pPropColor=new CMFCPropertyGridColorProperty(_T("ItemTextColor"),(LONG)RGB(0,0,0),NULL,_T("指定组项文本的颜色"),tagItemTextColor);//itemtextcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColor32Property(_T("ItemBkColor"),(LONG)ARGB(0,0,0,0),NULL,_T("指定组项背景的颜色"),tagItemBkColor);//itembkcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("ItemSelectedTextColor"),(LONG)RGB(0,0,0),NULL,_T("指定组项被选中后文本的颜色"),tagItemSelectedTextColor);//itemselectedtextcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColor32Property(_T("ItemSelectedBkColor"),(LONG)ARGB(0,0,0,0),NULL,_T("指定组项被选中后背景的颜色"),tagItemSelectedBkColor);//itemselectedbkcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("ItemHotTextColor"),(LONG)RGB(0,0,0),NULL,_T("指定组项获得热点时文本的颜色"),tagItemHotTextColor);//itemhottextcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColor32Property(_T("ItemHotBkColor"),(LONG)ARGB(0,0,0,0),NULL,_T("指定组项获得热点时背景的颜色"),tagItemHotBkColor);//itemhotbkcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("ItemDisabledTextColor"),(LONG)RGB(0,0,0),NULL,_T("指定组项被禁用后文本的颜色"),tagItemDisabledTextColor);//itemdisabledtextcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColor32Property(_T("ItemDisabledBkColor"),(LONG)ARGB(0,0,0,0),NULL,_T("指定组项被禁用后背景的颜色"),tagItemDisabledBkColor);//itemdisabledbkcolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pPropColor=new CMFCPropertyGridColorProperty(_T("ItemLineColor"),(LONG)RGB(0,0,0),NULL,_T("指定组项分割线的颜色"),tagItemLineColor);//itemlinecolor
	pPropColor->EnableOtherButton(_T("其他..."));
	pPropColor->EnableAutomaticButton(_T("默认"),::GetSysColor(COLOR_3DFACE));
	pPropUI->AddSubItem(pPropColor);

	pProp=new CMFCPropertyGridProperty(_T("ItemShowHtml"),(_variant_t)false,_T("指示是否使用Html格式文本"),tagItemShowHtml);//itemshowhtml
	pPropUI->AddSubItem(pProp);

	font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);
	pProp=new CMFCPropertyGridFontProperty(_T("ItemFont"),lf,CF_EFFECTS|CF_SCREENFONTS,_T("指定组项文本的字体"),tagItemFont);//itemfont
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//HorizontalLayout
	pPropUI=new CMFCPropertyGridProperty(_T("HorizontalLayout"),classHorizontalLayout);

	pProp=new CMFCPropertyGridProperty(_T("SepWidth"),(_variant_t)(LONG)0,_T("------"),tagSepWidth);//sepwidth
	pPropUI->AddSubItem(pProp);

	pProp=new CMFCPropertyGridProperty(_T("SepImm"),(_variant_t)false,_T("------"),tagSepImm);//sepimm
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	//TileLayout
	pPropUI=new CMFCPropertyGridProperty(_T("TileLayout"),classTileLayout);

	pProp=new CMFCPropertyGridProperty(_T("Columns"),(_variant_t)(LONG)0,_T("指定并列布局的列数"),tagColumns);//columns
	pPropUI->AddSubItem(pProp);

	m_wndPropList.AddProperty(pPropUI);

	HideAllProperties();
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}

void CPropertiesWnd::SetVSDotNetLook(BOOL bSet)
{
	m_wndPropList.SetVSDotNetLook(bSet);
	m_wndPropList.SetGroupNameFullWidth(bSet);
}

LRESULT CPropertiesWnd::OnPropertyChanged(WPARAM wp, LPARAM lp)
{
	CMFCPropertyGridProperty* pProp=(CMFCPropertyGridProperty *)lp;

	int nLevel=pProp->GetHierarchyLevel();
	for(;nLevel>1;nLevel--)
		pProp=pProp->GetParent();

	SetUIValue(pProp,pProp->GetData());

	return TRUE;
}

void CPropertiesWnd::HideAllProperties(BOOL bHide/*=TRUE*/,BOOL bAdjustLayout/*=FALSE*/)
{
	for(int i=0;i<m_wndPropList.GetPropertyCount();i++)
		m_wndPropList.GetProperty(i)->Show(!bHide,bAdjustLayout);

	m_pControl=NULL;
}

void CPropertiesWnd::ShowProperty(CControlUI* pControl)
{
	if(m_pControl==pControl)
		return;

	HideAllProperties();

	if(pControl==NULL)
		return;

	m_wndPropList.SetCurSel(NULL,FALSE);

	ExtendedAttributes* pExtended=(ExtendedAttributes*)pControl->GetTag();
	switch(pExtended->nClass)
	{
	case classForm:
		ShowFormProperty(pControl);
		break;
	case classControl:
		ShowControlProperty(pControl);
		break;
	case classLabel:
	case classText:
		ShowLabelProperty(pControl);
		break;
	case classButton:
		ShowButtonProperty(pControl);
		break;
	case classEdit:
		ShowEditProperty(pControl);
		break;
	case classOption:
		ShowOptionProperty(pControl);
		break;
	case classProgress:
		ShowProgressProperty(pControl);
		break;
	case classSlider:
		ShowSliderProperty(pControl);
		break;
	case classCombo: 
		ShowComboProperty(pControl);
		break;
	case classActiveX:
		ShowActiveXProperty(pControl);
		break;
	case classContainer:
	case classVerticalLayout:
	case classDialogLayout:
	case classTabLayout:
		ShowContainerProperty(pControl);
		break;
	case classHorizontalLayout:
		ShowHorizontalLayoutProperty(pControl);
		break;
	case classTileLayout:
		ShowTileLayoutProperty(pControl);
		break;
	default:
		return;
	}

	m_pControl=pControl;
	m_wndPropList.AdjustLayout();
}

void CPropertiesWnd::ShowFormProperty(CControlUI* pControl)
{
	ASSERT(pControl);
	CFormUI* pForm=static_cast<CFormUI*>(pControl->GetInterface(_T("Form")));
	ASSERT(pForm);

	CMFCPropertyGridProperty* pPropForm=m_wndPropList.FindItemByData(classForm,FALSE);
	ASSERT(pPropForm);

	//size
	SIZE size=pForm->GetInitSize();
	CMFCPropertyGridProperty* pValueList=pPropForm->GetSubItem(tagFormSize-tagForm);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)size.cx);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)size.cy);
	//caption
	RECT rect=pForm->GetCaptionRect();
	pValueList=pPropForm->GetSubItem(tagCaption-tagForm);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//sizebox
	rect=pForm->GetSizeBox();
	pValueList=pPropForm->GetSubItem(tagSizeBox-tagForm);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//roundcorner
	size=pForm->GetRoundCorner();
	pValueList=pPropForm->GetSubItem(tagRoundCorner-tagForm);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)size.cx);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)size.cy);
	//mininfo
	size=pForm->GetMinMaxInfo();
	pValueList=pPropForm->GetSubItem(tagMinInfo-tagForm);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)size.cx);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)size.cy);
	//showdirty
	pPropForm->GetSubItem(tagShowDirty-tagForm)->SetValue((_variant_t)pForm->IsShowUpdateRect());

	pPropForm->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowControlProperty(CControlUI* pControl)
{
	ASSERT(pControl);

	CMFCPropertyGridProperty* pPropControl=m_wndPropList.FindItemByData(classControl,FALSE);
	ASSERT(pPropControl);

	//name
	pPropControl->GetSubItem(tagName-tagControl)->SetValue((_variant_t)pControl->GetName());
	//text
	pPropControl->GetSubItem(tagText-tagControl)->SetValue((_variant_t)pControl->GetText());
	//pos
	SIZE szXY=pControl->GetFixedXY();
	int nWidth=pControl->GetFixedWidth();
	int nHeight=pControl->GetFixedHeight();
	CMFCPropertyGridProperty* pValueList=pPropControl->GetSubItem(tagPos-tagControl);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)szXY.cx);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)szXY.cy);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)(szXY.cx+nWidth));
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)(szXY.cy+nHeight));
	//relativepos
	TRelativePosUI posRelative=pControl->GetRelativePos();
	pValueList=pPropControl->GetSubItem(tagRelativePos-tagControl);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)posRelative.nMoveXPercent);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)posRelative.nMoveYPercent);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)posRelative.nZoomXPercent);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)posRelative.nZoomYPercent);
	//size
	pValueList=pPropControl->GetSubItem(tagSize-tagControl);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pControl->GetWidth());
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pControl->GetHeight());
	//minsize
	pValueList=pPropControl->GetSubItem(tagMinSize-tagControl);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pControl->GetMinWidth());
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pControl->GetMinHeight());
	//maxsize
	pValueList=pPropControl->GetSubItem(tagMaxSize-tagControl);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pControl->GetMaxWidth());
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pControl->GetMaxHeight());
	//padding
	pValueList=pPropControl->GetSubItem(tagPadding-tagControl);
	RECT rect=pControl->GetPadding();
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//bkimage
	pPropControl->GetSubItem(tagBkImage-tagControl)->SetValue((_variant_t)pControl->GetBkImage());
	//bkcolor
	static_cast<CMFCPropertyGridColor32Property*>(pPropControl->GetSubItem(tagBkColor-tagControl))->SetColor((_variant_t)(LONG)(pControl->GetBkColor()));
	//bkcolor2
	static_cast<CMFCPropertyGridColor32Property*>(pPropControl->GetSubItem(tagBkColor2-tagControl))->SetColor((_variant_t)(LONG)(pControl->GetBkColor2()));
	//bordercolor
	static_cast<CMFCPropertyGridColor32Property*>(pPropControl->GetSubItem(tagBorderColor-tagControl))->SetColor((_variant_t)(LONG)(pControl->GetBorderColor()));
	//bordersize
	pPropControl->GetSubItem(tagBorderSize-tagControl)->SetValue((_variant_t)(LONG)pControl->GetBorderSize());
	//enabled
	pPropControl->GetSubItem(tagEnabled-tagControl)->SetValue((_variant_t)pControl->IsEnabled());
	//float
	pPropControl->GetSubItem(tagFloat-tagControl)->SetValue((_variant_t)pControl->IsFloat());
	//visible
	pPropControl->GetSubItem(tagVisible-tagControl)->SetValue((_variant_t)pControl->IsVisible());

	pPropControl->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowLabelProperty(CControlUI* pControl)
{
	ShowControlProperty(pControl);

	ASSERT(pControl);
	CLabelUI* pLabel=static_cast<CLabelUI*>(pControl->GetInterface(_T("Label")));
	ASSERT(pLabel);

	CMFCPropertyGridProperty* pPropLabel=m_wndPropList.FindItemByData(classLabel,FALSE);
	ASSERT(pPropLabel);

	//align
	UINT uStyle=pLabel->GetTextStyle();
	CString strStyle;
	if(uStyle&DT_CENTER)
		strStyle=_T("Center");
	else if(uStyle&DT_LEFT)
		strStyle=_T("Left");
	else if(uStyle&DT_RIGHT)
		strStyle=_T("Right");
	else if(uStyle&DT_TOP)
		strStyle=_T("Top");
	else if(uStyle&DT_BOTTOM)
		strStyle=_T("Bottom");
	pPropLabel->GetSubItem(tagAlign-tagLabel)->SetValue((_variant_t)strStyle);
	//textcolor
	static_cast<CMFCPropertyGridColor32Property*>(pPropLabel->GetSubItem(tagTextColor-tagLabel))->SetColor((_variant_t)(LONG)(pLabel->GetTextColor()));
	//disabledtextcolor
	static_cast<CMFCPropertyGridColor32Property*>(pPropLabel->GetSubItem(tagTextColor-tagLabel))->SetColor((_variant_t)(LONG)(pLabel->GetTextColor()));
	//font
	//textpadding
	CMFCPropertyGridProperty* pValueList=pPropLabel->GetSubItem(tagTextPadding-tagLabel);
	RECT rect=pLabel->GetTextPadding();
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//showhtml
	pPropLabel->GetSubItem(tagShowHtml-tagLabel)->SetValue((_variant_t)pLabel->IsShowHtml());

	pPropLabel->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowButtonProperty(CControlUI* pControl)
{
	ShowLabelProperty(pControl);

	ASSERT(pControl);
	CButtonUI* pButton=static_cast<CButtonUI*>(pControl->GetInterface(_T("Button")));
	ASSERT(pButton);

	CMFCPropertyGridProperty* pPropButton=m_wndPropList.FindItemByData(classButton,FALSE);
	ASSERT(pPropButton);

	//normalimage
	pPropButton->GetSubItem(tagNormalImage-tagButton)->SetValue((_variant_t)pButton->GetNormalImage());
	//hotimage
	pPropButton->GetSubItem(tagHotImage-tagButton)->SetValue((_variant_t)pButton->GetHotImage());
	//pushedimage
	pPropButton->GetSubItem(tagPushedImage-tagButton)->SetValue((_variant_t)pButton->GetPushedImage());
	//focusedimage
	pPropButton->GetSubItem(tagFocusedImage-tagButton)->SetValue((_variant_t)pButton->GetFocusedImage());
	//disabledimage
	pPropButton->GetSubItem(tagDisabledImage-tagButton)->SetValue((_variant_t)pButton->GetDisabledImage());

	pPropButton->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowEditProperty(CControlUI* pControl)
{
	ShowLabelProperty(pControl);

	ASSERT(pControl);
	CEditUI* pEdit=static_cast<CEditUI*>(pControl->GetInterface(_T("Edit")));
	ASSERT(pEdit);

	CMFCPropertyGridProperty* pPropEdit=m_wndPropList.FindItemByData(classEdit,FALSE);
	ASSERT(pPropEdit);

	//normalimage
	pPropEdit->GetSubItem(tagEditNormalImage-tagEdit)->SetValue((_variant_t)pEdit->GetNormalImage());
	//hotimage
	pPropEdit->GetSubItem(tagEditHotImage-tagEdit)->SetValue((_variant_t)pEdit->GetHotImage());
	//focusedimage
	pPropEdit->GetSubItem(tagEditFocusedImage-tagEdit)->SetValue((_variant_t)pEdit->GetFocusedImage());
	//disabledimage
	pPropEdit->GetSubItem(tagEditDisabledImage-tagEdit)->SetValue((_variant_t)pEdit->GetDisabledImage());
	//readonly
	pPropEdit->GetSubItem(tagReadOnly-tagEdit)->SetValue((_variant_t)pEdit->IsReadOnly());
	//password
	pPropEdit->GetSubItem(tagPassword-tagEdit)->SetValue((_variant_t)pEdit->IsPasswordMode());

	pPropEdit->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowOptionProperty(CControlUI* pControl)
{
	ShowButtonProperty(pControl);

	ASSERT(pControl);
	COptionUI* pOption=static_cast<COptionUI*>(pControl->GetInterface(_T("Option")));
	ASSERT(pOption);

	CMFCPropertyGridProperty* pPropOption=m_wndPropList.FindItemByData(classOption,FALSE);
	ASSERT(pPropOption);

	//selectedimage
	pPropOption->GetSubItem(tagSelectedImage-tagOption)->SetValue((_variant_t)pOption->GetSelectedImage());
	//selected
	pPropOption->GetSubItem(tagSelected-tagOption)->SetValue((_variant_t)pOption->IsSelected());
	//group
	pPropOption->GetSubItem(tagGroup-tagOption)->SetValue((_variant_t)pOption->GetGroup());

	pPropOption->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowProgressProperty(CControlUI* pControl)
{
	ShowLabelProperty(pControl);

	ASSERT(pControl);
	CProgressUI* pProgress=static_cast<CProgressUI*>(pControl->GetInterface(_T("Progress")));
	ASSERT(pProgress);

	CMFCPropertyGridProperty* pPropProgress=m_wndPropList.FindItemByData(classProgress,FALSE);
	ASSERT(pPropProgress);

	//fgimage
	pPropProgress->GetSubItem(tagFgImage-tagProgress)->SetValue((_variant_t)pProgress->GetFgImage());
	//minmax
	CMFCPropertyGridProperty* pValueList=pPropProgress->GetSubItem(tagMinMax-tagProgress);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pProgress->GetMinValue());
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pProgress->GetMaxValue());
	//value
	pPropProgress->GetSubItem(tagValue-tagProgress)->SetValue((_variant_t)(LONG)pProgress->GetValue());
	//hor
	pPropProgress->GetSubItem(tagHor-tagProgress)->SetValue((_variant_t)pProgress->IsHorizontal());

	pPropProgress->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowSliderProperty(CControlUI* pControl)
{
	ShowProgressProperty(pControl);

	ASSERT(pControl);
	CSliderUI* pSlider=static_cast<CSliderUI*>(pControl->GetInterface(_T("Slider")));
	ASSERT(pSlider);

	CMFCPropertyGridProperty* pPropSlider=m_wndPropList.FindItemByData(classSlider,FALSE);
	ASSERT(pPropSlider);

	//thumbimage
	pPropSlider->GetSubItem(tagThumbImage-tagSlider)->SetValue((_variant_t)pSlider->GetThumbImage());
	//thumbhotimage
	pPropSlider->GetSubItem(tagThumbHotImage-tagSlider)->SetValue((_variant_t)pSlider->GetThumbHotImage());
	//thumbpushedimage
	pPropSlider->GetSubItem(tagThumbPushedImage-tagSlider)->SetValue((_variant_t)pSlider->GetThumbPushedImage());
	//thumbsize
	CMFCPropertyGridProperty* pValueList=pPropSlider->GetSubItem(tagThumbSize-tagSlider);
	RECT rect=pSlider->GetThumbRect();
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)(rect.right-rect.left));
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)(rect.bottom-rect.top));

	pPropSlider->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowComboProperty(CControlUI* pControl)
{
	ShowContainerProperty(pControl);

	ASSERT(pControl);
	CComboUI* pCombo=static_cast<CComboUI*>(pControl->GetInterface(_T("Combo")));
	ASSERT(pCombo);

	CMFCPropertyGridProperty* pPropCombo=m_wndPropList.FindItemByData(classCombo,FALSE);
	ASSERT(pPropCombo);

	//normalimage
	pPropCombo->GetSubItem(tagComboNormalImage-tagCombo)->SetValue((_variant_t)pCombo->GetNormalImage());
	//hotimage
	pPropCombo->GetSubItem(tagComboHotImage-tagCombo)->SetValue((_variant_t)pCombo->GetHotImage());
	//pushedimage
	pPropCombo->GetSubItem(tagComboPushedImage-tagCombo)->SetValue((_variant_t)pCombo->GetPushedImage());
	//focusedimage
	pPropCombo->GetSubItem(tagComboFocusedImage-tagCombo)->SetValue((_variant_t)pCombo->GetFocusedImage());
	//disabledimage
	pPropCombo->GetSubItem(tagComboDisabledImage-tagCombo)->SetValue((_variant_t)pCombo->GetDisabledImage());
	//itemimage
	TListInfoUI* pListInfo=pCombo->GetListInfo();
	pPropCombo->GetSubItem(tagItemImage-tagCombo)->SetValue((_variant_t)pListInfo->sImage);
	//itemhotimage
	pPropCombo->GetSubItem(tagItemHotImage-tagCombo)->SetValue((_variant_t)pListInfo->sHotImage);
	//itemselectedimage
	pPropCombo->GetSubItem(tagItemSelectedImage-tagCombo)->SetValue((_variant_t)pListInfo->sSelectedImage);
	//itemdisabledimage
	pPropCombo->GetSubItem(tagItemDisabledImage-tagCombo)->SetValue((_variant_t)pListInfo->sDisabledImage);
	//textpadding
	RECT rect=pCombo->GetTextPadding();
	CMFCPropertyGridProperty* pValueList=pPropCombo->GetSubItem(tagComboTextPadding-tagCombo);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//itemtextpadding
	pValueList=pPropCombo->GetSubItem(tagItemTextPadding-tagCombo);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pListInfo->rcTextPadding.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pListInfo->rcTextPadding.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)pListInfo->rcTextPadding.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)pListInfo->rcTextPadding.bottom);
	//itemalign
	CString strStyle;
	if(pListInfo->uTextStyle&DT_CENTER)
		strStyle=_T("Center");
	else if(pListInfo->uTextStyle&DT_LEFT)
		strStyle=_T("Left");
	else if(pListInfo->uTextStyle&DT_RIGHT)
		strStyle=_T("Right");
	pPropCombo->GetSubItem(tagItemAlign-tagCombo)->SetValue((_variant_t)strStyle);
	//itemtextcolor
	pPropCombo->GetSubItem(tagItemTextColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwTextColor);
	//itembkcolor
	pPropCombo->GetSubItem(tagItemBkColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwBkColor);
	//itemselectedtextcolor
	pPropCombo->GetSubItem(tagItemSelectedTextColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwSelectedTextColor);
	//itemselectedbkcolor
	pPropCombo->GetSubItem(tagItemSelectedBkColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwSelectedBkColor);
	//itemhottextcolor
	pPropCombo->GetSubItem(tagItemHotTextColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwHotTextColor);
	//itemhotbkcolor
	pPropCombo->GetSubItem(tagItemHotBkColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwHotBkColor);
	//itemdisabledtextcolor
	pPropCombo->GetSubItem(tagItemDisabledTextColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwDisabledTextColor);
	//itemdisabledbkcolor
	pPropCombo->GetSubItem(tagItemDisabledBkColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwDisabledBkColor);
	//itemlinecolor
	pPropCombo->GetSubItem(tagItemLineColor-tagCombo)->SetValue((_variant_t)(LONG)pListInfo->dwLineColor);
	//itemshowhtml
	pPropCombo->GetSubItem(tagItemShowHtml-tagCombo)->SetValue((_variant_t)pCombo->IsItemShowHtml());

	pPropCombo->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowActiveXProperty(CControlUI* pControl)
{
	ShowControlProperty(pControl);

	ASSERT(pControl);
	CActiveXUI* pActiveX=static_cast<CActiveXUI*>(pControl->GetInterface(_T("ActiveX")));
	ASSERT(pActiveX);

	CMFCPropertyGridProperty* pPropActiveX=m_wndPropList.FindItemByData(classActiveX,FALSE);
	ASSERT(pPropActiveX);

	CLSID clsid=pActiveX->GetClisd();
	TCHAR strCLSID[48];
	StringFromGUID2(clsid,strCLSID,48);
	pPropActiveX->GetSubItem(tagClsid-tagActiveX)->SetValue((_variant_t)strCLSID);//clsid

	pPropActiveX->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowContainerProperty(CControlUI* pControl)
{
	ShowControlProperty(pControl);

	ASSERT(pControl);
	CContainerUI* pContainer=static_cast<CContainerUI*>(pControl->GetInterface(_T("Container")));
	ASSERT(pContainer);

	CMFCPropertyGridProperty* pPropContainer=m_wndPropList.FindItemByData(classContainer,FALSE);
	ASSERT(pPropContainer);

	//inset
	RECT rect=pContainer->GetInset();
	CMFCPropertyGridProperty* pValueList=pPropContainer->GetSubItem(tagInset-tagContainer);
	pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)rect.left);
	pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)rect.top);
	pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)rect.right);
	pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)rect.bottom);
	//childpadding
	pPropContainer->GetSubItem(tagChildPadding-tagContainer)->SetValue((_variant_t)(LONG)pContainer->GetChildPadding());
	//mousechild
	pPropContainer->GetSubItem(tagMouseChild-tagContainer)->SetValue((_variant_t)pContainer->IsMouseChildEnabled());
	//hscrollbar
	pPropContainer->GetSubItem(tagHScrollBar-tagContainer)->SetValue((_variant_t)(pContainer->GetHorizontalScrollBar()==NULL?false:true));
	//vscrollbar
	pPropContainer->GetSubItem(tagMouseChild-tagContainer)->SetValue((_variant_t)(pContainer->GetVerticalScrollBar()==NULL?false:true));

	pPropContainer->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowHorizontalLayoutProperty(CControlUI* pControl)
{
	ShowContainerProperty(pControl);

	ASSERT(pControl);
	CHorizontalLayoutUI* pHorizontalLayout=static_cast<CHorizontalLayoutUI*>(pControl->GetInterface(_T("HorizontalLayout")));
	ASSERT(pHorizontalLayout);

	CMFCPropertyGridProperty* pPropHorizontalLayout=m_wndPropList.FindItemByData(classHorizontalLayout,FALSE);
	ASSERT(pPropHorizontalLayout);

	//sepwidth
	pPropHorizontalLayout->GetSubItem(tagSepWidth-tagHorizontalLayout)->SetValue((_variant_t)(LONG)pHorizontalLayout->GetSepWidth());
	//sepimm
	pPropHorizontalLayout->GetSubItem(tagSepImm-tagHorizontalLayout)->SetValue((_variant_t)pHorizontalLayout->IsSepImmMode());

	pPropHorizontalLayout->Show(TRUE,FALSE);
}

void CPropertiesWnd::ShowTileLayoutProperty(CControlUI* pControl)
{
	ShowContainerProperty(pControl);

	ASSERT(pControl);
	CTileLayoutUI* pTileLayout=static_cast<CTileLayoutUI*>(pControl->GetInterface(_T("TileLayout")));
	ASSERT(pTileLayout);

	CMFCPropertyGridProperty* pPropTileLayout=m_wndPropList.FindItemByData(classTileLayout,FALSE);
	ASSERT(pPropTileLayout);

	//sepwidth
	pPropTileLayout->GetSubItem(tagSepWidth-tagHorizontalLayout)->SetValue((_variant_t)(LONG)pTileLayout->GetColumns());

	pPropTileLayout->Show(TRUE,FALSE);
}

void CPropertiesWnd::SetPropValue(CControlUI* pControl,int nTag)
{
	if(m_pControl!=pControl)
		return;

	CMFCPropertyGridProperty* pPropUI=NULL;
	CMFCPropertyGridProperty* pValueList=NULL;
	switch(nTag)
	{
	case tagFormSize:
		{
			pPropUI=m_wndPropList.FindItemByData(classForm,FALSE);

			RECT rect=pControl->GetPos();
			pValueList=pPropUI->GetSubItem(tagFormSize-tagForm);
			pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)(rect.right-rect.left));
			pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)(rect.bottom-rect.top));

			break;
		}
	case tagPos:
		{
			pPropUI=m_wndPropList.FindItemByData(classControl,FALSE);

			SIZE szXY=pControl->GetFixedXY();
			int nWidth=pControl->GetFixedWidth();
			int nHeight=pControl->GetFixedHeight();
			pValueList=pPropUI->GetSubItem(tagPos-tagControl);
			pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)szXY.cx);
			pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)szXY.cy);
			pValueList->GetSubItem(2)->SetValue((_variant_t)(LONG)(szXY.cx+nWidth));
			pValueList->GetSubItem(3)->SetValue((_variant_t)(LONG)(szXY.cy+nHeight));

			break;
		}
	case tagSize:
		{
			pPropUI=m_wndPropList.FindItemByData(classControl,FALSE);

			pValueList=pPropUI->GetSubItem(tagSize-tagControl);
			pValueList->GetSubItem(0)->SetValue((_variant_t)(LONG)pControl->GetWidth());
			pValueList->GetSubItem(1)->SetValue((_variant_t)(LONG)pControl->GetHeight());

			break;
		}
	}
}

void CPropertiesWnd::SetUIValue(CMFCPropertyGridProperty* pProp,int nTag)
{
	if(m_pControl==NULL)
		return;

	int nUpdate=UPDATE_REDRAW_CONTROL;
	CString strName=pProp->GetName();
	strName.MakeLower();
	CString strVal;
	switch(nTag)
	{
	case tagPos:
	case tagPadding:
		strVal.Format(_T("%s,%s,%s,%s"),pProp->GetSubItem(0)->FormatProperty(),
			pProp->GetSubItem(1)->FormatProperty(),
			pProp->GetSubItem(2)->FormatProperty(),
			pProp->GetSubItem(3)->FormatProperty());
		SetPropValue(m_pControl,tagSize);

		nUpdate=UPDATE_POS;
		break;
	case tagInset:
	case tagTextPadding:
		strVal.Format(_T("%s,%s,%s,%s"),pProp->GetSubItem(0)->FormatProperty(),
			pProp->GetSubItem(1)->FormatProperty(),
			pProp->GetSubItem(2)->FormatProperty(),
			pProp->GetSubItem(3)->FormatProperty());

		break;
	case tagSize:
	case tagMinSize:
	case tagMaxSize:
	case tagMinMax:
		SetUIValue(pProp->GetSubItem(0),-1);
		SetUIValue(pProp->GetSubItem(1),-1);
		SetPropValue(m_pControl,tagPos);

		nUpdate=UPDATE_POS;
		break;
	case tagFloat:
	case tagColumns:
		strVal=pProp->FormatProperty();

		nUpdate=UPDATE_POS;
		break;
	case tagVisible:
		strVal=pProp->FormatProperty();

		nUpdate=UPDATE_REDRAW_PARENT;
		break;
	case tagThumbSize:
		strVal.Format(_T("%s,%s"),pProp->GetSubItem(0)->FormatProperty(),
			pProp->GetSubItem(1)->FormatProperty());

		break;
	default:
		strVal=pProp->FormatProperty();

		break;
	}
	if(strName.Find(_T("image"))==-1)
	{
		strVal.MakeLower();
	}
	m_pControl->SetAttribute(strName,strVal);

	if(nTag==tagFormSize)
	{
		CUIDesignerView* pUIView=g_pMainFrame->GetActiveUIView();
		ASSERT(pUIView);
		if(pUIView==NULL)
			return;

		TNotifyUI Msg;
		Msg.pSender=m_pControl;
		Msg.sType=_T("formsize");
		Msg.wParam=0;
		Msg.lParam=NULL;
		pUIView->Notify(Msg);
	}

	CControlUI* pParent=m_pControl->GetParent();
	if(pParent==NULL)
		pParent=m_pControl;
	switch(nUpdate)
	{
	case UPDATE_POS:
		pParent->SetPos(pParent->GetPos());

		break;
	case UPDATE_REDRAW_CONTROL:
		m_pControl->NeedUpdate();

		break;
	case UPDATE_REDRAW_PARENT:
		pParent->NeedUpdate();
	}
}