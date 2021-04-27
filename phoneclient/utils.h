#ifndef UTILS_H
#define UTILS_H

#include "../global.h"
#include <QJsonObject>

Record		jsonToRec(const QJsonObject &obj);
QJsonObject recToJson(const Record &rec);

#endif // UTILS_H
