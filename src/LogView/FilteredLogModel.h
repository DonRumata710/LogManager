#pragma once

#include "LogModel.h"
#include "../LogFilter.h"

class FilteredLogModel : public LogModel
{
    Q_OBJECT
public:
    explicit FilteredLogModel(LogService* logService, const LogFilter& filter, QObject* parent = nullptr);

    void fetchUpMore() override;
    void fetchDownMore() override;

    const LogFilter& getFilter() const;
    void setFilter(const LogFilter& newFilter);

private:
    LogFilter filter;
};

