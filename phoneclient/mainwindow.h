#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "phoneclient.h"

namespace Ui {
	class MainWindow;
}

class RecordModel;
class SortModel;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void slotAddRecord();
	void slotRmRecord();
	void slotEditRecord(const QModelIndex &index);
	void slotSettings();
	void slotConnected();
	void slotDisconnected();
	void slotDBUpdate(const QList<Record> &recs);
	void slotShowMessage(const QString &msg);

private:
	RecordModel		*m_recordModel;
	SortModel		*m_sortModel;
	PhoneClient		m_phoneClient;
	Ui::MainWindow	*ui;
};
#endif // MAINWINDOW_H
