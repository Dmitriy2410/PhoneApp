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
			sData.m_blob.resize(sData.m_jsonObj["blobSize"].toInt(0));
			sData.m_waitState = WaitBlob;
		}
		// fall through
		case WaitBlob: {
			if (sData.m_readBuffer.size() < (sData.m_jsonSize + 4 + sData.m_blob.size())) {
				return;
			}
			sData.m_blob = sData.m_readBuffer.mid(4 + sData.m_jsonSize, sData.m_blob.size());
			sData.m_waitState = WaitSize;
			sData.m_readBuffer.clear();
		}
		}
	}
	prepareRequest(sock, sData.m_jsonObj, sData.m_blob);
}

void PhoneServer::prepareRequest(QTcpSocket *sock, const QJsonObject &obj, const QByteArray &blob)
{
	QString cmd = obj["cmd"].toString();
	if (cmd == "GetState") {
		prepareGetState(sock);
	} else if (cmd == "GetData") {
		prepareGetData(sock);
	} else if (cmd == "SetRecord") {
		int id = obj["recId"].toInt(-1);
		prepareSetRecord(sock, id, blob);
	} else if (cmd == "AddRecord") {
		prepareAddRecord(sock, blob);
	} else if (cmd == "RmRecord") {
		QJsonArray arr = obj["recIds"].toArray();
		QList<int> ids;
		for (int i = 0; i < arr.size(); ++i) {
			ids.append(arr[i].toInt(-1));
		}
		prepareRmRecord(sock, ids);
	} else {
		// неизвестная команда
		QJsonObject reqObj;
		reqObj["cmd"] = cmd;
		reqObj["State"] = DBError;
		request(sock, reqObj, QByteArray());
	}
}

void PhoneServer::prepareGetState(QTcpSocket *sock)
{
	QJsonObject reqObj;
	reqObj["cmd"] = "GetState";
	reqObj["State"] = DBReady;
	reqObj["LastMod"] = m_dataJson.lastModified();
	request(sock, reqObj, QByteArray());
}

void PhoneServer::prepareGetData(QTcpSocket *sock)
{
	QJsonObject reqObj;
	QByteArray reqBlob;
	QList<Record> recs = m_dataJson.records().values();
	reqObj["cmd"] = "GetData";
	reqObj["State"] = DBReady;
	reqObj["RecSize"] = recs.size();
	reqObj["LastMod"] = m_dataJson.lastModified();
	reqBlob.resize(recs.size() * sizeof(Record));
	QBuffer buf(&reqBlob);
	buf.open(QIODevice::WriteOnly);
	for (int i = 0; i < recs.size(); ++i) {
		if (buf.write((const char*)(&(recs[i])), sizeof(Record)) == 0) {
			reqObj["State"] = DBError;
			break;
		}
	}
	buf.close();
	reqObj["blobSize"] = reqBlob.size();
	request(sock, reqObj, reqBlob);
}

void PhoneServer::prepareSetRecord(QTcpSocket *sock, int id, const QByteArray &blob)
{
	QJsonObject reqObj;
	reqObj["cmd"] = "SetRecord";
	QBuffer buf;
	Record rec;
	buf.setData(blob);
	buf.open(QIODevice::ReadOnly);
	if (buf.read((char*)(&rec), sizeof(rec)) != 0) {
		bool ok = false;
		m_dataJson.setRecord(id, rec);
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
	} else {
		reqObj["State"] = DBError;
	}
	buf.close();
	request(sock, reqObj, QByteArray());
}

void PhoneServer::prepareAddRecord(QTcpSocket *sock, const QByteArray &blob)
{
	QJsonObject reqObj;
	QBuffer buf;
	Record rec;
	buf.setData(blob);
	buf.open(QIODevice::ReadOnly);
	if (buf.read((char*)(&rec), sizeof(rec)) != 0) {
		m_dataJson.addRecord(rec);
		if (m_dataJson.save()) {
			reqObj["State"] = DBReady;
		} else {
			reqObj["State"] = DBError;
			qDebug()<<"Record addition error!";
		}
	} else {
		reqObj["State"] = DBError;
	}
	buf.close();
	request(sock, reqObj, QByteArray());
}

void PhoneServer::prepareRmRecord(QTcpSocket *sock, const QList<int> &ids)
{
	QJsonObject reqObj;
	reqObj["State"] = DBReady;
	for (int i = 0; i < ids.size(); ++i) {
		m_dataJson.rmRecord(ids[i]);
	}
	if (m_dataJson.save()) {
		reqObj["State"] = DBReady;
	} else {
		reqObj["State"] = DBError;
	}
	request(sock, reqObj, QByteArray());
}

void PhoneServer::request(QTcpSocket *sock, const QJsonObject &obj, const QByteArray &blob)
{
	QByteArray jsonData = QJsonDocument(obj).toBinaryData();
	qint32 jsonSize = jsonData.size();
	QByteArray ba;
	ba.resize(sizeof(qint32));
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << jsonSize;
	ba.append(jsonData);
	ba.append(blob);
	sock->write(ba);
	sock->flush();
}
