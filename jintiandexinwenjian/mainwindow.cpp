#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
float level = 0.0;

//更新日期和时间
void MainWindow::updateDateTime()
{
    initialDate = QDate(year, month, date);  // 自定义初始日期
    initialTime = QTime(hour, min, sec);     // 自定义初始时间（午夜开始）
    QTime currentTime = initialTime.addSecs(elapsedTime);
    QString dateTimeText = initialDate.toString("  yyyy-M-d") + " " + currentTime.toString("hh:mm:ss");
    dateTimeLabel->setText(dateTimeText);
    elapsedTime++;
}

//更新电池
void MainWindow::updatePower()
{
    serialPort.write("cmd_power_bat\r\n");
    BatteryWidget* batteryWidget = new BatteryWidget(this);
    batteryWidget->setGeometry(736, 370, 30, 10);  // 设置电池图标的位置和大小
    batteryWidget->setBatteryLevel(level);  // 设置电量等级
    if (level < 10) {
        chargeStatusLabel->setText(" 电量不足");
        chargeStatusLabel->show();
    }
    else {

        if (chargeStatus == 1)
        {
            chargeStatusLabel->setText(" 充电中");
            chargeStatusLabel->show();
        }
        else
        {
            chargeStatusLabel->setText("");
            chargeStatusLabel->hide();
        }
    }
    batteryWidget->show();  // 确保 batteryWidget 被显示
}

// 创建按钮
QPushButton* MainWindow::createRotatedButton(QGraphicsScene* scene, const QString& text, int xPos, int yPos, int rotationAngle)
{
    QPushButton* button = new QPushButton(text);
    button->setFixedSize(110, 50);
    button->setStyleSheet(
                "QPushButton {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #2196F3, stop:1 #0d8bf2);"
                "   color: white;"
                "   border-radius: 5px;"
                "   padding: 0px;"
                "   border: 1px solid #0a7bda;"
                "}"
                "QPushButton:hover {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #42A5F5, stop:1 #1E88E5);"
                "}"
                "QPushButton:pressed {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #0d8bf2, stop:1 #2196F3);"
                "}"
                );
    // 设置字体 - 加粗，12pt
    QFont font = button->font();
    font.setPointSize(9);
    font.setBold(true);
    button->setFont(font);
    // 将按钮添加到场景中并设置位置和旋转
    QGraphicsProxyWidget* proxy = scene->addWidget(button);
    proxy->setPos(xPos, yPos);
    proxy->setTransform(QTransform().rotate(rotationAngle));
    return button;
}

// 创建标签
QLabel* MainWindow::createRotatedLabel(QGraphicsScene* scene, const QString& text, int xPos, int yPos, int rotationAngle, const QColor& textColor, int fontSize, QFont::Weight fontWeight)
{
    QLabel* label = new QLabel(text);
    // 设置标签样式
    label->setStyleSheet(QString("color: %1;").arg(textColor.name()));
    label->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    // 设置字体
    QFont font = label->font();
    font.setPointSize(fontSize);
    font.setWeight(fontWeight);
    label->setFont(font);
    // 设置文字对齐方式
    label->setAlignment(Qt::AlignCenter);
    // 将标签添加到场景中并设置位置和旋转
    QGraphicsProxyWidget* proxy = scene->addWidget(label);
    proxy->setPos(xPos, yPos);
    proxy->setTransform(QTransform().rotate(rotationAngle));
    return label;
}

void MainWindow::initializeModbusSerialPort()
{
    // 获取所有可用的串口
    QStringList availablePorts;
    foreach(const QSerialPortInfo & serialPortInfo, QSerialPortInfo::availablePorts()) {
        QString portName = serialPortInfo.portName();
        availablePorts.append(portName);
    }

    // 如果没有可用的串口，输出错误信息
    if (availablePorts.isEmpty()) {
        qDebug() << "No available serial ports.";
        return;
    }

    // 弹出对话框让用户选择串口
    bool ok;
    QString selectedPort = QInputDialog::getItem(this, "Select Serial Port", "Available Ports:", availablePorts, 0, false, &ok);
    if (ok && !selectedPort.isEmpty()) {
        modbusSerialPort.setPortName(selectedPort);
    }
    else {
        qDebug() << "User cancelled the selection.";
        return;
    }

    // 配置串口参数
    modbusSerialPort.setBaudRate(QSerialPort::Baud115200); // 设置波特率为115200
    modbusSerialPort.setDataBits(QSerialPort::Data8); // 设置数据位为8位
    modbusSerialPort.setParity(QSerialPort::NoParity); // 设置无校验位
    modbusSerialPort.setStopBits(QSerialPort::OneStop); // 设置停止位为1位
    modbusSerialPort.setFlowControl(QSerialPort::NoFlowControl); // 设置无流控制

    // 尝试打开串口
    if (!modbusSerialPort.open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open Modbus serial port:" << modbusSerialPort.errorString();
        return;
    }

    qDebug() << "Modbus serial port opened successfully.";
    connect(&modbusSerialPort, &QSerialPort::readyRead, this, &MainWindow::readModbusSerialData);
}
void MainWindow::readModbusSerialData()
{
    QByteArray newDatartu = modbusSerialPort.readAll();
    rtudataBuffer.append(newDatartu);  // 将新数据追加到缓冲区
    qDebug() << "Received ASCII data:" << rtudataBuffer;

    // 检查缓冲区中是否包含完整的报文（以 \r\n 结尾）
    while (rtudataBuffer.contains("\r\n")) {
        // 找到第一个 \r\n 的位置
        int endIndex = rtudataBuffer.indexOf("\r\n");

        // 提取完整的报文（不包括 \r\n）
        QByteArray completeMessage = rtudataBuffer.left(endIndex);
        rtudataBuffer = rtudataBuffer.mid(endIndex + 2); // 去掉已处理的报文和 \r\n

        // 将 ASCII 数据（十六进制字符串）转换为二进制数据
        // QByteArray binaryData;
        QString hexString = QString::fromLatin1(completeMessage).trimmed(); // 去掉首尾空格
        hexString.remove(QRegExp("[^0-9A-Fa-f]")); // 去掉所有非十六进制字符

        for (int i = 0; i < hexString.size(); i += 2) {
            QString hexPair = hexString.mid(i, 2);
            bool ok;
            quint8 byteValue = hexPair.toUInt(&ok, 16);
            if (ok) {
                binaryData.append(byteValue);
            }
        }

        qDebug() << "Converted binary data:" << binaryData.toHex();

        // 处理二进制数据
        if (binaryData.size() >= 4) { // 最小帧长
            // 检查CRC - 修正字节顺序（Modbus CRC是低字节在前）
            quint8 crcLow = static_cast<quint8>(binaryData.at(binaryData.size() - 2));
            quint8 crcHigh = static_cast<quint8>(binaryData.at(binaryData.size() - 1));
            quint16 receivedCrc = (crcHigh << 8) | crcLow;
            quint16 calculatedCrc = calculateCRC(binaryData.left(binaryData.size() - 2));

            qDebug() << "CRC check - Received:" << QString::number(receivedCrc, 16)
                     << "Calculated:" << QString::number(calculatedCrc, 16);

            if (receivedCrc != calculatedCrc) {
                qDebug() << "CRC mismatch, expected:" << QString::number(calculatedCrc, 16)
                         << "received:" << QString::number(receivedCrc, 16);
                continue; // 跳过当前报文，处理下一个
            }

            quint8 deviceAddress = static_cast<quint8>(binaryData[0]);
            quint8 functionCode = static_cast<quint8>(binaryData[1]);

            // 根据功能码分发处理
            switch (functionCode) {
            case 0x03:
                if (binaryData.size() >= 8) { // 03请求帧固定8字节
                    processReadHoldingRegistersRTU(deviceAddress, functionCode);
                }
                break;
            case 0x10:
                if (binaryData.size() >= (9 + binaryData[6])) { // 10请求帧变长
                    processWriteMultipleRegistersRTU(deviceAddress);
                }
                break;
            default:
                qDebug() << "Unknown function code:" << QString::number(functionCode, 16);
            }
        }
        else {
            qDebug() << "Incomplete frame received";
        }
    }
}
void MainWindow::processReadHoldingRegistersRTU(quint8 deviceAddress, quint8 functionCode)
{
    if (binaryData.size() < 8) { // Modbus-RTU读保持寄存器请求至少需要8字节
        qDebug() << "Insufficient data for Read Holding Registers request.";
        return;
    }
    quint16 startAddress = static_cast<quint16>(binaryData[2] << 8 | binaryData[3]);
    quint16 quantity = static_cast<quint16>(binaryData[4] << 8 | binaryData[5]);

    qDebug() << "Read Holding Registers Request:"
             << "Device Address:" << deviceAddress
             << "Function Code:" << functionCode
             << "Start Address:" << QString::number(startAddress, 16)
             << "Quantity:" << quantity;

    // 检查寄存器范围是否有效
    if (startAddress + quantity > 0x1000) { // 假设寄存器地址范围为0x0000到0x0FFF
        qDebug() << "Invalid register range.";
        binaryData.clear(); // 清空缓冲区
        return;
    }

    // 准备响应数据
    QByteArray response;
    response.append(static_cast<char>(deviceAddress)); // 设备地址
    response.append(static_cast<char>(functionCode)); // 功能码
    quint8 byteCount = quantity * 2; // 每个寄存器2字节
    response.append(static_cast<char>(byteCount)); // 字节计数
    for (quint16 i = 0; i < quantity; ++i) {
        quint16 address = startAddress + i;
        quint16 value = getRegisterValue(address); // 从寄存器中读取值
        response.append(static_cast<char>(value >> 8)); // 高字节
        response.append(static_cast<char>(value & 0xFF)); // 低字节


    }
    // 添加CRC校验码
    quint16 crc = calculateCRC(response);
    response.append(static_cast<char>(crc >> 8)); // CRC高字节
    response.append(static_cast<char>(crc & 0xFF)); // CRC低字节

    // 输出响应数据
    qDebug() << "Response for Read Holding Registers:" << response.toHex();

    modbusSerialPort.write(response+ "\r\n");
    modbusSerialPort.flush();
    binaryData.clear(); // 清空缓冲区
}
void MainWindow::processWriteMultipleRegistersRTU(quint8 deviceAddress)
{
    if (binaryData.size() < 10) { // Modbus-RTU写多个寄存器请求至少需要10字节
        qDebug() << "Insufficient data for Write Multiple Registers request." << binaryData;
        binaryData.clear(); // 清空缓冲区
        return;
    }

    // 解析起始地址和寄存器数量
    // 修正地址解析：确保正确读取16位无符号数
    quint16 startAddress = (static_cast<quint16>(binaryData[2]) << 8) | (static_cast<quint16>(binaryData[3]) & 0xFF);
    quint16 quantity = (static_cast<quint16>(binaryData[4]) << 8) | (static_cast<quint16>(binaryData[5]) & 0xFF);
    quint8 byteCount = binaryData[6];

    qDebug() << "Write Multiple Registers Request:"
             << "Device Address:" << deviceAddress
             << "Start Address:" << QString::number(startAddress, 16)
             << "Quantity:" << quantity
             << "Byte Count:" << byteCount;

    // 解析寄存器值
    for (quint16 i = 0; i < quantity; ++i) {
        quint16 address = startAddress + i;
        quint16 value = static_cast<quint16>(binaryData[7 + i * 2] << 8 | binaryData[8 + i * 2]);
        setRegisterValueU16(address, value); // 更新寄存器值

        qDebug() << "Writing to Register Address:" << QString::number(address, 16)
                 << "Value:" << QString::number(value, 16);
    }

    // 检查寄存器0x00A0到0x00A3的值
    registerMap[0x0501] = 0x0001;


    // 检查寄存器0x00A0到0x00A3的值
    quint16 commandA0 = registerMap.value(0x00A0, 0xFFFF);
    quint16 commandA1 = registerMap.value(0x00A1, 0xFFFF);
    // 握手命令
    if (commandA0 == 0xAABB && commandA1 == 0xCCDD) {
        qDebug() << "执行握手命令";
    }

    // 插座识别号命令
    if (commandA0 == 0x0000 && commandA1) {
        qDebug() << "执行插座识别号命令";
        registerMap[0x0001] = registerMap[0x00A1];
    }
    // 表笔（ac）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        acButton->click();
    }
    // 表笔（dc）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0001) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        dcButton->click();

    }
    // 表笔（res）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0002) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();

    }

    // 表笔（Current）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0003) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        qDebug() << "执行表笔（Current）命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        currentButton->click();

    }

    // 表笔（mRes）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0004) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        qDebug() << "执行表笔（mRes）命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        mresButton->click();

    }

    // 插座命令
    if (commandA0 == 0x0200 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令1p2w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S12Button->click();


    }
    if (commandA0 == 0x0200 && commandA1 == 0x0001) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令1p3w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S13Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0002) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p3w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S33Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0003) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p3w+g";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S33gButton->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0004) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p4w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S34Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0005) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p4w+g";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S34gButton->click();

    }

    // 温湿度检测命令win1
    if (registerMap[0x00A0] == 0x0300 && registerMap[0x00A1] == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);

    }

    // 光照检测命令win3
    if (commandA0 == 0x0301 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行光照检测命令";
        mainTabWidget->setCurrentIndex(1);

    }

    // 噪声分析命令
    if (commandA0 == 0x0302 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行噪声分析命令";
        mainTabWidget->setCurrentIndex(1);

    }

    // 可燃气检测命令
    if (commandA0 == 0x0303 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行可燃气检测命令";
        mainTabWidget->setCurrentIndex(1);

    }

    // 红外测温命令win2
    if (commandA0 == 0x0304 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_sensor_gdir\r\n");
        qDebug() << "执行红外测温命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);
        environmentSubTabWidget->setCurrentIndex(2);


    }
    // 大气压检测
    if (commandA0 == 0x0305 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_sensor_cps171\r\n");
        qDebug() << "执行大气压检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);
        environmentSubTabWidget->setCurrentIndex(1);
    }

    // 二极管
    if (commandA0 == 0x0306 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_probe_diode\r\n");
        qDebug() << "执行二极管检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();
        diodeButton->click();
    }

    // 通断
    if (commandA0 == 0x0307 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_probe_short\r\n");
        qDebug() << "执行通断检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();
        continuityButton->click();
    }

    // 24V电压输出控制命令
    if (commandA0 == 0x0400 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_power_24von\r\n");
        setGPIO7071State11();
        qDebug() << "执行24V电压输出控制命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(2);
        V24Button->click();

    }
    // 准备响应数据
    QByteArray response;
    response.append(static_cast<char>(deviceAddress)); // 设备地址
    response.append(static_cast<char>(0x10)); // 功能码
    response.append(static_cast<char>(startAddress >> 8)); // 起始地址高字节
    response.append(static_cast<char>(startAddress & 0xFF)); // 起始地址低字节
    response.append(static_cast<char>(quantity >> 8)); // 寄存器数量高字节
    response.append(static_cast<char>(quantity & 0xFF)); // 寄存器数量低字节

    // 添加CRC校验码
    quint16 crc = calculateCRC(response);
    response.append(static_cast<char>(crc >> 8)); // CRC高字节
    response.append(static_cast<char>(crc & 0xFF)); // CRC低字节

    // 发送响应数据
    modbusSerialPort.write(response+ "\r\n");
    modbusSerialPort.flush();

    //  qDebug() << "Response sent for Write Multiple Registers:" << response.toHex();

    // 清空缓冲区
    binaryData.clear();
}

quint16 MainWindow::calculateCRC(const QByteArray& data)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<quint8>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void MainWindow::onBluetoothButtonClicked()
{
    if (bluetoothButton->property("selected").toBool()) {
        // 如果蓝牙按钮已经被选中，取消选中
        bluetoothButton->setProperty("selected", false);
        bluetoothButton->setStyleSheet("QPushButton { "
                                       "background-color: transparent; "
                                       "color: black; "
                                       "font-size: 16px; "
                                       "font-weight: bold; "
                                       "border: 2px solid black; "

                                       "}"
                                       "QPushButton:hover { "
                                       "background-color: lightgray; "
                                       "}"
                                       "QPushButton:pressed { "
                                       "background-color: darkgray; "
                                       "}");
        setGPIO7071State10();
        constylevalTimeLabel->setText("");
        ipLineEdit->hide();
        ipLabel->hide();
    }
    else {
        // 如果蓝牙按钮未被选中，选中它并取消网口按钮的选中
        bluetoothButton->setProperty("selected", true);
        bluetoothButton->setStyleSheet("QPushButton { "
                                       "background-color: lightblue; "
                                       "color: white; "
                                       "font-size: 16px; "
                                       "font-weight: bold; "
                                       "border: 2px solid black; "
                                       "}"
                                       "QPushButton:hover { "
                                       "background-color: lightgray; "
                                       "}"
                                       "QPushButton:pressed { "
                                       "background-color: darkgray; "
                                       "}");
        ethernetButton->setProperty("selected", false);
        ethernetButton->setStyleSheet("QPushButton { "
                                      "background-color: transparent; "
                                      "color: black; "
                                      "font-size: 16px; "
                                      "font-weight: bold; "
                                      "border: 2px solid black; "

                                      "}"
                                      "QPushButton:hover { "
                                      "background-color: lightgray; "
                                      "}"
                                      "QPushButton:pressed { "
                                      "background-color: darkgray; "
                                      "}");
        initializeModbusSerialPort(); // 初始化新的Modbus串口
        constylevalTimeLabel->setText("蓝牙");
        ipLineEdit->hide();
        ipLabel->hide();
        setGPIO7071State01();
    }
}

void MainWindow::onEthernetButtonClicked()
{
    if (ethernetButton->property("selected").toBool()) {
        // 如果网口按钮已经被选中，取消选中
        ethernetButton->setProperty("selected", false);
        ethernetButton->setStyleSheet("QPushButton { "
                                      "background-color: transparent; "
                                      "color: black; "
                                      "font-size: 16px; "
                                      "font-weight: bold; "
                                      "border: 2px solid black; "

                                      "}"
                                      "QPushButton:hover { "
                                      "background-color: lightgray; "
                                      "}"
                                      "QPushButton:pressed { "
                                      "background-color: darkgray; "
                                      "}");
        setGPIO7071State10();
        constylevalTimeLabel->setText("");
        ipLineEdit->hide();
        ipLabel->hide();
    }
    else {
        // 如果网口按钮未被选中，选中它并取消蓝牙按钮的选中
        ethernetButton->setProperty("selected", true);
        ethernetButton->setStyleSheet("QPushButton { "
                                      "background-color: lightblue; "
                                      "color: white; "
                                      "font-size: 16px; "
                                      "font-weight: bold; "
                                      "border: 2px solid black; "

                                      "}"
                                      "QPushButton:hover { "
                                      "background-color: lightgray; "
                                      "}"
                                      "QPushButton:pressed { "
                                      "background-color: darkgray; "
                                      "}");
        bluetoothButton->setProperty("selected", false);
        bluetoothButton->setStyleSheet("QPushButton { "
                                       "background-color: transparent; "
                                       "color: black; "
                                       "font-size: 16px; "
                                       "font-weight: bold; "
                                       "border: 2px solid black; "

                                       "}"
                                       "QPushButton:hover { "
                                       "background-color: lightgray; "
                                       "}"
                                       "QPushButton:pressed { "
                                       "background-color: darkgray; "
                                       "}");
        constylevalTimeLabel->setText("网口");
        ipLineEdit->hide();
        ipLabel->hide();
        enterModbusTcpMode(); // 启动TCP服务器
    }
}

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
  , gpioTimer(new QTimer(this))  // 初始化gpioTimer
  , powerTimer(new QTimer(this))
  , sensorTimer(new QTimer(this))
  , airconditionTimer(new QTimer(this))
  , infraredTimer(new QTimer(this))
  , probeTimer(new QTimer(this))
  , socketTimer(new QTimer(this))
  , constantVoltageTimer(new QTimer(this))
{
    ui->setupUi(this);
    resize(800, 480);
    // 创建 QGraphicsScene 和 QGraphicsView
    QGraphicsScene* scene0 = new QGraphicsScene(this);
    QGraphicsView* view0 = new QGraphicsView(scene0, this);//view0是日期等旋转布局
    view0->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    view0->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    view0->setFixedSize(200, 480); // 设置视图大小为800x480
    view0->move(640, 0);
    // 设置 QGraphicsView 的样式以去除边框
    view0->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );
    // 旋转整个场景90度
    QTransform transform0;
    transform0.rotate(90);
    view0->setTransform(transform0);
    // 初始化初始时间和日期
    year = 2025;
    month = 5;
    date = 10;
    hour = 10;
    min = 10;
    sec = 10;
    initialDate = QDate(year, month, date);  // 自定义初始日期
    initialTime = QTime(hour, min, sec);     // 自定义初始时间（午夜开始）
    dateTimeLabel = new QLabel();
    dateTimeLabel->setStyleSheet("QLabel { background: transparent; color: black; font-size: 25px; }");
    connectLabel = new QLabel("连接状态:");// 连接状态：
    connectLabel->setStyleSheet("QLabel { background: transparent; color: black; font-size: 18px; }");
    connectvalueTimeLabel = new QLabel("正常"); // 正常/异常
    connectvalueTimeLabel->setStyleSheet("QLabel { background: transparent; color: black; font-size: 18px; }");
    constyleLabel = new QLabel("通信方式:");// 连接方式：
    constyleLabel->setStyleSheet("QLabel { background: transparent; color: black; font-size: 18px; }");
    constylevalTimeLabel = new QLabel("   "); // 网口/蓝牙
    constylevalTimeLabel->setStyleSheet("QLabel { background: transparent; color: black; font-size: 18px; }");
    constylevalTimeLabel->setFixedSize(140, 30); // 设置默认大小为宽度100，高度30
    // 初始化日期和时间显示
    updateDateTime();
    // 定时更新日期和时间
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000); // 每秒更新一次
    // 初始化寄存器0x0001到0x500为999999
    for (quint16 address = 0x0001; address <= 0x501; ++address) {
        registerMap[address] = 999999;
    }
    // 将时间标签添加到 QGraphicsScene 中
    QGraphicsProxyWidget* proxyDateTimeLabel = scene0->addWidget(dateTimeLabel);
    proxyDateTimeLabel->setPos(0, 0); // 时间
    QGraphicsProxyWidget* proxyconnectLabel = scene0->addWidget(connectLabel);
    proxyconnectLabel->setPos(250, 45); // 连接状态
    QGraphicsProxyWidget* proxyconnectvalueTimeLabel = scene0->addWidget(connectvalueTimeLabel);
    proxyconnectvalueTimeLabel->setPos(365, 45); // 正常/异常
    QGraphicsProxyWidget* proxyconstyleLabel = scene0->addWidget(constyleLabel);
    proxyconstyleLabel->setPos(50, 45); // 连接方式
    QGraphicsProxyWidget* proxyconstylevalTimeLabel = scene0->addWidget(constylevalTimeLabel);
    proxyconstylevalTimeLabel->setPos(165, 45); // 网口/蓝牙

    // 创建电量标签
    QLabel* chargeStatusLabel = new QLabel("");
    chargeStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chargeStatusLabel->setFixedSize(250, 80); // 设置标签大小
    chargeStatusLabel->setStyleSheet("QLabel {"
                                     "color: red;"
                                     "background-color: transparent;"
                                     "font: system;"
                                     "font: 12pt;"
                                     "}");
    QGraphicsProxyWidget* proxychargeStatusLabel = scene0->addWidget(chargeStatusLabel);
    proxychargeStatusLabel->setPos(260, -30); // 根据需要调整位置
    // 启动定时器，每100ms读取一次power状态
    connect(powerTimer, &QTimer::timeout, this, &MainWindow::updatePower);
    // 连接按钮的点击信号到槽函数
    powerTimer->start(5000);  // 每5s读取一次电池状态
    // 初始化串口参数
    serialPort.setBaudRate(QSerialPort::Baud115200);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setFlowControl(QSerialPort::NoFlowControl);

    // 自动弹出串口选择对话框并尝试连接
    connectSerialPort();
    // 断开当前窗口的串口连接
    disconnectSerialPort();
    serialPort.write("cmd_power_bat\r\n");
    // 重新连接串口到主窗口
    connect(&serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    // 创建主窗口容器
    QWidget* mainContainer = new QWidget(this);
    mainContainer->resize(680, 480);
    mainContainer->move(0, 0);
    // 创建主标签页控件
    mainTabWidget = new QTabWidget(mainContainer);
    // 创建设置主标签页
    QWidget* settingsTabPage = new QWidget();
    QVBoxLayout* settingsTabLayout = new QVBoxLayout(settingsTabPage);
    QGraphicsScene* settingsScene = new QGraphicsScene(this);
    QGraphicsView* settingsView = new QGraphicsView(settingsScene, settingsTabPage);
    settingsView->setStyleSheet("background-color: transparent;");
    settingsScene->setSceneRect(0, 0, 520, 340);
    settingsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    settingsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    // 设置 QGraphicsView 的样式以去除边框
    settingsView->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );

    // 添加按钮
    BIButton = createRotatedButton(settingsScene, "连接", 200, 100, 90);
    connect(BIButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);

    // 添加文本标签
    createRotatedLabel(settingsScene, "通信方式", 300, 0, 90, Qt::black, 12, QFont::Normal);

    ipLabel = createRotatedLabel(settingsScene, "IP地址", 250, 50, 90, Qt::black, 12, QFont::Normal);
    ipLabel->hide();

    // 添加蓝牙按钮
    bluetoothButton = createRotatedButton(settingsScene, "蓝牙", 320, 100, 90);
    bluetoothButton->setProperty("selected", false);
    bluetoothButton->setStyleSheet("QPushButton { "
                                   "background-color: transparent; "
                                   "color: black; "
                                   "font-size: 16px; "
                                   "font-weight: bold; "
                                   "border: 2px solid black; "
                                   "}"
                                   "QPushButton:hover { "
                                   "background-color: lightgray; "
                                   "}"
                                   "QPushButton:pressed { "
                                   "background-color: darkgray; "
                                   "}");
    connect(bluetoothButton, &QPushButton::clicked, this, &MainWindow::onBluetoothButtonClicked);

    // 添加网口按钮
    ethernetButton = createRotatedButton(settingsScene, "网口", 320, 250, 90);
    ethernetButton->setProperty("selected", false);
    ethernetButton->setStyleSheet("QPushButton { "
                                  "background-color: transparent; "
                                  "color: black; "
                                  "font-size: 16px; "
                                  "font-weight: bold; "
                                  "border: 2px solid black; "
                                  "}"
                                  "QPushButton:hover { "
                                  "background-color: lightgray; "
                                  "}"
                                  "QPushButton:pressed { "
                                  "background-color: darkgray; "
                                  "}");
    connect(ethernetButton, &QPushButton::clicked, this, &MainWindow::onEthernetButtonClicked);


    // 添加文本框192.168.1.10
    ipLineEdit = new QLineEdit("");
    QGraphicsProxyWidget* proxyIPLineEdit = settingsScene->addWidget(ipLineEdit);
    proxyIPLineEdit->setRotation(90);
    proxyIPLineEdit->setPos(250, 150);
    // 连接文本框的 clicked 信号到槽函数
    connect(ipLineEdit, &QLineEdit::editingFinished, this, &MainWindow::showKeyboard);
    settingsTabLayout->addWidget(settingsView);
    settingsTabPage->setLayout(settingsTabLayout);
    mainTabWidget->addTab(settingsTabPage, "    设置    ");//设置的标题


    // 创建环境参数主标签页
    QWidget* environmentTabPage = new QWidget();
    environmentSubTabWidget = new QTabWidget();
    environmentSubTabWidget->setTabPosition(QTabWidget::East);
    // 创建环境参数子标签页1
    QWidget* environmentSubPage1 = new QWidget();
    QVBoxLayout* environmentSubLayout1 = new QVBoxLayout(environmentSubPage1);
    QGraphicsScene* environmentScene1 = new QGraphicsScene(this);
    QGraphicsView* environmentView1 = new QGraphicsView(environmentScene1, environmentSubPage1);
    environmentView1->setStyleSheet("background-color: transparent;");
    environmentScene1->setSceneRect(0, 0, 520, 340);
    environmentView1->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    environmentView1->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    environmentView1->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );
    // 创建测量值标签


    gasValueLabel = new QLabel("");
    temperatureValueLabel = new QLabel("");
    humidityValueLabel = new QLabel("");
    lightValueLabel = new QLabel("");
    noisedbValueLabel = new QLabel("");
    noisehzValueLabel = new QLabel("");
    noisehzValueLabel2 = new QLabel("");
    noisehzValueLabel3 = new QLabel("");
    percentnoisehzValueLabel = new QLabel("");
    percentnoisehzValueLabel2 = new QLabel("");
    percentnoisehzValueLabel3 = new QLabel("");

    gdirValueLabel = new QLabel("31.3");

    gasValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gasValueLabel->setFixedSize(150, 50); // 设置标签大小
    gasValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyGasValueLabel = environmentScene1->addWidget(gasValueLabel);
    proxyGasValueLabel->setPos(540, 220);  // 气体标签位置（旋转后右侧偏中上）
    proxyGasValueLabel->setTransform(QTransform().rotate(90));

    temperatureValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    temperatureValueLabel->setFixedSize(150, 50); // 设置标签大小
    temperatureValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyTemperatureLabel = environmentScene1->addWidget(temperatureValueLabel);
    proxyTemperatureLabel->setPos(470, 220);  // 温度标签位置（旋转后右侧偏中上）
    proxyTemperatureLabel->setTransform(QTransform().rotate(90));

    humidityValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    humidityValueLabel->setFixedSize(150, 50); // 设置标签大小
    humidityValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyHumidityValueLabel = environmentScene1->addWidget(humidityValueLabel);
    proxyHumidityValueLabel->setPos(400, 220);  // 湿度标签位置（旋转后右侧偏中上）
    proxyHumidityValueLabel->setTransform(QTransform().rotate(90));

    lightValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lightValueLabel->setFixedSize(150, 50); // 设置标签大小
    lightValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyLightValueLabel = environmentScene1->addWidget(lightValueLabel);
    proxyLightValueLabel->setPos(330, 220);  // 光照度标签位置（旋转后右侧偏中上）
    proxyLightValueLabel->setTransform(QTransform().rotate(90));

    noisedbValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    noisedbValueLabel->setFixedSize(150, 50); // 设置标签大小
    noisedbValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyNoisedbValueLabel = environmentScene1->addWidget(noisedbValueLabel);
    proxyNoisedbValueLabel->setPos(260, 220);  // 噪声分贝标签位置（旋转后右侧偏中上）
    proxyNoisedbValueLabel->setTransform(QTransform().rotate(90));

    noisehzValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    noisehzValueLabel->setFixedSize(150, 50); // 设置标签大小
    noisehzValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyNoisehzValueLabel = environmentScene1->addWidget(noisehzValueLabel);
    proxyNoisehzValueLabel->setPos(190, 160);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxyNoisehzValueLabel->setTransform(QTransform().rotate(90));

    noisehzValueLabel2->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    noisehzValueLabel2->setFixedSize(150, 50); // 设置标签大小
    noisehzValueLabel2->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyNoisehzValueLabel2 = environmentScene1->addWidget(noisehzValueLabel2);
    proxyNoisehzValueLabel2->setPos(120, 160);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxyNoisehzValueLabel2->setTransform(QTransform().rotate(90));

    noisehzValueLabel3->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    noisehzValueLabel3->setFixedSize(150, 50); // 设置标签大小
    noisehzValueLabel3->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyNoisehzValueLabel3 = environmentScene1->addWidget(noisehzValueLabel3);
    proxyNoisehzValueLabel3->setPos(50, 160);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxyNoisehzValueLabel3->setTransform(QTransform().rotate(90));


    percentnoisehzValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    percentnoisehzValueLabel->setFixedSize(150, 50); // 设置标签大小
    percentnoisehzValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxypercentNoisehzValueLabel = environmentScene1->addWidget(percentnoisehzValueLabel);
    proxypercentNoisehzValueLabel->setPos(190, 250);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxypercentNoisehzValueLabel->setTransform(QTransform().rotate(90));

    percentnoisehzValueLabel2->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    percentnoisehzValueLabel2->setFixedSize(150, 50); // 设置标签大小
    percentnoisehzValueLabel2->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxypercentNoisehzValueLabel2 = environmentScene1->addWidget(percentnoisehzValueLabel2);
    proxypercentNoisehzValueLabel2->setPos(120, 250);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxypercentNoisehzValueLabel2->setTransform(QTransform().rotate(90));

    percentnoisehzValueLabel3->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    percentnoisehzValueLabel3->setFixedSize(150, 50); // 设置标签大小
    percentnoisehzValueLabel3->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxypercentNoisehzValueLabel3 = environmentScene1->addWidget(percentnoisehzValueLabel3);
    proxypercentNoisehzValueLabel3->setPos(50, 250);  // 噪声频率标签位置（旋转后右侧偏中上）
    proxypercentNoisehzValueLabel3->setTransform(QTransform().rotate(90));





    // 手动添加每个标签和对应的测量值标签
    createRotatedLabel(environmentScene1, "可燃气(ppm):", 540 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "温度(℃):", 470 - 15, 70, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "湿度(%):", 400 - 15, 70, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "光照度(Lux):", 330 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "噪声分贝(dB):", 260 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "噪声频率1(Hz):", 190 - 15, 0, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "噪声频率2(Hz):", 120 - 15, 0, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene1, "噪声频率3(Hz):", 50 - 15, 0, 90, Qt::black, 14, QFont::Normal);



    environmentSubLayout1->addWidget(environmentView1);
    environmentSubPage1->setLayout(environmentSubLayout1);
    environmentSubTabWidget->addTab(environmentSubPage1, "环境参数");


    // 创建空调参数子标签页3
    QWidget* environmentSubPage3 = new QWidget();
    QVBoxLayout* environmentSubLayout3 = new QVBoxLayout(environmentSubPage3);

    QGraphicsScene* environmentScene3 = new QGraphicsScene(this);
    QGraphicsView* environmentView3 = new QGraphicsView(environmentScene3, environmentSubPage1);
    environmentView3->setStyleSheet("background-color: transparent;");
    environmentScene3->setSceneRect(0, 0, 520, 340);
    environmentView3->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    environmentView3->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    environmentView3->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );

    // 新增的标签
    airValueLabel = new QLabel(""); // 气压
    dryBulbValueLabel = new QLabel(""); // 干球温度
    wetBulbValueLabel = new QLabel(""); // 湿球温度
    relativeHumidityValueLabel = new QLabel(""); // 相对湿度
    dewPointValueLabel = new QLabel(""); // 露点温度
    humidityContentValueLabel = new QLabel(""); // 含湿量
    sensibleHeatValueLabel = new QLabel(""); // 焓值
    partialPressureValueLabel = new QLabel(""); // 分压力

    // 设置标签属性
    airValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    airValueLabel->setFixedSize(150, 50);
    airValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyairValueLabel = environmentScene3->addWidget(airValueLabel);
    proxyairValueLabel->setPos(555 - 15, 200); // 大气压强标签位置（旋转后右侧偏中上）
    proxyairValueLabel->setTransform(QTransform().rotate(90));

    dryBulbValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dryBulbValueLabel->setFixedSize(150, 50);
    dryBulbValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxydryBulbValueLabel = environmentScene3->addWidget(dryBulbValueLabel);
    proxydryBulbValueLabel->setPos(485 - 15, 200); // 湿球温度标签位置（旋转后右侧偏中上）
    proxydryBulbValueLabel->setTransform(QTransform().rotate(90));

    wetBulbValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    wetBulbValueLabel->setFixedSize(150, 50);
    wetBulbValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyWetBulbValueLabel = environmentScene3->addWidget(wetBulbValueLabel);
    proxyWetBulbValueLabel->setPos(415 - 15, 200); // 湿球温度标签位置（旋转后右侧偏中上）
    proxyWetBulbValueLabel->setTransform(QTransform().rotate(90));

    relativeHumidityValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    relativeHumidityValueLabel->setFixedSize(150, 50);
    relativeHumidityValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyRelativeHumidityValueLabel = environmentScene3->addWidget(relativeHumidityValueLabel);
    proxyRelativeHumidityValueLabel->setPos(345 - 15, 200); // 相对湿度标签位置（旋转后右侧偏中上）
    proxyRelativeHumidityValueLabel->setTransform(QTransform().rotate(90));

    dewPointValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dewPointValueLabel->setFixedSize(150, 50);
    dewPointValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyDewPointValueLabel = environmentScene3->addWidget(dewPointValueLabel);
    proxyDewPointValueLabel->setPos(275 - 15, 200); // 露点温度标签位置（旋转后右侧偏中上）
    proxyDewPointValueLabel->setTransform(QTransform().rotate(90));

    humidityContentValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    humidityContentValueLabel->setFixedSize(150, 50);
    humidityContentValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyHumidityContentValueLabel = environmentScene3->addWidget(humidityContentValueLabel);
    proxyHumidityContentValueLabel->setPos(205 - 15, 200); // 含湿量标签位置（旋转后右侧偏中上）
    proxyHumidityContentValueLabel->setTransform(QTransform().rotate(90));

    sensibleHeatValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    sensibleHeatValueLabel->setFixedSize(150, 50);
    sensibleHeatValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxySensibleHeatValueLabel = environmentScene3->addWidget(sensibleHeatValueLabel);
    proxySensibleHeatValueLabel->setPos(135 - 15, 200); // 焓值标签位置（旋转后右侧偏中上）
    proxySensibleHeatValueLabel->setTransform(QTransform().rotate(90));

    partialPressureValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    partialPressureValueLabel->setFixedSize(150, 50);
    partialPressureValueLabel->setStyleSheet("QLabel {color: black; background-color: transparent;font: system;font: 14pt;}");
    QGraphicsProxyWidget* proxyPartialPressureValueLabel = environmentScene3->addWidget(partialPressureValueLabel);
    proxyPartialPressureValueLabel->setPos(65 - 15, 200); // 分压力标签位置（旋转后右侧偏中上）
    proxyPartialPressureValueLabel->setTransform(QTransform().rotate(90));

    // 手动添加每个标签和对应的测量值标签
    createRotatedLabel(environmentScene3, "大气压(kpa):", 540 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "干球温度(℃):", 470 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "湿球温度(℃):", 400 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "相对湿度(%):", 330 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "露点温度(℃):", 260 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "含湿量(g/kg):", 190 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "焓值(kJ/kg):", 120 - 15, 50, 90, Qt::black, 14, QFont::Normal);
    createRotatedLabel(environmentScene3, "分压力(kPa):", 50 - 15, 50, 90, Qt::black, 14, QFont::Normal);

    environmentSubLayout3->addWidget(environmentView3);
    environmentSubPage3->setLayout(environmentSubLayout3);
    environmentSubTabWidget->addTab(environmentSubPage3, "空调参数");

    // 创建红外测温子标签页
    QWidget* environmentSubPage2 = new QWidget();
    QVBoxLayout* environmentSubLayout2 = new QVBoxLayout(environmentSubPage2);

    QGraphicsScene* environmentScene2 = new QGraphicsScene(this);
    QGraphicsView* environmentView2 = new QGraphicsView(environmentScene2, environmentSubPage2);
    environmentView2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    environmentView2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    environmentView2->setStyleSheet("background-color: transparent;");
    environmentScene2->setSceneRect(0, 0, 520, 340);
    gdirValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    gdirValueLabel->setFixedSize(150, 50); // 设置标签大小
    gdirValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxyGdirValueLabel = environmentScene2->addWidget(gdirValueLabel);
    proxyGdirValueLabel->setPos(270, 200);  // 红外温度标签位置（旋转后右侧偏中上）
    proxyGdirValueLabel->setTransform(QTransform().rotate(90));
    environmentView2->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );
    // 添加温度标签和测量值标签
    createRotatedLabel(environmentScene2, "温度(℃):", 255, 60, 90, Qt::black, 12, QFont::Normal);


    environmentSubLayout2->addWidget(environmentView2);
    environmentSubPage2->setLayout(environmentSubLayout2);
    environmentSubTabWidget->addTab(environmentSubPage2, "红外测温");

    QVBoxLayout* environmentTabLayout = new QVBoxLayout(environmentTabPage);
    environmentTabLayout->addWidget(environmentSubTabWidget);
    environmentTabPage->setLayout(environmentTabLayout);
    //mainTabWidget->addTab(environmentTabPage, "            ");//环境参数的标题
    mainTabWidget->addTab(environmentTabPage, "    环境参数    ");//环境参数的标题
    QFont tabFont = mainTabWidget->font();
    tabFont.setPointSize(14); // 设置字体大小为 16pt
    mainTabWidget->setFont(tabFont);


    // 创建电参数参数主标签页
    QWidget* electricTabPage = new QWidget();
    electricSubTabWidget = new QTabWidget();
    electricSubTabWidget->setTabPosition(QTabWidget::East);

    // 创建表笔模式子标签页
    QWidget* probeModePage = new QWidget();
    QVBoxLayout* probeModeLayout = new QVBoxLayout(probeModePage);
    QGraphicsScene* probeScene = new QGraphicsScene(this);
    QGraphicsView* probeView = new QGraphicsView(probeScene, probeModePage);
    probeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    probeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    probeView->setStyleSheet("background-color: white;");
    probeScene->setSceneRect(0, 0, 620, 440);
    probeView->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: white;" // 设置背景透明
                "}"
                );

    // 添加按钮
    acButton = createRotatedButton(probeScene, "交流", 160 - 25, -15, 90);
    dcButton = createRotatedButton(probeScene, "直流", 160 - 25, 115, 90);
    resButton = createRotatedButton(probeScene, "电阻(Ω)", 160 - 25, 240, 90);
    currentButton = createRotatedButton(probeScene, "电流", 73 - 20, -15, 90);
    ONEtenmulA = createRotatedButton(probeScene, "X1", 247 - 45, 115, 90);
    mresButton = createRotatedButton(probeScene, "电阻(mΩ)", 73 - 20, 115, 90);
    continuityButton = createRotatedButton(probeScene, "通断", 247 - 45, 40, 90);
    diodeButton = createRotatedButton(probeScene, "二极管", 247 - 45, 180, 90);
    continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true);
    // 连接按钮点击信号到槽函数
    connect(acButton, &QPushButton::clicked, this, [this]() {acButton->setStyleSheet("background: green; color: white;"); serialPort.write("cmd_probe_shutdown\r\n"); qDebug() << "cmd_probe_shutdown\r\n"; dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); currentButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); startProbeTimer("交流"); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); });
    connect(dcButton, &QPushButton::clicked, this, [this]() {dcButton->setStyleSheet("background: green; color: white;"); serialPort.write("cmd_probe_shutdown\r\n"); qDebug() << "cmd_probe_shutdown\r\n"; acButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); currentButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle);  startProbeTimer("直流"); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); });
    connect(resButton, &QPushButton::clicked, this, [this, probeScene]() { resButton->setStyleSheet("background: green; color: white;"); serialPort.write("cmd_probe_shutdown\r\n"); qDebug() << "cmd_probe_shutdown\r\n"; dcButton->setStyleSheet(defaultStyle); acButton->setStyleSheet(defaultStyle); currentButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); continuityButton->setStyleSheet(defaultStyle); diodeButton->setStyleSheet(defaultStyle); currentMode = ""; startProbeTimer("电阻（Ω）"); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); continuityButton->show(); continuityButton->setDisabled(false); diodeButton->show(); diodeButton->setDisabled(false); });
    connect(currentButton, &QPushButton::clicked, this, [this]() {currentButton->setStyleSheet("background: green; color: white;"); serialPort.write("cmd_probe_shutdown\r\n"); qDebug() << "cmd_probe_shutdown\r\n"; dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); acButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); startProbeTimer("电流"); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); ONEtenmulA->show(); ONEtenmulA->setDisabled(false); });
    connect(ONEtenmulA, &QPushButton::clicked, this, [this]() { XONEORXTEN = !XONEORXTEN; if (XONEORXTEN) { ONEtenmulA->setText("X10"); } else { ONEtenmulA->setText("X1"); }ONEtenmulA->setStyleSheet("background: green; color: white;"); qDebug() << "cmd_probe_shutdown\r\n"; serialPort.write("cmd_probe_shutdown\r\n"); dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); acButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); startProbeTimer("电流"); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); });
    connect(mresButton, &QPushButton::clicked, this, [this]() {mresButton->setStyleSheet("background: green; color: white;"); dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); currentButton->setStyleSheet(defaultStyle); acButton->setStyleSheet(defaultStyle); startProbeTimer("电阻（mΩ）"); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); });
    connect(continuityButton, &QPushButton::clicked, this, [this]() { continuityButton->setStyleSheet("background: green; color: white;"); dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); qDebug() << "cmd_probe_shutdown\r\n"; serialPort.write("cmd_probe_shutdown\r\n"); currentButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); diodeButton->setStyleSheet(defaultStyle); currentMode = "diode"; startProbeTimer("通断档"); sendProbeCommand("通断档"); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); });
    connect(diodeButton, &QPushButton::clicked, this, [this]() {diodeButton->setStyleSheet("background: green; color: white;"); dcButton->setStyleSheet(defaultStyle); resButton->setStyleSheet(defaultStyle); qDebug() << "cmd_probe_shutdown\r\n"; serialPort.write("cmd_probe_shutdown\r\n"); currentButton->setStyleSheet(defaultStyle); mresButton->setStyleSheet(defaultStyle); continuityButton->setStyleSheet(defaultStyle); currentMode = "continuity"; startProbeTimer("二极管"); sendProbeCommand("二极管"); ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); });

    // 创建表笔测量值标签
    daValueLabel = new QLabel("");
    acValueLabel = new QLabel("");
    resValueLabel = new QLabel("");
    currentValueLabel = new QLabel("");
    mresValueLabel = new QLabel("");
    daValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    daValueLabel->setFixedSize(180, 40); // 设置标签大小
    daValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxydaValueLabel = probeScene->addWidget(daValueLabel);
    proxydaValueLabel->setPos(450, -20);
    proxydaValueLabel->setTransform(QTransform().rotate(90));

    acValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    acValueLabel->setFixedSize(150, 50); // 设置标签大小
    acValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyacValueLabel = probeScene->addWidget(acValueLabel);
    proxyacValueLabel->setPos(455, 240);
    proxyacValueLabel->setTransform(QTransform().rotate(90));

    resValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    resValueLabel->setFixedSize(150, 50); // 设置标签大小
    resValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyresValueLabel = probeScene->addWidget(resValueLabel);
    proxyresValueLabel->setPos(350, 0);
    proxyresValueLabel->setTransform(QTransform().rotate(90));

    currentValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    currentValueLabel->setFixedSize(150, 50); // 设置标签大小
    currentValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxycurrentValueLabel = probeScene->addWidget(currentValueLabel);
    proxycurrentValueLabel->setPos(355, 190);
    proxycurrentValueLabel->setTransform(QTransform().rotate(90));

    mresValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mresValueLabel->setFixedSize(150, 50); // 设置标签大小
    mresValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxymresValueLabel = probeScene->addWidget(mresValueLabel);
    proxymresValueLabel->setPos(135 + 30, 200);
    proxymresValueLabel->setTransform(QTransform().rotate(90));



    probeModeLayout->addWidget(probeView);
    probeModePage->setLayout(probeModeLayout);
    electricSubTabWidget->addTab(probeModePage, "表笔模式");
    // 创建一个浅绿色的矩形背景
    //QGraphicsRectItem* backgroundRect = probeScene->addRect(-30, -40, 230, 400, QPen(Qt::transparent), QBrush(QColor("lightyellow")));
    QGraphicsRectItem* backgroundRect = probeScene->addRect(-30, -40, 600, 400, QPen(Qt::transparent), QBrush());

    // 创建一个线性渐变对象，从左到右
    QLinearGradient gradient0(0, 0, 250, 0); // 起点为矩形的左边界，终点为矩形的右边界
    gradient0.setColorAt(0, QColor("lightyellow")); // 左边为黄色
    gradient0.setColorAt(1, QColor("white"));  // 右边为白色（变淡的效果）

    // 设置渐变对象到矩形的填充
    backgroundRect->setBrush(QBrush(gradient0));
    backgroundRect->setZValue(-1); // 将矩形置于按钮下方


    // 创建插头模式子标签页
    QWidget* plugModePage = new QWidget();
    QVBoxLayout* plugModeLayout = new QVBoxLayout(plugModePage);

    QGraphicsScene* plugScene = new QGraphicsScene(this);


    QGraphicsView* plugView = new QGraphicsView(plugScene, plugModePage);
    plugView->setStyleSheet("background-color: transparent;");
    plugScene->setSceneRect(0, 0, 520, 340);
    plugView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    plugView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    plugView->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );

    // 创建一个浅绿色的矩形背景
    //QGraphicsRectItem* backgroundRect2 = plugScene->addRect(-30, -40, 230, 400, QPen(Qt::transparent), QBrush(QColor("lightyellow")));
    QGraphicsRectItem* backgroundRect2 = plugScene->addRect(-30, -40, 600, 400, QPen(Qt::transparent), QBrush());

    // 创建一个线性渐变对象，从左到右
    QLinearGradient gradient2(-30, 0, 250, 0); // 起点为矩形的左边界，终点为矩形的右边界
    gradient2.setColorAt(0, QColor("lightyellow")); // 左边为黄色
    gradient2.setColorAt(1, QColor("white"));  // 右边为白色（变淡的效果）
    // 设置渐变对象到矩形的填充
    backgroundRect2->setBrush(QBrush(gradient2));
    backgroundRect2->setZValue(-1); // 将矩形置于按钮下方

    // 添加按钮
    S12Button = createRotatedButton(plugScene, "单相两线", 160, -15, 90);
    S13Button = createRotatedButton(plugScene, "单相三线", 160, 115, 90);
    S33Button = createRotatedButton(plugScene, "三相三线", 160, 240, 90);
    S33gButton = createRotatedButton(plugScene, "三相三线+地", 73, -15, 90);
    S34Button = createRotatedButton(plugScene, "三相四线", 73, 115, 90);
    S34gButton = createRotatedButton(plugScene, "三相四线+地", 73, 240, 90);
    connect(S12Button, &QPushButton::clicked, this, [this]() {S12Button->setStyleSheet("background: green; color: white;"); S13Button->setStyleSheet(defaultStyle); S33Button->setStyleSheet(defaultStyle); S33gButton->setStyleSheet(defaultStyle); S34Button->setStyleSheet(defaultStyle); S34gButton->setStyleSheet(defaultStyle); startSocketTimer("1p2w"); });
    connect(S13Button, &QPushButton::clicked, this, [this]() {S13Button->setStyleSheet("background: green; color: white;"); S12Button->setStyleSheet(defaultStyle); S33Button->setStyleSheet(defaultStyle); S33gButton->setStyleSheet(defaultStyle); S34Button->setStyleSheet(defaultStyle); S34gButton->setStyleSheet(defaultStyle); startSocketTimer("1p3w"); });
    connect(S33Button, &QPushButton::clicked, this, [this]() {S33Button->setStyleSheet("background: green; color: white;"); S13Button->setStyleSheet(defaultStyle); S12Button->setStyleSheet(defaultStyle); S33gButton->setStyleSheet(defaultStyle); S34Button->setStyleSheet(defaultStyle); S34gButton->setStyleSheet(defaultStyle); startSocketTimer("3p3w"); });
    connect(S33gButton, &QPushButton::clicked, this, [this]() {S33gButton->setStyleSheet("background: green; color: white;"); S13Button->setStyleSheet(defaultStyle); S33Button->setStyleSheet(defaultStyle); S12Button->setStyleSheet(defaultStyle); S34Button->setStyleSheet(defaultStyle); S34gButton->setStyleSheet(defaultStyle); startSocketTimer("3p3w+g"); });
    connect(S34Button, &QPushButton::clicked, this, [this]() {S34Button->setStyleSheet("background: green; color: white;"); S13Button->setStyleSheet(defaultStyle); S33Button->setStyleSheet(defaultStyle); S33gButton->setStyleSheet(defaultStyle); S12Button->setStyleSheet(defaultStyle); S34gButton->setStyleSheet(defaultStyle); startSocketTimer("3p4w"); });
    connect(S34gButton, &QPushButton::clicked, this, [this]() {S34gButton->setStyleSheet("background: green; color: white;"); S13Button->setStyleSheet(defaultStyle); S33Button->setStyleSheet(defaultStyle); S33gButton->setStyleSheet(defaultStyle); S34Button->setStyleSheet(defaultStyle); S12Button->setStyleSheet(defaultStyle); startSocketTimer("3p4w+g"); });

    ABValueLabel = new QLabel("");
    ACValueLabel = new QLabel("");
    CBValueLabel = new QLabel("");
    LAValueLabel = new QLabel("");
    BNValueLabel = new QLabel("");
    CNValueLabel = new QLabel("");
    LGValueLabel = new QLabel("");
    BGValueLabel = new QLabel("");
    CGValueLabel = new QLabel("");
    NGValueLabel = new QLabel("");

    ABValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ABValueLabel->setFixedSize(150, 50); // 设置标签大小
    ABValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyABValueLabel = plugScene->addWidget(ABValueLabel);
    proxyABValueLabel->setPos(550, 0);
    proxyABValueLabel->setTransform(QTransform().rotate(90));

    ACValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ACValueLabel->setFixedSize(150, 50); // 设置标签大小
    ACValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyACValueLabel = plugScene->addWidget(ACValueLabel);
    proxyACValueLabel->setPos(520, 0);
    proxyACValueLabel->setTransform(QTransform().rotate(90));

    CBValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    CBValueLabel->setFixedSize(150, 50); // 设置标签大小
    CBValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyCBValueLabel = plugScene->addWidget(CBValueLabel);
    proxyCBValueLabel->setPos(490, 0);
    proxyCBValueLabel->setTransform(QTransform().rotate(90));

    LAValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    LAValueLabel->setFixedSize(150, 50); // 设置标签大小
    LAValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyLAValueLabel = plugScene->addWidget(LAValueLabel);
    proxyLAValueLabel->setPos(530, 20);
    proxyLAValueLabel->setTransform(QTransform().rotate(90));

    BNValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    BNValueLabel->setFixedSize(150, 50); // 设置标签大小
    BNValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyBNValueLabel = plugScene->addWidget(BNValueLabel);
    proxyBNValueLabel->setPos(560, 20);
    proxyBNValueLabel->setTransform(QTransform().rotate(90));

    CNValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    CNValueLabel->setFixedSize(150, 50); // 设置标签大小
    CNValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyCNValueLabel = plugScene->addWidget(CNValueLabel);
    proxyCNValueLabel->setPos(325, 140);
    proxyCNValueLabel->setTransform(QTransform().rotate(90));

    LGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    LGValueLabel->setFixedSize(150, 50); // 设置标签大小
    LGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyLGValueLabel = plugScene->addWidget(LGValueLabel);
    proxyLGValueLabel->setPos(550, 180);
    proxyLGValueLabel->setTransform(QTransform().rotate(90));

    BGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    BGValueLabel->setFixedSize(150, 50); // 设置标签大小
    BGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyBGValueLabel = plugScene->addWidget(BGValueLabel);
    proxyBGValueLabel->setPos(520, 180);
    proxyBGValueLabel->setTransform(QTransform().rotate(90));

    CGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    CGValueLabel->setFixedSize(150, 50); // 设置标签大小
    CGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyCGValueLabel = plugScene->addWidget(CGValueLabel);
    proxyCGValueLabel->setPos(490, 180);
    proxyCGValueLabel->setTransform(QTransform().rotate(90));

    NGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    NGValueLabel->setFixedSize(150, 50); // 设置标签大小
    NGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyNGValueLabel = plugScene->addWidget(NGValueLabel);
    proxyNGValueLabel->setPos(185, 140);
    proxyNGValueLabel->setTransform(QTransform().rotate(90));

    VALUEABValueLabel = new QLabel("");
    VALUEACValueLabel = new QLabel("");
    VALUECBValueLabel = new QLabel("");
    VALUELAValueLabel = new QLabel("");
    VALUEBNValueLabel = new QLabel("");
    VALUECNValueLabel = new QLabel("");
    VALUELGValueLabel = new QLabel("");
    VALUEBGValueLabel = new QLabel("");
    VALUECGValueLabel = new QLabel("");
    VALUENGValueLabel = new QLabel("");

    VALUEABValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUEABValueLabel->setFixedSize(150, 40); // 设置标签大小
    VALUEABValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxyVALUEABValueLabel = plugScene->addWidget(VALUEABValueLabel);
    proxyVALUEABValueLabel->setPos(550, 100);
    proxyVALUEABValueLabel->setTransform(QTransform().rotate(90));

    VALUEACValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUEACValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUEACValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyACValueLabel = plugScene->addWidget(VALUEACValueLabel);
    VALUEproxyACValueLabel->setPos(520, 100);
    VALUEproxyACValueLabel->setTransform(QTransform().rotate(90));

    VALUECBValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUECBValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUECBValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyCBValueLabel = plugScene->addWidget(VALUECBValueLabel);
    VALUEproxyCBValueLabel->setPos(490, 100);
    VALUEproxyCBValueLabel->setTransform(QTransform().rotate(90));

    VALUELAValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUELAValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUELAValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyLAValueLabel = plugScene->addWidget(VALUELAValueLabel);
    VALUEproxyLAValueLabel->setPos(395, 230);
    VALUEproxyLAValueLabel->setTransform(QTransform().rotate(90));

    VALUEBNValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUEBNValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUEBNValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyBNValueLabel = plugScene->addWidget(VALUEBNValueLabel);
    VALUEproxyBNValueLabel->setPos(360, 250);
    VALUEproxyBNValueLabel->setTransform(QTransform().rotate(90));

    VALUECNValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUECNValueLabel->setFixedSize(150, 40); // 设置标签大小
    VALUECNValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyCNValueLabel = plugScene->addWidget(VALUECNValueLabel);
    VALUEproxyCNValueLabel->setPos(325, 250);
    VALUEproxyCNValueLabel->setTransform(QTransform().rotate(90));

    VALUELGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUELGValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUELGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyLGValueLabel = plugScene->addWidget(VALUELGValueLabel);
    VALUEproxyLGValueLabel->setPos(550, 280);
    VALUEproxyLGValueLabel->setTransform(QTransform().rotate(90));

    VALUEBGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUEBGValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUEBGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyBGValueLabel = plugScene->addWidget(VALUEBGValueLabel);
    VALUEproxyBGValueLabel->setPos(520, 280);
    VALUEproxyBGValueLabel->setTransform(QTransform().rotate(90));

    VALUECGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUECGValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUECGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyCGValueLabel = plugScene->addWidget(VALUECGValueLabel);
    VALUEproxyCGValueLabel->setPos(490, 280);
    VALUEproxyCGValueLabel->setTransform(QTransform().rotate(90));

    VALUENGValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    VALUENGValueLabel->setFixedSize(150, 50); // 设置标签大小
    VALUENGValueLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    VALUEproxyNGValueLabel = plugScene->addWidget(VALUENGValueLabel);
    VALUEproxyNGValueLabel->setPos(185, 250);
    VALUEproxyNGValueLabel->setTransform(QTransform().rotate(90));


    statusLabelDutyCycle = new QLabel("");
    statusLabelId = new QLabel("");
    statusLabelFrequency = new QLabel("");
    statusLabelPhaseSequence = new QLabel("");
    statusLabelPhaseSequence->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabelPhaseSequence->setFixedSize(150, 50); // 设置标签大小
    statusLabelPhaseSequence->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxystatusLabelPhaseSequence = plugScene->addWidget(statusLabelPhaseSequence);
    proxystatusLabelPhaseSequence->setPos(290, 70);
    proxystatusLabelPhaseSequence->setTransform(QTransform().rotate(90));

    statusLabelDutyCycle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabelDutyCycle->setFixedSize(150, 50); // 设置标签大小
    statusLabelDutyCycle->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxystatusLabelDutyCycle = plugScene->addWidget(statusLabelDutyCycle);
    proxystatusLabelDutyCycle->setPos(370, 70);
    proxystatusLabelDutyCycle->setTransform(QTransform().rotate(90));

    statusLabelId->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabelId->setFixedSize(150, 50); // 设置标签大小
    statusLabelId->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxystatusLabelId = plugScene->addWidget(statusLabelId);
    proxystatusLabelId->setPos(140, 140);
    proxystatusLabelId->setTransform(QTransform().rotate(90));

    statusLabelFrequency->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabelFrequency->setFixedSize(150, 50); // 设置标签大小
    statusLabelFrequency->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    proxystatusLabelFrequency = plugScene->addWidget(statusLabelFrequency);
    proxystatusLabelFrequency->setPos(330, 70);
    proxystatusLabelFrequency->setTransform(QTransform().rotate(90));

    plugModeLayout->addWidget(plugView);
    plugModePage->setLayout(plugModeLayout);
    electricSubTabWidget->addTab(plugModePage, "插头模式");

    // 创建24V恒压子标签页
    QWidget* constantVoltagePage = new QWidget();
    QVBoxLayout* constantVoltageLayout = new QVBoxLayout(constantVoltagePage);

    QGraphicsScene* constantVoltageScene = new QGraphicsScene(this);
    QGraphicsView* constantVoltageView = new QGraphicsView(constantVoltageScene, constantVoltagePage);
    constantVoltageView->setStyleSheet("background-color: transparent;");
    constantVoltageScene->setSceneRect(0, 0, 520, 340);
    constantVoltageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    constantVoltageView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    constantVoltageView->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );
    powerStatusLabel = new QLabel("");
    powerStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    powerStatusLabel->setFixedSize(150, 50); // 设置标签大小
    powerStatusLabel->setStyleSheet("QLabel {""color: black;" "background-color: ""transparent;""font: system;""font: 14pt;""}");
    QGraphicsProxyWidget* proxypowerStatusLabel = constantVoltageScene->addWidget(powerStatusLabel);
    proxypowerStatusLabel->setPos(440, 120);  // 24v电压输出标签位置

    proxypowerStatusLabel->setTransform(QTransform().rotate(90));
    // 添加按钮
    V24Button = createRotatedButton(constantVoltageScene, "24V电压", 100, 120, 90);
    connect(V24Button, &QPushButton::clicked, this, &MainWindow::on24vButtonClicked);

    constantVoltageLayout->addWidget(constantVoltageView);
    constantVoltagePage->setLayout(constantVoltageLayout);
    electricSubTabWidget->addTab(constantVoltagePage, "24V恒压");
    QGraphicsRectItem* backgroundRect3 = constantVoltageScene->addRect(-30, -40, 600, 400, QPen(Qt::transparent), QBrush());

    // 创建一个线性渐变对象，从左到右
    QLinearGradient gradient(-30, 0, 250, 0); // 起点为矩形的左边界，终点为矩形的右边界
    gradient.setColorAt(0, QColor("lightyellow")); // 左边为黄色
    gradient.setColorAt(1, QColor("transparent"));  // 右边为白色（变淡的效果）

    // 设置渐变对象到矩形的填充
    backgroundRect3->setBrush(QBrush(gradient));
    backgroundRect3->setZValue(-1); // 将矩形置于按钮下方

    QVBoxLayout* electricTabLayout = new QVBoxLayout(electricTabPage);
    electricTabLayout->addWidget(electricSubTabWidget);
    electricTabPage->setLayout(electricTabLayout);
    //mainTabWidget->addTab(electricTabPage, "                ");//电参数的标题
    mainTabWidget->addTab(electricTabPage, "    电参数    ");
    // 连接主标签页切换信号
    connect(mainTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    // 连接子标签页切换信号
    connect(environmentSubTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onEnvironmentSubTabChanged);
    connect(electricSubTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onElectricSubTabChanged);
    // 初始化定时器
    sensorTimer = new QTimer(this);
    connect(sensorTimer, &QTimer::timeout, this, &MainWindow::sendSensorCommand);
    airconditionTimer = new QTimer(this);
    connect(airconditionTimer, &QTimer::timeout, this, &MainWindow::sendAirCommand);
    infraredTimer = new QTimer(this);
    connect(infraredTimer, &QTimer::timeout, this, &MainWindow::sendInfraredCommand);
    // 隐藏IP输入框，直到网口被选中
    ipLineEdit->hide();
    // 设置主标签页的样式表，去掉边框并添加内边距
    mainTabWidget->setStyleSheet(
                "QTabWidget::pane {"
                "  border: none;"  // 去掉主标签页的边框
                "}"
                "QTabBar::tab {"
                "  border: 1px solid #ccc;"  // 保留标签的边框
                "  background-color: #f0f0f0;"  // 设置标签的背景颜色
                "  padding: 10px;"  // 设置内边距，可以根据需要调整值
                "}"
                "QTabBar::tab:selected {"
                "  background-color: #e0e0e0;"  // 设置选中标签的背景颜色
                "}"
                );

    // 设置环境参数子标签页的样式表，保留边框并添加内边距
    environmentSubTabWidget->setStyleSheet(
                "QTabWidget::pane {"
                "  border: 1px solid #ccc;"  // 保留子标签页的边框
                "}"
                "QTabBar::tab {"
                "  border: 1px solid #ccc;"  // 保留标签的边框
                "  background-color: #f0f0f0;"  // 设置标签的背景颜色
                "  padding: 10px;"  // 设置内边距，可以根据需要调整值
                "}"
                "QTabBar::tab:selected {"
                "  background-color: #e0e0e0;"  // 设置选中标签的背景颜色
                "}"
                );

    // 设置电参数子标签页的样式表，保留边框并添加内边距
    electricSubTabWidget->setStyleSheet(
                "QTabWidget::pane {"
                "  border: 1px solid #ccc;"  // 保留子标签页的边框
                "}"
                "QTabBar::tab {"
                "  border: 1px solid #ccc;"  // 保留标签的边框
                "  background-color: #f0f0f0;"  // 设置标签的背景颜色
                "  padding: 10px;"  // 设置内边距，可以根据需要调整值
                "}"
                "QTabBar::tab:selected {"
                "  background-color: #e0e0e0;"  // 设置选中标签的背景颜色
                "}"
                );


    // 设置主窗口布局
    QVBoxLayout* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->addWidget(mainTabWidget);
    mainContainer->setLayout(mainLayout);

    this->chargeStatusLabel = chargeStatusLabel;
    this->temperatureValueLabel = temperatureValueLabel;
}


void MainWindow::onConnectButtonClicked()
{

}

void MainWindow::pleaseline()
{
    // 创建 QGraphicsScene 和 QGraphicsView
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsView* view = new QGraphicsView(scene, this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFixedSize(800, 480);  // 设置视图大小为旋转后的尺寸
    view->setStyleSheet(
                "QGraphicsView {"
                "  border: none;"         // 去除边框
                "  background-color: transparent;" // 设置背景透明
                "}"
                );


    // 创建对话框内容
    QDialog* buttonDialog = new QDialog(this);
    QVBoxLayout* layout = new QVBoxLayout(buttonDialog);
    QPushButton* dcTestButton2 = new QPushButton("网络断开！请连接网线！", buttonDialog);
    // 设置按钮样式，并增加按钮的宽度和高度
    dcTestButton2->setFixedSize(500, 100);  // 宽度为480，高度为100
    dcTestButton2->setStyleSheet("QPushButton { color: white; background-color: darkred; font-size: 30px; }");
    // 将按钮添加到布局中
    layout->addWidget(dcTestButton2);
    buttonDialog->setLayout(layout);

    // 将对话框添加到场景中
    QGraphicsProxyWidget* proxyDialog = scene->addWidget(buttonDialog);
    // 旋转对话框 90 度
    proxyDialog->setRotation(90);
    // 调整对话框位置
    proxyDialog->setPos(-30, 0);

    // 显示旋转后的对话框
    view->show();

    // 设置定时器，2秒后隐藏对话框和视图
    QTimer::singleShot(2000, this, [buttonDialog, proxyDialog, view, scene]() {
        buttonDialog->deleteLater();  // 删除对话框
        proxyDialog->deleteLater();   // 删除代理
        view->deleteLater();          // 删除视图
        scene->deleteLater();         // 删除场景
    });
}

void MainWindow::processReadHoldingRegisters(quint16 transactionID, quint8 unitID)
{
    // 检查是否有足够的数据来处理一个完整的12字节请求
    while (buffer.size() >= 12) {
        // 提取12字节的数据
        QByteArray request = buffer.mid(0, 12);
        buffer = buffer.mid(12); // 去掉已处理的12字节

        // 解析请求
        quint16 startAddress = static_cast<quint16>(request[8] << 8 | request[9]);
        quint16 quantity = static_cast<quint16>(request[10] << 8 | request[11]);

        qDebug() << "Read Holding Registers Request:"
                 << "Transaction ID:" << transactionID
                 << "Unit ID:" << unitID
                 << "Start Address:" << QString::number(startAddress, 16)
                 << "Quantity:" << quantity;

        // 检查请求的寄存器范围是否有效
        if (startAddress + quantity > 0x1000) { // 假设寄存器地址范围为0x0000到0x0FFF
            qDebug() << "Invalid register range.";
            continue; // 跳过无效请求，处理下一个
        }

        // 准备响应数据
        QByteArray response;
        response.append(static_cast<char>(transactionID >> 8)); // 事务标识符高字节
        response.append(static_cast<char>(transactionID & 0xFF)); // 事务标识符低字节
        response.append(static_cast<char>(0x00)); // 协议ID高字节
        response.append(static_cast<char>(0x00)); // 协议ID低字节
        quint8 byteCount = quantity * 2; // 每个寄存器2字节
        quint16 length = 2 + 1 + 1 + 1 + byteCount; // 单元ID(1字节) + 功能码(1字节) + 字节计数(1字节) + 寄存器值(byteCount字节)
        response.append(static_cast<char>(length >> 8)); // 数据长度高字节
        response.append(static_cast<char>(length & 0xFF)); // 数据长度低字节
        response.append(static_cast<char>(unitID)); // 单元ID
        response.append(static_cast<char>(0x03)); // 功能码
        response.append(static_cast<char>(byteCount)); // 字节计数
        for (quint16 i = 0; i < quantity; ++i) {
            quint16 address = startAddress + i;
            quint16 value = getRegisterValue(address); // 从寄存器中读取值
            response.append(static_cast<char>(value >> 8)); // 高字节
            response.append(static_cast<char>(value & 0xFF)); // 低字节
        }

        tcpSocket->write(response);
        tcpSocket->flush();
        //qDebug() << "Response sent for Read Holding Registers:" << response.toHex();
    }

    // 如果数据不足12字节，等待更多数据
    if (buffer.size() < 12) {
        qDebug() << "Data length is insufficient for function code 03. Waiting for more data.";
    }
}

void MainWindow::onReadyRead()
{
    buffer.append(tcpSocket->readAll());
    qDebug() << "Received data:" << buffer.toHex();

    if (buffer.size() < 12) { // Modbus-TCP最小数据长度
        qDebug() << "Data length is insufficient.";
        return;
    }

    quint16 transactionID = static_cast<quint16>(buffer[0] << 8 | buffer[1]);
    quint16 protocolID = static_cast<quint16>(buffer[2] << 8 | buffer[3]);
    quint16 length = static_cast<quint16>(buffer[4] << 8 | buffer[5]);
    quint8 unitID = buffer[6];
    quint8 functionCode = buffer[7];

    if (functionCode == 0x03) { // 读保持寄存器
        registerMap[0x0500] = 0x0000;
        processReadHoldingRegisters(transactionID, unitID);
    }
    else if (functionCode == 0x10) { // 写多个寄存器
        processWriteMultipleRegisters(transactionID, unitID);

    }
    else {
        qDebug() << "Unknown function code:" << QString::number(functionCode, 16);
    }

    buffer.clear();
}
//10功能码
void MainWindow::processWriteMultipleRegisters(quint16 transactionID, quint8 unitID)
{
    if (buffer.size() < 12) {
        qDebug() << "Data length is insufficient for function code 10.";
        return;
    }

    // 解析起始地址和寄存器数量
    QDataStream stream(buffer.mid(8, 4)); // 从第8字节开始，读取4字节（地址+数量）
    stream.setByteOrder(QDataStream::BigEndian);
    quint16 startAddress, quantity;
    stream >> startAddress >> quantity;

    // byteCount 应该是 quantity * 2，而不是 buffer[12]
    quint8 byteCount = quantity * 2;  // 2 registers × 2 bytes = 4 bytes

    qDebug() << "Write Multiple Registers Request:"
             << "Start Address:" << QString::number(startAddress, 16)
             << "Quantity:" << quantity
             << "Byte Count:" << byteCount;

    // 解析寄存器值（跳过头部12字节 + 1字节的byteCount字段）
    QDataStream valueStream(buffer.mid(13, byteCount));  // 从第13字节开始，读取4字节
    valueStream.setByteOrder(QDataStream::BigEndian);

    // 读取所有寄存器值
    for (quint16 i = 0; i < quantity; ++i) {
        quint16 address = startAddress + i;
        quint16 value;
        valueStream >> value;

        qDebug() << "Writing to Register Address:" << QString::number(address, 16)
                 << "Value:" << QString::number(value, 16);

        setRegisterValueU16(address, value);
    }

    registerMap[0x0501] = 0x0001;

    // 检查寄存器0x00A0到0x00A3的值
    quint16 commandA0 = registerMap.value(0x00A0, 0xFFFF);
    quint16 commandA1 = registerMap.value(0x00A1, 0xFFFF);
    // 握手命令
    if (commandA0 == 0xAABB && commandA1 == 0xCCDD) {
        qDebug() << "执行握手命令";
    }

    // 插座识别号命令
    if (commandA0 == 0x0000 && commandA1) {
        qDebug() << "执行插座识别号命令";
        registerMap[0x0001] = registerMap[0x00A1];
    }
    // 表笔（ac）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;

        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        acButton->click();
    }
    // 表笔（dc）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0001) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        dcButton->click();

    }
    // 表笔（res）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0002) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();

    }

    // 表笔（Current）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0003) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        qDebug() << "执行表笔（Current）命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        currentButton->click();

    }

    // 表笔（mRes）命令
    if (commandA0 == 0x0100 && commandA1 == 0x0004) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State00();
        qDebug() << "执行表笔（mRes）命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        mresButton->click();

    }

    // 插座命令
    if (commandA0 == 0x0200 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令1p2w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S12Button->click();


    }
    if (commandA0 == 0x0200 && commandA1 == 0x0001) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令1p3w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S13Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0002) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p3w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S33Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0003) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p3w+g";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S33gButton->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0004) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p4w";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S34Button->click();

    }
    if (commandA0 == 0x0200 && commandA1 == 0x0005) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State01();
        qDebug() << "执行插座命令3p4w+g";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(1);
        S34gButton->click();

    }

    // 温湿度检测命令win1
    if (registerMap[0x00A0] == 0x0300 && registerMap[0x00A1] == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);

    }

    // 光照检测命令win3
    if (commandA0 == 0x0301 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行光照检测命令";
        mainTabWidget->setCurrentIndex(1);

    }

    // 噪声分析命令
    if (commandA0 == 0x0302 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行噪声分析命令";
        mainTabWidget->setCurrentIndex(1);

    }

    // 可燃气检测命令
    if (commandA0 == 0x0303 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        setGPIO7071State10();
        qDebug() << "执行可燃气检测命令";
        mainTabWidget->setCurrentIndex(1);

    }



    // 24V电压输出控制命令
    if (commandA0 == 0x0400 && commandA1 == 0x0000) {
        serialPort.write("cmd_power_24von\r\n");
        setGPIO7071State11();
        qDebug() << "执行24V电压输出控制命令";
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(2);

        V24Button->click();


    }    
    // 红外测温命令win2
    if (commandA0 == 0x0304 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_sensor_gdir\r\n");
        qDebug() << "执行红外测温命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);
        environmentSubTabWidget->setCurrentIndex(2);


    }
    // 大气压检测
    if (commandA0 == 0x0305 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_sensor_cps171\r\n");
        qDebug() << "执行大气压检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(1);
        environmentSubTabWidget->setCurrentIndex(1);
    }

    // 二极管
    if (commandA0 == 0x0306 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_probe_diode\r\n");
        qDebug() << "执行二极管检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();
        diodeButton->click();
    }

    // 通断
    if (commandA0 == 0x0307 && commandA1 == 0x0000) {
        registerMap[0x00A1] = 0xFFFF;
        serialPort.write("cmd_probe_short\r\n");
        qDebug() << "执行通断检测命令";
        setGPIO7071State10();
        mainTabWidget->setCurrentIndex(2);
        electricSubTabWidget->setCurrentIndex(0);
        resButton->click();
        continuityButton->click();
    }


    // 准备响应数据
    QByteArray response;
    response.append(static_cast<char>(transactionID >> 8)); // 事务标识符高字节
    response.append(static_cast<char>(transactionID & 0xFF)); // 事务标识符低字节
    response.append(static_cast<char>(0x00)); // 协议ID高字节
    response.append(static_cast<char>(0x00)); // 协议ID低字节
    response.append(static_cast<char>(0x00)); // 协议ID低字节
    response.append(static_cast<char>(0x06)); // 数据长度
    response.append(static_cast<char>(unitID)); // 单元ID
    response.append(static_cast<char>(0x10)); // 功能码
    response.append(static_cast<char>(startAddress >> 8)); // 起始地址高字节
    response.append(static_cast<char>(startAddress & 0xFF)); // 起始地址低字节
    response.append(static_cast<char>(quantity >> 8)); // 寄存器数量高字节
    response.append(static_cast<char>(quantity & 0xFF)); // 寄存器数量低字节

    tcpSocket->write(response);
    tcpSocket->flush();
    qDebug() << "Response sent for Write Multiple Registers:" << response.toHex();
}



void MainWindow::onNewConnection()
{
    tcpSocket = tcpServer->nextPendingConnection();
    if (tcpSocket) {
        connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected); // 添加断开连接的信号
        qDebug() << "New client connected.";

    }
    else {
        qDebug() << "Failed to accept new connection.";
        pleaseline(); // 在连接失败时调用 pleaseline()
    }
}
void MainWindow::onClientDisconnected()
{
    qDebug() << "Client disconnected.";
    pleaseline(); // 在客户端断开连接时调用 pleaseline()

}
void MainWindow::onStateChanged(bool state)
{
    qDebug() << "TCP server state changed:" << state;
}
void MainWindow::enterModbusTcpMode()
{
    // 初始化 TCP 服务器
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    connect(tcpServer, &QTcpServer::acceptError, this, &MainWindow::onStateChanged);

    // 设置IP和端口号
    if (!tcpServer->listen(QHostAddress("127.0.0.1"), 5004)) {
        pleaseline();
        qDebug() << "TCP server failed to start:" << tcpServer->errorString();
    }
    else {
        qDebug() << "TCP server started on port" << tcpServer->serverPort();
        isModbusTcpActive = true;
    }
}

void MainWindow::exitModbusTcpMode()
{
    if (tcpServer->isListening()) {
        tcpServer->close();
        isModbusTcpActive = false;
        qDebug() << "TCP server stopped.";
    }
}
void MainWindow::on24vButtonClicked()
{
    if (is24vOn) {
        // 如果当前状态为打开，发送关闭命令
        serialPort.write("cmd_power_24voff\r\n");
        // 改变按钮颜色为默认样式
        V24Button->setStyleSheet(defaultStyle);
        is24vOn = false;  // 更新状态为关闭
    }
    else {
        // 如果当前状态为关闭，发送打开命令
        serialPort.write("cmd_power_24von\r\n");
        // 改变按钮颜色为绿色
        V24Button->setStyleSheet("background: green; color: white;");
        is24vOn = true;  // 更新状态为打开
    }
}
void MainWindow::onButtonClicked(QPushButton* button) {
    // 恢复所有按钮的默认样式
    if (currentActiveButton) {
        currentActiveButton->setStyleSheet(defaultStyle);
    }

    // 设置当前按钮为选中样式
    button->setStyleSheet(selectedStyle);
    currentActiveButton = button;
}

void MainWindow::sendProbeCommand(const QString& mode) {
    QString command;
    if (mode == "交流") {
        command = "cmd_probe_ac\r\n";
        proxydaValueLabel->setPos(450, 80);
        proxyacValueLabel->setPos(453, 220);
        proxyresValueLabel->setPos(350, 80);
        proxycurrentValueLabel->setPos(348, 200);
        daValueLabel->setText("交流电压(V):");
        acValueLabel->setText("3");
        resValueLabel->setText("频率(Hz):");
        currentValueLabel->setText("50");
    }
    else if (mode == "直流") {
        command = "cmd_probe_dc\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 220);
        resValueLabel->setText("");
        currentValueLabel->setText("");
        daValueLabel->setText("直流电压(V):");
        acValueLabel->setText("");
    }
    else if (mode == "电阻（Ω）") {
        command = "cmd_probe_res\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 200);
        resValueLabel->setText("");
        currentValueLabel->setText("");
        daValueLabel->setText("电阻(Ω):");
        acValueLabel->setText("");
    }
    else if (mode == "电流") {
        command = "cmd_probe_current\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 180);

        resValueLabel->setText("");
        currentValueLabel->setText("");
        daValueLabel->setText("电流(A):");
        acValueLabel->setText("");
    }
    else if (mode == "电阻（mΩ）") {
        command = "cmd_probe_mres\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 200);
        daValueLabel->setText("电阻(mΩ):");
        acValueLabel->setText("30");
        resValueLabel->setText("");
        currentValueLabel->setText("");
    }
    else if (mode == "通断档") {
        command = "cmd_probe_short\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 200);
        daValueLabel->setText("通断(Ω):");
        acValueLabel->setText("30");
        resValueLabel->setText("");
        currentValueLabel->setText("");
    }
    else if (mode == "二极管") {
        command = "cmd_probe_diode\r\n";
        proxydaValueLabel->setPos(350, 80);
        proxyacValueLabel->setPos(353, 200);
        daValueLabel->setText("二极管(V):");
        acValueLabel->setText("30");
        resValueLabel->setText("");
        currentValueLabel->setText("");
    }
    else {
        command = "";
    }

    if (!command.isEmpty()) {
        serialPort.write(command.toUtf8());
    }
}
void MainWindow::sendSocketCommand(const QString& mode) {
    QString command;
    if (mode == "1p2w") {
        command = "cmd_socket_1p2w\r\n";
        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("");
        VALUEABValueLabel->setText("");
        ACValueLabel->setText("");
        VALUEACValueLabel->setText("");
        CBValueLabel->setText("");
        VALUECBValueLabel->setText("");
        LAValueLabel->setText("");
        VALUELAValueLabel->setText("");
        BNValueLabel->setText("");
        VALUEBNValueLabel->setText("");
        CNValueLabel->setText("");
        VALUECNValueLabel->setText("");
        LGValueLabel->setText("");
        VALUELGValueLabel->setText("");
        BGValueLabel->setText("");
        VALUEBGValueLabel->setText("");
        CGValueLabel->setText("");
        VALUECGValueLabel->setText("");
        NGValueLabel->setText("");
        VALUENGValueLabel->setText("");
        statusLabelFrequency->setText("");
        statusLabelDutyCycle->setText("");
        proxyLAValueLabel->setPos(400 + 50, 60 + 20);
        VALUEproxyLAValueLabel->setPos(400 + 50, 170 + 20);
        proxystatusLabelFrequency->setPos(350 + 50, 70 + 20);
        proxystatusLabelDutyCycle->setPos(300 + 50, 70 + 20);

        LAValueLabel->setText("L/A-N(V):");
        VALUELAValueLabel->setText("220");
        statusLabelFrequency->setText(tr("频率:%1Hz").arg("50"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
    }
    else if (mode == "1p3w") {
        command = "cmd_socket_1p3w\r\n";
        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("");
        VALUEABValueLabel->setText("");
        ACValueLabel->setText("");
        VALUEACValueLabel->setText("");
        CBValueLabel->setText("");
        VALUECBValueLabel->setText("");
        LAValueLabel->setText("");
        VALUELAValueLabel->setText("");
        BNValueLabel->setText("");
        VALUEBNValueLabel->setText("");
        CNValueLabel->setText("");
        VALUECNValueLabel->setText("");
        LGValueLabel->setText("");
        VALUELGValueLabel->setText("");
        BGValueLabel->setText("");
        VALUEBGValueLabel->setText("");
        CGValueLabel->setText("");
        VALUECGValueLabel->setText("");
        NGValueLabel->setText("");
        VALUENGValueLabel->setText("");
        statusLabelFrequency->setText("");
        statusLabelDutyCycle->setText("");

        proxyLAValueLabel->setPos(450 + 50, 60 + 20);
        VALUEproxyLAValueLabel->setPos(450 + 50, 170 + 20);

        LAValueLabel->setText("L/A-N(V):");
        VALUELAValueLabel->setText("220");

        proxyLGValueLabel->setPos(400 + 50, 60 + 20);
        VALUEproxyLGValueLabel->setPos(400 + 50, 170 + 20);
        LGValueLabel->setText("L/A-G(V):");
        VALUELGValueLabel->setText("221");
        proxyNGValueLabel->setPos(350 + 50, 60 + 20);
        VALUEproxyNGValueLabel->setPos(350 + 50, 170 + 20);
        NGValueLabel->setText("N-G(V):");
        VALUENGValueLabel->setText("220");
        proxystatusLabelFrequency->setPos(300 + 50, 20 + 20);
        proxystatusLabelDutyCycle->setPos(300 + 50, 160 + 20);
        statusLabelFrequency->setText(tr("频率:%1Hz").arg("50"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
    }
    else if (mode == "3p3w") {
        command = "cmd_socket_3p3w\r\n";
        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("");
        VALUEABValueLabel->setText("");
        ACValueLabel->setText("");
        VALUEACValueLabel->setText("");
        CBValueLabel->setText("");
        VALUECBValueLabel->setText("");
        LAValueLabel->setText("");
        VALUELAValueLabel->setText("");
        BNValueLabel->setText("");
        VALUEBNValueLabel->setText("");
        CNValueLabel->setText("");
        VALUECNValueLabel->setText("");
        LGValueLabel->setText("");
        VALUELGValueLabel->setText("");
        BGValueLabel->setText("");
        VALUEBGValueLabel->setText("");
        CGValueLabel->setText("");
        VALUECGValueLabel->setText("");
        NGValueLabel->setText("");
        VALUENGValueLabel->setText("");
        statusLabelFrequency->setText("");
        statusLabelDutyCycle->setText("");
        proxyABValueLabel->setPos(450 + 50, 60 + 20);
        proxyVALUEABValueLabel->setPos(450 + 50, 170);
        ABValueLabel->setText("A-B(V):");
        VALUEABValueLabel->setText("220");
        proxyACValueLabel->setPos(400 + 50, 60 + 20);
        VALUEproxyACValueLabel->setPos(400 + 50, 170);
        ACValueLabel->setText("A-C(V):");
        VALUEACValueLabel->setText("220");
        proxyCBValueLabel->setPos(350 + 50, 60 + 20);
        VALUEproxyCBValueLabel->setPos(350 + 50, 170);
        CBValueLabel->setText("C-B(V):");
        VALUECBValueLabel->setText("220");
        proxystatusLabelFrequency->setPos(300 + 50, 20 + 20);
        proxystatusLabelDutyCycle->setPos(300 + 50, 160 + 20);
        statusLabelFrequency->setText(tr("频率:%1Hz").arg("50"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
    }
    else if (mode == "3p3w+g") {
        command = "cmd_socket_3p3w+g\r\n";
        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("");
        VALUEABValueLabel->setText("");
        ACValueLabel->setText("");
        VALUEACValueLabel->setText("");
        CBValueLabel->setText("");
        VALUECBValueLabel->setText("");
        LAValueLabel->setText("");
        VALUELAValueLabel->setText("");
        BNValueLabel->setText("");
        VALUEBNValueLabel->setText("");
        CNValueLabel->setText("");
        VALUECNValueLabel->setText("");
        LGValueLabel->setText("");
        VALUELGValueLabel->setText("");
        BGValueLabel->setText("");
        VALUEBGValueLabel->setText("");
        CGValueLabel->setText("");
        VALUECGValueLabel->setText("");
        NGValueLabel->setText("");
        VALUENGValueLabel->setText("");
        statusLabelFrequency->setText("");
        statusLabelDutyCycle->setText("");
        proxyABValueLabel->setPos(500 + 40, 60);
        proxyVALUEABValueLabel->setPos(500 + 40, 170);
        ABValueLabel->setText("A-B(V):");
        VALUEABValueLabel->setText("220");
        proxyACValueLabel->setPos(460 + 40, 60);
        VALUEproxyACValueLabel->setPos(460 + 40, 170);
        ACValueLabel->setText("A-C(V):");
        VALUEACValueLabel->setText("20");
        proxyCBValueLabel->setPos(420 + 40, 60);
        VALUEproxyCBValueLabel->setPos(420 + 40, 170);
        CBValueLabel->setText("C-B(V):");
        VALUECBValueLabel->setText("220");

        proxyLGValueLabel->setPos(380 + 40, 60);
        VALUEproxyLGValueLabel->setPos(380 + 40, 170);
        LGValueLabel->setText("A-G(V):");
        VALUELGValueLabel->setText("20");

        proxyBGValueLabel->setPos(340 + 40, 60);
        VALUEproxyBGValueLabel->setPos(340 + 40, 170);
        BGValueLabel->setText("B-G(V):");
        VALUEBGValueLabel->setText("220");

        proxyCGValueLabel->setPos(300 + 40, 60);
        VALUEproxyCGValueLabel->setPos(300 + 40, 170);
        CGValueLabel->setText("C-G(V):");
        VALUECGValueLabel->setText("2");
        proxystatusLabelFrequency->setPos(260 + 40, 20);
        proxystatusLabelDutyCycle->setPos(260 + 40, 160);
        proxystatusLabelPhaseSequence->setPos(220 + 40, 90);
        statusLabelFrequency->setText(tr("频率:%1Hz").arg("0"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
        statusLabelPhaseSequence->setText("相序错误");
    }
    else if (mode == "3p4w") {
        command = "cmd_socket_3p4w\r\n";
        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("");
        VALUEABValueLabel->setText("");
        ACValueLabel->setText("");
        VALUEACValueLabel->setText("");
        CBValueLabel->setText("");
        VALUECBValueLabel->setText("");
        LAValueLabel->setText("");
        VALUELAValueLabel->setText("");
        BNValueLabel->setText("");
        VALUEBNValueLabel->setText("");
        CNValueLabel->setText("");
        VALUECNValueLabel->setText("");
        LGValueLabel->setText("");
        VALUELGValueLabel->setText("");
        BGValueLabel->setText("");
        VALUEBGValueLabel->setText("");
        CGValueLabel->setText("");
        VALUECGValueLabel->setText("");
        NGValueLabel->setText("");
        statusLabelFrequency->setText("");
        statusLabelDutyCycle->setText("");
        VALUENGValueLabel->setText("");

        proxyABValueLabel->setPos(500 + 40, 60);
        proxyVALUEABValueLabel->setPos(500 + 40, 170);
        ABValueLabel->setText("A-B(V):");
        VALUEABValueLabel->setText("20");

        proxyACValueLabel->setPos(460 + 40, 60);
        VALUEproxyACValueLabel->setPos(460 + 40, 170);
        ACValueLabel->setText("A-C(V):");
        VALUEACValueLabel->setText("220");

        proxyCBValueLabel->setPos(420 + 40, 60);
        VALUEproxyCBValueLabel->setPos(420 + 40, 170);
        CBValueLabel->setText("C-B(V):");
        VALUECBValueLabel->setText("220");

        proxyLAValueLabel->setPos(380 + 40, 60);
        VALUEproxyLAValueLabel->setPos(380 + 40, 170);
        LAValueLabel->setText("L/A-N(V):");
        VALUELAValueLabel->setText("220");

        proxyBNValueLabel->setPos(340 + 40, 60);
        VALUEproxyBNValueLabel->setPos(340 + 40, 170);
        BNValueLabel->setText("B-N(V):");
        VALUEBNValueLabel->setText("20");

        proxyCNValueLabel->setPos(300 + 40, 60);
        VALUEproxyCNValueLabel->setPos(300 + 40, 170);
        CNValueLabel->setText("C-N(V):");
        VALUECNValueLabel->setText("20");

        proxystatusLabelFrequency->setPos(260 + 40, 20);
        proxystatusLabelDutyCycle->setPos(260 + 40, 160);
        proxystatusLabelPhaseSequence->setPos(220 + 40, 90);
        statusLabelFrequency->setText(tr("频率:%1 Hz").arg("50"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
        statusLabelPhaseSequence->setText("相序正确");

    }
    else if (mode == "3p4w+g") {
        command = "cmd_socket_3p4w+g\r\n";
        proxyABValueLabel->setPos(500 + 40, 30 - 20);
        proxyVALUEABValueLabel->setPos(500 + 40, 140 - 20);

        proxyACValueLabel->setPos(460 + 40, 30 - 20);
        VALUEproxyACValueLabel->setPos(460 + 40, 140 - 20);

        proxyCBValueLabel->setPos(420 + 40, 30 - 20);
        VALUEproxyCBValueLabel->setPos(420 + 40, 140 - 20);

        proxyLAValueLabel->setPos(500 + 40, 30 - 20 + 170);
        VALUEproxyLAValueLabel->setPos(500 + 40, 140 - 20 + 170);

        proxyBNValueLabel->setPos(460 + 40, 30 - 20 + 170);
        VALUEproxyBNValueLabel->setPos(460 + 40, 140 - 20 + 170);

        proxyCNValueLabel->setPos(420 + 40, 30 - 20 + 170);
        VALUEproxyCNValueLabel->setPos(420 + 40, 140 - 20 + 170);

        proxyLGValueLabel->setPos(380 + 40, 30 - 20);
        VALUEproxyLGValueLabel->setPos(380 + 40, 140 - 20);

        proxyBGValueLabel->setPos(340 + 40, 30 - 20);
        VALUEproxyBGValueLabel->setPos(340 + 40, 140 - 20);

        proxyCGValueLabel->setPos(380 + 40, 30 - 20 + 170);
        VALUEproxyCGValueLabel->setPos(380 + 40, 140 - 20 + 170);

        proxyNGValueLabel->setPos(340 + 40, 30 - 20 + 170);
        VALUEproxyNGValueLabel->setPos(340 + 40, 140 - 20 + 170);


        statusLabelPhaseSequence->setText("");
        ABValueLabel->setText("A-B(V):");
        VALUEABValueLabel->setText("220");
        ACValueLabel->setText("A-C(V):");
        VALUEACValueLabel->setText("220");
        CBValueLabel->setText("C-B(V):");
        VALUECBValueLabel->setText("220");
        LAValueLabel->setText("L/A-N(V):");
        VALUELAValueLabel->setText("220");
        BNValueLabel->setText("B-N(V):");
        VALUEBNValueLabel->setText("220");
        CNValueLabel->setText("C-N(V):");
        VALUECNValueLabel->setText("220");

        LGValueLabel->setText("L/A-G(V):");
        VALUELGValueLabel->setText("0");
        BGValueLabel->setText("B-G(V):");
        VALUEBGValueLabel->setText("0");
        CGValueLabel->setText("C-G(V):");
        VALUECGValueLabel->setText("0");
        NGValueLabel->setText("N-G(V):");
        VALUENGValueLabel->setText("0");
        proxystatusLabelFrequency->setPos(260 + 60, 20);
        proxystatusLabelDutyCycle->setPos(260 + 60, 160);
        proxystatusLabelPhaseSequence->setPos(220 + 60, 90);
        statusLabelFrequency->setText(tr("频率:%1 Hz").arg("50"));
        statusLabelDutyCycle->setText(tr("占空比:%1%").arg("50"));
        statusLabelPhaseSequence->setText("相序正确");
    }
    else {
        command = "";
    }

    if (!command.isEmpty()) {
        serialPort.write(command.toUtf8());
    }
}




void MainWindow::startProbeTimer(const QString& mode) {
    // 停止其他定时器
    probeTimer->stop();
    constantVoltageTimer->stop();
    sensorTimer->stop();
    airconditionTimer->stop();
    infraredTimer->stop();
    // 断开之前的连接
    disconnect(probeTimer, &QTimer::timeout, nullptr, nullptr);
    // 启动表笔模式定时器
    probeTimer->start(200);  // 每200ms发送一次命令
    connect(probeTimer, &QTimer::timeout, this, [this, mode]() { sendProbeCommand(mode); });
}

void MainWindow::startSocketTimer(const QString& mode) {
    // 停止其他定时器
    if (socketTimer)socketTimer->stop();
    if (probeTimer)probeTimer->stop();
    if (constantVoltageTimer)constantVoltageTimer->stop();
    if (sensorTimer)sensorTimer->stop();
    if (airconditionTimer)airconditionTimer->stop();
    if (infraredTimer)infraredTimer->stop();
    // 断开之前的连接
    disconnect(socketTimer, &QTimer::timeout, nullptr, nullptr);
    // 启动表笔模式定时器
    socketTimer->start(200);  // 每200ms发送一次命令
    connect(socketTimer, &QTimer::timeout, this, [this, mode]() { sendSocketCommand(mode); });
}
// 主标签页切换槽函数
void MainWindow::onTabChanged(int index)
{
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LAValueLabel->setText("");
    VALUELAValueLabel->setText("");
    BNValueLabel->setText("");
    VALUEBNValueLabel->setText("");
    CNValueLabel->setText("");
    VALUECNValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    NGValueLabel->setText("");
    VALUENGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    statusLabelPhaseSequence->setText("");
    daValueLabel->setText("");
    acValueLabel->setText("");
    resValueLabel->setText("");
    currentValueLabel->setText("");
    mresValueLabel->setText("");
    acButton->setStyleSheet(defaultStyle);
    dcButton->setStyleSheet(defaultStyle);
    resButton->setStyleSheet(defaultStyle);
    currentButton->setStyleSheet(defaultStyle);
    mresButton->setStyleSheet(defaultStyle);
    continuityButton->setStyleSheet(defaultStyle);
    diodeButton->setStyleSheet(defaultStyle);
    S12Button->setStyleSheet(defaultStyle);
    S13Button->setStyleSheet(defaultStyle);
    S33Button->setStyleSheet(defaultStyle);
    S33gButton->setStyleSheet(defaultStyle);
    S34Button->setStyleSheet(defaultStyle);
    S34gButton->setStyleSheet(defaultStyle);
    V24Button->setStyleSheet(defaultStyle);
    if (socketTimer)socketTimer->stop();
    if (probeTimer)probeTimer->stop();
    if (constantVoltageTimer)constantVoltageTimer->stop();
    if (sensorTimer)sensorTimer->stop();
    if (airconditionTimer)airconditionTimer->stop();
    if (infraredTimer)infraredTimer->stop();
    ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true);

    // 根据当前主标签页索引设置样式
    switch (index) {
    case 0:  // 电参数主标签页
        setTabBackground(mainTabWidget, 0, "background-color: green; color: white;");  // 设置电参数主标签页背景为绿色
        break;
    case 1:  // 环境参数主标签页
        setTabBackground(mainTabWidget, 1, "background-color: green; color: white;");  // 设置环境参数主标签页背景为绿色
        sensorTimer->start(700);  // 每700ms发送一次环境参数命令
        break;
    case 2:  // 设置主标签页
        setTabBackground(mainTabWidget, 2, "background-color: green; color: white;");  // 设置设置主标签页背景为绿色
        break;
    default:
        break;
    }
}
// 设置标签页背景颜色
void MainWindow::setTabBackground(QTabWidget* tabWidget, int index, const QString& style)
{
    QTabBar* tabBar = tabWidget->tabBar();
    tabBar->setStyleSheet(QString("QTabBar::tab:selected { %1 }").arg(style));  // 设置选中标签页的样式
}

// 定时发送检测命令
void MainWindow::sendSensorCommand()
{
    setGPIO7071State10();
    static int commandIndex = 0;  // 用于循环发送多个命令
    const QStringList commands = { "cmd_sensor_gb402\r\n","cmd_sensor_dht22\r\n","cmd_sensor_brsl30\r\n", "cmd_sensor_mmic\r\n" };
    QString command = commands[commandIndex];
    serialPort.write(command.toUtf8());
    commandIndex = (commandIndex + 1) % commands.size();
}
// 定时发送空调检测命令
void MainWindow::sendAirCommand()
{
    setGPIO7071State10();
    static int commandIndex = 0;  // 用于循环发送多个命令
    const QStringList commands = { "cmd_sensor_dht22\r\n","cmd_sensor_cps171\r\n" };
    QString command = commands[commandIndex];
    serialPort.write(command.toUtf8());
    commandIndex = (commandIndex + 1) % commands.size();
}
void MainWindow::sendInfraredCommand()
{

    setGPIO7071State10();
    QString command = "cmd_sensor_gdir\r\n";
    serialPort.write(command.toUtf8());
}
void MainWindow::onEnvironmentSubTabChanged(int index)
{
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LAValueLabel->setText("");
    VALUELAValueLabel->setText("");
    BNValueLabel->setText("");
    VALUEBNValueLabel->setText("");
    CNValueLabel->setText("");
    VALUECNValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    NGValueLabel->setText("");
    VALUENGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    statusLabelPhaseSequence->setText("");
    daValueLabel->setText("");
    acValueLabel->setText("");
    resValueLabel->setText("");
    currentValueLabel->setText("");
    mresValueLabel->setText("");
    acButton->setStyleSheet(defaultStyle);
    dcButton->setStyleSheet(defaultStyle);
    resButton->setStyleSheet(defaultStyle);
    currentButton->setStyleSheet(defaultStyle);
    mresButton->setStyleSheet(defaultStyle);
    continuityButton->setStyleSheet(defaultStyle);
    diodeButton->setStyleSheet(defaultStyle);
    S12Button->setStyleSheet(defaultStyle);
    S13Button->setStyleSheet(defaultStyle);
    S33Button->setStyleSheet(defaultStyle);
    S33gButton->setStyleSheet(defaultStyle);
    S34Button->setStyleSheet(defaultStyle);
    S34gButton->setStyleSheet(defaultStyle);
    V24Button->setStyleSheet(defaultStyle);
    if (socketTimer)socketTimer->stop();
    if (probeTimer)probeTimer->stop();
    if (constantVoltageTimer)constantVoltageTimer->stop();
    if (sensorTimer)sensorTimer->stop();
    if (airconditionTimer)airconditionTimer->stop();
    if (infraredTimer)infraredTimer->stop();
    ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true);
    // 根据当前子标签页索引设置样式
    switch (index) {
    case 0:  // 环境参数子标签页
        setTabBackground(environmentSubTabWidget, 0, "background-color: green; color: white;");  // 设置环境参数子标签页背景为绿色
        sensorTimer->start(200);  // 每200ms发送一次环境参数命令
        break;

    case 1:  // 空调子标签页
        setTabBackground(environmentSubTabWidget, 1, "background-color: green; color: white;");  // 设置空调子标签页背景为绿色
        airconditionTimer->start(200);  // 每200ms发送一次红外测温命令
        break;
    case 2:  // 红外测温子标签页
        setTabBackground(environmentSubTabWidget, 2, "background-color: green; color: white;");  // 设置红外测温子标签页背景为绿色
        infraredTimer->start(200);  // 每200ms发送一次红外测温命令
        break;
    default:
        break;
    }
}
void MainWindow::onElectricSubTabChanged(int index)
{
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LAValueLabel->setText("");
    VALUELAValueLabel->setText("");
    BNValueLabel->setText("");
    VALUEBNValueLabel->setText("");
    CNValueLabel->setText("");
    VALUECNValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    NGValueLabel->setText("");
    VALUENGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    ABValueLabel->setText("");
    VALUEABValueLabel->setText("");
    ACValueLabel->setText("");
    VALUEACValueLabel->setText("");
    CBValueLabel->setText("");
    VALUECBValueLabel->setText("");
    LGValueLabel->setText("");
    VALUELGValueLabel->setText("");
    BGValueLabel->setText("");
    VALUEBGValueLabel->setText("");
    CGValueLabel->setText("");
    VALUECGValueLabel->setText("");
    statusLabelFrequency->setText("");
    statusLabelDutyCycle->setText("");
    statusLabelPhaseSequence->setText("");
    daValueLabel->setText("");
    acValueLabel->setText("");
    resValueLabel->setText("");
    currentValueLabel->setText("");
    mresValueLabel->setText("");
    acButton->setStyleSheet(defaultStyle);
    dcButton->setStyleSheet(defaultStyle);
    resButton->setStyleSheet(defaultStyle);
    currentButton->setStyleSheet(defaultStyle);
    mresButton->setStyleSheet(defaultStyle);
    continuityButton->setStyleSheet(defaultStyle);
    diodeButton->setStyleSheet(defaultStyle);
    S12Button->setStyleSheet(defaultStyle);
    S13Button->setStyleSheet(defaultStyle);
    S33Button->setStyleSheet(defaultStyle);
    S33gButton->setStyleSheet(defaultStyle);
    S34Button->setStyleSheet(defaultStyle);
    S34gButton->setStyleSheet(defaultStyle);
    V24Button->setStyleSheet(defaultStyle);
    if (socketTimer)socketTimer->stop();
    if (probeTimer)probeTimer->stop();
    if (constantVoltageTimer)constantVoltageTimer->stop();
    if (sensorTimer)sensorTimer->stop();
    if (airconditionTimer)airconditionTimer->stop();
    if (infraredTimer)infraredTimer->stop();
    ONEtenmulA->hide(); ONEtenmulA->setDisabled(true); continuityButton->hide(); continuityButton->setDisabled(true); diodeButton->hide(); diodeButton->setDisabled(true);

    // 根据当前子标签页索引设置样式
    switch (index) {
    case 0:  // 表笔模式子标签页
        setTabBackground(electricSubTabWidget, 0, "background-color: green; color: white;");  // 设置表笔模式子标签页背景为绿色
        break;
    case 1:  // 插头模式子标签页
        setTabBackground(electricSubTabWidget, 1, "background-color: green; color: white;");  // 设置插头模式子标签页背景为绿色
        break;
    case 2:  // 24V恒压子标签页
        setTabBackground(electricSubTabWidget, 2, "background-color: green; color: white;");  // 设置24V恒压子标签页背景为绿色
        break;
    default:
        break;
    }
}
void MainWindow::connectSerialPort()
{
    // 获取可用串口列表，过滤掉名称结尾是 "0" 的串口
    QStringList availablePorts;
    for (const QSerialPortInfo& serialPortInfo : QSerialPortInfo::availablePorts()) {
        QString portName = serialPortInfo.portName();
        if (!portName.endsWith("0")) {
            availablePorts.append(portName + " - " + serialPortInfo.description());
        }
    }

    if (availablePorts.isEmpty()) {
        QMessageBox::warning(this, "串口未找到", "没有检测到可用的串口！");
        return;
    }

    QString autoSelectedPort;
    for (const QString& port : availablePorts) {
        QString portName = port.section(' ', 0, 0);
        if (portName.endsWith("3")) {
            autoSelectedPort = port;
            break;
        }
    }

    if (!autoSelectedPort.isEmpty()) {
        QString portName = autoSelectedPort.section(' ', 0, 0);
        serialPort.setPortName(portName);
        if (!serialPort.open(QIODevice::ReadWrite)) {
            QMessageBox::critical(this, "串口打开失败", QString("无法自动连接到串口 %1，请检查串口配置！").arg(portName));
            return;
        }

        qDebug() << "SERIAL IPEN：" << portName;
        return;
    }

    bool ok;
    QString selectedPort = QInputDialog::getItem(this, "选择串口", "可用串口列表", availablePorts, 0, false, &ok);

    if (!ok || selectedPort.isEmpty()) {
        QMessageBox::warning(this, "连接取消", "未选择串口，程序将无法与设备通信！");
        return;
    }

    QString portName = selectedPort.section(' ', 0, 0);
    serialPort.setPortName(portName);
    if (!serialPort.open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "串口打开失败", QString("无法打开串口 %1，请检查串口配置！").arg(portName));
        return;
    }
}

void MainWindow::readSerialData()//读取串口
{

    QByteArray newData = serialPort.readAll();
    dataBuffer.append(newData);  // 将新数据追加到缓冲区

    // 检查是否接收到完整的数据（以 "\r\n" 结尾）
    while (dataBuffer.contains("\r\n"))
    {
        int endIndex = dataBuffer.indexOf("\r\n");
        QString completeData = dataBuffer.left(endIndex);  // 提取完整数据
        dataBuffer = dataBuffer.mid(endIndex + 2);  // 去掉已处理的部分

        qDebug() << "Received complete data in MainWindow:" << completeData;

        // 处理完整数据
        if (completeData.contains("data_power_24v_1"))
        {
            powerStatusLabel->setText("电压输出中");

            isPowerOutputActive = true;
        }
        else if (completeData.contains("data_power_24v_0"))
        {
            powerStatusLabel->setText("");

            isPowerOutputActive = false;
        }
        else if (completeData.startsWith("data_power_bat_"))
        {
            // 去掉前缀 "data_power_bat_"
            QString batteryData = completeData.mid(15);
            // 去掉多余的 "_"（如果存在）
            batteryData = batteryData.trimmed();
            while (batteryData.endsWith('_')) {
                batteryData.chop(1);
            }

            QStringList batteryInfo = batteryData.split('_');
            if (batteryInfo.size() == 2)
            {
                level = batteryInfo[0].toFloat();  // 转换为浮点数
                chargeStatus = batteryInfo[1].toFloat();
            }
        }
        else if (completeData.startsWith("data_sensor_dht22"))
        {
            QString dataPart = completeData.mid(18);  // 去掉前缀 "data_sensor_dht22_"
            QStringList values = dataPart.split('_');

            if (values.size() == 3)
            {
                QString temperature = values[0];
                QString humidity = values[1];
                allTEM = values[0];
                allHUM = values[1];

                // 更新标签内容
                temperatureValueLabel->setText(allTEM);
                humidityValueLabel->setText(allHUM);

                // 将数据存储到寄存器中
                quint16 temperatureAddress = 0x0300;  // 温度寄存器地址
                quint16 humidityAddress = 0x0302;     // 湿度寄存器地址

                // 转换为浮点数
                bool ok;
                float temperatureValue = temperature.toFloat(&ok);
                float humidityValue = humidity.toFloat(&ok);

                if (ok)
                {
                    // 将数据存储到寄存器中
                    setRegisterValueInt(temperatureAddress, temperatureValue);
                    setRegisterValueInt(humidityAddress, humidityValue);
                    registerMap[0x0501] = 0x0001;
                }
                else {
                    registerMap[0x0501] = 0x0000;
                    QThread::msleep(1);
                }
            }
        }
        else if (completeData.startsWith("data_sensor_brsl30"))
        {
            QString dataPart = completeData.mid(19);  // 去掉前缀 "data_sensor_bsrl30_"
            // 去掉末尾的 "_" 字符（如果存在）
            if (dataPart.endsWith('_'))
            {
                dataPart.chop(1);  // 去掉最后一个字符
            }
            QString lightIntensity = dataPart.trimmed();  // 去除可能的多余空格


            // 更新标签内容
            lightValueLabel->setText(lightIntensity);

            // 将光照度数据存储到寄存器中
            quint16 lightIntensityAddress = 0x0304;  // 光照度寄存器地址

            // 转换为浮点数
            bool ok;
            float lightIntensityValue = lightIntensity.toFloat(&ok);

            if (ok)
            {
                // 将数据存储到寄存器中
                setRegisterValueInt(lightIntensityAddress, lightIntensityValue);
                registerMap[0x0501] = 0x0001;
            }
            else {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }

        }
        else if (completeData.startsWith("data_sensor_gb402_"))
        {
            QString dataPart = completeData.mid(18);  // 去掉前缀 "data_sensor_gb402_"
            // 去掉末尾的 "_" 字符（如果存在）
            if (dataPart.endsWith('_'))
            {
                dataPart.chop(1);  // 去掉最后一个字符
            }
            QString gasConcentration = dataPart;  // 可燃气浓度值


            // 更新标签内容
            gasValueLabel->setText(gasConcentration);

            // 将可燃气浓度数据存储到寄存器
            quint16 gasConcentrationAddress = 0x030A;  // 可燃气浓度寄存器地址

            // 转换为浮点数
            bool ok;
            float gasConcentrationValue = gasConcentration.toFloat(&ok);

            if (ok)
            {
                // 将数据存储到寄存器中
                setRegisterValueInt(gasConcentrationAddress, gasConcentrationValue);
                registerMap[0x0501] = 0x0001;
            }
            else {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }
        }
        else  if (completeData.startsWith("data_sensor_mmic_"))
        {
            QString dataPart = completeData.mid(17);  // 去掉前缀 "data_sensor_mmic_"
            QStringList values = dataPart.split('_');  // 使用 '_' 分割数据

            if (values.size() == 8)  // 确保有两部分数据
            {
                QString noiseIntensity = values[0];  // 噪声强度值
                QString noiseFrequency0 = values[1];  // 噪声频率值
                QString noiseIntensity0 = values[2];  // 噪声强度值
                QString noiseFrequency1 = values[3];  // 噪声频率值
                QString noiseIntensity1 = values[4];  // 噪声强度值
                QString noiseFrequency2 = values[5];  // 噪声频率值
                QString noiseIntensity2 = values[6];  // 噪声强度值


                // 更新标签内容
                noisedbValueLabel->setText(noiseIntensity);
                noisehzValueLabel->setText(noiseFrequency0);
                noisehzValueLabel2->setText(noiseFrequency1);
                noisehzValueLabel3->setText(noiseFrequency2);
                percentnoisehzValueLabel->setText("(" + noiseIntensity0 + "%)");
                percentnoisehzValueLabel2->setText("(" + noiseIntensity1 + "%)");
                percentnoisehzValueLabel3->setText("(" + noiseIntensity2 + "%)");


                // 将噪声分贝数和主频率数据存储到寄存器
                quint16 noiseIntensityAddress = 0x0330;  // 噪声分贝数寄存器地址
                quint16 noiseFrequencyAddress0 = 0x0332;  // 噪声主频率0寄存器地址
                quint16 noiseIntensityAddress0 = 0x0334;  // 噪声主频率百分比0寄存器地址
                quint16 noiseFrequencyAddress1 = 0x0336;  // 噪声主频率1寄存器地址
                quint16 noiseIntensityAddress1 = 0x0338;  // 噪声主频率百分比1寄存器地址
                quint16 noiseFrequencyAddress2 = 0x033A;  // 噪声主频率2寄存器地址
                quint16 noiseIntensityAddress2 = 0x033C;  // 噪声主频率百分比2寄存器地址

                // 转换为浮点数
                bool ok;
                float noiseIntensityValue = noiseIntensity.toFloat(&ok);
                float noiseFrequencyValue0 = noiseFrequency0.toFloat(&ok);
                float noiseIntensityValue0 = noiseIntensity0.toFloat(&ok);
                float noiseFrequencyValue1 = noiseFrequency1.toFloat(&ok);
                float noiseIntensityValue1 = noiseIntensity1.toFloat(&ok);
                float noiseFrequencyValue2 = noiseFrequency2.toFloat(&ok);
                float noiseIntensityValue2 = noiseIntensity2.toFloat(&ok);


                if (ok) {
                    // 将数据存储到寄存器中
                    setRegisterValueInt(noiseIntensityAddress, noiseIntensityValue);
                    setRegisterValueInt(noiseFrequencyAddress0, noiseFrequencyValue0);
                    setRegisterValueInt(noiseIntensityAddress0, noiseIntensityValue0);
                    setRegisterValueInt(noiseFrequencyAddress1, noiseFrequencyValue1);
                    setRegisterValueInt(noiseIntensityAddress1, noiseIntensityValue1);
                    setRegisterValueInt(noiseFrequencyAddress2, noiseFrequencyValue2);
                    setRegisterValueInt(noiseIntensityAddress2, noiseIntensityValue2);
                    registerMap[0x0501] = 0x0001;
                }
                else {
                    registerMap[0x0501] = 0x0000;
                    QThread::msleep(1);
                }
            }

        }
        else  if (completeData.startsWith("data_sensor_gdir_"))
        {
            QString dataPart = completeData.mid(17);  // 去掉前缀 "data_sensor_gdir_"
            // 去掉末尾的 "_" 字符（如果存在）
            if (dataPart.endsWith('_'))
            {
                dataPart.chop(1);  // 去掉最后一个字符
            }
            // 转换为浮点数
            bool ok;
            float temperatureValue = dataPart.toFloat(&ok);
            if (temperatureValue > 1000)
            {
                continue;
            }
            // 更新标签内容
            gdirValueLabel->setText(dataPart);

            // 将数据存储到寄存器中
            quint16 temperatureAddress = 0x030C;  // 红外测温寄存器地址



            if (ok)
            {
                if (temperatureValue > 1000)
                {
                    registerMap[0x0501] = 0x0000; // 标记为无效数据
                }
                // 将数据存储到寄存器中
                setRegisterValueInt(temperatureAddress, temperatureValue);
                registerMap[0x0501] = 0x0001;

            }
            else
            {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }
        }
        else  if (completeData.startsWith("data_sensor_cps171_"))
        {
            QString dataPart = completeData.mid(19);  // 去掉前缀 "data_sensor_cps171_"
            // 去掉末尾的 "_" 字符（如果存在）
            if (dataPart.endsWith('_'))
            {
                dataPart.chop(1);  // 去掉最后一个字符
            }
            // 转换为浮点数
            bool ok;
            float temperatureValue = dataPart.toFloat(&ok);

            // 获取已知参数值（假设单位：温度°C，压力kPa，湿度%）
            float pressure = airValueLabel->text().toFloat(&ok);       // 大气压 (kPa)
            float dryBulb = allTEM.toFloat(&ok);                     // 干球温度 (°C)
            float rh = allHUM.toFloat(&ok);                         // 相对湿度 (%)

            // 计算过程（直接使用公式）
            // 1. 计算饱和水蒸气压力Pws(Pa)
            float pws = 611.2 * exp((17.67 * dryBulb) / (dryBulb + 243.5));
            //转为kpa：
             pws/=1000.0;
            // 2. 计算水蒸气分压力Pv(kPa)
            float pv = (rh / 100.0) * pws;

            // 3. 计算含湿量d(g/kg)
            float d = 622.0 * (pv / (pressure - pv));

            // 4. 计算露点温度td(°C)
            float pv_pa = pv * 1000; // 转Pa用于公式
            float td = (243.5 * log(pv_pa / 610.78)) / (17.67 - log(pv_pa / 610.78));

            // 5. 计算焓值h(kJ/kg)
            float h = 1.005 * dryBulb + (d / 1000) * (2500 + 1.86 * dryBulb);

            // 6. 近似计算湿球温度tw(°C) - 简化公式
            float tw = dryBulb * atan(0.151977 * sqrt(rh + 8.313659))
                    + atan(dryBulb + rh) - atan(rh - 1.676331)
                    + 0.00391838 * pow(rh, 1.5) * atan(0.023101 * rh)
                    - 4.686035;

            // 更新所有标签
            airValueLabel->setText(dataPart);
            dryBulbValueLabel->setText(allTEM);
            wetBulbValueLabel->setText(QString::number(tw, 'f', 2)); // 保留2位小数
            relativeHumidityValueLabel->setText(allHUM);
            dewPointValueLabel->setText(QString::number(td, 'f', 2));
            humidityContentValueLabel->setText(QString::number(d, 'f', 2));
            sensibleHeatValueLabel->setText(QString::number(h, 'f', 2));
            partialPressureValueLabel->setText(QString::number(pv, 'f', 2));

            // 将数据存储到寄存器中
            quint16 temperatureAddress = 0x0310;  // cps大气压
            quint16 temperatureAddress1 = 0x0312;  // 干球温度
            quint16 temperatureAddress2 = 0x0314;  // 湿球温度
            quint16 temperatureAddress3 = 0x0316;  // 相对湿度
            quint16 temperatureAddress4 = 0x0318;  // 露点温度
            quint16 temperatureAddress5 = 0x031A;  // 含湿量
            quint16 temperatureAddress6 = 0x031C;  // 焓值
            quint16 temperatureAddress7 = 0x031E;  // 分压力
            if (ok)
            {
                // 将数据存储到寄存器中
                setRegisterValueInt(temperatureAddress, pressure);
                setRegisterValueInt(temperatureAddress1, dryBulb);
                setRegisterValueInt(temperatureAddress2, tw);
                setRegisterValueInt(temperatureAddress3, rh);
                setRegisterValueInt(temperatureAddress4, td);
                setRegisterValueInt(temperatureAddress5, d);
                setRegisterValueInt(temperatureAddress6, h);
                setRegisterValueInt(temperatureAddress7, tw);
                registerMap[0x0501] = 0x0001;

            }
            else
            {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }
        }
        else  if (completeData.startsWith("data_probe_ac_")) {

            QString dataPart = completeData.mid(14);  // 去掉前缀 "data_probe_ac_"
            QStringList values = dataPart.split('_');  // 使用 '_' 分割数据

            if (values.size() == 3) {
                QString voltage = values[0];  // 电压值
                QString frequency = values[1];  // 频率值

                // 更新 UI
                daValueLabel->setText("交流电压(V):");


                resValueLabel->setText("频率(Hz):");
                currentValueLabel->setText(frequency);

                // 将电压和频率数据存储到寄存器
                quint16 voltageAddress = 0x0100;  // 电压寄存器地址
                quint16 frequencyAddress = 0x0102;  // 频率寄存器地址

                // 转换为浮点数
                bool ok;
                float voltageValue0 = voltage.toFloat(&ok);

                QString voltageValue = QString::number(voltageValue0, 'f', 1);
                acValueLabel->setText(voltageValue);
                float frequencyValue = frequency.toFloat(&ok);

                if (ok) {

                    // 将数据存储到寄存器中
                    setRegisterValueInt(voltageAddress, voltageValue0);
                    setRegisterValueInt(frequencyAddress, frequencyValue);
                }
            }
        }
        else if (completeData.startsWith("data_probe_current_")) {
            // 处理电流模式数据
            QString dataPart = completeData.mid(19);  // 去掉前缀 "data_probe_current_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }
            QString   current = dataPart;  // 电流值
            float currentValue1 = current.toFloat();
            if (XONEORXTEN) {
                currentValue1 *= 10;  // 如果 XONEORXTEN 为 true，则电流值乘以 10
            }

            QString formattedCurrent = QString::number(currentValue1, 'f', 1);

            // 更新 UI
            daValueLabel->setText("电流(A):");

            acValueLabel->setText(formattedCurrent);
            resValueLabel->setText("");
            currentValueLabel->setText("");


            // 将电流数据存储到寄存器
            quint16 currentAddress = 0x0108;  // 电流寄存器地址

            // 转换为浮点数
            bool ok;
            float currentValue = current.toFloat(&ok);

            if (ok) {
                // 将数据存储到寄存器中
                setRegisterValueInt(currentAddress, currentValue);
            }
        }
        else if (completeData.startsWith("data_probe_mres_")) {
            // 处理m电阻模式数据
            QString dataPart = completeData.mid(16);  // 去掉前缀 "data_probe_mres_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }
            QString resistance = dataPart;  // 电阻值
            // 更新 UI
            daValueLabel->setText("电阻（mΩ）:");
            acValueLabel->setText(resistance);
            resValueLabel->setText("");
            currentValueLabel->setText("");

            // 将电阻数据存储到寄存器
            quint16 resistanceAddress = 0x0106;  // 电阻寄存器地址

            // 转换为浮点数
            bool ok;
            float resistanceValue = resistance.toFloat(&ok);
            // 如果电阻值为 9999，弹出提示框
            if (resistanceValue >= 9999) {
                resValueLabel->setText(">9999");
                QThread::msleep(1);
                resValueLabel->setText("");
            }
            if (ok) {
                // 将数据存储到寄存器中
                setRegisterValueInt(resistanceAddress, resistanceValue);

            }
        }
        else if (completeData.startsWith("data_probe_dc_")) {
            // 处理电阻模式数据
            QString dataPart = completeData.mid(14);  // 去掉前缀 "data_probe_dc_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }

            // 将 QString 转换为浮点数
            bool ok;
            float resistanceValue = dataPart.toFloat(&ok);  // 将字符串转换为浮点数

            if (ok) {  // 如果转换成功
                resistanceValue /= 1000.0f;  // 电阻值除以1000
                QString resistance = QString::number(resistanceValue, 'f', 1);  // 将浮点数转换为字符串，并保留1位小数

                // 更新 UI
                daValueLabel->setText("直流电压(V):");
                acValueLabel->setText(resistance);
                resValueLabel->setText("");
                currentValueLabel->setText("");
                // 将电阻数据存储到寄存器
                quint16 resistanceAddress = 0x010B;  // 电阻寄存器地址

                // 将数据存储到寄存器中
                setRegisterValueInt(resistanceAddress, resistanceValue);

            }
            else {
                QThread::msleep(1);
            }
        }

        else if (completeData.startsWith("data_probe_res_")) {
            QString dataPart = completeData.mid(15);  // 去掉前缀 "data_probe_res_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }
            QString resistance = dataPart;  // 电阻值

            // 转换为浮点数
            bool ok;
            float resistanceValue = resistance.toFloat(&ok);

            if (ok) {

                if (currentMode == "") {
                    QString formattedResistance;

                    if (resistanceValue < 500) {
                        // [0-500) 显示一位小数，单位Ω
                        formattedResistance = QString::number(resistanceValue, 'f', 1);
                        daValueLabel->setText("电阻(Ω):");
                    }
                    else if (resistanceValue < 500000) {
                        // [500-500000)，显示4位有效数字，单位KΩ
                        float kOhmValue = resistanceValue / 1000.0f;
                        formattedResistance = QString::number(kOhmValue, 'g', 4);
                        daValueLabel->setText("电阻(KΩ):");
                    }
                    else {
                        // 大于等于500000，显示4位有效数字，单位MΩ
                        float mOhmValue = resistanceValue / 1000000.0f;
                        formattedResistance = QString::number(mOhmValue, 'g', 4);
                        daValueLabel->setText("电阻(MΩ):");
                    }
                    resValueLabel->setText("");
                    currentValueLabel->setText("");

                    // 更新 UI 显示格式化后的电阻值
                    acValueLabel->setText(formattedResistance);

                    // 将电阻数据存储到寄存器
                    quint16 resistanceAddress = 0x0104;  // 电阻寄存器地址
                    setRegisterValueInt(resistanceAddress, resistanceValue);

                }


            }
            else {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }

        }
        else if (completeData.startsWith("data_probe_diode_")) {
            QString dataPart = completeData.mid(17);  // 去掉前缀 "data_probe_res_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }
            QString resistance = dataPart;  // 电阻值

            // 转换为浮点数
            bool ok;
            float resistanceValue = resistance.toFloat(&ok);

            if (ok) {
                // 处理二极管数据
                const float diodeThreshold = 1.5f;  // 二极管正向导通电压阈值，例如1.0V
                if (resistanceValue < diodeThreshold) {
                    // 二极管正向导通
                    daValueLabel->setText("二极管(V):");
                    acValueLabel->setText(dataPart);

                }
                else {
                    // 二极管反向截止或开路
                    daValueLabel->setText("二极管:");
                    acValueLabel->setText("O.L.");  // 显示“O.L.”表示开路
                }

                // 将二极管数据存储到寄存器
                quint16 diodeAddress = 0x010D;  // 二极管寄存器地址
                setRegisterValueInt(diodeAddress, resistanceValue);
                registerMap[0x0501] = 0x0001;
            }
            else {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }

        }
        else if (completeData.startsWith("data_probe_short_")) {
            QString dataPart = completeData.mid(17);  // 去掉前缀 "data_probe_res_"
            if (dataPart.endsWith("_")) {
                dataPart.chop(1);
            }
            QString resistance = dataPart;  // 电阻值

            // 转换为浮点数
            bool ok;
            float resistanceValue = resistance.toFloat(&ok);

            if (ok) {
                // 处理通断数据
                const float continuityThreshold = 50.0f;  // 通断阈值
                if (resistanceValue < continuityThreshold) {
                    // 线路导通
                    daValueLabel->setText("通断状态:");
                    acValueLabel->setText("0");  // 显示“0”表示线路导通
                    resValueLabel->setText("");
                    currentValueLabel->setText("");
                }
                else {
                    // 线路断开
                    daValueLabel->setText("通断状态:");
                    acValueLabel->setText("O.L.");  // 显示“O.L.”表示线路断开
                    resValueLabel->setText("");
                    currentValueLabel->setText("");
                }
                // 将通断数据存储到寄存器
                quint16 shortAddress = 0x010F;  // 通断寄存器地址
                setRegisterValueInt(shortAddress, resistanceValue);
                registerMap[0x0501] = 0x0001;
            }
            else {
                registerMap[0x0501] = 0x0000;
                QThread::msleep(1);
            }

        }
        else  if (completeData.startsWith("data_socket_1p2w_")) {

            QStringList values = completeData.mid(17).split('_');
            if (values.size() == 4) {
                QString voltage = values[0];//电压
                QString frequency = values[1];// 频率
                float value2 = (values[2].toFloat()) * 100;
                QString dutyCycle = QString::number(value2); // 转换为QString
                statusLabelPhaseSequence->setText("");
                ABValueLabel->setText("");
                VALUEABValueLabel->setText("");
                ACValueLabel->setText("");
                VALUEACValueLabel->setText("");
                CBValueLabel->setText("");
                VALUECBValueLabel->setText("");
                LAValueLabel->setText("");
                VALUELAValueLabel->setText("");
                BNValueLabel->setText("");
                VALUEBNValueLabel->setText("");
                CNValueLabel->setText("");
                VALUECNValueLabel->setText("");
                LGValueLabel->setText("");
                VALUELGValueLabel->setText("");
                BGValueLabel->setText("");
                VALUEBGValueLabel->setText("");
                CGValueLabel->setText("");
                VALUECGValueLabel->setText("");
                NGValueLabel->setText("");
                VALUENGValueLabel->setText("");
                statusLabelFrequency->setText("");
                statusLabelDutyCycle->setText("");
                LAValueLabel->setText("L/A-N(V):");
                VALUELAValueLabel->setText(voltage);
                statusLabelFrequency->setText(tr("频率:%1Hz").arg(frequency));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycle));
                // tableWidget-> setItem(3, 1, new QTableWidgetItem(voltage));
                // 存储到寄存器
                storeDataToRegisters("L/A-N", voltage, frequency, dutyCycle);
            }
        }
        else if (completeData.startsWith("data_socket_1p3w_")) {
            QStringList values = completeData.mid(17).split('_');
            if (values.size() == 10) {
                QString voltageLN = values[0];
                QString frequencyLN = values[1];
                QString dutyCycleLN = values[2];
                QString voltageLG = values[3];
                QString frequencyLG = values[4];
                QString dutyCycleLG = values[5];
                QString voltageNG = values[6];
                QString frequencyNG = values[7];
                float value8 = (values[8].toFloat()) * 100;
                QString dutyCycleNG = QString::number(value8); // 转换为QString
                statusLabelPhaseSequence->setText("");
                ABValueLabel->setText("");
                VALUEABValueLabel->setText("");
                ACValueLabel->setText("");
                VALUEACValueLabel->setText("");
                CBValueLabel->setText("");
                VALUECBValueLabel->setText("");
                LAValueLabel->setText("");
                VALUELAValueLabel->setText("");
                BNValueLabel->setText("");
                VALUEBNValueLabel->setText("");
                CNValueLabel->setText("");
                VALUECNValueLabel->setText("");
                LGValueLabel->setText("");
                VALUELGValueLabel->setText("");
                BGValueLabel->setText("");
                VALUEBGValueLabel->setText("");
                CGValueLabel->setText("");
                VALUECGValueLabel->setText("");
                NGValueLabel->setText("");
                VALUENGValueLabel->setText("");
                statusLabelFrequency->setText("");
                statusLabelDutyCycle->setText("");
                LAValueLabel->setText("L/A-N(V):");
                VALUELAValueLabel->setText(voltageLN);
                LGValueLabel->setText("L/A-G(V):");
                VALUELGValueLabel->setText(voltageLG);
                NGValueLabel->setText("N-G(V):");
                VALUENGValueLabel->setText(voltageNG);
                statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyNG));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleNG));

                // 存储到寄存器
                storeDataToRegisters("N-G", voltageNG, frequencyNG, dutyCycleNG);
                storeDataToRegisters("L/A-N", voltageLN, frequencyLN, dutyCycleLN);
                storeDataToRegisters("L/A-G", voltageLG, frequencyLG, dutyCycleLG);

            }
        }
        else if (completeData.startsWith("data_socket_3p3w_")) {
            QStringList values = completeData.mid(17).split('_');
            if (values.size() == 11) {

                QString voltageAB = values[0];
                QString frequencyAB = values[1];
                QString dutyCycleAB = values[2];
                QString voltageAC = values[3];
                QString frequencyAC = values[4];
                QString dutyCycleAC = values[5];
                QString voltageCB = values[6];
                QString frequencyCB = values[7];
                float value8 = (values[8].toFloat()) * 100;
                QString dutyCycleCB = QString::number(value8); // 转换为QString
                QString phaseSequence = values[9];

                statusLabelPhaseSequence->setText("");
                if (phaseSequence == "1") {
                    // 正确:设置文本为“相序:正确”，颜色为绿色
                    statusLabelPhaseSequence->setText(tr("相序正确"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: green;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 16pt;"
                                                            "}");
                }
                else if (phaseSequence == "0") {
                    // 错误:设置文本为“相序:错误”，颜色为红色
                    statusLabelPhaseSequence->setText(tr("相序错误"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: red;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 16pt;"
                                                            "}");
                }
                ABValueLabel->setText("");
                VALUEABValueLabel->setText("");
                ACValueLabel->setText("");
                VALUEACValueLabel->setText("");
                CBValueLabel->setText("");
                VALUECBValueLabel->setText("");
                LAValueLabel->setText("");
                VALUELAValueLabel->setText("");
                BNValueLabel->setText("");
                VALUEBNValueLabel->setText("");
                CNValueLabel->setText("");
                VALUECNValueLabel->setText("");
                LGValueLabel->setText("");
                VALUELGValueLabel->setText("");
                BGValueLabel->setText("");
                VALUEBGValueLabel->setText("");
                CGValueLabel->setText("");
                VALUECGValueLabel->setText("");
                NGValueLabel->setText("");
                VALUENGValueLabel->setText("");
                statusLabelFrequency->setText("");
                statusLabelDutyCycle->setText("");
                ABValueLabel->setText("A-B(V):");
                VALUEABValueLabel->setText(voltageAB);
                ACValueLabel->setText("A-C(V):");
                VALUEACValueLabel->setText(voltageAC);
                CBValueLabel->setText("C-B(V):");
                VALUECBValueLabel->setText(voltageCB);
                statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCB));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCB));
                // 存储到寄存器
                storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
                storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
                storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
                quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
                quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
                setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
            }
        }
        else if (completeData.startsWith("data_socket_3p3w+g_")) {
            QStringList values = completeData.mid(19).split('_');
            if (values.size() == 20) {
                QString voltageAB = values[0];
                QString frequencyAB = values[1];
                QString dutyCycleAB = values[2];
                QString voltageAC = values[3];
                QString frequencyAC = values[4];
                QString dutyCycleAC = values[5];
                QString voltageCB = values[6];
                QString frequencyCB = values[7];
                QString dutyCycleCB = values[8];
                QString voltageAG = values[9];
                QString frequencyAG = values[10];
                QString dutyCycleAG = values[11];
                QString voltageBG = values[12];
                QString frequencyBG = values[13];
                QString dutyCycleBG = values[14];
                QString voltageCG = values[15];
                QString frequencyCG = values[16];
                float value8 = (values[17].toFloat()) * 100;
                QString dutyCycleCG = QString::number(value8); // 转换为QString
                QString phaseSequence = values[18];
                ABValueLabel->setText("");
                VALUEABValueLabel->setText("");
                ACValueLabel->setText("");
                VALUEACValueLabel->setText("");
                CBValueLabel->setText("");
                VALUECBValueLabel->setText("");
                LAValueLabel->setText("");
                VALUELAValueLabel->setText("");
                BNValueLabel->setText("");
                VALUEBNValueLabel->setText("");
                CNValueLabel->setText("");
                VALUECNValueLabel->setText("");
                LGValueLabel->setText("");
                VALUELGValueLabel->setText("");
                BGValueLabel->setText("");
                VALUEBGValueLabel->setText("");
                CGValueLabel->setText("");
                VALUECGValueLabel->setText("");
                NGValueLabel->setText("");
                VALUENGValueLabel->setText("");
                statusLabelFrequency->setText("");
                statusLabelDutyCycle->setText("");
                ABValueLabel->setText("A-B(V):");
                VALUEABValueLabel->setText(voltageAB);
                ACValueLabel->setText("A-C(V):");
                VALUEACValueLabel->setText(voltageAC);
                CBValueLabel->setText("C-B(V):");
                VALUECBValueLabel->setText(voltageCB);
                LGValueLabel->setText("A-G(V):");
                VALUELGValueLabel->setText(voltageAG);
                BGValueLabel->setText("B-G(V):");
                VALUEBGValueLabel->setText(voltageBG);
                CGValueLabel->setText("C-G(V):");
                VALUECGValueLabel->setText(voltageCG);
                statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCG));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCG));
                statusLabelPhaseSequence->setText("");
                if (phaseSequence == "1") {
                    // 正确:设置文本为“相序:正确”，颜色为绿色
                    statusLabelPhaseSequence->setText(tr("相序正确"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: green;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }
                else if (phaseSequence == "0") {
                    // 错误:设置文本为“相序:错误”，颜色为红色
                    statusLabelPhaseSequence->setText(tr("相序错误"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: red;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }

                // 存储到寄存器
                storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
                storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
                storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
                storeDataToRegisters("L/A-G", voltageAG, frequencyAG, dutyCycleAG);
                storeDataToRegisters("B-G", voltageBG, frequencyBG, dutyCycleBG);
                storeDataToRegisters("C-G", voltageCG, frequencyCG, dutyCycleCG);
                quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
                quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
                setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
            }
        }
        else if (completeData.startsWith("data_socket_3p4w_")) {
            QStringList values = completeData.mid(17).split('_');
            if (values.size() == 20) {
                QString voltageAB = values[0];
                QString frequencyAB = values[1];
                QString dutyCycleAB = values[2];
                QString voltageAC = values[3];
                QString frequencyAC = values[4];
                QString dutyCycleAC = values[5];
                QString voltageCB = values[6];
                QString frequencyCB = values[7];
                QString dutyCycleCB = values[8];
                QString voltageAN = values[9];
                QString frequencyAN = values[10];
                QString dutyCycleAN = values[11];
                QString voltageBN = values[12];
                QString frequencyBN = values[13];
                QString dutyCycleBN = values[14];
                QString voltageCN = values[15];
                QString frequencyCN = values[16];
                float value8 = (values[17].toFloat()) * 100;
                QString dutyCycleCN = QString::number(value8); // 转换为QString
                QString phaseSequence = values[18];
                statusLabelPhaseSequence->setText("");

                if (phaseSequence == "1") {
                    // 正确:设置文本为“相序:正确”，颜色为绿色
                    statusLabelPhaseSequence->setText(tr("相序正确"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: green;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }
                else if (phaseSequence == "0") {
                    // 错误:设置文本为“相序:错误”，颜色为红色
                    statusLabelPhaseSequence->setText(tr("相序错误"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: red;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }

                ABValueLabel->setText("");
                VALUEABValueLabel->setText("");
                ACValueLabel->setText("");
                VALUEACValueLabel->setText("");
                CBValueLabel->setText("");
                VALUECBValueLabel->setText("");
                LAValueLabel->setText("");
                VALUELAValueLabel->setText("");
                BNValueLabel->setText("");
                VALUEBNValueLabel->setText("");
                CNValueLabel->setText("");
                VALUECNValueLabel->setText("");
                LGValueLabel->setText("");
                VALUELGValueLabel->setText("");
                BGValueLabel->setText("");
                VALUEBGValueLabel->setText("");
                CGValueLabel->setText("");
                VALUECGValueLabel->setText("");
                NGValueLabel->setText("");
                statusLabelFrequency->setText("");
                statusLabelDutyCycle->setText("");
                VALUENGValueLabel->setText("");
                ABValueLabel->setText("A-B(V):");
                VALUEABValueLabel->setText(voltageAB);
                ACValueLabel->setText("A-C(V):");
                VALUEACValueLabel->setText(voltageAC);
                CBValueLabel->setText("C-B(V):");
                VALUECBValueLabel->setText(voltageCB);
                LAValueLabel->setText("L/A-N(V):");
                VALUELAValueLabel->setText(voltageAN);
                BNValueLabel->setText("B-N(V):");
                VALUEBNValueLabel->setText(voltageBN);
                CNValueLabel->setText("C-N(V):");
                VALUECNValueLabel->setText(voltageCN);
                statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCN));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCN));


                // 存储到寄存器
                storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
                storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
                storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
                storeDataToRegisters("L/A-N", voltageAN, frequencyAN, dutyCycleAN);
                storeDataToRegisters("B-N", voltageBN, frequencyBN, dutyCycleBN);
                storeDataToRegisters("C-N", voltageCN, frequencyCN, dutyCycleCN);
                quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
                quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
                setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
            }
        }
        else if (completeData.startsWith("data_socket_3p4w+g_")) {
            QStringList values = completeData.mid(19).split('_');
            if (values.size() == 32) {//读取32
                QString voltageAB = values[0];
                QString frequencyAB = values[1];
                QString dutyCycleAB = values[2];
                QString voltageAC = values[3];
                QString frequencyAC = values[4];
                QString dutyCycleAC = values[5];
                QString voltageCB = values[6];
                QString frequencyCB = values[7];
                QString dutyCycleCB = values[8];
                QString voltageAN = values[9];
                QString frequencyAN = values[10];
                QString dutyCycleAN = values[11];
                QString voltageBN = values[12];
                QString frequencyBN = values[13];
                QString dutyCycleBN = values[14];
                QString voltageCN = values[15];
                QString frequencyCN = values[16];
                QString dutyCycleCN = values[17];
                QString voltageAG = values[18];
                QString frequencyAG = values[19];
                QString dutyCycleAG = values[20];
                QString voltageBG = values[21];
                QString frequencyBG = values[22];
                QString dutyCycleBG = values[23];
                QString voltageCG = values[24];
                QString frequencyCG = values[25];
                QString dutyCycleCG = values[26];
                QString voltageNG = values[27];
                QString frequencyNG = values[28];

                float value8 = (values[29].toFloat()) * 100;
                QString dutyCycleNG = QString::number(value8); // 转换为QString
                QString phaseSequence = values[30];
                qDebug() << "处理完毕:";
                statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyNG));
                statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleNG));
                if (phaseSequence == "1") {
                    // 正确:设置文本为“相序:正确”，颜色为绿色
                    statusLabelPhaseSequence->setText(tr("相序正确"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: green;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }
                else if (phaseSequence == "0") {
                    // 错误:设置文本为“相序:错误”，颜色为红色
                    statusLabelPhaseSequence->setText(tr("相序错误"));
                    statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                            "color: red;"
                                                            "background-color: transparent;"
                                                            "font: system;"
                                                            "font: 15pt;"
                                                            "}");
                }

                ABValueLabel->setText("A-B(V):");
                VALUEABValueLabel->setText(voltageAB);
                ACValueLabel->setText("A-C(V):");
                VALUEACValueLabel->setText(voltageAC);
                CBValueLabel->setText("C-B(V):");
                VALUECBValueLabel->setText(voltageCB);
                ABValueLabel->setText("L/A-N(V):");
                VALUELAValueLabel->setText(voltageAN);
                BNValueLabel->setText("B-N(V):");
                VALUEBNValueLabel->setText(voltageBN);
                CNValueLabel->setText("C-N(V):");
                VALUECNValueLabel->setText(voltageCN);

                LGValueLabel->setText("L/A-G(V):");
                VALUELGValueLabel->setText(voltageAG);
                BGValueLabel->setText("B-G(V):");
                VALUEBGValueLabel->setText(voltageBG);
                CGValueLabel->setText("C-G(V):");
                VALUECGValueLabel->setText(voltageCG);
                NGValueLabel->setText("N-G(V):");
                VALUENGValueLabel->setText(voltageNG);

                // 存储到寄存器
                storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
                storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
                storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
                storeDataToRegisters("L/A-N", voltageAN, frequencyAN, dutyCycleAN);
                storeDataToRegisters("B-N", voltageBN, frequencyBN, dutyCycleBN);
                storeDataToRegisters("C-N", voltageCN, frequencyCN, dutyCycleCN);
                storeDataToRegisters("L/A-G", voltageAG, frequencyAG, dutyCycleAG);
                storeDataToRegisters("B-G", voltageBG, frequencyBG, dutyCycleBG);
                storeDataToRegisters("C-G", voltageCG, frequencyCG, dutyCycleCG);
                storeDataToRegisters("N-G", voltageNG, frequencyNG, dutyCycleNG);
                quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
                quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
                setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);

            }

        }
    }
}

void MainWindow::showKeyboard() {
    // 创建键盘窗口
    Keyboard* keyboard = new Keyboard(this);
    keyboard->show();

    // 连接键盘窗口的 closed 信号到槽函数
    connect(keyboard, &Keyboard::closed, this, [this, keyboard]() {
        // 获取键盘输入的内容
        QString inputText = keyboard->getLineEditText();
        // 设置 IP 地址文本框的内容
        ipLineEdit->setText(inputText);
        // 关闭键盘窗口
        keyboard->close();
    });
}
//确保在连接新窗口之前，断开之前的连接。
void MainWindow::disconnectSerialPort()
{
    disconnect(&serialPort, &QSerialPort::readyRead, nullptr, nullptr);
}

MainWindow::~MainWindow()
{
    delete ui; // 释放主窗口UI对象
}
void MainWindow::setGPIOState(int gpioNumber, int state)
{
    QFile gpioValueFile(QString("/sys/class/gpio/gpio%1/value").arg(gpioNumber));
    if (gpioValueFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&gpioValueFile);
        out << state << "\n"; // 写入状态（0或1）
        gpioValueFile.close();
    }
    else {
        //qDebug() << "Failed to open GPIO" << gpioNumber << "value file";
    }
}
// 设置GPIO70和GPIO71的状态为00
void MainWindow::setGPIO7071State00()
{
    setGPIOState(44, 1);
    setGPIOState(45, 0);
    //qDebug() << "GPIO44 and GPIO45 set to 00";
}

// 设置GPIO70和GPIO71的状态为01
void MainWindow::setGPIO7071State01()
{
    setGPIOState(44, 1);
    setGPIOState(45, 0);
    //qDebug() << "GPIO44 and GPIO45 set to 01";
}

// 设置GPIO70和GPIO71的状态为10
void MainWindow::setGPIO7071State10()
{
    setGPIOState(44, 0);
    setGPIOState(45, 1);
    //qDebug() << "GPIO44 and GPIO45 set to 10";
}

// 设置GPIO70和GPIO71的状态为11
void MainWindow::setGPIO7071State11()
{
    setGPIOState(44, 0);
    setGPIOState(45, 1);
    qDebug() << "GPIO44 and GPIO45 set to 11";
}
void MainWindow::setRegisterValueInt(quint16 address, float value)
{
    // 将浮点数转换为字节表示
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian); // 设置为大端字节序
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision); // 设置为单精度浮点数
    stream << value; // 将浮点数写入字节数组

    // 检查字节数组长度是否为4字节
    if (byteArray.size() != 4) {
        qWarning() << "Error: Float conversion resulted in unexpected byte size:" << byteArray.size();
        return; // 转换失败，直接返回
    }

    // 提取高16位和低16位
    quint16 highWord = static_cast<quint8>(byteArray[0]) << 8 | static_cast<quint8>(byteArray[1]);
    quint16 lowWord = static_cast<quint8>(byteArray[2]) << 8 | static_cast<quint8>(byteArray[3]);

    // 存储到寄存器
    registerMap[address] = highWord; // 存储高16位
    registerMap[address + 1] = lowWord; // 存储低16位
}


void MainWindow::setRegisterValueU16(quint16 address, quint16 value)
{
    registerMap[address] = static_cast<qint16>(value); // 直接存储16位值
}
void MainWindow::setRegisterValueU32(quint16 address, quint32 value)
{
    qint16 highWord = static_cast<qint16>((value >> 16) & 0xFFFF); // 高16位
    qint16 lowWord = static_cast<qint16>(value & 0xFFFF); // 低16位
    registerMap[address] = highWord; // 存储高16位
    registerMap[address + 1] = lowWord; // 存储低16位
}

quint16 MainWindow::getRegisterValue(quint16 address) const
{
    return registerMap.value(address, 0);  // 如果地址不存在，返回默认值 0
}
void MainWindow::processCompleteData(const QString& data)
{
    if (data.startsWith("data_socket_1p2w_")) {
        QStringList values = data.mid(17).split('_');
        if (values.size() == 4) {
            QString voltage = values[0];//电压
            QString frequency = values[1];// 频率
            float value2 = (values[2].toFloat()) * 100;
            QString dutyCycle = QString::number(value2); // 转换为QString
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequency));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycle));
            //tableWidget->setItem(3, 1, new QTableWidgetItem(voltage));
            // 存储到寄存器
            storeDataToRegisters("L/A-N", voltage, frequency, dutyCycle);
        }
    }
    else if (data.startsWith("data_socket_1p3w_")) {
        QStringList values = data.mid(17).split('_');
        if (values.size() == 10) {
            QString voltageLN = values[0];
            QString frequencyLN = values[1];
            QString dutyCycleLN = values[2];
            QString voltageLG = values[3];
            QString frequencyLG = values[4];
            QString dutyCycleLG = values[5];
            QString voltageNG = values[6];
            QString frequencyNG = values[7];
            float value8 = (values[8].toFloat()) * 100;
            QString dutyCycleNG = QString::number(value8); // 转换为QString
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyNG));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleNG));

            // 存储到寄存器
            storeDataToRegisters("N-G", voltageNG, frequencyNG, dutyCycleNG);
            storeDataToRegisters("L/A-N", voltageLN, frequencyLN, dutyCycleLN);
            storeDataToRegisters("L/A-G", voltageLG, frequencyLG, dutyCycleLG);
        }
    }
    else if (data.startsWith("data_socket_3p3w_")) {
        QStringList values = data.mid(17).split('_');
        if (values.size() == 11) {
            QString voltageAB = values[0];
            QString frequencyAB = values[1];
            QString dutyCycleAB = values[2];
            QString voltageAC = values[3];
            QString frequencyAC = values[4];
            QString dutyCycleAC = values[5];
            QString voltageCB = values[6];
            QString frequencyCB = values[7];
            float value8 = (values[8].toFloat()) * 100;
            QString dutyCycleCB = QString::number(value8); // 转换为QString
            QString phaseSequence = values[9];
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCB));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCB));
            if (phaseSequence == "1") {
                // 正确:设置文本为“相序:正确”，颜色为绿色
                statusLabelPhaseSequence->setText(tr("   正确"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: green;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 16pt;"
                                                        "}");
            }
            else if (phaseSequence == "0") {
                // 错误:设置文本为“相序:错误”，颜色为红色
                statusLabelPhaseSequence->setText(tr("   错误"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: red;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 16pt;"
                                                        "}");
            }


            // 存储到寄存器
            storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
            storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
            storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
            quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
            quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
            setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
        }
    }
    else if (data.startsWith("data_socket_3p3w+g_")) {
        QStringList values = data.mid(19).split('_');
        if (values.size() == 20) {
            QString voltageAB = values[0];
            QString frequencyAB = values[1];
            QString dutyCycleAB = values[2];
            QString voltageAC = values[3];
            QString frequencyAC = values[4];
            QString dutyCycleAC = values[5];
            QString voltageCB = values[6];
            QString frequencyCB = values[7];
            QString dutyCycleCB = values[8];
            QString voltageAG = values[9];
            QString frequencyAG = values[10];
            QString dutyCycleAG = values[11];
            QString voltageBG = values[12];
            QString frequencyBG = values[13];
            QString dutyCycleBG = values[14];
            QString voltageCG = values[15];
            QString frequencyCG = values[16];
            float value8 = (values[17].toFloat()) * 100;
            QString dutyCycleCG = QString::number(value8); // 转换为QString
            QString phaseSequence = values[18];
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCG));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCG));
            if (phaseSequence == "1") {
                // 正确:设置文本为“相序:正确”，颜色为绿色
                statusLabelPhaseSequence->setText(tr("   正确"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: green;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }
            else if (phaseSequence == "0") {
                // 错误:设置文本为“相序:错误”，颜色为红色
                statusLabelPhaseSequence->setText(tr("   错误"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: red;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }

            // 存储到寄存器
            storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
            storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
            storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
            storeDataToRegisters("L/A-G", voltageAG, frequencyAG, dutyCycleAG);
            storeDataToRegisters("B-G", voltageBG, frequencyBG, dutyCycleBG);
            storeDataToRegisters("C-G", voltageCG, frequencyCG, dutyCycleCG);
            quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
            quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
            setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
        }
    }
    else if (data.startsWith("data_socket_3p4w_")) {
        QStringList values = data.mid(17).split('_');
        if (values.size() == 20) {
            QString voltageAB = values[0];
            QString frequencyAB = values[1];
            QString dutyCycleAB = values[2];
            QString voltageAC = values[3];
            QString frequencyAC = values[4];
            QString dutyCycleAC = values[5];
            QString voltageCB = values[6];
            QString frequencyCB = values[7];
            QString dutyCycleCB = values[8];
            QString voltageAN = values[9];
            QString frequencyAN = values[10];
            QString dutyCycleAN = values[11];
            QString voltageBN = values[12];
            QString frequencyBN = values[13];
            QString dutyCycleBN = values[14];
            QString voltageCN = values[15];
            QString frequencyCN = values[16];
            float value8 = (values[17].toFloat()) * 100;
            QString dutyCycleCN = QString::number(value8); // 转换为QString
            QString phaseSequence = values[18];
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyCN));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleCN));
            if (phaseSequence == "1") {
                // 正确:设置文本为“相序:正确”，颜色为绿色
                statusLabelPhaseSequence->setText(tr("   正确"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: green;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }
            else if (phaseSequence == "0") {
                // 错误:设置文本为“相序:错误”，颜色为红色
                statusLabelPhaseSequence->setText(tr("   错误"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: red;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }

            // 存储到寄存器
            storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
            storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
            storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
            storeDataToRegisters("L/A-N", voltageAN, frequencyAN, dutyCycleAN);
            storeDataToRegisters("B-N", voltageBN, frequencyBN, dutyCycleBN);
            storeDataToRegisters("C-N", voltageCN, frequencyCN, dutyCycleCN);
            quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
            quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
            setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);
        }
    }
    else if (data.startsWith("data_socket_3p4w+g_")) {
        QStringList values = data.mid(19).split('_');
        if (values.size() == 32) {//读取32
            QString voltageAB = values[0];
            QString frequencyAB = values[1];
            QString dutyCycleAB = values[2];
            QString voltageAC = values[3];
            QString frequencyAC = values[4];
            QString dutyCycleAC = values[5];
            QString voltageCB = values[6];
            QString frequencyCB = values[7];
            QString dutyCycleCB = values[8];
            QString voltageAN = values[9];
            QString frequencyAN = values[10];
            QString dutyCycleAN = values[11];
            QString voltageBN = values[12];
            QString frequencyBN = values[13];
            QString dutyCycleBN = values[14];
            QString voltageCN = values[15];
            QString frequencyCN = values[16];
            QString dutyCycleCN = values[17];
            QString voltageAG = values[18];
            QString frequencyAG = values[19];
            QString dutyCycleAG = values[20];
            QString voltageBG = values[21];
            QString frequencyBG = values[22];
            QString dutyCycleBG = values[23];
            QString voltageCG = values[24];
            QString frequencyCG = values[25];
            QString dutyCycleCG = values[26];
            QString voltageNG = values[27];
            QString frequencyNG = values[28];

            float value8 = (values[29].toFloat()) * 100;
            QString dutyCycleNG = QString::number(value8); // 转换为QString
            QString phaseSequence = values[30];
            qDebug() << "处理完毕:";
            statusLabelFrequency->setText(tr("频率:%1 Hz").arg(frequencyNG));
            statusLabelDutyCycle->setText(tr("占空比:%1%").arg(dutyCycleNG));
            if (phaseSequence == "1") {
                // 正确:设置文本为“相序:正确”，颜色为绿色
                statusLabelPhaseSequence->setText(tr("   正确"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: green;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }
            else if (phaseSequence == "0") {
                // 错误:设置文本为“相序:错误”，颜色为红色
                statusLabelPhaseSequence->setText(tr("   错误"));
                statusLabelPhaseSequence->setStyleSheet("QLabel {"
                                                        "color: red;"
                                                        "background-color: transparent;"
                                                        "font: system;"
                                                        "font: 15pt;"
                                                        "}");
            }

            // 存储到寄存器
            storeDataToRegisters("A-B", voltageAB, frequencyAB, dutyCycleAB);
            storeDataToRegisters("A-C", voltageAC, frequencyAC, dutyCycleAC);
            storeDataToRegisters("C-B", voltageCB, frequencyCB, dutyCycleCB);
            storeDataToRegisters("L/A-N", voltageAN, frequencyAN, dutyCycleAN);
            storeDataToRegisters("B-N", voltageBN, frequencyBN, dutyCycleBN);
            storeDataToRegisters("C-N", voltageCN, frequencyCN, dutyCycleCN);
            storeDataToRegisters("L/A-G", voltageAG, frequencyAG, dutyCycleAG);
            storeDataToRegisters("B-G", voltageBG, frequencyBG, dutyCycleBG);
            storeDataToRegisters("C-G", voltageCG, frequencyCG, dutyCycleCG);
            storeDataToRegisters("N-G", voltageNG, frequencyNG, dutyCycleNG);
            quint16 phaseSequenceAddress = 0x023C; // 相序标志位寄存器地址
            quint16 phaseSequenceValue = phaseSequence == "1" ? 1 : 0; // 假设相序正确为1，错误为0
            setRegisterValueU16(phaseSequenceAddress, phaseSequenceValue);

        }
    }

}

void MainWindow::storeDataToRegisters(const QString& mode, const QString& voltage, const QString& frequency, const QString& dutyCycle)
{
    //qDebug() << "storeDataToRegisters运行:" ;
    // 将电压、 频率和占空比转换为浮点数
    bool ok;
    float voltageValue = voltage.toFloat(&ok);
    float frequencyValue = frequency.toFloat(&ok);
    float dutyCycleValue = dutyCycle.toFloat(&ok);

    if (ok) {
        // 获取寄存器地址
        quint16 voltageAddress = getRegisterAddress(mode, "voltage");
        quint16 frequencyAddress = getRegisterAddress(mode, "frequency");
        quint16 dutyCycleAddress = getRegisterAddress(mode, "dutyCycle");

        // 存储到寄存器
        setRegisterValueInt(voltageAddress, voltageValue);
        setRegisterValueInt(frequencyAddress, frequencyValue);
        setRegisterValueInt(dutyCycleAddress, dutyCycleValue);
        registerMap[0x0501] = 0x0001;
    }
    else {
        registerMap[0x0501] = 0x0000;
    }
}


quint16 MainWindow::getRegisterAddress(const QString& mode, const QString& type)
{
    if (mode == "A-B") {
        if (type == "voltage") return 0x0200;  // 电压
        if (type == "frequency") return 0x0202;  //  频率
        if (type == "dutyCycle") return 0x0204;  // 占空比
    }
    else if (mode == "A-C") {
        if (type == "voltage") return 0x0206;  // 电压
        if (type == "frequency") return 0x0208;  //  频率
        if (type == "dutyCycle") return 0x020A;  // 占空比
    }
    else if (mode == "C-B") {
        if (type == "voltage") return 0x020C;  // 电压
        if (type == "frequency") return 0x020E;  //  频率
        if (type == "dutyCycle") return 0x0210;  // 占空比
    }
    else if (mode == "L/A-N") {
        if (type == "voltage") return 0x0212;  // 电压
        if (type == "frequency") return 0x0214;  //  频率
        if (type == "dutyCycle") return 0x0216;  // 占空比
    }
    else if (mode == "B-N") {
        if (type == "voltage") return 0x0218;  // 电压
        if (type == "frequency") return 0x021A;  //  频率
        if (type == "dutyCycle") return 0x021C;  // 占空比
    }
    else if (mode == "C-N") {
        if (type == "voltage") return 0x021E;  // 电压
        if (type == "frequency") return 0x0220;  //  频率
        if (type == "dutyCycle") return 0x0222;  // 占空比
    }
    else if (mode == "L/A-G") {
        if (type == "voltage") return 0x0224;  // 电压
        if (type == "frequency") return 0x0226;  //  频率
        if (type == "dutyCycle") return 0x0228;  // 占空比
    }
    else if (mode == "B-G") {
        if (type == "voltage") return 0x022A;  // 电压
        if (type == "frequency") return 0x022C;  //  频率
        if (type == "dutyCycle") return 0x022E;  // 占空比
    }
    else if (mode == "C-G") {
        if (type == "voltage") return 0x0230;  // 电压
        if (type == "frequency") return 0x0232;  //  频率
        if (type == "dutyCycle") return 0x0234;  // 占空比
    }
    else if (mode == "N-G") {
        if (type == "voltage") return 0x0236;  // 电压
        if (type == "frequency") return 0x0238;  //  频率
        if (type == "dutyCycle") return 0x023A;  // 占空比
    }

    return 0; // 默认   返回0，表示无效地址
}
