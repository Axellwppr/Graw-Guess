#include "mainwindow.h"

#include <time.h>

#include <QDebug>
#include <QPushButton>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "./ui_mainwindow.h"
#include "bmp.h"
#include "color.h"
#include "pointSoftenExternal_terminate.h"

#define BIND_EVENT(IO, EV, FN)             \
    do {                                   \
        socket::event_listener_aux l = FN; \
        IO->on(EV, l);                     \
    } while (0)
const QString REDSHEET = QString("QLabel { color : #B3261E; }");
const QString BLACKSHEET = QString("QLabel { color : #6750A4; }");
const QString defaultCount = QString("45");

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _io(new sio::client()) {
    // 初始化ui 建立websocket连接
    ui->setupUi(this);
    setWindowIcon(QIcon(":icon/icon.ico"));
    Painter = ui->myLabel;
    ui->label_problem->setAlignment(Qt::AlignCenter);
    ui->label_countdown->setAlignment(Qt::AlignCenter);
    ui->pushButton_clearPaint->setVisible(false);
    ui->pushButton_requestProblem->setVisible(false);
    ui->label_color->setVisible(false);
    ui->label_problem->setVisible(false);
    ui->horizontalSlider->setVisible(false);

    //绑定网络channel与函数
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    socket::ptr sock = _io->socket();
    BIND_EVENT(sock, "promotionChanged", std::bind(&MainWindow::onPromotionChanged, this));
    BIND_EVENT(sock, "promotionChangedByself", std::bind(&MainWindow::onPromotionChangedByself, this));
    BIND_EVENT(sock, "problemChanged", std::bind(&MainWindow::onProblemChanged, this));
    BIND_EVENT(sock, "problemChangedByself", std::bind(&MainWindow::onProblemChangedByself, this, _1, _2, _3, _4));
    BIND_EVENT(sock, "countdownChanged", std::bind(&MainWindow::onCountdownChanged, this, _1, _2, _3, _4));
    BIND_EVENT(sock, "remoteStatusChanged", std::bind(&MainWindow::onRemoteStatusChanged, this, _1, _2, _3, _4));
    BIND_EVENT(sock, "failed", std::bind(&MainWindow::onGameFailed, this));
    BIND_EVENT(sock, "succeeded", std::bind(&MainWindow::onGameSucceeded, this));
    BIND_EVENT(sock, "incorrect", std::bind(&MainWindow::onWrongAnswer, this));
    //绑定连接信息处理
    _io->set_socket_open_listener(std::bind(&MainWindow::onConnected, this, _1));
    _io->set_close_listener(std::bind(&MainWindow::onClosed, this, _1));
    _io->set_fail_listener(std::bind(&MainWindow::onFailed, this));

    connect(Painter, SIGNAL(statusUpdate(int const&, int const&)), this, SLOT(onStatusUpdate(int const&, int const&)));
    //绑定封装函数 将IO事件发送至UI线程处理
    connect(this, SIGNAL(requestTextLogger(QString const&)), this, SLOT(uiTextLogger(QString const&)));
    connect(this, SIGNAL(requestChangeState(int const&, int const&)), this, SLOT(uiChangeState(int const&, int const&)));
    connect(this, SIGNAL(requestChangePainterStatus(int const&, int const&)), Painter, SLOT(statusChange(int const&, int const&)));
    connect(this, SIGNAL(requestChangeProblem(QString const&)), this, SLOT(uiChangeProblem(QString const&)));
    connect(this, SIGNAL(requestChangeCountdown(QString const&)), this, SLOT(uiChangeCountdown(QString const&)));
    connect(this, SIGNAL(requestResetButton()), this, SLOT(uiResetButton()));
    connect(this, SIGNAL(requestShowInfoDialog(QString const&, QString const&)), this, SLOT(uiShowInfoDialog(QString const&, QString const&)));

    //实例初始化
    // 从config.json文件中读取服务器地址
    std::ifstream f("config.json");
    // 将json文件解析成nlohmann::json对象
    nlohmann::json config = nlohmann::json::parse(f);
    // 从json文件中取出服务器地址
    std::string server = config["server"].get<std::string>();
    // 从json文件中取出logStorage的值
    logStorage = (isEnable)config["logStorage"].get<bool>();
    // 从json文件中取出demoMode的值
    demoMode = (isEnable)config["demoMode"].get<bool>();
    // 如果logStorage的值为enabled，则打印开始记录日志的消息
    if (logStorage == enabled) {
        qDebug() << "start logging";
    }
    // 如果demoMode的值为enabled，则从log.json文件中读取操作日志，否则连接服务器
    if (demoMode == enabled) {
        qDebug() << "in demo mode, without Internet connection";
        std::ifstream input("log.json");
        operationLog = nlohmann::json::parse(input);
    } else {
        _io->connect(server);
    }
    if (demoMode == enabled) {
        startDemoAsync();
    }
    QPushButton* button;
    for (int i = 0; i <= 7; ++i) {
        for (int j = 0; j <= 4; ++j) {
            button = new QPushButton("", ui->label_color);
            connect(button, &QPushButton::clicked, this, [=] {
                onColorSwitch(i, j);
                if (lastColor != NULL)
                    lastColor->setChecked(false);
                button->setChecked(true);
                lastColor = button;
            });
            button->setFlat(true);
            button->setCheckable(true);
            button->setGeometry(j * 22, i * 22, 22, 22);
            button->setStyleSheet(colorStyleSheet.arg(colorPalette::data[i][j]));
        }
    }
    defaultColor = button;
}

MainWindow::~MainWindow() {
    pointSoftenExternal::pointSoftenExternal_terminate();
    delete ui;
}

void MainWindow::onColorSwitch(int const& a, int const& b) {
    qDebug() << a << b;
    Q_EMIT requestTextLogger(QString("switchToColor %1").arg(colorPalette::data[a][b]));
    Q_EMIT requestChangePainterStatus(opColorSwitch, a << 8 | b);
    onStatusUpdate(opColorSwitch, a << 8 | b);
}

void MainWindow::fileTextLogger(int const& op, int const& data) {
    if (logStorage == disabled || demoMode == enabled)
        return;
    clock_t timestamp = clock();
    operationLog.push_back({timestamp, op, data});
}

void MainWindow::startDemoAsync() {
    // 设置标题为“演示模式”
    uiChangeProblem("演示模式");
    // 在文本框中显示提示文字
    uiTextLogger("如需退出演示模式，\n请更改配置文件");
    // 隐藏答案框、提交按钮、倒计时、导出按钮和请求晋级按钮
    ui->answer->setVisible(false);
    ui->pushButton_submit->setVisible(false);
    ui->label_countdown->setVisible(false);
    ui->pushButton_export->setVisible(false);
    ui->pushButton_requestPromote->setVisible(false);

    ui->label_problem->setVisible(true);
    // 创建异步线程，模拟操作
    fu = std::async(std::launch::async, [this]() {
        // 等待500毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // 遍历操作日志
        for (int i = 0; i < operationLog.size(); ++i) {
            // 计算间隔时间
            int diff = operationLog[i][0].get<int>() - ((i > 0) ? operationLog[i - 1][0].get<int>() : 0);
            int op = operationLog[i][1].get<int>();
            // 如果操作为添加点或者修改鼠标，并且间隔时间大于0，则等待间隔时间，最大100毫秒
            if ((op == opPointAdd || op == opMouseChange) && diff > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(diff < 100 ? diff : 100));
            // 发出修改绘图状态的请求
            Q_EMIT requestChangePainterStatus(operationLog[i][1].get<int>(), operationLog[i][2].get<int>());
        }
    });
}

void MainWindow::uiTextLogger(QString const& s) {
    ui->label_2->setText(s);
}

void MainWindow::uiChangeState(int const& type, int const& value) {
    switch (type) {
    case 0:
        ui->label_promotion->setText(QString(value ? "Primary" : "Secondary"));
        if (value) {
            ui->pushButton_clearPaint->setVisible(true);
            ui->pushButton_requestProblem->setVisible(true);
            ui->answer->setVisible(false);
            ui->pushButton_submit->setVisible(false);
            ui->label_color->setVisible(true);
            ui->label_problem->setVisible(true);
            ui->horizontalSlider->setVisible(true);
        } else {
            ui->pushButton_clearPaint->setVisible(false);
            ui->pushButton_requestProblem->setVisible(false);
            ui->answer->setVisible(true);
            ui->pushButton_submit->setVisible(true);
            ui->label_color->setVisible(false);
            ui->label_problem->setVisible(false);
            ui->horizontalSlider->setVisible(false);
        }
        break;
    case 1:
        ui->label_connection->setText(QString(value ? "connected" : "disconnected"));
        break;
    }
}

void MainWindow::uiChangeProblem(QString const& s) {
    ui->label_problem->setText(s);
}

void MainWindow::uiChangeCountdown(QString const& s) {
    ui->label_countdown->setText(s);
    if (s.length() <= 1) {
        ui->label_countdown->setStyleSheet(REDSHEET);
    } else {
        ui->label_countdown->setStyleSheet(BLACKSHEET);
    }
}

void MainWindow::uiResetButton() {
    defaultColor->click();
    ui->horizontalSlider->setValue(0);
}

void MainWindow::uiShowInfoDialog(QString const& title, QString const& content) {
    QMessageBox::information(ui->label, title, content, QMessageBox::Yes);
    Painter->forceInterrupt();
}

void MainWindow::onStatusUpdate(int const& type, int const& arg) {
    _io->socket()->emit("statusUpdate", int_message::create((arg << 4) | type));
    if (logStorage == enabled)
        fileTextLogger(type, arg);
}

void MainWindow::on_pushButton_requestPromote_clicked() {
    qDebug() << "ask for promotion";
    _io->socket()->emit("requestPromote");
}

void MainWindow::onPromotionChanged() {
    Q_EMIT requestTextLogger("onPromotionChanged");
    isPrimary = false;
    Painter->isPrimary = false;
    Q_EMIT requestChangeState(0, 0);
    Q_EMIT requestChangePainterStatus(opClearPaint, 0);
    Q_EMIT requestChangeCountdown(defaultCount);
}

void MainWindow::onPromotionChangedByself() {
    Q_EMIT requestTextLogger("onPromotionChangedByself");
    isPrimary = true;
    Painter->isPrimary = true;
    Q_EMIT requestChangeState(0, 1);
    Q_EMIT requestChangePainterStatus(opClearPaint, 0);
    Q_EMIT requestResetButton();
    Q_EMIT requestChangeCountdown(defaultCount);
}

void MainWindow::onProblemChanged() {
    Q_EMIT requestTextLogger("onProblemChanged");
    Q_EMIT requestChangeProblem("");
    Q_EMIT requestChangePainterStatus(opClearPaint, 0);
}

void MainWindow::onProblemChangedByself(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp) {
    Q_EMIT requestTextLogger("onProblemChangedByself");
    QString p = QString::fromStdString(data->get_string());
    Q_EMIT requestChangeProblem(p);
    Q_EMIT requestChangePainterStatus(opClearPaint, 0);
}

void MainWindow::onCountdownChanged(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp) {
    std::string secs = data->get_string();
    Q_EMIT requestChangeCountdown(QString::fromStdString(secs));
}

void MainWindow::onRemoteStatusChanged(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp) {
    Q_EMIT requestTextLogger("onRemoteStatusChanged");
    int d = data->get_int();
    int x = d >> 4;
    int y = d & 0xf;
    Q_EMIT requestChangePainterStatus(y, x);
}

void MainWindow::onConnected(std::string const& nsp) {
    Q_EMIT requestTextLogger("onConnected");
    Q_EMIT requestChangeState(1, 1);
}

void MainWindow::onClosed(client::close_reason const& reason) {
    Q_EMIT requestTextLogger("onClosed");
    Q_EMIT requestChangeState(1, 0);
}

void MainWindow::onFailed() {
    Q_EMIT requestTextLogger("onFailed");
    Q_EMIT requestChangeState(1, 0);
}

void MainWindow::on_pushButton_clearPaint_clicked() {
    Q_EMIT requestTextLogger("clearPaint");
    if (!isPrimary)
        return;
    Q_EMIT requestChangePainterStatus(opClearPaint, 0);
    onStatusUpdate(opClearPaint, 0);
}

void MainWindow::on_horizontalSlider_valueChanged(int value) {
    Q_EMIT requestTextLogger("clearPaint");
    if (!isPrimary)
        return;
    Q_EMIT requestChangePainterStatus(opSizeSwitch, value);
    onStatusUpdate(opSizeSwitch, value);
}

void MainWindow::on_pushButton_requestProblem_clicked() {
    Q_EMIT requestTextLogger("requestProblem");
    if (!isPrimary)
        return;
    _io->socket()->emit("requestProblem");
}

void MainWindow::onGameFailed() {
    Q_EMIT requestShowInfoDialog("失败", "Oooops！\n你们没有在规定时间内完成游戏。\n再试一次吧！");
}

void MainWindow::onGameSucceeded() {
    Q_EMIT requestShowInfoDialog("成功", "好厉害！猜对了！");
}

void MainWindow::onWrongAnswer() {
    Q_EMIT requestShowInfoDialog("答案错误", "猜错了！\n再试一次吧！");
}

void MainWindow::on_pushButton_export_clicked() {
    //构建BMP图片
    QPixmap* pmap = Painter->getPixmap();
    QImage img = pmap->toImage();
    delete pmap;
    BMPGenerator t(img.width(), img.height());
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QColor c = img.pixel(x, y);
            t.setPixel(x, y, c.red(), c.green(), c.blue());
        }
    }
    t.writeBMP();

    //记录操作记录
    if (logStorage == enabled) {
        std::ofstream output("log.json", std::ios::trunc);
        output << operationLog;
    }
}

void MainWindow::on_answer_textChanged() {
    QString textContent = ui->answer->toPlainText();
    // 设置最大字符数
    const int maxLength = 6;

    // 判断textContent是否包含换行符
    if (textContent.contains('\n')) {
        // 获取当前光标位置
        int position = ui->answer->textCursor().position();
        QTextCursor textCursor = ui->answer->textCursor();
        int index = -1;
        // 查找换行符
        while ((index = textContent.indexOf('\n')) != -1) {
            // 删除换行符
            textContent.remove(index, 1);
            // 如果换行符位置小于光标位置，则将光标位置减一
            if (index < position)
                position--;
        }
        // 重新设置文本内容
        ui->answer->setPlainText(textContent);
        // 设置新的光标位置
        textCursor.setPosition(position);
        ui->answer->setTextCursor(textCursor);
    }

    // 判断textContent的长度是否大于maxLength
    if (textContent.length() > maxLength) {
        // 获取当前光标位置
        int position = ui->answer->textCursor().position();
        int length = textContent.length();
        QTextCursor textCursor = ui->answer->textCursor();
        // 删除多余字符
        textContent.remove(position - (length - maxLength), length - maxLength);
        // 如果文本内容仍大于maxLength，则取前maxLength个字符
        if (textContent.length() > maxLength) {
            textContent = textContent.first(10);
        }
        // 重新设置文本内容
        ui->answer->setPlainText(textContent);
        // 设置新的光标位置
        textCursor.setPosition(position - (length - maxLength));
        ui->answer->setTextCursor(textCursor);
    }
}

void MainWindow::on_pushButton_submit_clicked() {
    QString textContent = ui->answer->toPlainText();
    _io->socket()->emit("submit", string_message::create(textContent.toStdString()));
}
