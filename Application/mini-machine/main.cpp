#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "backend.h"
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    qmlRegisterType<BackEnd>("dht11", 1, 0, "DHT11");
    QQuickStyle::setStyle("Universal");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
