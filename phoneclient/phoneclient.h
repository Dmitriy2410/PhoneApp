#ifndef PHONECLIENT_H
#define PHONECLIENT_H

#include <QObject>
#include <QtNetwork>
#include "../global.h"

class QTcpSocket;

class PhoneClient : public QObject
{
	Q_OBJECT
public:
	enum State {
		Ready,
		WaitRequest
	};

	PhoneClient();

	void setAddr(const QString &addr);
	void setPort(int port);
	void addRecord(const Record &rec);
	void setRecord(int id, const Record &rec);
	void rmRecord(const QList<int> &ids);

private:
	void send(const QJsonObject &obj, const QByteArray &blob);
	void requestHandler(const QJsonObject &obj, const QByteArray &blob);
	void setLastMod(qint64 mod);
	void prepareDB(const QJsonObject &obj, const QByteArray &blob);
	void getData();
	void updateConnection();

private slots:
	void slotConnected();
	void slotDisconnected();
	void slotErrorHandler(QAbstractSocket::SocketError error);
	void slotReadyRead();
	void slotTimeout();
	void getState();

signals:
	void sigConnected();
	void sigDisconnected();
	void sigDBUpdated(const QList<Record> &recs);
	void sigShowMessage(const QString &msg);

private:
	QTcpSocket	m_socket;
	QTimer		m_timer;
	qint64		m_lastMod;
	QString		m_addr;
	int			m_port;
	int			m_state;
};

#endif // PHONECLIENT_H
