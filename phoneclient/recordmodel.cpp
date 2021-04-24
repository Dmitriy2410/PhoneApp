#include "recordmodel.h"

RecordModel::RecordModel(QObject *parent) :
	QAbstractTableModel(parent)
{

}

int RecordModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_records.size();
}

int RecordModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return 3;
}

QVariant RecordModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case 0:
				return tr("ФИО");
			case 1:
				return tr("Пол");
			case 2:
				return tr("Телефон");
			}
		} else if (orientation == Qt::Vertical) {
			return section + 1;
		}
	}
	return QVariant();
}

QVariant RecordModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}
	Record rec = m_records[index.row()];
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case 0:
			return QString("%1 %2 %3").arg(rec.surname, rec.name, rec.secondName);
		case 1:
			return rec.gender == 0 ? tr("М") : tr("Ж");
		case 2:
			return rec.phoneNumber;
		}
	} else if (role == Qt::TextAlignmentRole) {
		if (index.column() == 1) {
			return Qt::AlignCenter;
		} else {
			return int(Qt::AlignLeft | Qt::AlignVCenter);
		}
	} else if (role == Qt::UserRole) {
		return rec.id;
	}
	return QVariant();
}

QList<Record> RecordModel::records() const
{
	return m_records;
}

void RecordModel::setRecords(const QList<Record> &records)
{
	beginResetModel();
	m_records = records;
	endResetModel();
}
