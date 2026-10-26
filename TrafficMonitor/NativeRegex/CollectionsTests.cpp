#include "../stdafx.h"
#include "Collections.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

using namespace NativeRegex;
namespace
{
    int failures = 0, checks = 0;
    void Check(bool ok, const char* name) { ++checks; if (!ok) { ++failures; std::printf("FAIL %s\n", name); } }
    using Bytes = std::string;
    Bytes Slurp(const std::wstring& path)
    {
        std::ifstream in(path, std::ios::binary);
        return Bytes(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    void Spit(const std::wstring& path, const Bytes& bytes)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    SavedExpression Item(const std::wstring& name, const std::wstring& query, bool regex = true, const std::wstring& replacement = L"")
    {
        SavedExpression r; r.name = name; r.options.query = query; r.options.regex = regex; r.options.ignoreCase = false;
        r.operation = replacement.empty() ? Operation::Search : Operation::Replace; r.replacement = replacement; return r;
    }
    std::wstring MakeDirectory(const std::wstring& root, const wchar_t* leaf)
    {
        auto d = root + L"\\" + leaf; CreateDirectoryW(d.c_str(), nullptr); return d;
    }
    bool SaveAll(const std::wstring& dir, const std::vector<SavedExpression>& items)
    {
        for (const auto& i : items) if (!UpdateCollection(dir, CollectionAction::Save, i).complete) return false;
        return true;
    }
    std::wstring Primary(const std::wstring& dir) { return dir + L"\\regex-collections.dat"; }
    bool Contains(const std::wstring& text, const wchar_t* part) { return text.find(part) != std::wstring::npos; }
    void PutNumber(Bytes& b, size_t offset, unsigned n) { for (int i = 0; i < 4; ++i) b[offset + i] = static_cast<char>(n >> (i * 8)); }
    // Builds a file image by hand so malformed or oversized content can be injected.
    Bytes Image(const std::vector<std::wstring>& names)
    {
        Bytes b; auto put = [&](unsigned n) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<char>(n >> (i * 8))); };
        auto text = [&](const std::wstring& s) { put(static_cast<unsigned>(s.size())); for (wchar_t c : s) { b.push_back(static_cast<char>(c)); b.push_back(static_cast<char>(c >> 8)); } };
        put(0x58524d54); put(0x314c4f43); put(1); put(1); put(static_cast<unsigned>(names.size()));
        for (const auto& n : names) { text(n); text(L"a"); text(L""); put(1); put(1); }
        return b;
    }
}

int wmain()
{
    wchar_t temp[MAX_PATH]{}; GetTempPathW(MAX_PATH, temp);
    const std::wstring root = std::wstring(temp) + L"TmRegexCollections-" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(root.c_str(), nullptr);
    const auto source = MakeDirectory(root, L"source"), target = MakeDirectory(root, L"target");
    const auto file = root + L"\\export.bin";
    const std::vector<SavedExpression> base{Item(L"Alpha", L"a+"), Item(L"Beta", L"b", false, L"B")};

    // Round trip: export then import into an empty collection preserves every record.
    Check(SaveAll(source, base), "seed source");
    auto exported = ExportCollection(source, file);
    Check(exported.complete && exported.records == base, "export returns the saved records");
    auto imported = ImportCollection(target, file, ImportMode::Merge);
    Check(imported.complete && imported.records == base, "round trip equality (merge into empty)");
    Check(UpdateCollection(target, CollectionAction::Read).records == base, "round trip persisted on disk");
    Check(Slurp(file) == Slurp(Primary(source)), "export bytes match the saved collection bytes");

    // Empty export is a valid file and imports as an empty collection.
    const auto emptyDir = MakeDirectory(root, L"empty"), emptyFile = root + L"\\empty.bin";
    auto emptyExport = ExportCollection(emptyDir, emptyFile);
    Check(emptyExport.complete && emptyExport.records.empty() && !Slurp(emptyFile).empty(), "empty export produces a valid file");
    Check(ImportCollection(MakeDirectory(root, L"fromEmpty"), emptyFile, ImportMode::Replace).complete, "empty export imports");

    // Merge: identical duplicate (case-insensitive name) skipped, new record added.
    const auto mergeDir = MakeDirectory(root, L"merge");
    Check(SaveAll(mergeDir, {Item(L"alpha", L"a+")}), "seed merge");
    Check(SaveAll(MakeDirectory(root, L"mergeSrcA"), {Item(L"ALPHA", L"a+"), Item(L"Gamma", L"g")}) && ExportCollection(root + L"\\mergeSrcA", root + L"\\a.bin").complete, "seed identical-name source");
    auto skipped = ImportCollection(mergeDir, root + L"\\a.bin", ImportMode::Merge);
    Check(!skipped.complete && Contains(skipped.error, L"ALPHA"), "case-insensitive name with different case text is a conflict, not a skip");
    Check(SaveAll(MakeDirectory(root, L"mergeSrcC"), {Item(L"alpha", L"a+"), Item(L"Gamma", L"g")}) && ExportCollection(root + L"\\mergeSrcC", root + L"\\c.bin").complete, "seed identical source");
    auto merged = ImportCollection(mergeDir, root + L"\\c.bin", ImportMode::Merge);
    Check(merged.complete && merged.records.size() == 2 && merged.records[0].name == L"alpha" && merged.records[1].name == L"Gamma", "merge skips identical and adds new");

    // Merge conflict: same name with different content rejected, file bytes unchanged.
    Check(SaveAll(MakeDirectory(root, L"mergeSrcB"), {Item(L"Fresh", L"f"), Item(L"Alpha", L"different")}) && ExportCollection(root + L"\\mergeSrcB", root + L"\\b.bin").complete, "seed conflicting source");
    const auto before = Slurp(Primary(mergeDir));
    auto conflict = ImportCollection(mergeDir, root + L"\\b.bin", ImportMode::Merge);
    Check(!conflict.complete && Contains(conflict.error, L"Alpha") && conflict.records.empty(), "merge conflict rejected and names the record");
    Check(Slurp(Primary(mergeDir)) == before, "merge conflict leaves the file bytes unchanged");

    // Replace makes the imported set the collection.
    auto replaced = ImportCollection(mergeDir, root + L"\\b.bin", ImportMode::Replace);
    Check(replaced.complete && replaced.records.size() == 2 && replaced.records[0].name == L"Fresh", "replace adopts the imported set");
    Check(UpdateCollection(mergeDir, CollectionAction::Read).records.size() == 2, "replace persisted");

    // Recover returns the pre-import set after an import.
    const auto recoverDir = MakeDirectory(root, L"recover");
    Check(SaveAll(recoverDir, {Item(L"Old", L"o")}), "seed recover");
    Check(ImportCollection(recoverDir, file, ImportMode::Replace).complete, "replace import for recovery");
    auto recovered = UpdateCollection(recoverDir, CollectionAction::Recover);
    Check(recovered.complete && recovered.records.size() == 1 && recovered.records[0].name == L"Old", "Recover returns the pre-import set");

    // Rejections that must not change anything.
    const auto badDir = MakeDirectory(root, L"bad");
    Check(SaveAll(badDir, {Item(L"Keep", L"k")}), "seed bad");
    const auto keep = Slurp(Primary(badDir));
    auto expectRejected = [&](const Bytes& image, const wchar_t* needle, const char* name, ImportMode mode)
    {
        const auto path = root + L"\\bad.bin"; Spit(path, image);
        auto r = ImportCollection(badDir, path, mode);
        Check(!r.complete && Contains(r.error, needle) && Slurp(Primary(badDir)) == keep, name);
    };
    const auto good = Slurp(file);
    expectRejected(good.substr(0, good.size() - 3), L"truncated", "truncated file rejected", ImportMode::Merge);
    { auto b = good; PutNumber(b, 0, 0x12345678); expectRejected(b, L"Not a native", "wrong magic rejected", ImportMode::Replace); }
    { auto b = good; PutNumber(b, 12, 2); expectRejected(b, L"Unsupported engine/version", "wrong engine version rejected", ImportMode::Replace); }
    { auto b = good; PutNumber(b, 8, 2); expectRejected(b, L"schema", "wrong schema rejected", ImportMode::Merge); }
    expectRejected(Image({L"Same", L"SAME"}), L"duplicate", "in-file duplicate names rejected", ImportMode::Merge);
    expectRejected(Image({L"Lone\xd800"}), L"UTF-16", "invalid surrogate rejected", ImportMode::Replace);
    {
        std::vector<std::wstring> many; for (int i = 0; i < 64; ++i) many.push_back(L"N" + std::to_wstring(i));
        expectRejected(Image(many), L"exceed 64", "merged result over 64 records rejected", ImportMode::Merge);
        auto r = ImportCollection(badDir, root + L"\\bad.bin", ImportMode::Replace);
        Check(r.complete && r.records.size() == 64, "exactly 64 records replace successfully");
    }
    Check(!ImportCollection(badDir, root + L"\\missing.bin", ImportMode::Merge).complete, "missing source rejected");

    // Export to an existing destination replaces it.
    Spit(file, "stale");
    Check(ExportCollection(source, file).complete && Slurp(file) == Slurp(Primary(source)), "export replaces an existing destination");

    std::printf("%d checks, %d failures\n", checks, failures);
    wchar_t from[MAX_PATH + 2]{}; wcscpy_s(from, root.c_str());
    SHFILEOPSTRUCTW op{}; op.wFunc = FO_DELETE; op.pFrom = from; op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT; SHFileOperationW(&op);
    return failures ? 1 : 0;
}
