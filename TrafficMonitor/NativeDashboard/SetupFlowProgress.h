#pragma once

namespace NativeDashboard::Setup
{
    enum class OperationResult { Pending, Applying, Applied, Failed };

    // An acknowledged result belongs to the draft choice, not to current global
    // settings. Other workflows may legitimately change those settings later.
    template<class Save, class ReadEffective, class Execute>
    bool RunOperation(OperationResult& result, Save save, ReadEffective readEffective, Execute execute)
    {
        if (result == OperationResult::Applied) return true;
        if (result == OperationResult::Applying && readEffective())
        {
            result = OperationResult::Applied;
            return save();
        }
        result = OperationResult::Applying;
        if (!save()) return false;
        result = execute() ? OperationResult::Applied : OperationResult::Failed;
        return save();
    }
}
