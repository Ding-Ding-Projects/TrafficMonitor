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
        bool operator==(const SavedExpression& other) const;
    };
    enum class CollectionAction { Read, Save, Rename, Delete, Recover };
    struct CollectionResult
    {
        bool complete{};
        std::wstring error;
        std::vector<SavedExpression> records;
    };
    // Call on a worker. The directory is the application's existing data directory.
    CollectionResult UpdateCollection(const std::wstring& directory, CollectionAction,
        const SavedExpression& record = {}, const std::wstring& newName = L"");
}
