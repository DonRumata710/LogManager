#include "FilteredLogModel.h"

#include <chrono>

FilteredLogModel::FilteredLogModel(LogService* logService, const LogFilter& filter, QObject* parent) :
    LogModel(logService, parent),
    filter(filter)
{}

void FilteredLogModel::fetchUpMore()
{
    LogModel::fetchUpMore(filter);
}

void FilteredLogModel::fetchDownMore()
{
    LogModel::fetchDownMore(filter);
}

const LogFilter& FilteredLogModel::getFilter() const
{
    return filter;
}

void FilteredLogModel::applyFilter(const LogFilter& newFilter)
{
    filter.apply(newFilter);
    goToTime(std::chrono::system_clock::time_point::min());
}

