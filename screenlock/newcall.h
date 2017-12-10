#ifndef NEWCALL_H
#define NEWCALL_H

#include <QDialog>

namespace Ui {
class newCall;
}

class newCall : public QDialog
{
    Q_OBJECT

public:
    explicit newCall(QWidget *parent = 0);
    ~newCall();

private slots:

    void on_muteCallButton_clicked();

private:
    Ui::newCall *ui;
};

#endif // NEWCALL_H
