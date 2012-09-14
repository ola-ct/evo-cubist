// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QtGlobal>
#include <QtCore/QDebug>
#include <limits>
#include "gene.h"
#include "breedersettings.h"
#include "random/rnd.h"


Gene::Gene(bool randomize)
{
    if (randomize) {
        const int N = random(gSettings.minPointsPerGene(), gSettings.maxPointsPerGene());
        for (int x = 0; x < N; ++x)
            mPolygon.append(QPointF(random1(), random1()));
        mColor.setRgb(random(256), random(256), random(256), random(gSettings.minA(), gSettings.maxA()));
    }
}


void Gene::deepCopy(const QPolygonF& polygon)
{
    mPolygon.reserve(polygon.size());
    for (QPolygonF::const_iterator p = polygon.constBegin(); p != polygon.constEnd(); ++p)
        mPolygon.append(*p);
}


QVector<Gene> Gene::splice(void) const
{
    return (mPolygon.size() == 3)
            ? bisect()
            : triangulize();
}


/// cut triangle in halves by slicing it from the middle of the longest leg to the opposite edge
QVector<Gene> Gene::bisect(void) const
{
    Q_ASSERT(mPolygon.size() == 3);
    QPolygonF triangle = mPolygon;
    triangle.append(mPolygon.at(0));
    triangle.append(mPolygon.at(1));
    // find longest leg
    QPolygonF::const_iterator p0 = triangle.constBegin();
    qreal possibleLongestLength = QLineF(*p0, *(p0+1)).length();
    for (QPolygonF::const_iterator p = triangle.constBegin()+1; p != triangle.constEnd()-1; ++p) {
        const qreal l = QLineF(*p, *(p+1)).length();
        if (l > possibleLongestLength) {
            possibleLongestLength = l;
            p0 = p;
        }
    }
    QPointF mid = (*p0 + *(p0+1)) / 2;
    QPolygonF triangle1;
    triangle1 << *p0 << mid << *(p0+2);
    QPolygonF triangle2;
    triangle2 << *(p0+1) << *(p0+2) << mid;
    QVector<Gene> result;
    result << Gene(triangle1, mColor) << Gene(triangle2, mColor);
    return result;
}


/// convert point cloud to triangles (dumb O(n^2) implementation)
/// see http://de.wikipedia.org/wiki/Delaunay-Triangulation
QVector<Gene> Gene::triangulize(void) const
{
    QVector<Gene> result;
    QPolygonF Q = mPolygon;
    for (int i = 0; i < Q.size(); ++i) {
        for (int j = 0; j < Q.size(); ++j) {
            if (j == i)
                continue;
            // TODO: ...
        }
    }
    return result;
}


inline int cross(const QPointF& O, const QPointF& A, const QPointF& B)
{
    return (A.x() - O.x()) * (B.y() - O.y()) - (A.y() - O.y()) * (B.x() - O.x());
}


inline bool pointLessThan(const QPointF& a, const QPointF& b)
{
    return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
}


QPolygonF Gene::convexHull(void) const
{
    QPolygonF P = mPolygon;
    const int n = P.size();
    int k = 0;
    QPolygonF H(2*n);

    // sort points lexicographically
    qSort(P.begin(), P.end(), pointLessThan);

    // build lower hull
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k-2], H[k-1], P.at(i)) <= 0)
            --k;
        H[k++] = P.at(i);
    }
    // build upper hull
    for (int i = n-2, t = k+1; i >= 0; --i) {
        while (k >= t && cross(H[k-2], H[k-1], P.at(i)) <= 0)
            --k;
        H[k++] = P.at(i);
    }

    H.resize(k-1);
    return H;
}


inline bool Gene::willMutate(int probability) const {
    return random(probability) == 0;
}


void Gene::mutate(void)
{
    if (willMutate(gSettings.pointEmergenceProbability()) && mPolygon.size() < gSettings.maxPointsPerGene()) {
        const int i = random(mPolygon.size());
        const int j = (i+1) % mPolygon.size();
        const QPointF& p0 = mPolygon.at(i);
        const QPointF& p1 = mPolygon.at(j);
        mPolygon.insert(j, (p0 + p1) / 2);
    }
    if (willMutate(gSettings.pointKillProbability()) && mPolygon.size() > gSettings.minPointsPerGene())
        mPolygon.remove(random(mPolygon.size()));
    for (QPolygonF::iterator p = mPolygon.begin(); p != mPolygon.end(); ++p) {
        if (willMutate(gSettings.pointMutationProbability())) {
            p->setX(dReal(p->x(), gSettings.dXY(), 0.0, 1.0));
            p->setY(dReal(p->y(), gSettings.dXY(), 0.0, 1.0));
        }
    }
    if (willMutate(gSettings.colorMutationProbability())) {
        const int r = dInt(mColor.red(), gSettings.dR(), 0, 255);
        const int g = dInt(mColor.green(), gSettings.dR(), 0, 255);
        const int b = dInt(mColor.blue(), gSettings.dR(), 0, 255);
        const int a = dInt(mColor.alpha(), gSettings.dA(), gSettings.minA(), gSettings.maxA());
        mColor.setRgb(r, g, b, a);
    }
}


QTextStream& operator<< (QTextStream& s, const Gene& gene)
{
    const QColor& color = gene.color();
    s << "{\n"
      << "  \"color\": { " << QString("\"r\": %1, \"g\": %2, \"b\": %3, \"a\": %4").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF()) << " },\n";
    s << "  \"vertices\": [\n";
    for (QPolygonF::const_iterator p = gene.polygon().constBegin(); p != gene.polygon().constEnd(); ++p) {
        s << "    { " << QString("\"x\": %1, \"y\": %2").arg(p->x()).arg(p->y()) << " }";
        if ((p+1) != gene.polygon().constEnd())
            s << ",";
        s << "\n";
    }
    s << "  ]\n"
      << "}";
    return s;
}
