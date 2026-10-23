// Emit the accessibility GUID definitions directly from the installed Windows SDK.
// This translation unit intentionally does not use the application PCH, which
// already includes oleacc.h in declaration-only mode.
#include <Windows.h>
#include <initguid.h>
#include <oleacc.h>
