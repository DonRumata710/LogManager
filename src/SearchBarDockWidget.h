#pragma once

#include "SearchBar.h"

#include <QDockWidget>


class SearchBarDockWidget : public QDockWidget
{
public:
    SearchBarDockWidget(QWidget* parent = nullptr);

    SearchBar* getSearchBar() const;

private:
    SearchBar* searchBar;
};
