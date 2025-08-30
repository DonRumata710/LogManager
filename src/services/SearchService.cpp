#include "SearchService.h"
#include "SessionService.h"
#include "SearchController.h"
#include "Utils.h"

SearchService::SearchService(SessionService* sessionService, QObject* parent)
    : QObject(parent), sessionService(sessionService)
{}

void SearchService::search(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool regexEnabled, bool backward, bool findAll, int column, const QStringList& fields)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), 0);

    auto session = sessionService->getSession();
    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    if (searchTerm.isEmpty())
    {
        qWarning() << "Search term is empty.";
        return;
    }

    auto startTime = time;
    auto endTime = session->getMaxTime();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    auto iterator = LogEntryIterator<>(session->getIterator<true>(startTime, endTime));
    int lastPercent = 0;
    QMap<std::chrono::system_clock::time_point, QString> foundEntries;
    while(iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        auto curMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startTime).count();
        int percent = totalMs ? static_cast<int>(100LL * curMs / totalMs) : 0;
        if (percent != lastPercent && percent <= 100)
        {
            emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), percent);
            lastPercent = percent;
        }

        bool specificColumn = column >= 0 && column < entry->line.size();
        bool lastColumn = specificColumn && column == fields.size() - 1;
        QString textToSearch = (specificColumn ? entry->values[fields[column]].toString() : (lastColumn ? entry->values[fields[column]].toString() + entry->additionalLines : entry->line));

        if (SearchController::checkEntry(entry->line, searchTerm, regexEnabled))
        {
            if (findAll)
            {
                foundEntries.insert(entry->time, entry->line);
            }
            else
            {
                emit searchFinished(searchTerm, entry->time);
                emit progressUpdated(QStringLiteral("Search finished"), 100);
                return;
            }
        }
    }

    emit progressUpdated(QStringLiteral("Search finished"), 100);

    if (findAll)
        emit searchResults(foundEntries);

    QT_SLOT_END
}

void SearchService::searchWithFilter(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool regexEnabled, bool backward, bool findAll, int column, const QStringList& fields, const LogFilter& filter)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), 0);

    auto session = sessionService->getSession();
    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    if (searchTerm.isEmpty())
    {
        qWarning() << "Search term is empty.";
        return;
    }

    auto startTimeF = time;
    auto endTimeF = session->getMaxTime();
    auto totalMsF = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeF - startTimeF).count();
    auto iterator = LogEntryIterator<>(session->getIterator<true>(startTimeF, endTimeF));
    int lastPercentF = 0;
    QMap<std::chrono::system_clock::time_point, QString> foundEntries;
    while(iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        auto curMsF = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startTimeF).count();
        int percentF = totalMsF ? static_cast<int>(100LL * curMsF / totalMsF) : 0;
        if (percentF != lastPercentF && percentF <= 100)
        {
            emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), percentF);
            lastPercentF = percentF;
        }

        bool specificColumn = column >= 0 && column < entry->line.size();
        bool lastColumn = specificColumn && column == fields.size() - 1;
        QString textToSearch = (specificColumn ? entry->values[fields[column]].toString() : (lastColumn ? entry->values[fields[column]].toString() + entry->additionalLines : entry->line));

        if (SearchController::checkEntry(entry->line, searchTerm, regexEnabled) && filter.check(entry.value()))
        {
            if (findAll)
            {
                foundEntries.insert(entry->time, entry->line);
            }
            else
            {
                emit searchFinished(searchTerm, entry->time);
                emit progressUpdated(QStringLiteral("Search finished"), 100);
                return;
            }
        }
    }

    emit progressUpdated(QStringLiteral("Search finished"), 100);

    if (findAll)
        emit searchResults(foundEntries);

    QT_SLOT_END
}
