#include "curve.h"

#include <QDebug>

#include "coder_array.h"
#include "pointSoftenExternal.h"

QPainterPath* SmoothCurveGenerator::generateSmoothCurve(QList<QPointF> const& rawPoints) { //返回平滑后的路径
    // 定义曲线张力参数和曲线线段数量
    double tension = 0.5, numberOfSegments = 4;
    // 定义曲线坐标和切线向量
    double x, y;
    double t1x, t2x, t1y, t2y;
    // 定义Hermite插值系数
    double c1, c2, c3, c4;
    double st; //插值权重
    // 初始化绘制路径
    QPainterPath* path = new QPainterPath;
    QList<QPointF> points, cleanPoints;
    //点列预处理 包括消除抖动和滤波
    pointClean(rawPoints, points);

    // 移动到原始路径的起点
    path->moveTo(rawPoints[0]);
    // 三次 Hermite 样条曲线
    // https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Cardinal_spline
    if (points.size() < 5) { //当已有点不足以生成平滑曲线时，优先绘制直线，确保用户体验
        for (int i = 1; i < (int)points.size(); i++) {
            path->lineTo(points[i]);
        }
        return path;
    }
    // 循环遍历所有点，构建三次Hermite样条曲线
    for (int i = 1; i <= (int)points.size() - 3; i++) {
        // 计算切线向量
        t1x = (points[i + 1].x() - points[i - 1].x()) * tension;
        t2x = (points[i + 2].x() - points[i].x()) * tension;
        t1y = (points[i + 1].y() - points[i - 1].y()) * tension;
        t2y = (points[i + 2].y() - points[i].y()) * tension;

        double distanceSqr = qPow(points[i].x() - points[i + 1].x(), 2) + qPow(points[i].y() - points[i + 1].y(), 2);
        // 如果相邻两点间距过小，则直接连接两点
        if (distanceSqr <= 9) {
            path->lineTo(points[i]);
            continue;
        }
        // 对于较长的线段，根据线段长度计算所需的曲线线段数量
        numberOfSegments = std::min((int)floor(distanceSqr / 8) + 1, 8);
        // 根据线段数量进行插值
        for (int t = 1; t <= numberOfSegments; t++) {
            st = (double)t / (double)numberOfSegments; //插值权重
            // 计算Hermite插值系数
            c1 = 2 * qPow(st, 3) - 3 * qPow(st, 2) + 1;
            c2 = -2 * qPow(st, 3) + 3 * qPow(st, 2);
            c3 = qPow(st, 3) - 2 * qPow(st, 2) + st;
            c4 = qPow(st, 3) - qPow(st, 2);

            // 计算曲线上的点的坐标
            x = c1 * points[i].x() + c2 * points[i + 1].x() + c3 * t1x + c4 * t2x;
            y = c1 * points[i].y() + c2 * points[i + 1].y() + c3 * t1y + c4 * t2y;

            // 将曲线上的点添加到路径中
            path->lineTo(QPointF(x, y));
        }
    }
    // 绘制到原始路径的终点
    path->lineTo(rawPoints[(int)rawPoints.size() - 1]);
    // 返回生成的平滑路径
    return path;
}

inline bool check1(QPointF const& p1, QPointF const& p2, QPointF const& p3) {
    return (p1.x() == p2.x() && p2.y() == p3.y() && abs(p1.y() - p2.y()) == 1 && abs(p2.x() - p3.x()) == 1);
}
void SmoothCurveGenerator::pointClean(QList<QPointF> const& rawPoints, QList<QPointF>& result) { //清除一部分抖动点，并滤波平滑
    // 如果原始点列表长度小于5，则直接返回原始点列表
    if (rawPoints.size() < 5) {
        result = rawPoints;
        return;
    }
    // 初始化一个新的点列表
    QPointF points[rawPoints.size() + 3];
    // 初始化变量
    bool lastState = true;
    int it = 1, n = rawPoints.size();
    points[it] = rawPoints[0];
    // 对于点列表中的每一个点
    for (int i = 1; i < n - 1; ++i) {
        // 如果这个点的前后两个点满足check1()函数的条件，则将lastState设为false
        if (lastState && (check1(rawPoints[i - 1], rawPoints[i], rawPoints[i + 1]) || check1(rawPoints[i + 1], rawPoints[i], rawPoints[i - 1]))) {
            lastState = false;
        } else {
            // 否则，将lastState设为true，并将当前点复制到新点列表中
            lastState = true;
            points[++it] = rawPoints[i];
        }
    }
    // 将点列表的最后一个点复制到新点列表中
    points[++it] = rawPoints[n - 1];
    // 更新点列表的长度
    n = it, it = 0;
    // 对于点列表中的每一段连续的x值
    for (int l = 1; l <= n; ++l) {
        int r;
        // 找到连续段的末尾
        for (r = l; r < n && points[r + 1].x() == points[l].x(); ++r)
            ;
        // 如果连续段只有一个点，则直接将这个点加入新点列表中
        if (r == l)
            points[++it] = points[r];
        else if (abs(points[r].y() - points[l].y()) <= 5) {
            // 如果连续段的y值差小于等于5，则将这段连续段替换为其平均值
            points[++it] = QPointF(points[l].x(), (points[l].y() + points[r].y()) / 2);
        } else {
            // 否则，根据y值的增长方向，计算出这段连续段的两个控制点
            int dir = (points[r].y() - points[l].y() > 0) ? 1 : -1;
            QPointF Point1 = QPointF(points[l].x(), (points[l].y() + points[l].y() + 3 * dir) / 2);
            QPointF Point2 = QPointF(points[r].x(), (points[r].y() + points[r].y() - 3 * dir) / 2);
            // 将控制点和中间的点加入新点列表中
            points[++it] = Point1;
            for (int i = l + 1; i < r; i += 3) points[++it] = points[i];
            points[++it] = Point2;
        }
        // 更新连续段的起始位置
        l = r;
    }
    // 更新点列表的长度
    n = it, it = 0;
    // 对于点列表中的每一段连续的y值
    for (int l = 1; l <= n; ++l) {
        int r;
        // 找到连续段的末尾
        for (r = l; r < n && points[r + 1].y() == points[l].y(); ++r)
            ;
        // 如果连续段只有一个点，则直接将这个点加入新点列表中
        if (r == l)
            points[++it] = points[r];
        else if (abs(points[r].x() - points[l].x()) <= 5) {
            // 如果连续段的x值差小于等于5，则将这段连续段替换为其平均值
            points[++it] = QPointF((points[l].x() + points[r].x()) / 2, points[l].y());
        } else {
            // 否则，根据x值的增长方向，计算出这段连续段的两个控制点
            int dir = (points[r].x() - points[l].x() > 0) ? 1 : -1;
            QPointF Point1 = QPointF((points[l].x() + points[l].x() + 3 * dir) / 2, points[l].y());
            QPointF Point2 = QPointF((points[r].x() + points[r].x() - 3 * dir) / 2, points[r].y());
            // 将控制点和中间的点加入新点列表中
            points[++it] = Point1;
            for (int i = l + 1; i < r; i += 3) points[++it] = points[i];
            points[++it] = Point2;
        }
        // 更新连续段的起始位置
        l = r;
    }
    // 更新点列表的长度
    n = it;

    coder::array<double, 2U> datax;
    coder::array<double, 2U> outputx;
    coder::array<double, 2U> datay;
    coder::array<double, 2U> outputy;
    int size = n;
    datax.set_size(1, size);
    datay.set_size(1, size);
    for (int i = 0; i < size; ++i) {
        datax[i] = points[i + 1].x();
        datay[i] = points[i + 1].y();
    }
    //调用matlab函数用于实现滤波
    pointSoftenExternal::pointSoftenExternal(datax, outputx);
    pointSoftenExternal::pointSoftenExternal(datay, outputy);

    //复制数据
    for (int i = 0; i < size; ++i) {
        result.append(QPointF(outputx[i], outputy[i]));
    }
}
