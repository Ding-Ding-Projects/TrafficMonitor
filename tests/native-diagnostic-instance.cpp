// Standalone check for the diagnostic single-instance mutex-name decision (DiagnosticInstance.h).
#include "DiagnosticInstance.h"
#include <iostream>

using namespace DiagnosticInstance;

static int failures{};
static void Expect(bool value, const char* name)
{
    std::cout << (value ? "PASS " : "FAIL ") << name << std::endl;
    if (!value) ++failures;
}

int main()
{
    const std::wstring normal = L"TrafficMonitor-1419J3XLKL1w8OZc";
    const std::wstring a = L"C:\\fixtures\\a\\TrafficMonitor.exe", b = L"C:\\fixtures\\b\\TrafficMonitor.exe";
    const bool file = IsRegularFileAttributes(0x80), directory = IsRegularFileAttributes(0x10);
    Expect(file, "normal file attributes accepted as marker");
    Expect(!directory, "directory marker rejected");
    Expect(!IsRegularFileAttributes(0xFFFFFFFFul), "missing marker rejected");
    Expect(!IsRegularFileAttributes(0x400), "reparse point marker rejected");
    Expect(MutexName(normal, a, false, false) == normal, "no marker keeps the normal name (portable)");
    Expect(MutexName(normal, a, false, true) == normal, "no marker keeps the normal name (installed)");
    Expect(MutexName(normal, a, true, true) == normal, "marker in installed mode keeps the normal name");
    const std::wstring isolated = MutexName(normal, a, true, false);
    Expect(isolated != normal, "marker in portable mode isolates the name");
    Expect(isolated.compare(0, normal.size(), normal) == 0, "isolated name extends the normal name");
    Expect(isolated == MutexName(normal, a, true, false), "isolated name is stable for the same path");
    Expect(isolated != MutexName(normal, b, true, false), "isolated name differs for another path");
    Expect(MutexName(normal, a, directory, false) == normal, "directory marker keeps the normal name");
    Expect(PathHash(a).size() == 16, "path hash is 16 hex digits");
    return failures ? 1 : 0;
}