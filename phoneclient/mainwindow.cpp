#include "mainwindow.h"
#include "dianewrecord.h"
#include "QMessageBox"
#include "recordmodel.h"
#include "ui_mainwindow.h"
#include "sortmodel.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	  ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(ui->pb_addRecord, SIGNAL(clicked(bool)),
			this, SLOT(slotAddRecord()));
	connect(ui->pb_rmRecord, SIGNAL(clicked(bool)),
			this, SLOT(slotRmRecord()));
	connect(ui->pb_settings, SIGNAL(clicked(bool)),
			this, SLOT(slotSettings()));
	connect(ui->tv_records, SIGNAL(doubleClicked(QModelIndex)),
			this, SLOT(slotEditRecord(QModelIndex)));
	m_sortModel = new SortModel(ui->tv_records);
	m_recordModel = new RecordModel(ui->tv_records);
	m_sortModel->setSourceModel(m_recordModel);
	ui->tv_records->setModel(m_sortModel);
	ui->tv_records->resizeColumnsToContents();
	ui->w_reconn->show();
	connect(&m_phoneClient, SIGNAL(sigConnected()),
			this, SLOT(slotConnected()));
	connect(&m_phoneClient, SIGNAL(sigDisconnected()),
			this, SLOT(slotDisconnected()));
	connect(&m_phoneClient, SIGNAL(sigDBUpdated(QList<Record>)),
			this, SLOT(slotDBUpdate(QList<Record>)));
	connect(&m_phoneClient, SIGNAL(sigShowMessage(QString)),
			this, SLOT(slotShowMessage(QString)));
	ui->tv_records->setSortingEnabled(true);
	m_setting.hide();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::slotAddRecord()
{
	DiaNewRecord dia(this);
	dia.setWindowTitle(tr("Добавление записи"));
	int ans = dia.exec();
	if (ans == QDialog::Accepted) {
		m_phoneClient.addRecord(dia.record());
	}
}

void MainWindow::slotRmRecord()
{
	QModelIndexList indexes = ui->tv_records->selectionModel()->selectedRows(0);
	if (indexes.isEmpty()) {
		return;
	}
	QList<int> ids;
	for (int i = 0; i < indexes.size(); ++i) {
		ids << m_sortModel->data(indexes[i], Qt::UserRole).toInt();
	}
	int ans = QMessageBox::question(this, tr("Внимание"),
									tr("Удалить выбранные записи?"));
	if (ans == QMessageBox::Yes) {
		m_phoneClient.rmRecord(ids);
	}
}

void MainWindow::slotEditRecord(const QModelIndex &index)
{
	int num = m_sortModel->mapToSource(index).row();
	Record rec = m_recordModel->records()[num];
	DiaNewRecord dia(this);
	dia.setWindowTitle(tr("Редактирование записи"));
	dia.init(rec);
	if (dia.exec() == QDialog::Accepted) {
		m_phoneClient.setRecord(rec.id, dia.record());
	}
}

void MainWindow::slotSettings()
{
	if (m_setting.exec() == QDialog::Accepted) {
		m_phoneClient.setAddr(m_setting.getAddr());
		m_phoneClient.setPort(m_setting.getPort());
	}
}

void MainWindow::slotConnected()
{
	ui->w_reconn->hide();
}

void MainWindow::slotDisconnected()
{
	ui->w_reconn->show();
}

void MainWindow::slotDBUpdate(const QList<Record> &recs)
{
	m_recordModel->setRecords(recs);
	ui->tv_records->resizeColumnsToContents();
}

void MainWindow::slotShowMessage(const QString &msg)
{
	QMessageBox::information(this, tr("Внимание"), msg);
}
