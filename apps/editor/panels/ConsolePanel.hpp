#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <streambuf>
#include <string>

namespace rubia::editor
{

/// Thread-safe bounded text storage used by the editor console.
class ConsoleLogBuffer final
{
public:
    void append(const char* text, std::size_t size);
    void clear();
    [[nodiscard]] std::string snapshot(uint64_t& revision) const;

private:
    static constexpr std::size_t kMaximumCharacters = 512u * 1024u;

    mutable std::mutex mutex_;
    std::string text_;
    uint64_t revision_ = 0;
};

/// Mirrors one standard stream to ConsoleLogBuffer without hiding terminal
/// output.
class ConsoleTeeStreamBuffer final : public std::streambuf
{
public:
    ConsoleTeeStreamBuffer(
        std::streambuf* destination,
        ConsoleLogBuffer& log) noexcept;

protected:
    int_type overflow(int_type value) override;
    std::streamsize xsputn(
        const char* text,
        std::streamsize size) override;
    int sync() override;

private:
    std::streambuf* destination_ = nullptr;
    ConsoleLogBuffer* log_ = nullptr;
};

/// Owns the Console dock window and the lifetime of stdout/stderr capture.
class ConsolePanel final
{
public:
    ConsolePanel();
    ~ConsolePanel();

    ConsolePanel(const ConsolePanel&) = delete;
    ConsolePanel& operator=(const ConsolePanel&) = delete;

    void draw(bool* open = nullptr);

private:
    ConsoleLogBuffer log_;
    ConsoleTeeStreamBuffer coutCapture_;
    ConsoleTeeStreamBuffer clogCapture_;
    ConsoleTeeStreamBuffer cerrCapture_;
    std::streambuf* previousCout_ = nullptr;
    std::streambuf* previousClog_ = nullptr;
    std::streambuf* previousCerr_ = nullptr;
    std::string cachedText_;
    uint64_t cachedRevision_ = 0;
    bool autoScroll_ = true;
};

} // namespace rubia::editor
