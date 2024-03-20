#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QCameraInfo"
#include "QCamera"
#include "QList"
#include "QComboBox"
#include "QCameraImageCapture"
#include "QImage"
#include "QPixmap"
#include "QMediaRecorder"
#include "QString"
#include "QtMqtt/qmqttclient.h"
#include "QTimer"
#include "QTime"
#include "QBuffer"
#include "QDataStream"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_comboBox_currentIndexChanged(int index);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;
    QList<QCameraInfo> camera_info;//摄像头基本信息
    QCamera *camera;//摄像头对象
    QCameraImageCapture *cap;//截图捕获对象
    QMediaRecorder *media_recoder;//视频录像
    QString IP_Address = "127.0.0.1";//设置IP地址
    QString Port = "1883";//设置端口好
    QMqttClient *Client;//设置客户端，发送
    QMqttClient *recvClient;
    QString Topic;
    QTimer *VideoTimer;
    QByteArray getImageData2(const QImage &image);
    QImage getImage(const QString &data);



};
#endif // MAINWINDOW_H
