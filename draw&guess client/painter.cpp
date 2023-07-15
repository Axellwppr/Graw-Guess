#include "painter.h"

#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <chrono>

#include "color.h"
#include "curve.h"

painter::~painter() {
}

painter::painter(QWidget* parent)
    : QLabel(parent) {
    this->setMouseTracking(true); //启动鼠标跟踪
    this->isPressed = false;      //初始状态鼠标释放
    globalCacheMap.fill(Qt::white);
}

void painter::updateUI(bool const& forced) { //重绘界面
    if (lock)
        return;
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
    if (!forced && timeInterval.count() <= 20)
        return;
    lock = true;
    QPixmap current(globalCacheMap);
    QPainter painterCache(&globalCacheMap);

    QPen pen;
    painterCache.setRenderHint(QPainter::Antialiasing); //启用抗锯齿，但是效果一般
    for (int i = 0; i < lines.size(); i++) {            //绘制曲线，如果存在缓存则使用缓存
        pen.setColor(lines[i]->color);                  //自定义颜色
        pen.setWidth(lines[i]->size);
        painterCache.setPen(pen);
        if (lines[i]->Lazytag) { //本应用在传递数组类元素时，均采用指针方式传递，以降低复制开销提高绘制效率
            continue;
        } else if (i < lines.size() - 1) {
            QPainterPath* smoothCurvePath = SmoothCurveGenerator::generateSmoothCurve(*lines[i]->list);
            lines[i]->lazy = smoothCurvePath;
            lines[i]->Lazytag = true;
            painterCache.drawPath(*lines[i]->lazy);
        } else {
            QPainterPath* smoothCurvePath = SmoothCurveGenerator::generateSmoothCurve(*lines[i]->list);
            current = globalCacheMap;
            QPainter painterCurrent(&current);
            painterCurrent.setPen(pen);
            painterCurrent.setRenderHint(QPainter::Antialiasing); //启用抗锯齿，但是效果一般
            painterCurrent.drawPath(*smoothCurvePath);
            delete smoothCurvePath;
        }
    }
    setPixmap(current);
    lastTime = std::chrono::high_resolution_clock::now();
    timeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(lastTime - now);
    qDebug() << timeInterval.count();
    lock = false;
}

QPixmap* painter::getPixmap() {
    QPixmap* map = new QPixmap(1000, 1000);
    QPainter painter(map);
    QPen pen;
    painter.setRenderHint(QPainter::Antialiasing); //启用抗锯齿，但是效果不太理想
    map->fill(Qt::white);
    for (int i = 0; i < lines.size(); i++) { //绘制曲线，如果存在缓存则使用缓存
        pen.setColor(lines[i]->color);       //自定义颜色
        pen.setWidth(lines[i]->size);
        painter.setPen(pen);
        if (lines[i]->Lazytag) { //本应用在传递数组类元素时，均采用指针方式传递，以降低复制开销提高绘制效率
            painter.drawPath(*lines[i]->lazy);
        } else {
            QPainterPath* smoothCurvePath = SmoothCurveGenerator::generateSmoothCurve(*lines[i]->list);
            painter.drawPath(*smoothCurvePath);
            delete smoothCurvePath;
        }
    }
    setPixmap(*map);
    // 下面的代码可以用于将用户的鼠标点迹导出为文件，用于数据分析和调试
    //    QList<QPointF>& mlist = *lines[0]->list;
    //    FILE* x = fopen("./x", "w");
    //    FILE* y = fopen("./y", "w"); //export data for matlab
    //    for (int i = 0; i < mlist.size(); ++i) {
    //        fprintf(x, "%d\n", (int)mlist[i].x());
    //        fprintf(y, "%d\n", (int)mlist[i].y());
    //    }
    //    fclose(x);
    //    fclose(y);
    return map;
}

void painter::forceInterrupt() {
    mouseReleaseEvent((QMouseEvent*)NULL);
}

void painter::statusChange(int const& type, int const& arg) {
    switch (type) {
    case opMouseChange: {
        int sw = arg >> 24;
        if (sw == 1) { //switch == pressed
            int tx = (arg >> 12) & 0xfff;
            int ty = arg & 0xfff;
            qDebug() << sw << tx << ty;
            setCursor(Qt::PointingHandCursor);
            Line* x = new Line; //创建一条新的曲线并且将当前鼠标位置作为起点
            x->list = new QList<QPointF>;
            x->color = penColor; //当前颜色记录
            x->size = penSize;
            QPointF P = QPointF(tx, ty);
            x->list->append(P);
            x->list->append(P);
            lines.append(x);
            this->isPressed = true; //鼠标按下，开始记录轨迹
        } else if (sw == 0) {       //switch == released
            setCursor(Qt::ArrowCursor);
            this->isPressed = false;
        }
        break;
    }
    case opColorSwitch: {
        int a = arg >> 8, b = arg & 0xff;
        penColor = QColor(colorPalette::data[a][b]);
        qDebug() << penColor;
        break;
    }
    case opSizeSwitch: {
        penSize = double(arg) / 100 * 9 + 2;
        qDebug() << penSize;
        break;
    }
    case opClearPaint: {
        clearPaint();
        break;
    }
    case opPointAdd: {
        int x = arg >> 12;
        int y = arg & 0xfff;
        pointAdd(QPointF(x, y));
        break;
    }
    default:
        break;
    }
}

void painter::pointAdd(QPointF const& e) {
    QList<QPointF>& mlist = *lines[lines.size() - 1]->list; //引用简化编码
    //接下来的部分会根据鼠标位移情况丢弃一部分因鼠标抖动造成的偏移点，使绘制曲线更美观平滑
    //同时需要注意不能一口气丢弃过多的点，导致用户的正常操作受到干扰
    double dL = (mlist.size()) ? (qPow((mlist[mlist.size() - 1].x() - e.x()), 2) + qPow((mlist[mlist.size() - 1].y() - e.y()), 2))
                               : 100; //计算本次鼠标与上次鼠标距离dL
    double dToControl = (isAvailable) ? (qPow((lastControl.x() - e.x()), 2) + qPow((lastControl.y() - e.y()), 2))
                                      : 500;
    if (deleteTag) {
        if (mlist.size())
            mlist.pop_back();
        deleteTag = false;
    }
    if (dL >= 5) {
        delSize = 0;
        mlist.append(e);
        isAvailable = true;
        lastControl = e;
        updateUI();
    } else if (dToControl <= 15) {
        deleteTag = true; //遇到疑似抖动点不可直接丢弃，需标记删除后由下一个鼠标点替换，否则会出现点迹追不上鼠标的情况
        delSize++;
        mlist.append(e);
        updateUI();
    } else {
        delSize = 0; //当一次性删除过多点时，用户操作会受到影响，此时需立即加入该点（无论是否抖动）来补偿
        mlist.append(e);
        isAvailable = true;
        lastControl = e;
        updateUI();
    }
    if (mlist.size() >= 128) {
        statusChange(opMouseChange, (1 << 24) | (((int)e.x()) << 12) | (int)e.y());
        deleteTag = false;
    }
}

void painter::mouseMoveEvent(QMouseEvent* e) {
    if (!isPrimary)
        return;
    int x = e->pos().x();
    if (x < 0)
        x = 0;
    if (x > 1000)
        x = 1000;
    int y = e->pos().y();
    if (y < 0)
        y = 0;
    if (y > 1000)
        y = 1000;
    if (this->isPressed) {
        Q_EMIT statusUpdate(opPointAdd, (x << 12) | y);
        statusChange(opPointAdd, (x << 12) | y);
    }
}

void painter::mousePressEvent(QMouseEvent* e) {
    if (!isPrimary)
        return;
    if (isPressed)
        return;
    int x = e->pos().x();
    if (x < 0)
        x = 0;
    if (x > 1000)
        x = 1000;
    int y = e->pos().y();
    if (y < 0)
        y = 0;
    if (y > 1000)
        y = 1000;
    Q_EMIT statusUpdate(opMouseChange, (1 << 24) | (x << 12) | y);
    statusChange(opMouseChange, (1 << 24) | (x << 12) | y);
}

void painter::mouseReleaseEvent(QMouseEvent*) {
    if (!isPrimary)
        return;
    if (!isPressed)
        return;
    isAvailable = false;
    Q_EMIT statusUpdate(opMouseChange, 0);
    statusChange(opMouseChange, 0);
    updateUI(true);
}

void painter::clearPaint() {
    while (lines.length()) {
        Line* x = lines.back();
        lines.pop_back();
        delete x;
    }
    globalCacheMap.fill(Qt::white);
    updateUI();
}
