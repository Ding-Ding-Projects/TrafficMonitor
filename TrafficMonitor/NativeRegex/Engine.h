#pragma once
#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace NativeRegex
{
    struct Options { std::wstring query; bool regex{}, ignoreCase{true}; };
    struct Span { size_t start{}, length{}; };
    struct Result
    {
        bool complete{};
        std::wstring error, output;
        std::vector<bool> rows;
        std::vector<Span> matches;
    };
    enum class Operation { Filter, Search, Replace, Split };
    // One worker per owner. Cancellation replaces pending input, never spawns a fleet.
    class Session final
    {
    public:
        ~Session();
        void Request(const Options&, std::vector<std::wstring>, Operation = Operation::Filter, std::wstring replacement = L"");
        bool Poll(Result&);
        void Cancel();
        bool Busy() const { return m_pending || m_worker.valid(); }
    private:
        struct RequestData { Options options; std::vector<std::wstring> text; Operation operation{}; std::wstring replacement; unsigned long long generation{}; };
        void Start();
        unsigned long long m_generation{},m_running{};
        std::unique_ptr<RequestData> m_pending;
        std::shared_ptr<std::atomic_bool> m_cancel;
        std::future<Result> m_worker;
    };
    class Filter final
    {
    public:
        Options options;
        std::wstring status;
        void Request(const std::vector<std::wstring>& text);
        bool Poll();
        bool Visible(size_t row) const;
        bool Busy() const { return m_session.Busy(); }
    private:
        Options m_requested;
        std::vector<std::wstring> m_text,m_acceptedText;
        bool m_initialized{};
        std::vector<bool> m_rows;
        Session m_session;
    };
}
