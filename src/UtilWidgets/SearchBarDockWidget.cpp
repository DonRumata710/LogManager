#include "SearchBarDockWidget.h"

#include "SearchBar.h"


SearchBarDockWidget::SearchBarDockWidget(QWidget* parent) :
    QDockWidget(parent),
    searchBar(new SearchBar(this))
{
    setWidget(searchBar);

    setAllowedAreas(Qt::DockWidgetArea::TopDockWidgetArea | Qt::DockWidgetArea::BottomDockWidgetArea);
}

SearchBar* SearchBarDockWidget::getSearchBar() const
{
    return searchBar;
}
