#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <QtSerialPort/QSerialPort>
#include <fstream>

#include <QtMath>

namespace Ui {
class frmMain;
}

class frmMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit frmMain(QWidget *parent = nullptr);
    ~frmMain();

    typedef enum
    {
        FULL_STEP = 1,
        HALF_STEP = 2,
        QUARTER_STEP = 4,
        EIGHTH_STEP = 8,
        SIXTEENTH_STEP = 16
    } MotorMode;

public slots:
    void processFrameAndUpdateGUI();
    //void showValue();

private slots:
    void on_btnPauseOrResume_clicked();

    void on_btnStart_clicked();

private:
    Ui::frmMain *ui;
    cv::VideoCapture cap;
    cv::Mat imgOriginal;
    QSerialPort* serialport;
    QTimer* qtimer;
    bool value = false;
    uint16_t old_step = 0;
    int old_angle = 0;

    std::ofstream fout;

    QImage convertOpenCVMatToQtQImage(cv::Mat mat);
    void exitProgram();
    uint16_t DegreeToStep(MotorMode mode, double degree);
};

#endif // FRMMAIN_H
