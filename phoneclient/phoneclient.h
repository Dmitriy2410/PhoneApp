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
	~PhoneClient();

	void setAddr(const QString &addr);
	void setPort(int port);
	void addRecord(const Record &rec);
	void setRecord(int id, const Record &rec);
	void rmRecord(const QList<int> &ids);

private:
	void send(const QJsonObject &obj);
	void requestHandler(const QJsonObject &obj);
	void prepareDB(const QJsonObject &obj);
	void getData();
	void updateConnection();

private slots:
	void slotConnected();
	void slotDisconnected();
	void slotErrorHandler(QAbstractSocket::SocketError error);
	void slotReadyRead();
	void slotTimeout();
	void slotReqTimeout();
	void getState();

signals:
	void sigConnected();
	void sigDisconnected();
	void sigDBUpdated(const QList<Record> &recs);
	void sigShowMessage(const QString &msg);

private:
	QTcpSocket	m_socket;
	QTimer		m_timer;
	QTimer		m_requestTimer;
	qint64		m_lastMod;
	QString		m_addr;
	int			m_port;
	int			m_state;
	// read
	qint32		m_jsonSize;
	QByteArray	m_readBuffer;
	QJsonObject	m_jsonObj;
	int			m_waitState;
};

#endif // PHONECLIENT_H
