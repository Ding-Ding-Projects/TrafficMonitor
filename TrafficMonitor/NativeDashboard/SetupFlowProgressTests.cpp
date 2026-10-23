#include "SetupFlowProgress.h"
#include <array>
#include <cassert>
#include <cstdio>

using NativeDashboard::Setup::OperationResult;
using NativeDashboard::Setup::RunOperation;

int main()
{
    using R = OperationResult;
    std::array<R, 3> draft{R::Applied, R::Applied, R::Failed};
    // Save/close retains per-operation results. A legacy workflow subsequently
    // changes the adapter and taskbar. Retrying startup must not undo either.
    const auto persisted = draft;
    draft = persisted;
    std::array<int, 3> effective{8, 0, 0}, oldChoice{2, 1, 1}, writes{};
    int saves = 0;
    for (int i = 0; i < 3; ++i)
        assert(RunOperation(draft[i], [&]{++saves; return true;},
            [&]{return effective[i] == oldChoice[i];},
            [&]{++writes[i]; effective[i] = oldChoice[i]; return true;}));
    assert(effective[0] == 8 && effective[1] == 0 && effective[2] == 1);
    assert(writes[0] == 0 && writes[1] == 0 && writes[2] == 1 && saves == 2);
    // Changing this draft's taskbar choice invalidates only that operation.
    draft[1] = R::Pending;
    assert(RunOperation(draft[1], []{return true;}, []{return false;}, [&]{++writes[1]; return true;}));
    assert(writes[0] == 0 && writes[1] == 1 && writes[2] == 1);
    // Crash after effect but before acknowledgement: confirm effective state
    // before touching the real consumer, then persist the recovered result.
    R interrupted = R::Applying;
    int reads = 0, effects = 0;
    assert(RunOperation(interrupted, []{return true;}, [&]{++reads; return true;}, [&]{++effects; return true;}));
    assert(interrupted == R::Applied && reads == 1 && effects == 0);
    // Persisting intent is required before a new effect may occur.
    R pending = R::Pending;
    assert(!RunOperation(pending, []{return false;}, []{return false;}, [&]{++effects; return true;}));
    assert(effects == 0);
    // Failed consumers remain incomplete and retryable.
    pending = R::Pending;
    assert(RunOperation(pending, []{return true;}, []{return false;}, []{return false;}));
    assert(pending == R::Failed);
    std::puts("PASS: retained partial results, intervening settings, interrupted apply, write barrier, failed retry");
}
