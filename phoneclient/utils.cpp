#include "utils.h"

Record jsonToRec(const QJsonObject &obj)
{
	Record rec;
	rec.id = obj["id"].toInt();
	rec.surname = obj["surname"].toString();
	rec.name = obj["name"].toString();
	rec.secondName = obj["secondName"].toString();
	rec.phoneNumber = obj["phoneNumber"].toString();
	rec.gender = obj["gender"].toInt();
	return rec;
}

QJsonObject recToJson(const Record &rec)
{
	QJsonObject obj;
	obj["id"] = rec.id;
	obj["surname"] = rec.surname;
	obj["name"] = rec.name;
	obj["secondName"] = rec.secondName;
	obj["gender"] = rec.gender;
	obj["phoneNumber"] = rec.phoneNumber;
	return obj;
}
