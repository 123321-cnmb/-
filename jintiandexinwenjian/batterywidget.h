#ifndef BATTERYWIDGET_H
#define BATTERYWIDGET_H
#include <QLabel>
#include <QWidget>
#include <QPainter>

class BatteryWidget : public QWidget {
public:
    explicit BatteryWidget(QWidget* parent = nullptr);  // 声明构造函数
    void setBatteryLevel(float level);  // 设置电量等级
    void paintEvent(QPaintEvent* event) override;  // 重写绘制事件
    void setChargeStatus(int status);
    int batteryLevel = 0;  // 当前电量等级，默认为0
    QLabel *chargeStatusLabel;  // 用于显示充电状态
};
#endif // BATTERYWIDGET_H
