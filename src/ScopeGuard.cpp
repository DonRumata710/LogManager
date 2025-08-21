#include <ScopeGuard.h>

#include <QDebug>


ScopeGuard::ScopeGuard(std::function<void ()> _onExitScope) : onExitScope(_onExitScope)
{}

ScopeGuard::~ScopeGuard()
{
    try
    {
        onExitScope();
    }
    catch(const std::exception& e)
    {
        qWarning() << "Exception in ScopeGuard destructor:" << e.what();
    }
    catch(...)
    {
        qWarning() << "Unknown exception in ScopeGuard destructor";
    }
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
