#ifndef RECORDMODEL_H
#define RECORDMODEL_H

#include "../global.h"
#include <QAbstractTableModel>

class RecordModel : public QAbstractTableModel
{
public:
	RecordModel(QObject *parent = nullptr);

	int			rowCount(const QModelIndex &parent) const;
	int			columnCount(const QModelIndex &parent) const;
	QVariant	headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant	data(const QModelIndex &index, int role) const;

	QList<Record> records() const;
	void		setRecords(const QList<Record> &records);


private:
	QList<Record> m_records;
};

#endif // RECORDMODEL_H
