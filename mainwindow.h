// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QDateTime>
#include <QString>
#include <QTimer>
#include <QFile>
#include <QtCore/QDebug>

#include "imagewidget.h"
#include "generationwidget.h"
#include "breeder.h"
#include "optionsform.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();


protected:
    void closeEvent(QCloseEvent*);

private:
    Ui::MainWindow *ui;
    OptionsForm mOptionsForm;
    ImageWidget* mImageWidget;
    GenerationWidget* mGenerationWidget;
    Breeder mBreeder;
    QDateTime mStartTime;
    QString mLastSavedDNA;
    QString mSaveDirectory;
    QThread::Priority mPriority;
    QFile mLog;

    void saveAppSettings(void);
    void restoreAppSettings(void);
    void startBreeding(void);
    void stopBreeding(void);
    void loadOriginalImage(const QString& filename);

    QTimer mAutoSaveTimer;

private slots:
    void loadDNA(const QString& filename);
    void evolved(const QImage&, const DNA&, quint64 fitness, unsigned long selected, unsigned long generation);
    void imageDropped(const QImage&);
    void proceeded(unsigned long);
    void startStop(void);
    void saveDNA(void);
    void openDNA(void);
    void openOriginalImage(void);
    void resetBreeder(void);
    void about(void);
    void aboutQt(void);
    void autoSaveGeneratedImage(void);
    void autoSaveIntervalChanged(int);
    void autoSaveToggled(bool);
    void priorityChanged(QThread::Priority);

    void breederFinished(void) { qDebug() << "breeder has finished"; }
};

#endif // __MAINWINDOW_H_
