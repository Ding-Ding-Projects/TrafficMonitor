#pragma once
#include "Engine.h"

namespace NativeRegex
{
    struct SavedExpression
    {
        std::wstring name;
        Options options;
        Operation operation{Operation::Search};
        std::wstring replacement;
        // Optional note, at most 1024 UTF-16 units. Stored from collection schema 2 onward.
        std::wstring description;
        bool operator==(const SavedExpression& other) const;
    };
    enum class CollectionAction { Read, Save, Rename, Delete, Recover };
    struct CollectionResult
    {
        bool complete{};
        std::wstring error;
        std::vector<SavedExpression> records;
    };
    enum class ImportMode { Merge, Replace };
    // Call on a worker. The directory is the application's existing data directory.
    CollectionResult UpdateCollection(const std::wstring& directory, CollectionAction,
        const SavedExpression& record = {}, const std::wstring& newName = L"");
    // Writes the saved collection to an arbitrary file atomically. An empty collection is a valid export.
    CollectionResult ExportCollection(const std::wstring& directory, const std::wstring& destinationPath);
    // All-or-nothing import. Merge skips identical names and rejects different content under a case-insensitive name match.
    CollectionResult ImportCollection(const std::wstring& directory, const std::wstring& sourcePath, ImportMode mode);
}
