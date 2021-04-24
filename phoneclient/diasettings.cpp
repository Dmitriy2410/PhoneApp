#include "diasettings.h"
#include <QRegExpValidator>
#include <QtNetwork>
#include "ui_diasettings.h"

const QString gc_ipVal = "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";

DiaSettings::DiaSettings(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DiaSettings)
{	
	ui->setupUi(this);
	QRegExp re(gc_ipVal);
	QRegExpValidator *val = new QRegExpValidator(re, ui->le_addr);
	ui->le_addr->setValidator(val);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QList<QHostAddress> allAddr = QNetworkInterface::allAddresses();
	for (int i = 0; i < allAddr.size(); ++i) {
		if (allAddr[i].toIPv4Address()) {
			ui->cb_server->addItem(allAddr[i].toString());
		}
	}
	connect(ui->pb_test, SIGNAL(clicked(bool)),
			this, SLOT(slotTest()));
	m_testSock = nullptr;
}

DiaSettings::~DiaSettings()
{
	delete ui;
}

QString DiaSettings::getAddr()
{
	QString addr;
	if (ui->rb_combo->isChecked()) {
		addr = ui->cb_server->currentText();
	} else {
		addr = ui->le_addr->text();
	}
	return addr;
}

int DiaSettings::getPort()
{
	return ui->spinBox->value();
}

void DiaSettings::slotTest()
{
	if (m_testSock) {
		delete m_testSock;
	}
	m_testSock = new QTcpSocket(this);
	connect(m_testSock, &QTcpSocket::connected,
			this, [=]
	{
		ui->l_mess->setText("Соединение установлено");
	});
	connect(m_testSock, &QTcpSocket::disconnected,
			this, [=]
	{
		ui->l_mess->setText("");
	});

	connect(m_testSock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
			this, [=] (QAbstractSocket::SocketError socketError)
	{
		Q_UNUSED(socketError)
		ui->l_mess->setText("Ошибка соединения");
	});
	QString addr;
	if (ui->rb_combo->isChecked()) {
		addr = ui->cb_server->currentText();
	} else {
		addr = ui->le_addr->text();
	}
	m_testSock->connectToHost(addr,
						ui->spinBox->value());
}
