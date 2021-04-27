#ifndef SERVER_H
#define SERVER_H

#include <QScopedPointer>
#include <QObject>
#include "../global.h"
#include "datajson.h"
#include <QJsonObject>

class QTcpServer;
class QTcpSocket;
class QNetworkSession;

struct SockData {
	SockData() {
		m_waitState = WaitSize;
	}
	qint32		m_jsonSize;
	QByteArray	m_readBuffer;
	QJsonObject	m_jsonObj;
	int			m_waitState;
};

class PhoneServer : public QObject
{
	Q_OBJECT
public:
	PhoneServer();

private:
	void prepareRequest(QTcpSocket *sock, const QJsonObject &obj);

	void prepareGetState(QTcpSocket *sock);
	void prepareGetData(QTcpSocket *sock);
	void prepareSetRecord(QTcpSocket *sock, const QJsonObject &obj);
	void prepareAddRecord(QTcpSocket *sock, const QJsonObject &obj);
	void prepareRmRecord(QTcpSocket *sock, const QJsonObject &obj);

	void request(QTcpSocket *sock, const QJsonObject &obj);

private slots:
	void slotNewConnection();
	void slotReadyRead();
	void slotDisconnected();

private:
	QHash<QTcpSocket*, SockData>	m_sockData;
	QTcpServer						*m_server;
	DataJson						m_dataJson;
};

#endif // SERVER_H
