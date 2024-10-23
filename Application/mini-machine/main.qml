import QtQuick 2.9
import QtQuick.Window 2.1
import QtQuick.Controls 2.2
import QtCharts 2.8
import dht11 1.0
ApplicationWindow {
    visible: true
    width: 320
    height: 240

    header: Rectangle {
        width: parent.width
        height: 50
        color: "beige"
        Row {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            Image {
                id: uet_logo
                width: parent.height
                height: parent.height
                source: "file:///home/nam/output/vnu.png"
            }
            Rectangle {
                width: parent.width - uet_logo.width
                height: parent.height
                Column {
                    anchors.centerIn: parent
                    spacing: 3
                    Text {
                        id: vnu
                        anchors.horizontalCenter: parent.horizontalCenter
                        font {
                            pointSize: 12
                            bold: true
                        }
                        text: "Đại Học Quốc Gia Hà Nội"
                    }
                    Text {
                        id: uet
                        anchors.horizontalCenter: parent.horizontalCenter
                        font {
                            pointSize: 11
                        }
                        text: "Trường Đại Học Công Nghệ"
                    }
                }
            }
        }
    }
    Rectangle {
        id: main_component
        anchors.fill: parent
        border {
            id: main_border
            width: 2
            color: "Black"
        }
        ChartView {
            legend.visible: true
            margins.top: 0
            margins.bottom: 0
            antialiasing: true
            anchors.fill: parent
            SplineSeries {
                id: temperature
                name: "Temperature"
                width: 1.0
                XYPoint {x: 1; y: 0}
                XYPoint {x: 2; y: 0}
                XYPoint {x: 3; y: 0}
                XYPoint {x: 4; y: 0}
                XYPoint {x: 5; y: 0}
                XYPoint {x: 6; y: 0}
                XYPoint {x: 7; y: 0}
                XYPoint {x: 8; y: 0}
                XYPoint {x: 9; y: 0}
                XYPoint {x: 10; y: 0}
                axisX: ValueAxis {
                    min: 1
                    max: 10
                    labelFormat: "%d"
                    tickCount: 10
                }
                axisY: ValueAxis {
                    min: 0
                    max: 45
                    labelFormat: "%d"
                    tickCount: 4
                }
            }
        }
    }

    footer: Rectangle {
        width: parent.width
        height: 15
        Row {
            anchors.fill: parent
            spacing: 20
            Text {
                id: foot_txt
                font {
                    pointSize: 11
                }
                color: "Red"
                text: "LuuHoaiNam"
            }
            Text {
                id: cpu
                font {
                    pointSize: 11
                }
                color: "black"
                text: "BuiNhatMinh"
            }
            Text {
                id: time
                font {
                    pointSize: 11
                }
                color: "green"
                text: "UET"
            }
        }
    }

    DHT11 {
        id: sensor
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            var temp;
            temp = sensor.getTemperature().toFixed(2);
            sensor.getTime();
            cpu.text = "RAM: " + sensor.getMeminfo().toFixed(2) + "%"

            for (var i = 0; i < temperature.count - 1; i++) {
                temperature.replace(temperature.at(i).x, temperature.at(i).y, temperature.at(i).x, temperature.at(i+1).y);
            }
            temperature.replace(temperature.at(temperature.count - 1).x, temperature.at(temperature.count - 1).y, temperature.at(temperature.count - 1).x, temp);

            foot_txt.text = "T: " + temp + "°C"

            uet.font.bold = !uet.font.bold
            vnu.font.bold = !uet.font.bold

            if (uet.font.bold) {
                uet_logo.source = "file:///home/nam/output/uet.jpg";
            } else {
                uet_logo.source = "file:///home/nam/output/vnu.png";
            }
        }
    }

    Connections {
        target: sensor
        function onTimeChanged(h, m, s) {
            time.text = h + " : " + m + " : " + s
        }
    }
}
