#ifndef RUIMODEL_H
#define RUIMODEL_H

#include <queue>
#include <mutex>
#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <QString>
#include <QObject>

#include "imagereceivelistener.h"
#include "roboteventlistener.h"
#include "robotmode.h"

#include <QDebug>

#define PRESS 1
#define RELEASE 2

using namespace std;
using namespace yolo;

class RUIModel : public QObject
{

    Q_OBJECT

signals:
    void UpdateRobotMode(RobotMode mode);
    void UpdateRobotError(int error);
    void UpdateRobotConnectionStatus(int status);
    void UpdateRobotDebugInfo(QString debug);
    void UpdateRobotInvalidDisconnection();

public:
    explicit RUIModel(QObject *parent = 0);
    ~RUIModel();

    static RUIModel* GetInstance();

    virtual void HandleRobotForwardOperation(int operation);
    virtual void HandleRobotBackwardOperation(int operation);
    virtual void HandleRobotLeftOperation(int operation);
    virtual void HandleRobotRightOperation(int operation);
    virtual void HandleRobotUturnOperation();

    virtual void SetRobotMode(RobotMode mode);

    virtual void SetImageOnOff(int operation);
    virtual void HandlePanOperation();
    virtual void HandleTiltOperation();

    bool IsEmpty() const;
    int PushImage(cv::Mat&& image);
    cv::Mat GetImage();

    virtual void SendTextInput(QString string);
    void SetRobotHandler(int handle) { cur_robot_handle_ = handle; }

private:
    std::queue<cv::Mat> image_queue_;
    std::mutex image_mutex_;
    int cur_robot_handle_;

    class ImageReceiveListenerImp : public yolo::ImageReceiveListener {
        private:
            RUIModel* ruimodel_;

        public:
        ImageReceiveListenerImp(RUIModel* ruimodel) : ruimodel_(ruimodel) {

        }

        virtual void OnImageReceived(cv::Mat&& image) override
        {
            ruimodel_->PushImage(std::move(image));
        }
    };

    class RobotEventListenerImp : public yolo::RobotEventListener {
        private:
            RUIModel* ruimodel_;

        public:
        RobotEventListenerImp(RUIModel* ruimodel) : ruimodel_(ruimodel) {

        }

        virtual void OnRobotModeChanged(int handler, RobotMode mode)
        {
            qDebug() << "OnRobotModeChanged";
            qDebug() << "id : " << handler << " , mode : " << static_cast<int>(mode);
            emit ruimodel_->UpdateRobotMode(mode);
        }

         virtual void OnRobotErrorEventReceived(int handler, int error)
        {
            qDebug() << "OnRobotModeChanged";
            qDebug() << "id : " << handler << " , error : " << error;
            emit ruimodel_->UpdateRobotError(error);
        }

        virtual void OnRobotConnected(int handler)
        {
            qDebug() << "OnRobotConnected : " << handler;
            // update robot status on RUI
            ruimodel_->SetRobotHandler(handler);
            emit ruimodel_->UpdateRobotConnectionStatus(1);

        }

        virtual void OnRobotDisconnected(int handler)
        {
            qDebug() << "OnRobotDisconnected : " << handler;
            // update robot status on RUI
			ruimodel_->SetRobotHandler(-1);
            emit ruimodel_->UpdateRobotConnectionStatus(0);
        }

        virtual void OnRobotDebugInfoReceived(int handler, const std::string debug_info, int state)
        {
            qDebug() << "OnRobotDebugInfoReceived : ";
            qDebug() << "id : " << handler << ", debug_info : " << debug_info.c_str() << " , state : " << state;
            emit ruimodel_->UpdateRobotDebugInfo((QString)debug_info.c_str());
        }

        virtual void OnRobotInvalidDisconnected(int handler)
        {
            qDebug() << "OnRobotInvalidDisconnected : " << handler;
            emit ruimodel_->UpdateRobotInvalidDisconnection();

        }
    };

    std::shared_ptr<ImageReceiveListenerImp> image_receive_listener_impl_;
    std::shared_ptr<RobotEventListenerImp> robot_event_listener_impl_;
};

#endif // RUIMODEL_H
