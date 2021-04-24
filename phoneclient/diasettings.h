#ifndef DIASETTINGS_H
#define DIASETTINGS_H

#include <QDialog>

namespace Ui {
class DiaSettings;
}

class QTcpSocket;

class DiaSettings : public QDialog
{
	Q_OBJECT

public:
	explicit DiaSettings(QWidget *parent = nullptr);
	~DiaSettings();
	QString getAddr();
	int		getPort();

private slots:
	void	slotTest();

private:
	QTcpSocket		*m_testSock;
	Ui::DiaSettings *ui;
};

#endif // DIASETTINGS_H
