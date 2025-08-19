#pragma once

#include "LogModel.h"
#include "../LogFilter.h"


class FilteredLogModel : public LogModel
{
    Q_OBJECT

public:
    explicit FilteredLogModel(SessionService* sessionService, const LogFilter& filter, QObject* parent = nullptr);

    void fetchUpMore() override;
    void fetchDownMore() override;

    const LogFilter& getFilter() const;
    void applyFilter(const LogFilter& newFilter);

private:
    LogFilter filter;
};

