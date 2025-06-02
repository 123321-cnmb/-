#include "BatteryWidget.h"

// 定义构造函数
BatteryWidget::BatteryWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(25, 100);  // 设置最小尺寸为垂直方向
}

// 定义设置电量等级的函数
void BatteryWidget::setBatteryLevel(float level) {
    // 将电量百分比映射到 0-5 的格子数量
    if (level <= 20) {
        batteryLevel = 1;
    } else if (level <= 40) {
        batteryLevel = 2;
    } else if (level <= 60) {
        batteryLevel = 3;
    } else if (level <= 80) {
        batteryLevel = 4;
    } else {
        batteryLevel = 5;
    }
    update();  // 触发重绘
}

// 重写绘制事件
void BatteryWidget::paintEvent(QPaintEvent* event) {
    (void)event;  // 避免未使用参数的警告
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制电池外壳
    painter.setBrush(Qt::gray);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));  // 留出边框

    // 绘制电量条
    int cellHeight = rect().height() / 5;  // 每个电量格的高度
    for (int i = 0; i < 5; ++i) {  // 总共绘制 5 个格子
        // 从上往下填充电量格
        if (i < batteryLevel - 1) {  // 已充满的格子显示绿色
            painter.setBrush(Qt::green);
        } else if (i == batteryLevel - 1) {  // 当前电量格
            if (batteryLevel == 1) {
                painter.setBrush(Qt::red);  // 电量最低时最上面的格子显示红色
            } else {
                painter.setBrush(Qt::green);  // 其他电量格显示绿色
            }
        } else {
            painter.setBrush(Qt::gray);  // 未充满的格子显示灰色
        }
        QRect cellRect(0, i * cellHeight, rect().width(), cellHeight);
        painter.drawRect(cellRect);
    }

    // 绘制分隔线
    painter.setPen(Qt::black);
    for (int i = 1; i < 5; ++i) {
        painter.drawLine(0, i * cellHeight, rect().width(), i * cellHeight);
    }
}

void BatteryWidget::setChargeStatus(int status) {
    if (status == 1) {
        chargeStatusLabel->setText("充电中");
        chargeStatusLabel->show();  // 确保显示
    } else {
        chargeStatusLabel->setText("");
        chargeStatusLabel->hide();  // 隐藏
    }
}
