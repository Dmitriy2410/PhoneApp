#include <QCoreApplication>
#include "phoneserver.h"
#include "QFile"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	PhoneServer server;

	return a.exec();
}
