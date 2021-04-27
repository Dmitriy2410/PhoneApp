#ifndef RECORD_H
#define RECORD_H

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
	WaitDoc,
	WaitBlob
};

#pragma pack(push, 1)
struct Record
{
	Record() {
		id = -1;
		gender = Male;
	}
	int32_t	id;
	char	surname[60];
	char	name[60];
	char	secondName[60];
	int8_t	gender;
	char	phoneNumber[25];
};
#pragma pack(pop)

#endif // RECORD_H
