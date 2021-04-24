#ifndef SERVER_H
#define SERVER_H

#include <QScopedPointer>
#include <QObject>
#include "../global.h"
#include "datajson.h"

class QTcpServer;
class QTcpSocket;
class QNetworkSession;

class PhoneServer : public QObject
{
	Q_OBJECT
public:
	PhoneServer();

private:
	void prepareRequest(QTcpSocket *sock, const QJsonObject &obj, const QByteArray &blob);

	void prepareGetState(QTcpSocket *sock);
	void prepareGetData(QTcpSocket *sock);
	void prepareSetRecord(QTcpSocket *sock, int id, const QByteArray &blob);
	void prepareAddRecord(QTcpSocket *sock, const QByteArray &blob);
	void prepareRmRecord(QTcpSocket *sock, const QList<int> &ids);

	void request(QTcpSocket *sock, const QJsonObject &obj, const QByteArray &blob);

private slots:
	void slotNewConnection();
	void slotReadyRead();

private:
	QTcpServer	*m_server;
	DataJson	m_dataJson;
};

#endif // SERVER_H
