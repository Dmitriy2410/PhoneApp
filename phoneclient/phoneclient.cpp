#include "phoneclient.h"
#include <QBuffer>
#include <QTcpSocket>
#include <QTimer>
#include "../global.h"
#include "utils.h"

const int gc_interval = 1000;
const int gc_timeoutInterval = 15000;

PhoneClient::PhoneClient()
{
	m_timer.setInterval(gc_interval);
	m_timer.setSingleShot(true);
	m_requestTimer.setInterval(gc_timeoutInterval);
	m_requestTimer.setSingleShot(true);
	connect(&m_socket, SIGNAL(connected()),
			this, SLOT(slotConnected()));
	connect(&m_socket, SIGNAL(disconnected()),
			this, SLOT(slotDisconnected()));
	connect(&m_socket, SIGNAL(readyRead()),
			this, SLOT(slotReadyRead()));
	connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(slotErrorHandler(QAbstractSocket::SocketError)));
	connect(&m_timer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()));
	connect(&m_requestTimer, SIGNAL(timeout()),
			this, SLOT(slotReqTimeout()));
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

PhoneClient::~PhoneClient()
{
	if (m_socket.state() == QAbstractSocket::ConnectedState) {
		m_socket.abort();
	}
}

void PhoneClient::getState()
{
	QJsonObject obj;
	obj["cmd"] = "GetState";
	send(obj);
}

void PhoneClient::getData()
{
	QJsonObject obj;
	obj["cmd"] = "GetData";
	send(obj);
}

void PhoneClient::addRecord(const Record &rec)
{
	if (m_state != Ready) {
		emit sigShowMessage(tr("Ошибка соединения"));
		return;
	}
	QJsonObject obj;
	obj["cmd"] = "AddRecord";
	obj["Record"] = recToJson(rec);
	send(obj);
}

void PhoneClient::setRecord(int id, const Record &rec)
{
	if (m_state != Ready) {
		emit sigShowMessage(tr("Ошибка соединения"));
		return;
	}
	QJsonObject obj;
	obj["cmd"] = "SetRecord";
	obj["RecId"] = id;
	obj["Record"] = recToJson(rec);
	send(obj);
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
	obj["RecIds"] = arr;
	send(obj);
}


void PhoneClient::send(const QJsonObject &obj)
{
	QByteArray jsonData = QJsonDocument(obj).toBinaryData();
	qint32 jsonSize = jsonData.size();
	QByteArray ba;
	ba.resize(sizeof(qint32));
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << jsonSize;
	ba.append(jsonData);
	m_state = WaitRequest;
	m_socket.write(ba);
	m_socket.flush();
	m_requestTimer.start();
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

void PhoneClient::slotReqTimeout()
{
	// слишком долго нет ответа, переподключение
	m_socket.disconnectFromHost();
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
	}
	m_timer.start();
}

void PhoneClient::slotReadyRead()
{
	m_requestTimer.start();
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
			m_state = Ready;
			m_waitState = WaitSize;
			m_requestTimer.stop();
			m_readBuffer.clear();
		}
		}
	}
	requestHandler(m_jsonObj);
}

void PhoneClient::requestHandler(const QJsonObject &obj)
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
		prepareDB(obj);
	} else if (cmd == "SetRecord" ||
			   cmd == "AddRecord" ||
			   cmd == "RmRecord") {
		// успешно отправили
	} else {
		// неизвестная команда
	}
	m_timer.start();
}

void PhoneClient::prepareDB(const QJsonObject &obj)
{
	m_lastMod = obj["LastMod"].toInt();
	QJsonArray arr = obj["Data"].toArray();
	QList<Record> recs;
	for (int i = 0; i < arr.size(); ++i) {
		recs << jsonToRec(arr[i].toObject());
	}
	emit sigDBUpdated(recs);
}

void PhoneClient::slotErrorHandler(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error)
	m_timer.start();
}
