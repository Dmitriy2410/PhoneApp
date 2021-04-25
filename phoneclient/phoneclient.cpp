#include "phoneclient.h"
#include <QBuffer>
#include <QTcpSocket>
#include <QTimer>
#include "../global.h"

const int gc_interval = 1000;

PhoneClient::PhoneClient()
{
	m_timer.setInterval(gc_interval);
	m_timer.setSingleShot(true);
	connect(&m_socket, SIGNAL(connected()),
			this, SLOT(slotConnected()));
	connect(&m_socket, SIGNAL(disconnected()),
			this, SIGNAL(sigDisconnected()));
	connect(&m_socket, SIGNAL(readyRead()),
			this, SLOT(slotReadyRead()));
	connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(slotErrorHandler(QAbstractSocket::SocketError)));
	connect(&m_timer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()));
	m_lastMod = 0;
	QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
	QHostAddress addr = QHostAddress::LocalHost;
	for (int i = 0; i < addrs.size(); ++i) {
		if (addrs[i] != QHostAddress::LocalHost) {
			if (addrs[i].toIPv4Address()) {
				addr = addrs[i];
				break;
			}
		}
	}
	m_addr = addr.toString();
	m_port = 1234;
	m_socket.connectToHost(m_addr, m_port);
	m_state = WaitRequest;
	m_waitState = WaitSize;
}

void PhoneClient::getState()
{
	QJsonObject obj;
	obj["cmd"] = "GetState";
	send(obj, QByteArray());
}

void PhoneClient::getData()
{
	QJsonObject obj;
	obj["cmd"] = "GetData";
	send(obj, QByteArray());
}

void PhoneClient::addRecord(const Record &rec)
{
	if (m_state != Ready) {
		emit sigShowMessage(tr("Ошибка соединения"));
		return;
	}
	QJsonObject obj;
	obj["cmd"] = "AddRecord";
	QByteArray ba;
	ba.resize(sizeof(rec));
	QBuffer buf(&ba);
	buf.open(QIODevice::WriteOnly);
	if (buf.write((const char*)(&rec), sizeof(rec)) == 0) {
		return;
	}
	buf.close();
	obj["blobSize"] = ba.size();
	send(obj, ba);
}

void PhoneClient::setRecord(int id, const Record &rec)
{
	if (m_state != Ready) {
		emit sigShowMessage(tr("Ошибка соединения"));
		return;
	}
	QJsonObject obj;
	obj["cmd"] = "SetRecord";
	obj["recId"] = id;
	QByteArray ba;
	ba.resize(sizeof(rec));
	QBuffer buf(&ba);
	buf.open(QIODevice::WriteOnly);
	if (buf.write((const char*)(&rec), sizeof(rec)) == 0) {
		return;
	}
	buf.close();
	obj["blobSize"] = ba.size();
	send(obj, ba);
}

void PhoneClient::rmRecord(const QList<int> &ids)
{
	if (m_state != Ready) {
		emit sigShowMessage(tr("Ошибка соединения"));
		return;
	}
	QJsonObject obj;
	obj["cmd"] = "RmRecord";
	QJsonArray arr;
	for (int i = 0; i < ids.size(); ++i) {
		arr.append(ids[i]);
	}
	obj["recIds"] = arr;
	send(obj, QByteArray());
}


void PhoneClient::send(const QJsonObject &obj, const QByteArray &blob)
{
	QByteArray jsonData = QJsonDocument(obj).toBinaryData();
	qint32 jsonSize = jsonData.size();
	QByteArray ba;
	ba.resize(sizeof(qint32));
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << jsonSize;
	ba.append(jsonData);
	ba.append(blob);
	m_state = WaitRequest;
	m_socket.write(ba);
	m_socket.flush();
}

void PhoneClient::slotConnected()
{
	m_state = Ready;
	getData();
	emit sigConnected();
}

void PhoneClient::slotDisconnected()
{
	m_state = WaitRequest;
	m_timer.start();
	emit sigDisconnected();
}

void PhoneClient::slotTimeout()
{
	if (m_socket.state() == QAbstractSocket::ConnectedState) {
		getState();
	} else {
		m_state = WaitRequest;
		m_socket.connectToHost(m_addr, m_port);
	}
}

void PhoneClient::setPort(int port)
{
	m_port = port;
	updateConnection();
}

void PhoneClient::setAddr(const QString &addr)
{
	m_addr = addr;
	updateConnection();
}

void PhoneClient::updateConnection()
{
	if (m_socket.state() == QAbstractSocket::ConnectedState) {
		m_socket.abort();
		if (m_timer.isActive()) {
			m_timer.stop();
		}
		m_timer.start();
	}
}

void PhoneClient::slotReadyRead()
{
	while(m_socket.bytesAvailable()) {
		QByteArray ba = m_socket.readAll();
		m_readBuffer.append(ba);
		switch (m_waitState) {
		case WaitSize: {
			if (m_readBuffer.size() < 4) {
				return;
			}
			QDataStream ds(&m_readBuffer, QIODevice::ReadOnly);
			ds >> m_jsonSize;
			m_waitState = WaitDoc;
		}
		// fall through
		case WaitDoc: {
			if (m_readBuffer.size() < (m_jsonSize + 4)) {
				return;
			}
			QByteArray jsonDoc;
			jsonDoc.resize(m_jsonSize);
			jsonDoc = m_readBuffer.mid(4, m_jsonSize);
			m_jsonObj = QJsonDocument::fromBinaryData(jsonDoc).object();
			m_blob.resize(m_jsonObj["blobSize"].toInt(0));
			m_waitState = WaitBlob;
		}
		// fall through
		case WaitBlob: {
			if (m_readBuffer.size() < (m_jsonSize + 4 + m_blob.size())) {
				return;
			}
			m_blob = m_readBuffer.mid(4 + m_jsonSize, m_blob.size());
			m_state = Ready;
			m_waitState = WaitSize;
			m_readBuffer.clear();
		}
		}
	}
	requestHandler(m_jsonObj, m_blob);
}

void PhoneClient::requestHandler(const QJsonObject &obj, const QByteArray &blob)
{
	if (obj["State"].toInt(DBError) == DBError) {
		m_timer.start();
		return;
	}
	QString cmd = obj["cmd"].toString();
	if (cmd == "GetState") {
		int lastMod = obj["LastMod"].toInt();
		if (lastMod != m_lastMod) {
			getData();
		}
	} else if (cmd == "GetData") {
		prepareDB(obj, blob);
	} else if (cmd == "SetRecord" ||
			   cmd == "AddRecord" ||
			   cmd == "RmRecord") {
		// успешно отправили
	} else {
		// неизвестная команда
	}
	m_timer.start();
}

void PhoneClient::prepareDB(const QJsonObject &obj, const QByteArray &blob)
{
	QBuffer buf;
	m_lastMod = obj["LastMod"].toInt();
	int recSize = obj["RecSize"].toInt();
	buf.setData(blob);
	buf.open(QIODevice::ReadOnly);
	QList<Record> recs;
	for (int i = 0; i < recSize; ++i) {
		Record rec;
		if (buf.read((char*)(&rec), sizeof(rec)) == 0) {
			return;
		}
		recs << rec;
	}
	buf.close();
	emit sigDBUpdated(recs);
}

void PhoneClient::slotErrorHandler(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error)
	m_timer.start();
}
