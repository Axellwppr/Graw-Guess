#ifndef PAINTER_H
#define PAINTER_H

#include <QColor>
#include <QLabel>
#include <QList>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QWidget>

#include "color.h"

struct Line {             //线段结构体，存储单次绘制的所有点轨迹
    QList<QPointF>* list; //原始点迹
    QColor color;
    QPainterPath* lazy;   //算法生成的平滑曲线，存储以加速渲染
    bool Lazytag = false; //懒惰标记，确保每一条线只计算一次
    double size = 2.0;
};

class painter : public QLabel {
    Q_OBJECT
public:
    ~painter();
    painter(QWidget* parent);
    enum oper {
        opMouseChange = 0,
        opColorSwitch,
        opSizeSwitch,
        opClearPaint,
        opPointAdd
    };
    void mousePressEvent(QMouseEvent* e) override;   //监听鼠标按下事件
    void mouseMoveEvent(QMouseEvent* e) override;    //监听鼠标移动事件
    void mouseReleaseEvent(QMouseEvent* e) override; //监听鼠标释放事件
    void forceInterrupt();                           //游戏结束时打断用户当前的绘制
    QPixmap* getPixmap();                            //负责图片导出功能
    bool isPrimary = false;                          //记录当前是否处于Primary状态
public Q_SLOTS:
    void statusChange(int const& type, int const& arg); //处理所有画布状态的改变
Q_SIGNALS:
    void statusUpdate(int const& type, int const& arg); //信号，向 `onStatusUpdate`槽 传递画笔状态改变
private:
    bool isPressed;                                     //标记当前鼠标是否压下
    QList<Line*> lines;                                 //储存鼠标点运动轨迹
    QColor penColor = QColor(colorPalette::data[7][4]); //当前画笔颜色，默认为黑色
    double penSize = 2;                                 //当前画笔粗细
    int delSize = 0;                                    //已经删除的点的个数，用于过滤无效的输入
    bool deleteTag = false;                             //删除标记，用于过滤无效输入
    void clearPaint();                                  //处理清空画笔请求（local/remote）
    void pointAdd(QPointF const& x);                    //处理单点增加请求（local/remote）
    void updateUI(bool const& forced = false);          //在画笔内容发生改变时刷新UI
    QPixmap globalCacheMap = QPixmap(1000, 1000);       //对于已经绘制完成的部分进行缓存

    //局部变量
    bool lock = false;                                       //界面重绘锁
    std::chrono::high_resolution_clock::time_point lastTime; //上一次界面重绘的时间，用于控制刷新率，减小运算负载
    QPointF lastControl;                                     //上一个有效鼠标点坐标，用于鼠标防抖
    bool isAvailable = false;                                //上一个有效鼠标点是否合法，每次创建一条新线段时，该值会被置为false
};

#endif // PAINTER_H
