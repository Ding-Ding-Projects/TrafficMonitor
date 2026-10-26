#include "../stdafx.h"
#include "RowControls.h"
#include "DraftValidation.h"

namespace NativeSettings
{
    bool RowControls::Create(CWnd* parent, UINT firstId, const RowShape& rowShape, const CString& text, const CString& detailsLabel)
    {
        shape = rowShape;
        const CRect box(0, 0, 100, 48);
        const DWORD child = WS_CHILD | WS_VISIBLE;
        if (shape.toggle)
        {
            if (!toggle.Create(text, box, parent, firstId + PartToggle)) return false;
        }
        else if (!label.Create(text, child | SS_NOTIFY, box, parent, firstId + PartToggle)) return false;
        if (shape.number)
        {
            if (!number.Create(L"", box, parent, firstId + PartNumber)) return false;
            number.SetRange(shape.minimum, shape.maximum);
            number.SetStep(shape.step);
            if (shape.trafficUnits)
            {
                if (!unit.Create(box, parent, firstId + PartUnit)) return false;
                unit.SetSegments({ L"MB", L"GB" });
                unit.SetAccessibleName(L"MB / GB");
            }
        }
        if (!details.Create(detailsLabel, box, parent, firstId + PartDetails)) return false;
        details.SetVariant(NativeMaterial::ButtonVariant::Text);
        return description.Create(L"", child | SS_NOTIFY, box, parent, firstId + PartDescription)
            && defaults.Create(L"", child | SS_NOTIFY, box, parent, firstId + PartDefaults)
            && (!shape.notification || unavailable.Create(L"", child, box, parent, firstId + PartUnavailable));
    }

    void RowControls::SetAppearance(NativeMaterial::Appearance appearance)
    {
        if (toggle.GetSafeHwnd()) toggle.SetAppearance(appearance);
        if (number.GetSafeHwnd()) number.SetAppearance(appearance);
        if (unit.GetSafeHwnd()) unit.SetAppearance(appearance);
        details.SetAppearance(appearance);
    }

    void RowControls::ShowToggle(bool on, bool available)
    {
        if (!toggle.GetSafeHwnd()) return;
        toggle.SetChecked(on);
        toggle.EnableWindow(available);
    }

    void RowControls::ShowNumber(int value, bool available)
    {
        if (!number.GetSafeHwnd()) return;
        number.SetValue(value);
        number.EnableWindow(available);
    }

    void RowControls::ShowTrafficUnit(int selected, bool available)
    {
        if (!unit.GetSafeHwnd()) return;
        unit.SetSelected(selected == 0 || selected == 1 ? selected : -1);
        unit.EnableWindow(available);
    }

    RowChange RowControls::Receive(UINT part, UINT code, bool* toggleDraft, int* numberDraft, int* unitDraft)
    {
        if (part == PartToggle && code == BN_CLICKED && shape.toggle && toggleDraft)
        {
            *toggleDraft = toggle.IsChecked();
            return RowChange::Toggle;
        }
        if (part == PartNumber && code == EN_CHANGE && shape.number && numberDraft)
        {
            const int value = number.GetValue();
            int accepted = 0;
            if (!ParseNumber(std::to_wstring(value), shape.minimum, shape.maximum, shape.step, accepted))
            {
                number.SetValue(*numberDraft);
                return RowChange::NumberRefused;
            }
            *numberDraft = value;
            return RowChange::Number;
        }
        if (part == PartUnit && code == CBN_SELCHANGE && shape.trafficUnits && unitDraft)
        {
            *unitDraft = unit.GetSelected();
            return RowChange::Unit;
        }
        return RowChange::None;
    }

    bool RowControls::CommitNumber()
    {
        if (!shape.number || !number.GetSafeHwnd()) return false;
        CString text;
        if (auto* entry = CWnd::FromHandle(number.EditWindow())) entry->GetWindowText(text);
        int typed = 0;
        return ParseNumber(text.GetString(), shape.minimum, shape.maximum, shape.step, typed) && number.CommitText();
    }
}
