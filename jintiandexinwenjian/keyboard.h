#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QMainWindow>
#include <QPushButton>
#include "mainwindow.h"
namespace Ui {
class Keyboard;  // 注意这里是 Keyboard，而不是 KeyBoard
}

class Keyboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Keyboard(QWidget *parent = nullptr);
    QString getLineEditText() const;  // 添加这个方法
    ~Keyboard();
signals:
    void closed();
private slots:

    // 0~9的函数声明
    void on_pushButton_0_clicked();
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    // a~z的函数声明
    void on_pushButton_a_clicked();
    void on_pushButton_b_clicked();
    void on_pushButton_c_clicked();
    void  on_pushButton_d_clicked();
    void  on_pushButton_e_clicked();
    void  on_pushButton_f_clicked();
    void  on_pushButton_g_clicked();
    void  on_pushButton_h_clicked();
    void  on_pushButton_i_clicked();
    void  on_pushButton_j_clicked();
    void  on_pushButton_k_clicked();
    void  on_pushButton_l_clicked();
    void  on_pushButton_m_clicked();
    void  on_pushButton_n_clicked();
    void  on_pushButton_o_clicked();
    void  on_pushButton_p_clicked();
    void  on_pushButton_q_clicked();
    void  on_pushButton_r_clicked();
    void  on_pushButton_s_clicked();
    void  on_pushButton_t_clicked();
    void  on_pushButton_u_clicked();
    void  on_pushButton_v_clicked();
    void  on_pushButton_w_clicked();
    void  on_pushButton_x_clicked();
    void  on_pushButton_y_clicked();
    void  on_pushButton_z_clicked();
    void on_pushButton_BIG_clicked();
    void on_pushButton_delete_clicked();
    void on_pushButton_ok_clicked();

private:
    Ui::Keyboard *ui;
    bool isUpperCase;  // 标志变量，用于记录当前大小写状态
    void insertLetter(const QString &letter);  // 辅助函数，用于插入字母

};

#endif // MAINWINDOW_H
