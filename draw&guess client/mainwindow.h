#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <sio_client.h>
#include <sio_message.h>

#include <QButtonGroup>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QSignalMapper>
#include <QString>
#include <functional>
#include <future>
#include <nlohmann/json.hpp>

#include "painter.h"
using namespace sio;
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    enum isEnable {
        enabled = true,
        disabled = false
    };
    enum oper {
        opMouseChange = 0,
        opColorSwitch,
        opSizeSwitch,
        opClearPaint,
        opPointAdd,
    };
public Q_SLOTS:
    //槽：用于接受画笔事件，并向服务端发送，type与oper枚举变量保持一致
    void onStatusUpdate(int const& type, int const& arg);
    //槽：用于向ui界面输出重要的日志
    void uiTextLogger(QString const& s);
    //槽：用于在配置文件中启用了logStorage时将所有鼠标操作进行记录
    void fileTextLogger(int const& op, int const& data);
private Q_SLOTS:
    //槽：用户要求将绘制权限提升为primary
    void on_pushButton_requestPromote_clicked();
    //槽：用户要求清空画布
    void on_pushButton_clearPaint_clicked();
    //槽：用户要求切换题目
    void on_pushButton_requestProblem_clicked();
    //槽：用户要求导出图片
    void on_pushButton_export_clicked();
    //槽：用户输入答案，对答案的字符进行合法化处理
    void on_answer_textChanged();
    //槽：用于提交答案
    void on_pushButton_submit_clicked();
    //槽：用户要求更改画笔粗细
    void on_horizontalSlider_valueChanged(int value);

    //槽：用于改变ui界面中连接与权限状态
    void uiChangeState(int const& type, int const& value);
    //槽：用于改变ui界面中的问题
    void uiChangeProblem(QString const& s);
    //槽：用于改变ui界面中的倒计时
    void uiChangeCountdown(QString const& s);
    //槽：用于初始化画笔选择
    void uiResetButton();
    //槽：用于展示成功或失败信息
    void uiShowInfoDialog(QString const& title, QString const& content);
    //槽：颜色选择器更新
    void onColorSwitch(int const& a, int const& b);

Q_SIGNALS:
    //信号：用于激活名称对应的槽函数
    void requestTextLogger(QString const& s);
    void requestChangeState(int const& type, int const& value);
    void requestChangePainterStatus(int const& type, int const& arg);
    void requestChangeProblem(QString const& s);
    void requestChangeCountdown(QString const& s);
    void requestResetButton();
    void requestShowInfoDialog(QString const& title, QString const& content);

private:
    Ui::MainWindow* ui;
    std::unique_ptr<client> _io;
    painter* Painter;
    QPushButton *defaultColor, *lastColor = NULL;

    bool isPrimary = false;                              //当前权限状态
    isEnable logStorage = disabled, demoMode = disabled; //配置文件中的开关状态
    nlohmann::json operationLog;                         //暂存的日志信息
    std::future<void> fu;                                //暂存async返回值，阻止提前析构
    QString colorStyleSheet =
        "QPushButton{ \
        background-color:%1; \
        border:0px outset rgb(0, 0, 0); \
        border-radius:0px; \
} \
    QPushButton:hover{ \
} \
    QPushButton:checked{ \
        background-color:%1; \
        border:2.5px outset rgb(0, 0, 0); \
        border-radius:0px; \
}";
    //网络事件：其他用户提升为primary
    void onPromotionChanged();
    //网络事件：当前用户提升为primary
    void onPromotionChangedByself();
    //网络事件：Primary用户重新抽题
    void onProblemChanged();
    //网络事件：本用户（Primary）抽题
    void onProblemChangedByself(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp);
    //网络事件：倒计时被服务器刷新
    void onCountdownChanged(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp);
    //网络事件：游戏失败
    void onGameFailed();
    //网络事件：游戏成功
    void onGameSucceeded();
    //网络事件：答案错误
    void onWrongAnswer();
    //网络事件：连接建立
    void onConnected(std::string const& nsp);
    //网络事件：连接关闭
    void onClosed(client::close_reason const& reason);
    //网络事件：连接异常断开
    void onFailed();
    //网络事件：远端Primary更改了画笔和画布的属性
    void onRemoteStatusChanged(std::string const& name, message::ptr const& data, bool hasAck, message::list& ack_resp);
    //启动演示模式
    void startDemoAsync();
};

#endif // MAINWINDOW_H
