#include "stdafx.h"
#include "TrafficMonitor.h"
#include "AboutDlg.h"
#include "MessageDlg.h"
#include "DrawCommon.h"
#include "NativeMaterial/Controls.h"

BEGIN_MESSAGE_MAP(CAboutDlg, CBaseDialog)
    ON_WM_PAINT()
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CBaseDialog(IDD_ABOUTBOX)
{
    EnableMaterialShell();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

CString CAboutDlg::GetDonateList()
{
    return CCommon::GetTextResource(IDR_ACKNOWLEDGEMENT_TEXT, 2);
}

CString CAboutDlg::GetDialogName() const
{
    return _T("AboutDlg");
}

int CAboutDlg::LayoutMaterialControls(const CRect& content, bool apply)
{
    const int margin = DPI(20), row_height = DPI(36), link_inset = DPI(15), link_gap = DPI(4);
    const int width = (std::max)(1, static_cast<int>(content.Width()) - margin * 2);
    int y = content.top;

    // Header picture across the full width.
    const int picture_height = DPI(64);
    if (apply)
        m_pic_rect = CRect(content.left, y, content.right, y + picture_height);
    y += picture_height + DPI(12);

    auto paragraph = [&](NativeMaterial::Label* label, int after)
    {
        if (label == nullptr)
            return;
        const int height = label->MeasureHeight(width);
        if (apply)
            label->MoveWindow(content.left + margin, y, width, height);
        y += height + after;
    };
    // A row of text buttons that wraps; the text of the first button lines up with the paragraphs.
    auto flow = [&](std::initializer_list<NativeMaterial::Button*> buttons)
    {
        const int start = content.left + margin - link_inset;
        const int limit = content.left + margin + width + link_inset;
        int x = start;
        bool any = false;
        for (auto* button : buttons)
        {
            if (button == nullptr)
                continue;
            CString label;
            button->GetWindowText(label);
            const int button_width = MeasureMaterialButtonWidth(label, NativeMaterial::ButtonVariant::Text, 48);
            if (any && x + button_width > limit)
            {
                x = start;
                y += row_height;
            }
            if (apply)
                button->MoveWindow(x, y, button_width, row_height);
            x += button_width + link_gap;
            any = true;
        }
        if (any)
            y += row_height;
    };

    paragraph(m_version, DPI(4));
    paragraph(m_copyright, DPI(8));
    paragraph(m_translator_text, DPI(8));
    flow({ m_translator_link });
    paragraph(m_third_party_caption, DPI(0));
    flow({ m_openhardwaremonitor_link, m_tinyxml2_link });
    paragraph(m_other_software_caption, DPI(0));
    flow({ m_musicplayer2_link, m_simplenotepad_link });
    y += DPI(8);
    flow({ m_mail, m_github, m_gitee });
    flow({ m_license, m_acknowledgement });

    y += DPI(8);
    if (m_ok != nullptr)
    {
        CString ok;
        m_ok->GetWindowText(ok);
        const int ok_width = MeasureMaterialButtonWidth(ok, NativeMaterial::ButtonVariant::Primary, 88);
        if (apply)
            m_ok->MoveWindow(content.left + margin + width - ok_width, y, ok_width, DPI(40));
        y += DPI(40);
    }
    if (apply)
        Invalidate(FALSE);      // the header picture is painted by the dialog itself
    return y + margin - content.top;
}

BOOL CAboutDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    SetRememberDlgSize(false);

    // Version information
    CString version_info;
    GetDlgItemText(IDC_STATIC_VERSION, version_info);
    CString str_lite;
#ifdef WITHOUT_TEMPERATURE
    str_lite = CCommon::LoadText(_T(" ("), IDS_WITHOUT_TEMPERATURE, _T(")"));
#endif
    version_info = CCommon::StringFormat(version_info, { str_lite, VERSION });

#ifdef COMPILE_FOR_WINXP
    version_info += _T(" (For WinXP)");
#endif // COMPILE_FOR_WINXP

#ifdef _M_ARM64EC
    version_info += _T(" (Arm64EC)");
#elif _M_X64
    version_info += _T(" (x64)");
#endif

#ifdef _DEBUG
    version_info += _T(" (Debug)");
#endif

    SetDlgItemText(IDC_STATIC_VERSION, version_info);

    // Last build date
    CString temp_str;
    GetDlgItemText(IDC_STATIC_COPYRIGHT, temp_str);
    CString str_compile_time = CCommon::GetLastCompileTime();
    temp_str = CCommon::StringFormat(temp_str, { COPYRITE_YEAR, str_compile_time });
    SetDlgItemText(IDC_STATIC_COPYRIGHT, temp_str);

    // Translator information: nothing without a translator, a link with a contact address, plain text otherwise.
    const auto& language_info{ theApp.m_str_table.GetLanguageInfo() };
    std::wstring translator_url{ language_info.translator_url };
    const bool has_translator = !language_info.translator.empty();
    if (has_translator)
    {
        SetDlgItemText(IDC_TRANSLATOR_STATIC, theApp.m_str_table.LoadTextFormat(L"TXT_ABOUT_TRANSLATOR", { language_info.display_name, language_info.translator }).c_str());
        if (!translator_url.empty())
        {
            // An address with "@" but without "mailto:" is a mail address.
            if (translator_url.find(L'@') != std::wstring::npos && (translator_url.size() < 7 || translator_url.substr(0, 7) != L"mailto:"))
                translator_url = L"mailto:" + translator_url;
            m_translator_link = ReplaceWithButton(IDC_TRANSLATOR_STATIC, NativeMaterial::ButtonVariant::Text);
            m_urls[IDC_TRANSLATOR_STATIC] = translator_url.c_str();
        }
        else
            m_translator_text = ReplaceWithLabel(IDC_TRANSLATOR_STATIC);
    }
    else
        RemoveStockControl(IDC_TRANSLATOR_STATIC);

    // The link statics become Material text buttons; their address is opened on click.
    struct Link { UINT id; NativeMaterial::Button** target; const wchar_t* url; };
    const Link links[] = {
        { IDC_STATIC_MAIL, &m_mail, L"mailto:zhongyang219@hotmail.com" },
        { IDC_STATIC_GITHUB, &m_github, L"https://github.com/zhongyang219/TrafficMonitor" },
        { IDC_STATIC_GITEE, &m_gitee, L"https://gitee.com/zhongyang219/TrafficMonitor" },
        { IDC_STATIC_ACKNOWLEDGEMENT, &m_acknowledgement, nullptr },    // opens the donor list
        { IDC_STATIC_LICENSE, &m_license, nullptr },                    // opens the license text
        { IDC_OPENHARDWAREMONITOR_LINK, &m_openhardwaremonitor_link, L"https://github.com/LibreHardwareMonitor/LibreHardwareMonitor" },
        { IDC_TINYXML2_LINK, &m_tinyxml2_link, L"https://github.com/leethomason/tinyxml2" },
        { IDC_MUSICPLAYER2_LINK, &m_musicplayer2_link, L"https://github.com/zhongyang219/MusicPlayer2" },
        { IDC_SIMPLENOTEPAD_LINK, &m_simplenotepad_link, L"https://github.com/zhongyang219/SimpleNotePad" },
    };
    for (const auto& link : links)
    {
        *link.target = ReplaceWithButton(link.id, NativeMaterial::ButtonVariant::Text);
        if (link.url != nullptr)
            m_urls[link.id] = link.url;
    }

    m_version = ReplaceWithLabel(IDC_STATIC_VERSION);
    m_copyright = ReplaceWithLabel(IDC_STATIC_COPYRIGHT);
    // The two captions share IDC_STATIC; take them in template (z-order) order.
    {
        std::vector<HWND> captions;
        for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
        {
            wchar_t class_name[16]{};
            ::GetClassNameW(child, class_name, 15);
            if (::GetDlgCtrlID(child) == IDC_STATIC && _wcsicmp(class_name, L"Static") == 0)
                captions.push_back(child);
        }
        if (captions.size() >= 1)
            m_third_party_caption = ReplaceWithLabel(CWnd::FromHandle(captions[0]), NativeMaterial::LabelRole::Secondary);
        if (captions.size() >= 2)
            m_other_software_caption = ReplaceWithLabel(CWnd::FromHandle(captions[1]), NativeMaterial::LabelRole::Secondary);
    }
    m_ok = ReplaceWithButton(IDOK, NativeMaterial::ButtonVariant::Primary);

    // Hover text of the links.
    m_tool_tip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX);
    m_tool_tip.AddTool(m_mail, CCommon::LoadText(IDS_SEND_EMAIL_TO_ATHOUR, _T("\r\nmailto:zhongyang219@hotmail.com")));
    m_tool_tip.AddTool(m_github, CCommon::LoadText(IDS_GOTO_GITHUB, _T("\r\nhttps://github.com/zhongyang219/TrafficMonitor")));
    m_tool_tip.AddTool(m_gitee, CCommon::LoadText(IDS_GOTO_GITEE, _T("\r\nhttps://gitee.com/zhongyang219/TrafficMonitor")));
    m_tool_tip.AddTool(m_openhardwaremonitor_link, m_urls[IDC_OPENHARDWAREMONITOR_LINK]);
    m_tool_tip.AddTool(m_tinyxml2_link, m_urls[IDC_TINYXML2_LINK]);
    m_tool_tip.AddTool(m_musicplayer2_link, CCommon::LoadText(IDS_MUSICPLAYER2_DESCRIPTION) + _T("\r\n") + m_urls[IDC_MUSICPLAYER2_LINK]);
    m_tool_tip.AddTool(m_simplenotepad_link, CCommon::LoadText(IDS_SIMPLENOTEPAD_DESCRIPTION) + _T("\r\n") + m_urls[IDC_SIMPLENOTEPAD_LINK]);
    if (m_translator_link != nullptr)
        m_tool_tip.AddTool(m_translator_link, CCommon::LoadText(IDS_CONTACT_TRANSLATOR) + _T("\r\n") + m_urls[IDC_TRANSLATOR_STATIC]);
    m_tool_tip.SetDelayTime(300);   // delay
    m_tool_tip.SetMaxTipWidth(800);

    // Header picture
    m_about_pic.LoadBitmap(IDB_ABOUT_BACKGROUND_HD);

    FitMaterialContent(360);
    FinishMaterialShell();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // exception: OCX property pages should return FALSE
}

BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_MOUSEMOVE)
        m_tool_tip.RelayEvent(pMsg);

    return CBaseDialog::PreTranslateMessage(pMsg);
}

BOOL CAboutDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (HIWORD(wParam) == BN_CLICKED)
    {
        const UINT id = LOWORD(wParam);
        if (id == IDC_STATIC_ACKNOWLEDGEMENT)
        {
            CMessageDlg dlg;
            dlg.SetWindowTitle(CCommon::LoadText(IDS_TITLE_ACKNOWLEDGEMENT));
            dlg.SetMessageText(GetDonateList());
            dlg.DoModal();
            return TRUE;
        }
        if (id == IDC_STATIC_LICENSE)
        {
            CMessageDlg dlg;
            dlg.SetWindowTitle(CCommon::LoadText(IDS_LICENSE));
            dlg.SetInfoText(CCommon::LoadText(IDS_LICENSE_EXPLAIN));
            dlg.SetMessageText(CCommon::GetTextResource(IDR_LICENSE, 1));
            dlg.DoModal();
            return TRUE;
        }
        const auto url = m_urls.find(id);
        if (url != m_urls.end() && !url->second.IsEmpty())
        {
            ShellExecute(NULL, _T("open"), url->second, NULL, NULL, SW_SHOW);
            return TRUE;
        }
    }
    return CBaseDialog::OnCommand(wParam, lParam);
}

void CAboutDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
                       // The base class paint is intentionally not called: only the header picture is drawn here.
    if (m_pic_rect.IsRectEmpty())
        return;
    CDrawCommon draw;
    draw.Create(&dc, this);
    draw.GetDC()->FillSolidRect(m_pic_rect, RGB(161, 200, 255));
    draw.DrawBitmap(m_about_pic, m_pic_rect.TopLeft(), m_pic_rect.Size(), CDrawCommon::StretchMode::FIT);
}
