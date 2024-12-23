#include "backend.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

BackEnd::BackEnd(QObject *parent) : QObject(parent)
  , temperature(0), humidity(0), temp_desc(0), hum_desc(0), mem_desc(0)
{
    qDebug()<<"DinhNamUet Started!";
}

float BackEnd::getTemperature()
{
    char buf[6] = { 0 };
    if (read(temp_desc, buf, sizeof(buf)) < 0)
        return 0;
    else
        sscanf(buf, "%f", &temperature);
    return temperature;
}

float BackEnd::getHumidity()
{
    char buf[6] = { 0 };
    if (read(hum_desc, buf, sizeof(buf)) < 0)
        return 0;
    else
        sscanf(buf, "%f", &humidity);
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
        return;
    }

    if (read(fd, buffer, sizeof(buffer)) < 0) {
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
    char buffer[512] = { 0 };
    float totalMemory, freeMemory;
    if (read(mem_desc, buffer, sizeof(buffer)) < 0)
        return -1;
    char *memTotalLine = strstr(buffer, "MemTotal");
    if (memTotalLine)
        sscanf(memTotalLine, "MemTotal: %f kB", &totalMemory);
    char *memFreeLine = strstr(buffer, "MemFree");
    if (memFreeLine)
        sscanf(memFreeLine, "MemFree: %f kB", &freeMemory);
    if (totalMemory <= 0 || freeMemory < 0)
        return -1;
    return (totalMemory - freeMemory) * 100 / (totalMemory);
}

void BackEnd::machineInit(void)
{
    temp_desc = open(TEMPERATURE, O_RDONLY);
    if (-1 == temp_desc)
        while(1);

    hum_desc = open(HUMIDITY, O_RDONLY);
    if (-1 == hum_desc)
        while(1);

    mem_desc = open(MEMORY, O_RDONLY);
    if (-1 == mem_desc)
        while(1);
}

void BackEnd::machineDeInit(void)
{
    close(temp_desc);
    close(hum_desc);
    close(mem_desc);
}
