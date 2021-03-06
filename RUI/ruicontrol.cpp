#include "ruicontrol.h"
#include "ui_ruicontrol.h"
#include <QDebug>
#include <QMessageBox>

Controller::Controller(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Controller),
    m(NULL)
{
    ui->setupUi(this);
    m = RUIModel::GetInstance();
    m->SetRobotMode(RobotMode::MANUAL_MODE);

    QObject::connect(m, SIGNAL(UpdateRobotMode(RobotMode)), this, SLOT(RobotModeHandler(RobotMode)));
    QObject::connect(m, SIGNAL(UpdateRobotError(int)), this, SLOT(RobotErrorHandler(int)));
    QObject::connect(m, SIGNAL(UpdateRobotConnectionStatus(int)), this, SLOT(RobotConnectionHandler(int)));
    QObject::connect(m, SIGNAL(UpdateRobotDebugInfo(QString)), this, SLOT(RobotDebugInfoHandler(QString)));
    QObject::connect(m, SIGNAL(UpdateRobotInvalidDisconnection()), this, SLOT(RobotInvalidDisconnectionHandler()));

}

Controller::~Controller()
{
    delete ui;
}

// Robot -> RUI
// mode_changed
void Controller::RobotModeHandler(RobotMode mode)
{
    qDebug() << "robot mode is changed";
    // update robot mode to RUI

    switch(mode)
    {
        case RobotMode::AUTO_MODE:
            ui->auto_2->setChecked(true);
            ui->manual->setChecked(false);
            break;
        case RobotMode::MANUAL_MODE:
            ui->manual->setChecked(true);
            ui->auto_2->setChecked(false);
            break;
        case RobotMode::IDLE_MODE:
        case RobotMode::SUSPENDED_MODE:
        default:
            ui->auto_2->setChecked(false);
            ui->manual->setChecked(false);
             break;
    }
}

// error
void Controller::RobotErrorHandler(int error)
{
    QMessageBox msgBox;
    QString text;

    msgBox.setText("Autonomous mode is failed!!! Do you want to change Manual mode? ");

    switch(error)
    {
        case 1:
            text = "cause: robot cannot find red dot or road sign.";
            break;
        case 2:
            text = "cause: robot cannot recognize a road sign";
            break;
        case 3:
            text = "cause: robot is out of track";
            break;
        default:
            break;
    }

    msgBox.setInformativeText(text);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if(msgBox.exec() == QMessageBox::Yes)
    {
        // change to manual mode
        m->SetRobotMode(RobotMode::MANUAL_MODE);
        ui->manual->setChecked(true);
        ui->auto_2->setChecked(false);
    }
}

// connect, disconnect
void Controller::RobotConnectionHandler(int status)
{
    qDebug () << "Robot connection status" << status;

    QMessageBox msgBox;
    QString text;

    if(status) // connected
    {
        ui->status_green->setPixmap(QPixmap(":assets/bullet_green.png"));
        ui->status_green->show();

        text = "Robot is conneted!!!";
    }
    else // disconnected
    {
        ui->status_red->setPixmap(QPixmap(":assets/bullet_red.png"));
        ui->status_red->show();
        text = "Robot is disconnected!!!";
    }
    msgBox.setText(text);
    msgBox.exec();
}

// debug info
void Controller::RobotDebugInfoHandler(QString diag)
{
    ui->diag->setPlainText(diag);

}

// invalid disconnection
void Controller::RobotInvalidDisconnectionHandler()
{
    QMessageBox msgBox;

    msgBox.setText("Network connection is lost!!!");
    msgBox.setInformativeText("Please wait for connection re-establishment.");
    msgBox.exec();
}


// image
void Controller::timerEvent(QTimerEvent *event)
{
    //cv::Mat image;
    //int retvalue = m->GetImage(&image);

    if(m->IsEmpty())
    {
        qDebug() << "No Data";
        return;
    }

    cv::Mat image = m->GetImage();

    if(image.empty())
    {
        qDebug() << "Invalid Data";
        return;
    }

    ui->openCVviewer->showImage(image);
}


void Controller::on_start_toggled(bool checked)
{
    static int id = 0;

    if(checked)
    {
        m->SetImageOnOff(1);
        id = startTimer(20);
    }
    else
    {
        m->SetImageOnOff(0);
        killTimer(id);
    }
}

// robot operation
void Controller::on_go_pressed()
{
    m->HandleRobotForwardOperation(PRESS);
}

void Controller::on_go_released()
{
    m->HandleRobotForwardOperation(RELEASE);
}


void Controller::on_back_pressed()
{
    m->HandleRobotBackwardOperation(PRESS);
}

void Controller::on_back_released()
{
    m->HandleRobotBackwardOperation(RELEASE);
}


void Controller::on_right_pressed()
{
    m->HandleRobotRightOperation(PRESS);
}

void Controller::on_right_released()
{
    m->HandleRobotRightOperation(RELEASE);
}


void Controller::on_left_pressed()
{
    m->HandleRobotLeftOperation(PRESS);
}

void Controller::on_left_released()
{
    m->HandleRobotLeftOperation(RELEASE);
}


void Controller::on_uturn_clicked()
{
    m->HandleRobotUturnOperation();
}


// camera
void Controller::on_horizontalSlider_sliderMoved()
{
    m->HandlePanOperation();
}


void Controller::on_verticalSlider_sliderMoved()
{
    m->HandleTiltOperation();
}


// mode control
void Controller::on_auto_2_clicked(bool checked)
{
    if(checked)
        m->SetRobotMode(RobotMode::AUTO_MODE);
}

void Controller::on_manual_clicked(bool checked)
{
    if(checked)
        m->SetRobotMode(RobotMode::MANUAL_MODE);
}


void Controller::on_send_clicked()
{
    QString text;
    text = ui->command->text();

    m->SendTextInput(text);
}



