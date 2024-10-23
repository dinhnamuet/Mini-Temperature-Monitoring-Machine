#include "backend.h"
#include <unistd.h>
#include <fcntl.h>
#include <QDebug>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#undef DEBUG

BackEnd::BackEnd(QObject *parent) : QObject(parent)
  , temperature(0), humidity(0)
{
}

float BackEnd::getTemperature()
{
    int fd;
    char buf[6] = { 0 };
    fd = open(TEMPERATURE, O_RDONLY);
    if (-1 == fd) {
#if defined(DEBUG)
        qDebug()<<"Error when open "<<TEMPERATURE<<"\n";
#endif
        return temperature;
    } else if (read(fd, buf, sizeof(buf)) < 0) {
#if defined(DEBUG)
        qDebug()<<"Error when read "<<TEMPERATURE<<"\n";
#endif
    } else {
        sscanf(buf, "%f", &temperature);
    }
    close(fd);
    return temperature;
}

float BackEnd::getHumidity()
{
    int fd;
    char buf[6] = { 0 };
    fd = open(HUMIDITY, O_RDONLY);
    if (-1 == fd) {
#if defined(DEBUG)
        qDebug()<<"Error when open "<<HUMIDITY<<"\n";
#endif
        return humidity;
    } else if (read(fd, buf, sizeof(buf)) < 0) {
#if defined(DEBUG)
        qDebug()<<"Error when read "<<HUMIDITY<<"\n";
#endif
    } else {
        sscanf(buf, "%f", &humidity);
    }
    close(fd);
    return humidity;
}

void BackEnd::getTime()
{
    int fd;
    double upTime;
    int hours, minutes, seconds;
    char buffer[128] = { 0 };
    fd = open("/proc/uptime", O_RDONLY);

    if (-1 == fd) {
#if defined(DEBUG)
        qDebug() << "Error when opening " << "/proc/uptime" << "\n";
#endif
        return;
    }

    if (read(fd, buffer, sizeof(buffer)) < 0) {
#if defined(DEBUG)
        qDebug() << "Error when reading " << "/proc/uptime" << "\n";
#endif
        close(fd);
        return;
    }
    close(fd);
    upTime = atof(strtok(buffer, " "));
    hours = static_cast<int>(upTime) / 3600;
    minutes = (static_cast<int>(upTime) % 3600) / 60;
    seconds = static_cast<int>(upTime) % 60;

    emit timeChanged(hours, minutes, seconds);
}

float BackEnd::getMeminfo()
{
    int fd;
    char buffer[512] = { 0 };
    float totalMemory, freeMemory;
    fd = open("/proc/meminfo", O_RDONLY);
    if (-1 == fd) {
#if defined(DEBUG)
        qDebug() << "Error when opening " << "/proc/meminfo" << "\n";
#endif
        return -1;
    }
    if (read(fd, buffer, sizeof(buffer)) < 0) {
#if defined(DEBUG)
        qDebug() << "Error when reading " << "/proc/meminfo" << "\n";
#endif
        close(fd);
        return -1;
    }
    close(fd);
    char *memTotalLine = strstr(buffer, "MemTotal");
    if (memTotalLine) {
        sscanf(memTotalLine, "MemTotal: %f kB", &totalMemory);
    }
    char *memFreeLine = strstr(buffer, "MemFree");
    if (memFreeLine) {
        sscanf(memFreeLine, "MemFree: %f kB", &freeMemory);
    }
    if (totalMemory <= 0 || freeMemory < 0) {
#if defined(DEBUG)
        qDebug() << "Error when parsing memory values";
#endif
        return -1;
    }
    return (totalMemory - freeMemory) * 100 / (totalMemory);
}
