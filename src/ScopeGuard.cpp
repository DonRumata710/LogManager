#include <ScopeGuard.h>


ScopeGuard::ScopeGuard(std::function<void ()> _onExitScope) : onExitScope(_onExitScope)
{}

ScopeGuard::~ScopeGuard()
{
    onExitScope();
}

void ScopeGuard::commit()
{
    onExitScope();
    onExitScope = []{};
}

void ScopeGuard::dismiss()
{
    onExitScope = []{};
}
