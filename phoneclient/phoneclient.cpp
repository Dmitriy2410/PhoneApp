#include "phoneclient.h"
#include <QBuffer>
#include <QTcpSocket>
#include <QTimer>
#include "../global.h"

const int gc_interval = 1000;

PhoneClient::PhoneClient()
{
	m_timer.setInterval(gc_interval);
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
	ba.resize(sizeof(qint32) + jsonSize);
	QDataStream ds(&ba, QIODevice::WriteOnly);
	ds << jsonSize << jsonData << blob;
	m_state = WaitRequest;
	m_socket.write(ba);
	m_socket.flush();
}

void PhoneClient::slotConnected()
{
	m_state = Ready;
	getState();
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
	m_state = Ready;
	qint32 jsonSize;
	QByteArray ba;
	int count = 0;
	while(m_socket.bytesAvailable()) {
		ba.append(m_socket.readAll());
		count++;
	}
	QDataStream ds(&ba, QIODevice::ReadOnly);
	ds >> jsonSize;
	QByteArray jsonDoc;
	jsonDoc.resize(jsonSize);
	ds >> jsonDoc;
	QByteArray blob;
	blob.resize(ba.size() - (sizeof(qint32) + jsonSize));
	ds >> blob;
	QJsonObject obj = QJsonDocument::fromBinaryData(jsonDoc).object();
	requestHandler(obj, blob);
}

void PhoneClient::requestHandler(const QJsonObject &obj, const QByteArray &blob)
{
	if (obj["State"].toInt(DBError) == DBError) {
		m_timer.start();
		return;
	}
	QString cmd = obj["cmd"].toString();
	if (cmd == "GetState") {
		setLastMod(obj["LastMod"].toInt());
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

void PhoneClient::setLastMod(qint64 mod)
{
	if (mod != m_lastMod) {
		m_lastMod = mod;
		getData();
	}
}

void PhoneClient::prepareDB(const QJsonObject &obj, const QByteArray &blob)
{
	QBuffer buf;
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
