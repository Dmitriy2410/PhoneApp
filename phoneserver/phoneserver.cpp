#include "phoneserver.h"
#include <QtNetwork>
#include <QTcpServer>
#include <QDebug>
#include "datajson.h"
#include <QBuffer>
#include "../global.h"

PhoneServer::PhoneServer() :
	QObject()
{
	m_server = new QTcpServer(this);
	QString datPath = qApp->applicationDirPath() + "/data.json";
	m_dataJson.setFileName(datPath);
	m_dataJson.load();

	connect(m_server, SIGNAL(newConnection()),
			this, SLOT(slotNewConnection()));

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
	if (!m_server->listen(addr, 1234)) {
		qDebug()<<"Start error:"<<m_server->errorString();
		exit(1);
		return;
	}
	qDebug()<<"Addr:"<<m_server->serverAddress().toString()<<":"<<m_server->serverPort();
}

void PhoneServer::slotNewConnection()
{
	QTcpSocket *sock = m_server->nextPendingConnection();
	if (!sock) {
		return;
	}
	connect(sock, SIGNAL(disconnected()),
			this, SLOT(slotDisconnected()));
	connect(sock, SIGNAL(disconnected()),
			sock, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()),
			this, SLOT(slotReadyRead()));
	qDebug()<<"New connection:"<<sock->peerAddress().toString();
}

void PhoneServer::slotDisconnected()
{
	QTcpSocket *sock = static_cast<QTcpSocket*>(sender());
	qDebug()<<"Destroy connection:"<<sock->peerAddress().toString();
	m_sockData.remove(sock);
}


void PhoneServer::slotReadyRead()
{
	QTcpSocket *sock = static_cast<QTcpSocket*>(sender());
	SockData &sData = m_sockData[sock];
	while(sock->bytesAvailable()) {
		QByteArray ba = sock->readAll();
		sData.m_readBuffer.append(ba);
		switch (sData.m_waitState) {
		case WaitSize: {
			if (sData.m_readBuffer.size() < 4) {
				return;
			}
			QDataStream ds(&sData.m_readBuffer, QIODevice::ReadOnly);
			ds >> sData.m_jsonSize;
			sData.m_waitState = WaitDoc;
		}
		// fall through
		case WaitDoc: {
			if (sData.m_readBuffer.size() < (sData.m_jsonSize + 4)) {
				return;
			}
			QByteArray jsonDoc;
			jsonDoc.resize(sData.m_jsonSize);
			jsonDoc = sData.m_readBuffer.mid(4, sData.m_jsonSize);
			sData.m_jsonObj = QJsonDocument::fromBinaryData(jsonDoc).object();
			sData.m_waitState = WaitSize;
			sData.m_readBuffer.clear();
		}
		}
	}
	prepareRequest(sock, sData.m_jsonObj);
}

void PhoneServer::prepareRequest(QTcpSocket *sock, const QJsonObject &obj)
{
	QString cmd = obj["cmd"].toString();
	if (cmd == "GetState") {
		prepareGetState(sock);
	} else if (cmd == "GetData") {
		prepareGetData(sock);
	} else if (cmd == "SetRecord") {
		prepareSetRecord(sock, obj);
	} else if (cmd == "AddRecord") {
		prepareAddRecord(sock, obj);
	} else if (cmd == "RmRecord") {
		prepareRmRecord(sock, obj);
	} else {
		// неизвестная команда
		QJsonObject reqObj;
		reqObj["cmd"] = cmd;
		reqObj["State"] = DBError;
		request(sock, reqObj);
	}
}

void PhoneServer::prepareGetState(QTcpSocket *sock)
{
	QJsonObject reqObj;
	reqObj["cmd"] = "GetState";
	reqObj["State"] = DBReady;
	reqObj["LastMod"] = m_dataJson.lastModified();
	request(sock, reqObj);
}

void PhoneServer::prepareGetData(QTcpSocket *sock)
{
	QJsonObject reqObj;
	reqObj["cmd"] = "GetData";
	reqObj["State"] = DBReady;
	reqObj["LastMod"] = m_dataJson.lastModified();
	QList<QJsonObject> recs = m_dataJson.records().values();
	QJsonArray arr;
	for (int i = 0; i < recs.size(); ++i) {
		arr.append(recs[i]);
	}
	reqObj["Data"] = arr;
	request(sock, reqObj);
}

void PhoneServer::prepareSetRecord(QTcpSocket *sock, const QJsonObject &obj)
{
	QJsonObject reqObj;
	reqObj["cmd"] = obj["cmd"].toString();
	int id = obj["RecId"].toInt(-1);
	QJsonObject rec = obj["Record"].toObject();
	m_dataJson.setRecord(id, rec);
	bool ok = false;
	if (id != -1) {
		if (m_dataJson.save()) {
			ok = true;
		}
	}
	if (ok) {
		reqObj["State"] = DBReady;
	} else {
		reqObj["State"] = DBError;
		qDebug()<<"Edit record error!";
	}
	request(sock, reqObj);
}

void PhoneServer::prepareAddRecord(QTcpSocket *sock, const QJsonObject &obj)
{
	QJsonObject reqObj;
	reqObj["cmd"] = obj["cmd"].toString();
	QJsonObject rec = obj["Record"].toObject();
	m_dataJson.addRecord(rec);
	if (m_dataJson.save()) {
		reqObj["State"] = DBReady;
	} else {
		reqObj["State"] = DBError;
		qDebug()<<"Record addition error!";
	}
	request(sock, reqObj);
}

void PhoneServer::prepareRmRecord(QTcpSocket *sock, const QJsonObject &obj)
{
	QJsonObject reqObj;
	reqObj["State"] = DBReady;
	reqObj["cmd"] = obj["cmd"].toString();
	QJsonArray arr = obj["RecIds"].toArray();
	QList<int> ids;
	for (int i = 0; i < arr.size(); ++i) {
		ids.append(arr[i].toInt(-1));
	}
	for (int i = 0; i < ids.size(); ++i) {
		m_dataJson.rmRecord(ids[i]);
	}
	if (m_dataJson.save()) {
		reqObj["State"] = DBReady;
	} else {
		reqObj["State"] = DBError;
	}
	request(sock, reqObj);
}

void PhoneServer::request(QTcpSocket *sock, const QJsonObject &obj)
{
	QByteArray jsonData = QJsonDocument(obj).toBinaryData();
	qint32 jsonSize = jsonData.size();
	QByteArray ba;
	ba.resize(sizeof(qint32));
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << jsonSize;
	ba.append(jsonData);
	sock->write(ba);
	sock->flush();
}
