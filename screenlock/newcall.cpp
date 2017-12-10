#include "newcall.h"
#include "ui_newcall.h"

newCall::newCall(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::newCall)
{
    ui->setupUi(this);
}

newCall::~newCall()
{
    delete ui;
}

void newCall::on_muteCallButton_clicked()
{
    this->close();
}
