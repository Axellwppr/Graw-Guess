#ifndef CURVE_H
#define CURVE_H

#include <QList>
#include <QPainterPath>

class SmoothCurveGenerator {
public:
    //根据鼠标轨迹生成平滑曲线路径
    static QPainterPath* generateSmoothCurve(QList<QPointF> const& points);
    //清洗具有抖动特征的点，并利用matlab导出函数对曲线进行平滑
    static void pointClean(QList<QPointF> const& rawPoints, QList<QPointF>& result);
};

#endif // CURVE_H
