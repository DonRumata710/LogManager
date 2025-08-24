#pragma once

#include <QComboBox>
#include <QListWidget>
#include <QAction>

#include "LogFilterModel.h"


class MultiSelectComboBox : public QComboBox
{
    Q_OBJECT

public:
    MultiSelectComboBox(QWidget* aParent = Q_NULLPTR);
    void addItem(const QString& aText, const QVariant& aUserData = QVariant());
    void addItems(const QStringList& aTexts);
    QStringList currentText();
    QVariantList currentData();
    int count() const;
    void hidePopup() override;
    void SetSearchBarPlaceHolderText(const QString& aPlaceHolderText);
    void SetPlaceHolderText(const QString& aPlaceHolderText);
    void ResetSelection();
    void setMode(FilterType mode);
    FilterType mode() const { return mMode; }

signals:
    void selectionChanged();
    void filterModeChanged(FilterType mode);

public slots:
    void clear();
    void setCurrentText(const QString& aText);
    void setCurrentText(const QStringList& aText);

protected:
    void wheelEvent(QWheelEvent* aWheelEvent) override;
    bool eventFilter(QObject* aObject, QEvent* aEvent) override;
    void keyPressEvent(QKeyEvent* aEvent) override;

private:
    void stateChanged(int aState);
    void onSearch(const QString& aSearchString);
    void itemClicked(int aIndex);
    void updateModeAction();

    QListWidget* mListWidget;
    QLineEdit* mLineEdit;
    QLineEdit* mSearchBar;
    QVariantList mCurrentData;
    QStringList mItems;
    QAction* mModeAction;
    FilterType mMode = FilterType::Whitelist;
};
