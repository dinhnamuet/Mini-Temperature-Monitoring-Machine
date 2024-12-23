#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QDebug>
#define TEMPERATURE "/sys/bus/iio/devices/iio:device0/in_temp_hnam02_input"
#define HUMIDITY    "/sys/bus/iio/devices/iio:device0/in_humidityrelative_hnam02_input"
#define MEMORY      "/proc/meminfo"
class BackEnd : public QObject
{
    Q_OBJECT
public:
    explicit BackEnd(QObject *parent = nullptr);
    ~BackEnd() {
        qDebug()<<"DinhNamUet Stopped!";
    }
    Q_INVOKABLE float getTemperature(void);
    Q_INVOKABLE float getHumidity(void);
    Q_INVOKABLE void getTime(void);
    Q_INVOKABLE float getMeminfo(void);
    Q_INVOKABLE void machineInit(void);
    Q_INVOKABLE void machineDeInit(void);

private:
    float temperature;
    float humidity;
    int temp_desc;
    int hum_desc;
    int mem_desc;

signals:
    void timeChanged(int h, int m, int s);
};

#endif // BACKEND_H
