#pragma once

#include <functional>


class ScopeGuard
{
public:
    explicit ScopeGuard(std::function<void()> _onExitScope);

    ~ScopeGuard() noexcept;

    ScopeGuard(ScopeGuard&& other) = default;
    ScopeGuard& operator=(ScopeGuard&& other) = default;

    void commit();
    void dismiss();

private:
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    std::function<void()> onExitScope;
};
