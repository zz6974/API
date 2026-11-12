#include "UI/cameraui.h"
#include "D:\Project\CPP_Porject\API\API\cmake-build-debug\autogen_files\include\ui_CameraUI.h"

CameraUI::CameraUI(QWidget *parent) :
    QWidget(parent), ui(new Ui::CameraUI) {
    ui->setupUi(this);
}

CameraUI::~CameraUI() {
    delete ui;
}
