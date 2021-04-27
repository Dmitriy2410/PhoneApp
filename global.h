#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>

enum Gender {
	Male,
	Female
};

enum DBState {
	DBError = -1,
	DBReady = 0
};
enum ReadState {
	WaitSize,
	WaitDoc
};

struct Record
{
	Record() {
		id = -1;
		gender = Male;
	}
	int32_t	id;
	QString	surname;
	QString	name;
	QString	secondName;
	int8_t	gender;
	QString	phoneNumber;
};

#endif // GLOBAL_H
