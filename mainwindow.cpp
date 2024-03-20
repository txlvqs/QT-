#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include "QCameraViewfinder"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //初始化摄像头
    camera_info = QCameraInfo::availableCameras();
    for(int i = 0;i<this->camera_info.count();i++)
    {
        qDebug()<<"可用摄像头:"<<this->camera_info[i].description();
        ui->comboBox->addItem(this->camera_info[i].description());
    }

    if(this->camera_info.count() <= 0 ||camera_info[0].isNull() )
    {
        QMessageBox::critical(this,"错误","没有摄像头");
    }

    camera = new QCamera(camera_info[0],this);
    camera->setCaptureMode(QCamera::CaptureStillImage);
    this->cap = new QCameraImageCapture(this->camera);
    QCameraViewfinder *v2 = new QCameraViewfinder(ui->widget);
    //v2->resize(ui->widget->width(),ui->widget->height());
    v2->resize(800,480);
    this->camera->setViewfinder(v2);
    v2->show();

    this->camera->start();
    connect(this->cap,&QCameraImageCapture::imageCaptured,this,[=](int id, const QImage &preview){
        qDebug()<<"发送该帧,该帧的大小为:"<<this->getImageData2(preview).size()<<"发送的主题为:"<<ui->lineEdit_3->text();
        this->Client->publish(ui->lineEdit_3->text(),this->getImageData2(preview),1);//用Qos1质量
        //this->Client->publish(ui->lineEdit_3->text(),"this->getImageData2(preview)");
    });



    //初始化MQTT通信
    this->Client = new QMqttClient(this);
    this->Client->setHostname("127.0.0.1");
    ui->lineEdit->setText("127.0.0.1");
    this->Client->setPort(1883);
    ui->lineEdit_2->setText("1883");
    this->Client->subscribe(QString("/video"));
    ui->lineEdit_3->setText("/video");
    this->Client->setClientId(QString::asprintf("sender"));
    ui->lineEdit_7->setText("sender");
    //绑定mqtt客户端的连接状态
    connect(this->Client,&QMqttClient::stateChanged,this,[=](QMqttClient::ClientState state){
        qDebug()<<"当前mqtt的连接状态为:"<<state;
        if(this->Client->state() == QMqttClient::Connected)
        {
            qDebug()<<"连接成功";
            ui->textEdit->insertPlainText("连接成功\n");
        }
        else if(this->Client->state() == QMqttClient::Connecting)
        {
            ui->textEdit->insertPlainText("正在连接\n");
            qDebug()<<"正在连接";
        }
        else if(this->Client->state() == QMqttClient::Disconnected)
        {
            ui->textEdit->insertPlainText("断开连接\n");
            qDebug()<<"断开连接";
        }
    });

    //是否断开连接
    connect(this->Client,&QMqttClient::disconnected,this,[=](){
        qDebug()<<"mqtt连接已经断开";
        ui->textEdit->insertPlainText("mqtt连接已经断开\n");
    });

    //接收消息
    connect(this->Client, &QMqttClient::messageReceived, this, [=](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" Received Topic: ")
                                + topic.name()
                                + QLatin1String(" Message: ")
                                + message
                                + QLatin1Char('\n');
        qDebug()<<"mqtt收到消息为:"<<content;
    });

    //视频采样定时器
    VideoTimer = new QTimer(this);
    this->VideoTimer->setInterval(1);//时间间隔为3ms
    connect(this->VideoTimer,&QTimer::timeout,this,[=](){
        this->VideoTimer->start(1);
        qDebug()<<"定时器正常运行,当前时间为:"<<QTime::currentTime().toString();
        this->cap->capture();
    });


    //MQTT接收的客户端
    recvClient = new QMqttClient();
    this->recvClient->setHostname("127.0.0.1");
    ui->lineEdit_4->setText("127.0.0.1");
    this->recvClient->setPort(1883);
    ui->lineEdit_5->setText("1883");
    this->recvClient->subscribe(QString("/video"));
    ui->lineEdit_6->setText("/video");
    this->recvClient->setClientId(QString::asprintf("recever1"));
    ui->lineEdit_8->setText("recever");
    //绑定mqtt客户端的连接状态
    connect(this->recvClient,&QMqttClient::stateChanged,this,[=](QMqttClient::ClientState state){
        qDebug()<<"当前mqtt的连接状态为:"<<state;
        if(this->recvClient->state() == QMqttClient::Connected)
        {
            qDebug()<<"接收端连接成功";
            ui->textEdit->insertPlainText("接收端连接成功\n");
            this->recvClient->subscribe(ui->lineEdit_6->text());
        }
        else if(this->recvClient->state() == QMqttClient::Connecting)
        {
            ui->textEdit->insertPlainText("接收端正在连接\n");
            qDebug()<<"接收端正在连接";
        }
        else if(this->recvClient->state() == QMqttClient::Disconnected)
        {
            ui->textEdit->insertPlainText("接收端断开连接\n");
            qDebug()<<"接收端断开连接";
        }
    });

    //是否断开连接
    connect(this->recvClient,&QMqttClient::disconnected,this,[=](){
        qDebug()<<"接收端mqtt连接已经断开";
        ui->textEdit->insertPlainText("接收端mqtt连接已经断开\n");
    });




    //接收端接收消息
    connect(this->recvClient, &QMqttClient::messageReceived, this, [=](const QByteArray &message, const QMqttTopicName &topic) {
        qDebug()<<"接收端收到的消息为:"<<message;
        QImage image = this->getImage(QString::fromLocal8Bit(message));
        //ui->widget_2
        QPixmap pix = QPixmap::fromImage(image);
        ui->label_10->setPixmap(pix);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{

}


void MainWindow::on_pushButton_clicked()//连接云服务器
{
    if (Client->state() == QMqttClient::Disconnected) {
        ui->textEdit->insertPlainText("连接中 " + QTime::currentTime().toString() + "\n");
        Client->setHostname(ui->lineEdit->text());
        Client->setPort(ui->lineEdit_2->text().toUInt());
        Client->subscribe(ui->lineEdit_3->text());
        Client->setClientId(ui->lineEdit_7->text());
        Client->connectToHost();
        this->VideoTimer->start(3);
    }
}


void MainWindow::on_pushButton_2_clicked()//断开连接
{
    if (Client->state() == QMqttClient::Connected) {
        this->VideoTimer->stop();
        ui->textEdit->insertPlainText("断开连接 " + QTime::currentTime().toString() + "\n");
        Client->disconnectFromHost();
    }
}

// 获取图片并转为base64字符串
QByteArray MainWindow::getImageData2(const QImage &image)
{
    QByteArray imageData;
    QBuffer buffer(&imageData);
    image.save(&buffer, "jpg");
    imageData = imageData.toBase64();
    return imageData;
}



//base64字符串转图片
QImage MainWindow::getImage(const QString &data)
{
    QByteArray imageData = QByteArray::fromBase64(data.toLatin1());
    QImage image;
    image.loadFromData(imageData);
    return image;
}



void MainWindow::on_pushButton_3_clicked()//视频读取连接
{
    if (this->recvClient->state() == QMqttClient::Disconnected) {
        ui->textEdit->insertPlainText("接收端连接中 " + QTime::currentTime().toString() + "\n");
        recvClient->setHostname(ui->lineEdit_4->text());
        recvClient->setPort(ui->lineEdit_5->text().toUInt());
        recvClient->subscribe(ui->lineEdit_6->text());
        recvClient->setClientId(ui->lineEdit_8->text());
        recvClient->connectToHost();
    }
}


void MainWindow::on_pushButton_4_clicked()//视频读取断开连接
{
    if (this->recvClient->state() == QMqttClient::Connected) {
        ui->textEdit->insertPlainText("接收端断开连接 " + QTime::currentTime().toString() + "\n");
        this->recvClient->disconnectFromHost();
    }
}




