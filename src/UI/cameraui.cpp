//
// Created by ZhuanZ（无密码） on 2026/11/10.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CameraUI.h" resolved

#include "UI/cameraui.h"
#include "ui_CameraUI.h"


CameraUI::CameraUI(QWidget *parent) :
    QWidget(parent), ui(new Ui::CameraUI) {
    ui->setupUi(this);
}

CameraUI::~CameraUI() {
    delete ui;
}
