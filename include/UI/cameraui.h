//
// Created by ZhuanZ（无密码） on 2026/11/10.
//

#ifndef API_CAMERAUI_H
#define API_CAMERAUI_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class CameraUI; }
QT_END_NAMESPACE

class CameraUI : public QWidget {
Q_OBJECT

public:
    explicit CameraUI(QWidget *parent = nullptr);
    ~CameraUI() override;

private:
    Ui::CameraUI *ui;
};


#endif //API_CAMERAUI_H
