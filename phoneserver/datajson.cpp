#include "datajson.h"
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

DataJson::DataJson()
{

}

bool DataJson::load()
{
	m_records.clear();
	QFile f(m_fileName);
	if (!f.open(QIODevice::ReadOnly)) {
		return false;
	}
	QByteArray ba = f.readAll();
	QJsonArray arr = QJsonDocument::fromJson(ba).array();
	for (int i = 0; i < arr.size(); ++i) {
		QJsonObject obj = arr[i].toObject();
		int id = obj["id"].toInt();
		m_records[id] = obj;
	}
	return true;
}

bool DataJson::save()
{
	QFile f(m_fileName);
	if (!f.open(QIODevice::WriteOnly)) {
		return false;
	}
	QJsonArray arr;
	QList<QJsonObject> recs = m_records.values();
	for (int i = 0; i < recs.size(); ++i) {
		arr.append(recs[i]);
	}
	QJsonDocument doc;
	doc.setArray(arr);
	if (f.write(doc.toJson()) == 0) {
		return false;
	}
	return true;
}

QHash<int, QJsonObject> DataJson::records() const
{
	return m_records;
}

void DataJson::setRecord(int id, const QJsonObject &rec)
{
	if (id == -1) {
		return;
	}
	QJsonObject nRec = rec;
	nRec["id"] = id;
	m_records[id] = nRec;
	qDebug()<<"Set record - id:"<<id;
}

void DataJson::addRecord(const QJsonObject &rec)
{
	QJsonObject nRec = rec;
	int newId = 0;
	QList<int> keys = m_records.keys();
	while (keys.contains(newId)) {
		++newId;
	}
	nRec["id"] = newId;
	m_records[newId] = nRec;
	QString out = QString("Add record - id = %1, records count = %2")
			.arg(newId)
			.arg(m_records.size());
	qDebug()<<out.toStdString().c_str();
}

void DataJson::rmRecord(int id)
{
	m_records.remove(id);
	QString out = QString("Remove record - id = %1, records count = %2")
			.arg(id)
			.arg(m_records.size());
	qDebug()<<out.toStdString().c_str();
}

void DataJson::setFileName(const QString &fileName)
{
	m_fileName = fileName;
}

qint64 DataJson::lastModified()
{
	QFile datFile(m_fileName);
	if (!datFile.exists()) {
		return 0;
	} else {
		return QFileInfo(m_fileName).lastModified().toUTC().toSecsSinceEpoch();
	}
}
