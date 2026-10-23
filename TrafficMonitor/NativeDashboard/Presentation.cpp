#include "../stdafx.h"
#include "Presentation.h"
#include "../NativePreferences/Text.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace NativeDashboard
{
    BOOL DataView::Create(CWnd* parent,UINT id)
    {
        return CreateEx(0,AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,::LoadCursor(nullptr,IDC_ARROW)),
            L"Observed receive and send rates",WS_CHILD|WS_VISIBLE,CRect(0,0,100,100),parent,id);
    }
    void DataView::SetSnapshot(const NativeMonitoring::AdapterSnapshot* row,bool chinese)
    {
        m_chinese=chinese;
        if(row)m_row=*row;else m_row.reset();
        Invalidate(FALSE);
    }
    void DataView::SetAppearance(NativeMaterial::Appearance value)
    {
        m_appearance=value;Invalidate(FALSE);
    }
    LRESULT DataView::WindowProc(UINT message,WPARAM w,LPARAM l)
    {
        if(message==WM_ERASEBKGND)return TRUE;
        if(message==WM_DESTROY)m_canvas.Detach();
        if(message==WM_PAINT)
        {
            CPaintDC dc(this);CRect r;GetClientRect(r);const UINT dpi=GetDpiForWindow(m_hWnd);
            if(!m_canvas.Target()&&FAILED(m_canvas.Attach(m_hWnd,float(dpi))))return 0;
            m_canvas.Resize(r.Width(),r.Height(),float(dpi));
            const auto colors=NativeMaterial::Palette::Resolve(m_appearance);
            if(SUCCEEDED(m_canvas.Begin(colors.surface)))
            {
                DrawData(m_canvas,colors,r.Width()*96.0f/dpi,0,112,264,m_row?&*m_row:nullptr,m_chinese);
                m_canvas.End();
            }
            return 0;
        }
        if(message==WM_MOUSEWHEEL||message==WM_CONTEXTMENU)return GetParent()->SendMessage(message,w,l);
        return CWnd::WindowProc(message,w,l);
    }
    std::wstring Rate(const std::optional<double>& value, bool chinese)
    {
        if (!value || !std::isfinite(*value)) return NativePreferences::InlinePair(L"Unavailable",L"未有讀數").GetString();
        double amount = *value; const wchar_t* unit = L"B/s";
        if (amount >= 1048576) { amount /= 1048576; unit = L"MiB/s"; }
        else if (amount >= 1024) { amount /= 1024; unit = L"KiB/s"; }
        std::wostringstream text; text << std::fixed << std::setprecision(amount < 10 ? 2 : 1) << amount << L" " << unit;
        return text.str();
    }
    std::wstring RateStateText(NativeMonitoring::RateState state, bool chinese)
    {
        using NativeMonitoring::RateState;
        switch (state)
        {
        case RateState::Available: return NativePreferences::InlinePair(L"Live",L"即時").GetString();
        case RateState::CollectingFirstSample: return NativePreferences::InlinePair(L"Collecting first sample",L"正在收集首個樣本").GetString();
        case RateState::Disconnected: return NativePreferences::InlinePair(L"Disconnected",L"未連線").GetString();
        case RateState::IdentityUnavailable: return NativePreferences::InlinePair(L"Identity unavailable",L"識別資料未能取得").GetString();
        default: return NativePreferences::InlinePair(L"Stale",L"讀數已過時").GetString();
        }
    }
    std::wstring TimeText(std::uint64_t value)
    {
        if (!value) return L"--";
        FILETIME file{ static_cast<DWORD>(value), static_cast<DWORD>(value >> 32) }, local{};
        SYSTEMTIME time{};
        if (!FileTimeToLocalFileTime(&file, &local) || !FileTimeToSystemTime(&local, &time)) return L"--";
        wchar_t text[64]{};
        swprintf_s(text, L"%04u-%02u-%02u %02u:%02u:%02u", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
        return text;
    }
    std::wstring Details(const NativeMonitoring::AdapterSnapshot* row, bool selected, bool chinese)
    {
        if (!row) return selected ? (NativePreferences::InlinePair(L"Adapter no longer present. Select another adapter.",L"所選網絡介面已不存在。請重新選擇。").GetString())
            : (NativePreferences::InlinePair(L"Select an adapter to inspect its observed details.",L"選擇網絡介面以查看實際資料。").GetString());
        std::wostringstream text;
        text << row->alias << L"\r\n" << row->description << L"\r\n\r\n"
            << (NativePreferences::InlinePair(L"State: ",L"狀態：").GetString()) << RateStateText(row->rateState, chinese) << L"\r\n"
            << (NativePreferences::InlinePair(L"Observed: ",L"觀察時間：").GetString()) << TimeText(row->acquiredAtUtcFileTime) << L"\r\n"
            << (NativePreferences::InlinePair(L"Interval: ",L"採樣間隔：").GetString()) << row->intervalSeconds << L" s\r\n"
            << (NativePreferences::InlinePair(L"Receiving: ",L"接收：").GetString()) << Rate(row->receivedBytesPerSecond, chinese) << L"\r\n"
            << (NativePreferences::InlinePair(L"Sending: ",L"傳送：").GetString()) << Rate(row->sentBytesPerSecond, chinese) << L"\r\n\r\n"
            << L"GUID: " << (row->hasPersistentIdentity ? row->persistentId : (NativePreferences::InlinePair(L"Unavailable; not persistent",L"不可持續識別").GetString())) << L"\r\n"
            << L"LUID: " << row->interfaceLuid << L"\r\n"
            << (NativePreferences::InlinePair(L"Receive link speed: ",L"連線接收速度：").GetString()) << row->receiveLinkBitsPerSecond << L" bit/s\r\n"
            << (NativePreferences::InlinePair(L"Transmit link speed: ",L"連線傳送速度：").GetString()) << row->transmitLinkBitsPerSecond << L" bit/s\r\n\r\n"
            << (NativePreferences::InlinePair(L"Addresses:\r\n",L"位址：\r\n").GetString());
        if (row->addressError) text << (NativePreferences::InlinePair(L"Address acquisition error ",L"未能取得位址，代碼 ").GetString()) << row->addressError << L"\r\n";
        else if (row->addresses.empty()) text << (NativePreferences::InlinePair(L"No observed addresses",L"沒有已觀察位址").GetString()) << L"\r\n";
        else for (const auto& address : row->addresses) text << address << L"\r\n";
        text << L"\r\n" << (NativePreferences::InlinePair(L"This view is observational. It does not change monitor scope or traffic accounting.",L"此視圖只作觀察，不會改動監察範圍或流量記錄。").GetString());
        return text.str();
    }
    namespace
    {
        void Text(NativeMaterial::Canvas& canvas, const std::wstring& value, float size,
            D2D1_RECT_F rectangle, const D2D1_COLOR_F& color)
        {
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
            if (SUCCEEDED(canvas.TextFormat(size, DWRITE_FONT_WEIGHT_NORMAL, format.GetAddressOf())))
                canvas.Target()->DrawText(value.c_str(), static_cast<UINT32>(value.size()), format.Get(), rectangle, canvas.Brush(color));
        }
    }
    void DrawData(NativeMaterial::Canvas& canvas, const NativeMaterial::Palette& colors,
        float width, float metricsTop, float chartTop, float chartBottom, const NativeMonitoring::AdapterSnapshot* row, bool chinese)
    {
        auto* target = canvas.Target();
        const float left=0,right=width,third=width/3;
        for(int i=0;i<3;++i)
        {
            const float x=i*third;
            if(i)target->DrawLine(D2D1::Point2F(x-12,metricsTop+8),D2D1::Point2F(x-12,metricsTop+88),canvas.Brush(colors.outline));
            const auto label=i==0?NativePreferences::Pair(L"Receiving",L"接收"):
                (i==1?NativePreferences::Pair(L"Sending",L"傳送"):NativePreferences::Pair(L"Sample interval",L"採樣間隔"));
            Text(canvas,label.GetString(),14,D2D1::RectF(x,metricsTop+4,x+third-20,metricsTop+44),colors.secondaryText);
            CString value;
            if(i<2)value=Rate(row?(i?row->sentBytesPerSecond:row->receivedBytesPerSecond):std::optional<double>{},chinese).c_str();
            else if(row)value.Format(L"%.2f s",row->intervalSeconds);
            else value=L"--";
            Text(canvas,value.GetString(),24,D2D1::RectF(x,metricsTop+48,x+third-20,metricsTop+94),colors.onSurface);
        }
        target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, chartTop, right, chartBottom), 16, 16), canvas.Brush(colors.container));
        Text(canvas, NativePreferences::InlinePair(L"Selected adapter samples · receive solid / send dashed",L"所選介面樣本 · 接收實線 / 傳送短線").GetString(), 14,
            D2D1::RectF(left + 16, chartTop + 10, right - 16, chartTop + 46), colors.secondaryText);
        if (!row || row->history.size() < 2)
        {
            Text(canvas, NativePreferences::InlinePair(L"Waiting for valid samples",L"正在等待有效樣本").GetString(), 16,
                D2D1::RectF(left + 16, chartTop + 56, right - 16, chartBottom - 10), colors.secondaryText);
            return;
        }
        double maximum = 1;
        for (const auto& point : row->history)
            for (const auto& rate : {point.receivedBytesPerSecond, point.sentBytesPerSecond})
                if (rate && std::isfinite(*rate)) maximum = (std::max)(maximum, *rate);
        const float x0 = left + 16, x1 = right - 16, y0 = chartTop + 56, y1 = chartBottom - 22;
        target->DrawLine(D2D1::Point2F(x0, y1), D2D1::Point2F(x1, y1), canvas.Brush(colors.outline));
        const auto first = row->history.front().monotonicMilliseconds, last = row->history.back().monotonicMilliseconds;
        if (last <= first) return;
        for (std::size_t i = 1; i < row->history.size(); ++i)
        {
            const auto& a = row->history[i - 1]; const auto& b = row->history[i];
            if (a.sampleEpoch != b.sampleEpoch || a.state != NativeMonitoring::RateState::Available || b.state != NativeMonitoring::RateState::Available) continue;
            for (int series = 0; series < 2; ++series)
            {
                const auto av = series ? a.sentBytesPerSecond : a.receivedBytesPerSecond;
                const auto bv = series ? b.sentBytesPerSecond : b.receivedBytesPerSecond;
                if (!av || !bv || !std::isfinite(*av) || !std::isfinite(*bv)) continue;
                const auto p0 = D2D1::Point2F(x0 + static_cast<float>(double(a.monotonicMilliseconds - first) / double(last - first)) * (x1 - x0), y1 - static_cast<float>(*av / maximum) * (y1 - y0));
                const auto p1 = D2D1::Point2F(x0 + static_cast<float>(double(b.monotonicMilliseconds - first) / double(last - first)) * (x1 - x0), y1 - static_cast<float>(*bv / maximum) * (y1 - y0));
                if (!series) target->DrawLine(p0, p1, canvas.Brush(colors.primary), 2);
                else
                {
                    // Draw short segments so color is not the only series distinction.
                    const float length = std::hypot(p1.x - p0.x, p1.y - p0.y);
                    const int pieces = (std::max)(1, static_cast<int>(length / 5));
                    for (int dash = 0; dash < pieces; dash += 2)
                    {
                        const float start = float(dash) / pieces, end = float((std::min)(dash + 1, pieces)) / pieces;
                        target->DrawLine(D2D1::Point2F(p0.x + (p1.x - p0.x) * start, p0.y + (p1.y - p0.y) * start),
                            D2D1::Point2F(p0.x + (p1.x - p0.x) * end, p0.y + (p1.y - p0.y) * end), canvas.Brush(colors.secondaryText), 2);
                    }
                }
            }
        }
    }
}
