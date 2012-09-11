// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QDateTime>
#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QtCore/QDebug>

#include "qt-json/json.h"
#include "genome.h"
#include "dna.h"
#include "svgreader.h"
#include "breedersettings.h"
#include "main.h"

using namespace QtJson;


/// deep copy
DNA::DNA(const DNA& dna)
    : mSize(dna.mSize)
{
    mDNA.reserve(dna.mDNA.size());
    for (DNAType::const_iterator genome = dna.mDNA.constBegin(); genome != dna.mDNA.constEnd(); ++genome)
        mDNA.append(*genome);
}


inline bool DNA::willMutate(unsigned int rate) {
    return (MT::random() % rate) == 0;
}


void DNA::mutate(void)
{
    // maybe spawn a new genome
    if (willMutate(gBreederSettings.genomeEmergenceProbability()) && mDNA.size() < gBreederSettings.maxGenomes())
        mDNA.append(Genome(true));
    // maybe kill a genome
    if (willMutate(gBreederSettings.genomeKillProbability()) && mDNA.size() > gBreederSettings.minGenomes())
        mDNA.remove(MT::random() % mDNA.size());
    if (willMutate(gBreederSettings.genomeMoveProbability())) {
        const int oldIndex = MT::random() % mDNA.size();
        const int newIndex = MT::random() % mDNA.size();
        if (oldIndex != newIndex) {
            const Genome genome = mDNA.at(oldIndex);
            mDNA.remove(oldIndex);
            mDNA.insert(newIndex, genome);
        }
    }
    // mutate all contained genomes
    for (DNAType::iterator genome = mDNA.begin(); genome != mDNA.end(); ++genome)
        genome->mutate();
}


// XXX: move method to Breeder
bool DNA::save(const QString& filename, unsigned long generation, unsigned long selected, quint64 fitness)
{
    bool rc;
    QFile file(filename);
    rc = file.open(QIODevice::WriteOnly);
    if (!rc)
        return false;
    QTextStream out(&file);
    if (filename.endsWith(".json") || filename.endsWith(".dna")) {
        out << "{\n"
            << " \"datetime\": \"" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "\",\n"
            << " \"generation\": \"" << generation << "\",\n"
            << " \"selected\": \"" << selected << "\",\n"
            << " \"fitness\": \"" << fitness << "\",\n"
            << " \"size\": { \"width\": " << mSize.width() << ", \"height\": " << mSize.height() << " },\n"
            << " \"dna\": [\n";
        for (DNAType::const_iterator genome = mDNA.constBegin(); genome != mDNA.constEnd(); ++genome) {
            out << *genome;
            if ((genome+1) != mDNA.constEnd())
                out << ",";
            out << "\n";
        }
        out << "] }\n";
    }
    else if (filename.endsWith(".svg")) {
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"" << mSize.width() << "\" height=\"" << mSize.height() << "\">\n"
            << "<title>Mutation " << selected << " out of " << generation << "</title>\n"
            << "<desc>Generated by evo-cubist. Copyright (c) 2012 Oliver Lau</desc>\n"
            << "<desc xmlns:evocubist=\"http://von-und-fuer-lau.de/evo-cubist\" version=\"" << AppVersion << "\">\n"
            << " <evocubist:datetime>" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "</evocubist:datetime>\n"
            << " <evocubist:generation>" << generation << "</evocubist:generation>\n"
            << " <evocubist:selected>" << selected << "</evocubist:selected>\n"
            << " <evocubist:fitness>" << fitness << "</evocubist:fitness>\n"
            << "</desc>\n"
            << " <g transform=\"scale(" << mSize.width() << ", " << mSize.height() << ")\">\n";
        for (DNAType::const_iterator genome = mDNA.constBegin(); genome != mDNA.constEnd(); ++genome) {
            if (genome->polygon().isEmpty()) // just in case
                continue;
            const QColor& c = genome->color();
            out << "  <path style=\""
                << "fill-opacity:" << c.alphaF() << ";"
                << "fill:rgb(" << c.red() << "," << c.green() << "," << c.blue() << ");" << "\""
                << " d=\"M " << genome->polygon().first().x() << " " << genome->polygon().first().y();
            for (QPolygonF::const_iterator p = genome->polygon().constBegin() + 1; p != genome->polygon().constEnd(); ++p)
                out << " L " << p->x() << " " << p->y();
            out << " Z\" />\n";
        }
        out << " </g>\n"
            << "</svg>\n";
    }
    else {
        qWarning() << "DNA::save() unknown format";
    }
    file.close();
    return rc;
}


// XXX: move method to Breeder
bool DNA::load(const QString& filename)
{
    bool rc;
    QFile file(filename);
    rc = file.open(QIODevice::ReadOnly);
    if (!rc)
        return false;
    QTextStream in(&file);
    bool ok = false;
    if (filename.endsWith(".json") || filename.endsWith(".dna")) {
        QString jsonDNA = in.readAll();
        const QVariant& v = Json::parse(jsonDNA, ok);
        const QVariantMap& result = v.toMap();
        if (ok) {
            clear();
            const QVariantMap& size = result["size"].toMap();
            mSize = QSize(size["width"].toInt(), size["height"].toInt());
            const QVariantList& dna = result["dna"].toList();
            for (QVariantList::const_iterator genome = dna.constBegin(); genome != dna.constEnd(); ++genome) {
                const QVariantMap& g = genome->toMap();
                const QVariantMap& rgb = g["color"].toMap();
                QColor color(rgb["r"].toInt(), rgb["g"].toInt(), rgb["b"].toInt());
                color.setAlphaF(rgb["a"].toDouble());
                const QVariantList& vertices = g["vertices"].toList();
                QPolygonF polygon;
                for (QVariantList::const_iterator point = vertices.constBegin(); point != vertices.constEnd(); ++point) {
                    const QVariantMap& p = point->toMap();
                    polygon << QPointF(p["x"].toDouble(), p["y"].toDouble());
                }
                mDNA.append(Genome(polygon, color));
            }
        }
    }
    else if (filename.endsWith(".svg")) {
        SVGReader xml;
        ok = xml.readSVG(&file);
        if (ok) {
            clear();
            *this = xml.dna();
        }
        else {
            mErrorString = xml.errorString();
            qWarning() << xml.errorString();
        }
    }
    else {
        qWarning() << "DNA::load() unknown file format";
        ok = false;
    }
    file.close();

    return rc && ok;
}


unsigned int DNA::points(void) const
{
    unsigned int sum = 0;
    for (DNAType::const_iterator genome = mDNA.constBegin(); genome != mDNA.constEnd(); ++genome)
        sum += genome->polygon().size();
    return sum;
}
