#ifndef DIANEWRECORD_H
#define DIANEWRECORD_H

#include <QDialog>
#include "../global.h"

namespace Ui {
class DiaNewRecord;
}

class DiaNewRecord : public QDialog
{
	Q_OBJECT

public:
	explicit DiaNewRecord(QWidget *parent = nullptr);
	~DiaNewRecord();

	void init(const Record &record);
	Record record();

private slots:
	void slotAccept();

private:
	Ui::DiaNewRecord *ui;
};

#endif // DIANEWRECORD_H
