#include "sortmodel.h"

SortModel::SortModel(QObject *parent) :
	QSortFilterProxyModel(parent)
{

}

QVariant SortModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical) {
		if (role == Qt::DisplayRole) {
			return section + 1;
		}
	}
	return sourceModel()->headerData(section, orientation, role);
}

