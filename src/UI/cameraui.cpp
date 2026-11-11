#include "UI/cameraui.h"
#include "ui_CameraUI.h"


CameraUI::CameraUI(QWidget *parent) :
    QWidget(parent), ui(new Ui::CameraUI) {
    ui->setupUi(this);
}

CameraUI::~CameraUI() {
    delete ui;
}
