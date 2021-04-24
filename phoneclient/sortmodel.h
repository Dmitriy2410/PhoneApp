#ifndef SORTMODEL_H
#define SORTMODEL_H

#include <QSortFilterProxyModel>

class SortModel : public QSortFilterProxyModel
{
public:
	SortModel(QObject *parent = nullptr);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // SORTMODEL_H
