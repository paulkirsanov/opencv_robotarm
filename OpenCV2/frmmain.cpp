#include "frmmain.h"
#include "ui_frmmain.h"

#include <QtCore>
#include <QMessageBox>

frmMain::frmMain(QWidget *parent) : QMainWindow(parent), ui(new Ui::frmMain)
{
    ui->setupUi(this);

    serialport = new QSerialPort(this);
    serialport->setPortName("COM3");
    serialport->setBaudRate(QSerialPort::Baud115200);
    serialport->setDataBits(QSerialPort::Data8);
    serialport->setParity(QSerialPort::NoParity);
    serialport->setStopBits(QSerialPort::OneStop);
    serialport->setFlowControl(QSerialPort::NoFlowControl);
    serialport->open(QIODevice::ReadWrite);

    if(serialport->isWritable() != true)
    {
        QMessageBox::information(this, "error", "error: serial port");
    }

    cap.open("D:/opencv/test/5.mov");
    if (cap.isOpened() != true)
    {
        QMessageBox::information(this, "error", "error: file doesn't find\n");
        exitProgram();
        return;
    }

    fout.open("data.txt", std::ios_base::trunc);
    if (!fout.is_open())
    {
            QMessageBox::information(this, "error", "error: file isn't open\n");
    }

    qtimer = new QTimer(this);
    connect(qtimer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdateGUI()));
    qtimer->start(20);
}

frmMain::~frmMain()
{
    delete ui;
}

void frmMain::processFrameAndUpdateGUI()
{
    cv::Mat imageHSV;
    cv::Mat threshold;
    cv::Mat src_resize;
    std::vector<std::vector<cv::Point>> contours;
    cv::Point2f vtx[4];

    int16_t current_value = 0;
    uint16_t unsigned_value = 0;

    bool blnFrameReadSuccessfully = cap.read(imgOriginal);

    if(imgOriginal.empty())
    {
        exitProgram();
        return;
    }
    else if(!blnFrameReadSuccessfully)
    {
        QMessageBox::information(this, "error", "unable to read from file \n\n exiting program\n");
        exitProgram();
        return;
    }

    cv::cvtColor(imgOriginal, imageHSV, cv::COLOR_BGR2HSV);
    cv::inRange(imageHSV, cv::Scalar(80, 40, 240), cv::Scalar(100, 80, 255), threshold);
//    cv::inRange(imageHSV, cv::Scalar(2, 1, 130), cv::Scalar(160, 40, 255), threshold);        //video 3
    cv::findContours(threshold, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    if(value == true)
    {
        for (size_t idx = 0; idx < contours.size(); idx++)
        {
            cv::RotatedRect minRect = cv::minAreaRect(contours[idx]);
            minRect.points(vtx);

            if (cv::contourArea(contours[idx], false) > 10000)
            {
                for (uint8_t i = 0; i < 4; i++)
                    line(imgOriginal, vtx[i], vtx[(i + 1) % 4], cv::Scalar(0, 0, 255), 1);

                cv::circle(imgOriginal, minRect.center, 5, cv::Scalar(0, 0, 255), 1);
                cv::putText(imgOriginal, cv::format("height: %0.1f", static_cast<double>(minRect.size.height *  0.2636)), cv::Point(150, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);
                cv::putText(imgOriginal, cv::format("width: %0.1f", static_cast<double>(minRect.size.width *  0.2636)), cv::Point(300, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);
                cv::putText(imgOriginal, cv::format("angle: %0.1f", static_cast<double>(minRect.angle)), cv::Point(10, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);
                uint16_t step = DegreeToStep(SIXTEENTH_STEP, static_cast<double>(minRect.angle));

                //cv::putText(imgOriginal, cv::format("step: %d", step), cv::Point(450, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);

                //cv::putText(imgOriginal, cv::format("Angle: %0.1f", width), cv::Point(650, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);

                int angle = qFloor((4.4305 + static_cast<double>(minRect.size.width *  0.2636)) / 0.7452);

                if(angle != old_angle)
                {
                    cv::putText(imgOriginal, cv::format("Angle: %d", angle), cv::Point(650, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0, 0, 255), 1, 1, false);

                    serialport->putChar(static_cast<char>(0xF9)); //START BIT
                    serialport->putChar(static_cast<char>(angle));
                    serialport->putChar(0x00);
                    serialport->putChar(0x00);
                    serialport->putChar(0x00);
                    serialport->putChar(0x00);
                    serialport->putChar(0x00);
                    serialport->putChar(0x00);
                    serialport->putChar(static_cast<char>(0xE0)); //STOP BIT
                    qDebug() << "Open";
                    old_angle = angle;
                    break;
                }

                if (step != old_step)
                {
                    current_value = static_cast<int16_t>(step - old_step);
                    cv::putText(imgOriginal, cv::format("step: %d", current_value), cv::Point(450, 20), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(255, 255, 255), 1, 1, false);

                    if (current_value > 0)
                    {
                        fout << current_value << "\t" << "direct CW" << std::endl;

                        /*if(serialport->isWritable() == true)
                        {
                            serialport->putChar(static_cast<char>(step & 0xFF));
                            serialport->putChar(static_cast<char>(step >> 8));
                        }*/
                    }
                    else
                    {
                        unsigned_value = 0xFFFF - (static_cast<uint16_t>(current_value) - 1);
                        fout << unsigned_value << "\t" << "direct CCW" << std::endl;

                        /*if(serialport->isWritable() == true)
                        {
                            //serialport->putChar(static_cast<char>(step & 0xFF));
                            //serialport->putChar(static_cast<char>(step >> 8));
                        }*/
                    }
                    old_step = step;
                }
            }
        }
    }

    QImage qimgOriginal = convertOpenCVMatToQtQImage(imgOriginal);
    ui->lblOriginal->setPixmap(QPixmap::fromImage(qimgOriginal));
}

void frmMain::exitProgram()
{
    if(qtimer->isActive()) qtimer->stop();
    fout.close();
    QApplication::quit();
}

QImage frmMain::convertOpenCVMatToQtQImage(cv::Mat mat)
{
    if(mat.channels() == 1) {                                   // if 1 channel (grayscale or black and white) image
            return QImage(static_cast<uchar*>(mat.data), mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Indexed8);     // return QImage
        } else if(mat.channels() == 3) {                            // if 3 channel color image
            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);                     // flip colors
            return QImage(static_cast<uchar*>(mat.data), mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);       // return QImage
        } else {
            qDebug() << "in convertOpenCVMatToQtQImage, image was not 1 channel or 3 channel, should never get here";
        }
        return QImage();        // return a blank QImage if the above did not work
}

void frmMain::on_btnPauseOrResume_clicked()
{
    if(qtimer->isActive() == true)
    {
        qtimer->stop();
        ui->btnPauseOrResume->setText(" Resume ");
    }
    else
    {
        qtimer->start(20);
        ui->btnPauseOrResume->setText(" Pause ");
    }
}

uint16_t frmMain::DegreeToStep(frmMain::MotorMode mode, double degree)
{
    return static_cast<uint16_t>((degree / 1.8) * mode * 2.5);
}

void frmMain::on_btnStart_clicked()
{
    value = true;
}

/*-----------------------------------------------------------------*/
/*
serialport->putChar(static_cast<char>(0xF8));                   //START BIT
serialport->putChar(0x01);                                      //Driver 1
serialport->putChar(0x52);                                      //Down
serialport->putChar(0x53);                                      //SIXTEENTH STEP
serialport->putChar(static_cast<char>(step & 0xFF));            //STEP
serialport->putChar(static_cast<char>(step >> 8));              //STEP
serialport->putChar(0x00);
serialport->putChar(0x00);
serialport->putChar(static_cast<char>(0xE0));                   //STOP BIT

serialport->putChar(static_cast<char>(0xF8));                   //START BIT
serialport->putChar(0x02);                                      //Driver 2
serialport->putChar(0x52);                                      //Down
serialport->putChar(0x53);                                      //SIXTEENTH STEP
serialport->putChar(static_cast<char>(step & 0xFF));            //STEP
serialport->putChar(static_cast<char>(step >> 8));              //STEP
serialport->putChar(0x00);
serialport->putChar(0x00);
serialport->putChar(static_cast<char>(0xE0));                   //STOP BIT

serialport->putChar(static_cast<char>(0xF8));                   //START BIT
serialport->putChar(0x03);                                      //Driver 3
serialport->putChar(0x52);                                      //Down
serialport->putChar(0x53);                                      //SIXTEENTH STEP
serialport->putChar(static_cast<char>(step & 0xFF));            //STEP
serialport->putChar(static_cast<char>(step >> 8));              //STEP
serialport->putChar(0x00);
serialport->putChar(0x00);
serialport->putChar(static_cast<char>(0xE0));                   //STOP BIT
*/
/*-----------------------------------------------------------------*/
