// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __BREEDER_H_
#define __BREEDER_H_

#include <QObject>
#include <QImage>
#include <QRgb>
#include <QThread>

#include <limits>

#include "dna.h"
#include "genome.h"
#include "random/mersenne_twister.h"
#include "breedersettings.h"


class Breeder : public QThread
{
    Q_OBJECT

public:
    Breeder(QThread* parent = NULL);
    void reset(void);
    void populate(void);

    DNA dna(void);
    inline const QImage& image(void) const { return mGenerated; }
    inline const QImage& originalImage(void) const { return mOriginal; }
    inline unsigned long generation(void) const { return mGeneration; }
    inline quint64 currentFitness(void) const { return mFitness; }
    inline unsigned long selected(void) const { return mSelected; }

    // void proceed(void);
    void breed(QThread::Priority = QThread::InheritPriority);
    void stop(void);
    bool isDirty(void) const { return mDirty; }
    void setDNA(DNA);
    void setDirty(bool);
    void setGeneration(unsigned long);
    void setSelected(unsigned long);

protected:
    virtual void run(void);
    
private:
    void draw(void);

    bool mDirty;
    volatile bool mStopped;
    unsigned long mGeneration;
    quint64 mFitness;
    unsigned long mSelected;
    QImage mOriginal;
    QImage mGenerated;
    DNA mDNA;
    DNA mMutation;

signals:
    void evolved(const QImage&, const DNA&, quint64, unsigned long, unsigned long);
    void proceeded(unsigned long);
    
public slots:
    void setOriginalImage(const QImage&);
};

#endif // __BREEDER_H_

