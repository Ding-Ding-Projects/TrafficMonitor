#pragma once
#include <afxwin.h>
#include <array>
#include "../NativeMaterial/Controls.h"
#include "../NativeMaterial/InputControls.h"

namespace NativeSettings
{
    // Which Material components one settings row needs. Independent of the setting catalog so the
    // control set can be created and audited on its own.
    struct RowShape
    {
        bool toggle{};         // on/off setting: Switch
        bool number{};         // bounded whole number: NumberStepper
        bool trafficUnits{};   // MB/GB choice next to the number: SegmentedButton
        bool notification{};   // carries a line that explains an unavailable capability
        int minimum{}, maximum{}, step{ 1 };
    };

    // Child ids are firstId + the part offsets below. The parent receives
    //   toggle  -> BN_CLICKED, number -> EN_CHANGE (committed value), unit -> CBN_SELCHANGE, details -> BN_CLICKED.
    enum RowPart : UINT { PartToggle = 0, PartNumberLabel = 1, PartNumber = 2, PartUnit = 3, PartDetails = 4, PartDescription = 5, PartDefaults = 6, PartUnavailable = 7, PartCount = 8 };

    enum class RowChange { None, Toggle, Number, NumberRefused, Unit };

    // The controls of one row. No stock BUTTON, EDIT, COMBOBOX or spin window is created here; the only
    // EDIT inside a row is the text entry owned by the NumberStepper. Statics carry read-only text.
    class RowControls
    {
    public:
        bool Create(CWnd* parent, UINT firstId, const RowShape& shape, const CString& label, const CString& detailsLabel);
        void SetAppearance(NativeMaterial::Appearance appearance);

        // Draft transfer used by the settings editor: the controls never hold a value of their own.
        void ShowToggle(bool on, bool available);
        void ShowNumber(int value, bool available);
        void ShowTrafficUnit(int unit, bool available);   // 0 MB, 1 GB, anything else selects nothing
        bool ToggleValue() const { return toggle.IsChecked(); }
        int NumberValue() const { return number.GetValue(); }
        int TrafficUnitValue() const { return unit.GetSelected(); }

        // Moves one notification from a control into the draft the row edits. Only the destinations the row
        // owns are touched. A number that is not on the setting's step is turned back to the draft value
        // and reported as NumberRefused; the draft is never changed by a refused value.
        RowChange Receive(UINT part, UINT code, bool* toggleDraft, int* numberDraft, int* unitDraft);
        // Commits text typed into the stepper exactly as Enter does. False when the text is not a whole number
        // inside the range on the step; nothing is committed then.
        bool CommitNumber();

        NativeMaterial::Switch toggle;
        CStatic label, description, defaults, unavailable;
        NativeMaterial::NumberStepper number;
        NativeMaterial::SegmentedButton unit;
        NativeMaterial::Button details;
        std::array<CWnd*, 8> All() { return { &toggle, &label, &number, &unit, &details, &description, &defaults, &unavailable }; }
        RowShape shape;
    };
}
