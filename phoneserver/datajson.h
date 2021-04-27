#ifndef DATAJSON_H
#define DATAJSON_H

#include <QVector>
#include "../global.h"
#include <QHash>
#include "QJsonObject"

class DataJson
{
public:
	DataJson();

	bool				load();
	bool				save();

	QHash<int, QJsonObject>	records() const;
	void				setRecord(int id, const QJsonObject &rec);
	void				addRecord(const QJsonObject &rec);
	void				rmRecord(int id);
	void				setFileName(const QString &fileName);
	qint64				lastModified();

private:
	QHash<int, QJsonObject>	m_records;
	QString				m_fileName;
};

#endif // DATAJSON_H
