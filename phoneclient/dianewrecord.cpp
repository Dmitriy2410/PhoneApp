#include "dianewrecord.h"
#include <QMessageBox>
#include "ui_dianewrecord.h"

DiaNewRecord::DiaNewRecord(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DiaNewRecord)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->setupUi(this);
	ui->comboBox->addItem(tr("Мужской"));
	ui->comboBox->addItem(tr("Женский"));
	ui->comboBox->setCurrentIndex(0);

	connect(ui->buttonBox, SIGNAL(accepted()),
			this, SLOT(slotAccept()));
}

DiaNewRecord::~DiaNewRecord()
{
	delete ui;
}

void DiaNewRecord::init(const Record &rec)
{
	ui->le_surname->setText(rec.surname);
	ui->le_name->setText(rec.name);
	ui->le_secondname->setText(rec.secondName);
	ui->comboBox->setCurrentIndex(rec.gender);
	ui->le_number->setText(rec.phoneNumber);
}

Record DiaNewRecord::record()
{
	Record rec;
	rec.gender = ui->comboBox->currentIndex();
	sprintf(rec.surname, "%s", ui->le_surname->text().toStdString().c_str());
	sprintf(rec.name, "%s", ui->le_name->text().toStdString().c_str());
	sprintf(rec.secondName, "%s", ui->le_secondname->text().toStdString().c_str());
	sprintf(rec.phoneNumber, "%s", ui->le_number->text().toStdString().c_str());
	return rec;
}

void DiaNewRecord::slotAccept()
{
	if (ui->le_surname->text().isEmpty() ||
			ui->le_name->text().isEmpty() ||
			ui->le_secondname->text().isEmpty() ||
			ui->le_number->text().isEmpty()) {
		QMessageBox::critical(this, tr("Внимание"),
							  tr("Заполните все поля"));
		return;
	}
	accept();
}
