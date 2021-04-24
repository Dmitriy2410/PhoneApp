#ifndef DATAJSON_H
#define DATAJSON_H

#include <QVector>
#include "../global.h"
#include <QHash>

class DataJson
{
public:
	DataJson();

	bool				load();
	bool				save();

	QHash<int, Record>	records() const;
	void				setRecord(int id, const Record &rec);
	void				addRecord(const Record &rec);
	void				rmRecord(int id);
	void				setFileName(const QString &fileName);
	qint64				lastModified();

private:
	QHash<int, Record>	m_records;
	QString				m_fileName;
};

#endif // DATAJSON_H
