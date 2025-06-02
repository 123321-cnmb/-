#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QProcess>
#include <QFileDialog>
#include <QMainWindow>
#include <QSerialPort>
#include <QMap>
#include <QPainter>
#include <QLabel>  // 包含 QLabel 头文件
#include<QTimer>
#include <QDate>
#include <QButtonGroup>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QVBoxLayout>
#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QApplication>
#include <QWidget>
#include <QDebug>
#include "mainwindow.h"
#include "keyboard.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QTableWidget>
#include <QGraphicsEffect>
#include <QGraphicsBlurEffect>
#include <QFile>
#include <QTextStream>
#include "batterywidget.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QTransform>
#include <QLabel>
#include <QThread>
#include <QObject> // 添加这个头文件
#include "keyboard.h"
#include "batterywidget.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QGraphicsItem>
#include <QApplication>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QHeaderView>
#include <QGraphicsProxyWidget>
#include <QTcpServer>
#include <QTcpSocket>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QMap<quint16, qint16> registerMap;//存储寄存器地址和数据的哈希表
    QLabel* powerStatusLabel;  // 显示“电压输出中……”
    QLabel* batteryLevelLabel;  // 电池状态
    QByteArray binaryData;  // 数据缓冲区
    QByteArray dataBuffer;  // 数据缓冲区
    QByteArray rtudataBuffer;  // 数据缓冲区
    bool isPowerOutputActive = false;  // 电压输出状态标志
    bool isModbusTcpActive = false;  // 标记是否处于 Modbus-TCP 模式
    QTcpServer *tcpServer;  // TCP 服务器
    QTcpSocket *tcpSocket;  // 当前连接的客户端
    QByteArray buffer;  // 用于存储接收到的 TCP 数据
    MainWindow(QWidget *parent = nullptr);
    void updateDateTime();//更新日期
    void updatePower();//更新电池图标
    // 串口连接管理
    void connectSerialPort();
    void showKeyboard();//显示键盘
    void disconnectSerialPort();
    void setGPIOState(int gpioNumber, int state);
    void setGPIO7071State00();
    void setGPIO7071State01();
    void setGPIO7071State10();
    void setGPIO7071State11();
    // 寄存器访问接口
    quint16 getRegisterValue(quint16 address) const;
    void setRegisterValueInt(quint16 address, float value);
    void setRegisterValueU16(quint16 address, quint16 value);
    void setRegisterValueU32(quint16 address, quint32 value);
    void onTabChanged(int index);
    void onEnvironmentSubTabChanged(int index);
    void sendSensorCommand();
    void sendAirCommand();
    void sendInfraredCommand();
    void startProbeTimer(const QString& mode);
    void startSocketTimer(const QString& mode);
    void sendProbeCommand(const QString& mode);
    void sendSocketCommand(const QString& mode);
    void addResistanceButtons();
    void onButtonClicked(QPushButton* button);
    void on24vButtonClicked();
    void processCompleteData(const QString& data);
    void storeDataToRegisters(const QString& mode, const QString& voltage, const QString& frequency, const QString& dutyCycle);
    quint16 getRegisterAddress(const QString& mode, const QString& type);
    QPushButton *currentActiveButton;  // 当前激活的按钮
    void onElectricSubTabChanged(int index);
    void onBluetoothCheckboxStateChanged(int state);
    void onEthernetCheckboxStateChanged(int state);
    void enterModbusTcpMode();
    void exitModbusTcpMode();
    void pleaseline();
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    void processWriteSingleRegister(quint16 transactionID, quint8 unitID);
    void processReadHoldingRegisters(quint16 transactionID, quint8 unitID);
    void processWriteMultipleRegisters(quint16 transactionID, quint8 unitID);
    void sendResponse(quint16 transactionID, quint8 unitID, quint16 registerAddress, quint16 value);
    void onConnectButtonClicked();
    void onStateChanged(bool state);
    void readModbusSerialData();
    void processReadHoldingRegistersRTU(quint8 deviceAddress, quint8 functionCode);
    void processWriteMultipleRegistersRTU(quint8 deviceAddress);
    void initializeModbusSerialPort();
    quint16 calculateCRC(const QByteArray &data);
    void setTabBackground(QTabWidget* tabWidget, int index, const QString& style);
    void onBluetoothButtonClicked();
    void onEthernetButtonClicked();

    ~MainWindow();
private slots:
    void readSerialData();

private:
    Ui::MainWindow *ui;
    QTabWidget *tabWidget;  // 用于管理标签页
    QVBoxLayout *layout;    // 用于组织布局
    QGraphicsView *graphicsView;  // 用于显示场景
    QGraphicsScene *scene;        // 用于管理图形
    QDate initialDate;       // 初始日期
    QTime initialTime;       // 初始时间
    int elapsedTime = 0;     // 已过去的时间（秒）
    int year;//年
    int month;//月
    int date;//日
    int hour;//时
    int min;//分
    int sec;//秒
    QSerialPort serialPort;  // 串口对象
    QSerialPort modbusSerialPort; // 创建新的串口实例
    QLabel *dateTimeLabel;  // 显示日期和时间
    QLabel *connectLabel;  // 连接状态：
    QLabel *connectvalueTimeLabel;  // 正常/异常
    QLabel *constyleLabel;  // 连接方式：
    QLabel *constylevalTimeLabel;  // 网口/蓝牙
    QTimer *timer;          // 定时器，用于更新日期和时间
    QTimer* sensorTimer;  // 定义传感器定时器
    QTimer* infraredTimer;  // 初始化红外测温定时器
    QTimer* airconditionTimer;  // 初始化红外测温定时器
    int chargeStatus = 0;  // 充电状态
    QLabel *chargeStatusLabel;  // 用于显示充电状态
    QString selectedSocketMode;  // 当前选择的插座模式
    QString receivedData;  // 接收到的串口数据
    QTimer *gpioTimer;  // 定时器，用于定期读取GPIO状态
    QTimer *powerTimer;  // 定时器，用于定期读取power状态
    QTimer *probeTimer;  // 表笔模式定时器
    QTimer *socketTimer;  // 插座模式定时器
    QTimer *constantVoltageTimer;  // 24V恒压定时器
    // 在 MainWindow 类中定义成员变量
    QLabel* temperatureValueLabel;  // 温度测量值标签
    QLabel* humidityValueLabel;     // 湿度测量值标签
    QLabel* lightValueLabel;        // 光照度测量值标签
    QLabel* noisedbValueLabel;        // 噪声分贝测量值标签
    QLabel* noisehzValueLabel;        // 噪声频率测量值标签
    QLabel* noisehzValueLabel2;        // 噪声频率测量值标签2
    QLabel* noisehzValueLabel3;        // 噪声频率测量值标签3
    QLabel* percentnoisehzValueLabel;        // 噪声频率测量值标签
    QLabel* percentnoisehzValueLabel2;        // 噪声频率测量值标签2
    QLabel* percentnoisehzValueLabel3;        // 噪声频率测量值标签3
    QLabel* gasValueLabel;          // 可燃气测量值标签
    QString allTEM ;
    QString allHUM ;
    QLabel* gdirValueLabel;// 物温红外
    QLabel* airValueLabel;// 气压大小
    QLabel* dryBulbValueLabel; // 干球温度
    QLabel* wetBulbValueLabel; // 湿球温度
    QLabel* relativeHumidityValueLabel; // 相对湿度
    QLabel* dewPointValueLabel; // 露点温度
    QLabel* humidityContentValueLabel; // 含湿量
    QLabel* sensibleHeatValueLabel; // 焓值
    QLabel* partialPressureValueLabel; // 分压力
    QLabel* daValueLabel;// dc
    QLabel* acValueLabel;// ac
    QLabel* resValueLabel;// res
    QLabel* currentValueLabel;//current
    QLabel* mresValueLabel;//mres
    QLabel* ABValueLabel;//AB
    QLabel* ACValueLabel;
    QLabel* CBValueLabel;
    QLabel* LAValueLabel;
    QLabel* BNValueLabel;
    QLabel* CNValueLabel;
    QLabel* LGValueLabel;
    QLabel* BGValueLabel;
    QLabel* CGValueLabel;
    QLabel* NGValueLabel;
    QLabel* VALUEABValueLabel;//AB
    QLabel* VALUEACValueLabel;
    QLabel* VALUECBValueLabel;
    QLabel* VALUELAValueLabel;
    QLabel* VALUEBNValueLabel;
    QLabel* VALUECNValueLabel;
    QLabel* VALUELGValueLabel;
    QLabel* VALUEBGValueLabel;
    QLabel* VALUECGValueLabel;
    QLabel* VALUENGValueLabel;
    QPushButton *continuityButton;  // 通断按钮
    QPushButton *diodeButton;  // 二极管按钮
    QPushButton *ONEtenmulA;  // 1/10倍
    QPushButton *bluetoothButton;
    QPushButton *ethernetButton;

    bool XONEORXTEN =false;// 1/10倍

    QPushButton* acButton ;
    QPushButton* dcButton ;
    QPushButton* resButton;
    QPushButton* currentButton ;
    QPushButton* mresButton ;
    QPushButton* S12Button ;
    QPushButton* S13Button ;
    QPushButton* S33gButton ;
    QPushButton* S33Button ;
    QPushButton* S34Button ;
    QPushButton* S34gButton ;
    QPushButton* V24Button ;
    QPushButton* BIButton ;
    QString currentMode="";
    // 默认样式
    QString defaultStyle = (
                "QPushButton {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #2196F3, stop:1 #0d8bf2);"
                "   color: white;"
                "   border-radius: 5px;"
                "   padding: 3px;"
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

    // 选中样式
    QString selectedStyle = (
                "QPushButton {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #2196F3, stop:1 #0d8bf2);"
                "   color: white;"
                "   border-radius: 5px;"
                "   padding: 3px;"
                "   border: 1px solid #0a7bda;"
                "}"
                "QPushButton:hover {"
                "   background:green,"
                "               stop:0 #42A5F5, stop:1 #1E88E5);"
                "}"
                "QPushButton:pressed {"
                "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "               stop:0 #0d8bf2, stop:1 #2196F3);"
                "}"
                );
    // 新增的创建旋转按钮函数
    QPushButton* createRotatedButton(QGraphicsScene* scene,const QString& text,int xPos,int yPos,int rotationAngle = 90);
    // 创建旋转文本标签的函数
    QLabel* createRotatedLabel(QGraphicsScene* scene,const QString& text,int xPos,int yPos,int rotationAngle = 90,const QColor& textColor = Qt::black,int fontSize = 12,QFont::Weight fontWeight = QFont::Normal);

    QTableWidget *tableWidget;
    QLabel* label_3;
    QLabel* statusLabelDutyCycle;//插座占空比
    QLabel* statusLabelId;//插座编号
    QLabel* statusLabelFrequency;//插座频率
    QLabel* statusLabelPhaseSequence;//插座相位
    QLabel* VALUEstatusLabelDutyCycle;//插座占空比
    QLabel* VALUEstatusLabelId;//插座编号
    QLabel* VALUEstatusLabelFrequency;//插座频率
    QLabel* VALUEstatusLabelPhaseSequence;//插座相位
    QLabel* statusLabelPhaseSequence0;//
    QString selectedSocketMode02;  // 添加此成员变量
    QByteArray receiveBuffer;  // 添加接收缓冲区
    bool is24vOn=false;            // 24V按钮状态
    QTabWidget *mainTabWidget;
    QTabWidget *electricSubTabWidget;
    QTabWidget *environmentSubTabWidget;
    QCheckBox *bluetoothCheckbox;
    QCheckBox *ethernetCheckbox;
    QLineEdit *ipLineEdit;
    QLabel *ipLabel;
    QGraphicsProxyWidget* proxyLAValueLabel;
    QGraphicsProxyWidget* proxyABValueLabel;
    QGraphicsProxyWidget* proxyACValueLabel;
    QGraphicsProxyWidget* proxyCBValueLabel;
    QGraphicsProxyWidget* proxyBNValueLabel;
    QGraphicsProxyWidget* proxyCNValueLabel;
    QGraphicsProxyWidget* proxyLGValueLabel;
    QGraphicsProxyWidget* proxyBGValueLabel;
    QGraphicsProxyWidget* proxyCGValueLabel;
    QGraphicsProxyWidget* proxyNGValueLabel;
    QGraphicsProxyWidget* proxyVALUEABValueLabel;
    QGraphicsProxyWidget* VALUEproxyLAValueLabel;
    QGraphicsProxyWidget* VALUEproxyACValueLabel;
    QGraphicsProxyWidget* VALUEproxyCBValueLabel;
    QGraphicsProxyWidget* VALUEproxyBNValueLabel;
    QGraphicsProxyWidget* VALUEproxyCNValueLabel;
    QGraphicsProxyWidget* VALUEproxyLGValueLabel;
    QGraphicsProxyWidget* VALUEproxyBGValueLabel;
    QGraphicsProxyWidget* VALUEproxyCGValueLabel;
    QGraphicsProxyWidget* VALUEproxyNGValueLabel;
    QGraphicsProxyWidget* proxystatusLabelPhaseSequence;
    QGraphicsProxyWidget* proxystatusLabelDutyCycle ;
    QGraphicsProxyWidget* proxystatusLabelId;
    QGraphicsProxyWidget* proxystatusLabelFrequency;
    QGraphicsProxyWidget* proxydaValueLabel;
    QGraphicsProxyWidget* proxyacValueLabel;
    QGraphicsProxyWidget* proxyresValueLabel;
    QGraphicsProxyWidget* proxycurrentValueLabel;
    QGraphicsProxyWidget* proxymresValueLabel ;

};

#endif // MAINWINDOW_H
