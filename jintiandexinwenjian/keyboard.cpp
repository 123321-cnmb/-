#include "keyboard.h"
#include "ui_keyboard.h"
#include <QMessageBox>


#include <QFileDialog>
Keyboard::Keyboard(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Keyboard)
{
    ui->setupUi(this);
    isUpperCase = false;  // 初始化为小写状态
}

Keyboard::~Keyboard()
{
    delete ui;
}


// 0~9的槽函数
void Keyboard::on_pushButton_0_clicked()
{
    ui->lineEdit->insert("0");
}

void  Keyboard::on_pushButton_1_clicked()
{
    ui->lineEdit->insert("1");
}

void  Keyboard::on_pushButton_2_clicked()
{
    ui->lineEdit->insert("2");
}

void  Keyboard::on_pushButton_3_clicked()
{
    ui->lineEdit->insert("3");
}

void  Keyboard::on_pushButton_4_clicked()
{
    ui->lineEdit->insert("4");
}

void  Keyboard::on_pushButton_5_clicked()
{
    ui->lineEdit->insert("5");
}

void  Keyboard::on_pushButton_6_clicked()
{
    ui->lineEdit->insert("6");
}

void  Keyboard::on_pushButton_7_clicked()
{
    ui->lineEdit->insert("7");
}

void  Keyboard::on_pushButton_8_clicked()
{
    ui->lineEdit->insert("8");
}

void  Keyboard::on_pushButton_9_clicked()
{
    ui->lineEdit->insert("9");
}

// a~z的槽函数
void  Keyboard::on_pushButton_a_clicked()
{
    insertLetter("a");
}

void  Keyboard::on_pushButton_b_clicked()
{
    insertLetter("b");
}

void  Keyboard::on_pushButton_c_clicked()
{
    insertLetter("c");
}

void  Keyboard::on_pushButton_d_clicked()
{
    insertLetter("d");
}

void  Keyboard::on_pushButton_e_clicked()
{
    insertLetter("e");
}

void  Keyboard::on_pushButton_f_clicked()
{
    insertLetter("f");
}

void  Keyboard::on_pushButton_g_clicked()
{
    insertLetter("g");
}

void  Keyboard::on_pushButton_h_clicked()
{
    insertLetter("h");
}

void  Keyboard::on_pushButton_i_clicked()
{
    insertLetter("i");
}

void  Keyboard::on_pushButton_j_clicked()
{
    insertLetter("j");
}

void  Keyboard::on_pushButton_k_clicked()
{
    insertLetter("k");
}

void  Keyboard::on_pushButton_l_clicked()
{
    insertLetter("l");
}

void  Keyboard::on_pushButton_m_clicked()
{
    insertLetter("m");
}

void  Keyboard::on_pushButton_n_clicked()
{
    insertLetter("n");
}

void  Keyboard::on_pushButton_o_clicked()
{
    insertLetter("o");
}

void  Keyboard::on_pushButton_p_clicked()
{
    insertLetter("p");
}

void  Keyboard::on_pushButton_q_clicked()
{
    insertLetter("q");
}

void  Keyboard::on_pushButton_r_clicked()
{
    insertLetter("r");
}

void  Keyboard::on_pushButton_s_clicked()
{
    insertLetter("s");
}

void  Keyboard::on_pushButton_t_clicked()
{
    insertLetter("t");
}

void  Keyboard::on_pushButton_u_clicked()
{
    insertLetter("u");
}

void  Keyboard::on_pushButton_v_clicked()
{
    insertLetter("v");
}

void  Keyboard::on_pushButton_w_clicked()
{
    insertLetter("w");
}

void  Keyboard::on_pushButton_x_clicked()
{
    insertLetter("x");
}

void  Keyboard::on_pushButton_y_clicked()
{
    insertLetter("y");
}

void  Keyboard::on_pushButton_z_clicked()
{
    insertLetter("z");
}

void  Keyboard::on_pushButton_BIG_clicked()
{
    isUpperCase = !isUpperCase;  // 切换大小写状态  // 根据按钮的 checked 状态更新 isUpperCase

    // 更新所有字母按钮的文本
    ui->pushButton_a->setText(isUpperCase ? "A" : "a");
    ui->pushButton_b->setText(isUpperCase ? "B" : "b");
    ui->pushButton_c->setText(isUpperCase ? "C" : "c");
    ui->pushButton_d->setText(isUpperCase ? "D" : "d");
    ui->pushButton_e->setText(isUpperCase ? "E" : "e");
    ui->pushButton_f->setText(isUpperCase ? "F" : "f");
    ui->pushButton_g->setText(isUpperCase ? "G" : "g");
    ui->pushButton_h->setText(isUpperCase ? "H" : "h");
    ui->pushButton_i->setText(isUpperCase ? "I" : "i");
    ui->pushButton_j->setText(isUpperCase ? "J" : "j");
    ui->pushButton_k->setText(isUpperCase ? "K" : "k");
    ui->pushButton_l->setText(isUpperCase ? "L" : "l");
    ui->pushButton_m->setText(isUpperCase ? "M" : "m");
    ui->pushButton_n->setText(isUpperCase ? "N" : "n");
    ui->pushButton_o->setText(isUpperCase ? "O" : "o");
    ui->pushButton_p->setText(isUpperCase ? "P" : "p");
    ui->pushButton_q->setText(isUpperCase ? "Q" : "q");
    ui->pushButton_r->setText(isUpperCase ? "R" : "r");
    ui->pushButton_s->setText(isUpperCase ? "S" : "s");
    ui->pushButton_t->setText(isUpperCase ? "T" : "t");
    ui->pushButton_u->setText(isUpperCase ? "U" : "u");
    ui->pushButton_v->setText(isUpperCase ? "V" : "v");
    ui->pushButton_w->setText(isUpperCase ? "W" : "w");
    ui->pushButton_x->setText(isUpperCase ? "X" : "x");
    ui->pushButton_y->setText(isUpperCase ? "Y" : "y");
    ui->pushButton_z->setText(isUpperCase ? "Z" : "z");
}
void  Keyboard::insertLetter(const QString &letter)
{
    if (isUpperCase) {
        ui->lineEdit->insert(letter.toUpper());
    } else {
        ui->lineEdit->insert(letter);
    }
}

void  Keyboard::on_pushButton_delete_clicked()
{
    ui->lineEdit->backspace();
}

void  Keyboard::on_pushButton_ok_clicked()
{
    emit closed();  // 发出关闭信号
    this->close();
}
QString Keyboard::getLineEditText() const
{
    return ui->lineEdit->text();  // 返回 lineEdit 中的文本
}
