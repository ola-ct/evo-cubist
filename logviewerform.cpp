// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QDateTime>
#include <QAction>
#include <QString>
#include <QtCore/QDebug>
#include "logviewerform.h"
#include "ui_logviewerform.h"


LogViewerForm::LogViewerForm(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LogViewerForm)
{
    ui->setupUi(this);
    mMenu = new QMenu(this);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mMenu->addAction(tr("Show picture"))->setData("show");
    mMenu->addAction(tr("Copy picture to clipboard"))->setData("copy");
    mMenu->addAction(tr("Go to folder containing picture"))->setData("goto");
    QObject::connect(ui->tableWidget, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(provideContextMenu(const QPoint&)));
    QObject::connect(ui->tableWidget, SIGNAL(cellDoubleClicked(int,int)), SLOT(cellDoubleClicked(int,int)));
    QObject::connect(ui->clearPushButton, SIGNAL(clicked()), SLOT(clear()));
}


LogViewerForm::~LogViewerForm()
{
    delete ui;
}


void LogViewerForm::showEvent(QShowEvent*)
{
    raise();
}


void LogViewerForm::log(unsigned long generation, unsigned long selected, int numPoints, int numGenes, quint64 fitness, const QImage& image)
{
    const int N = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(N);
    QTableWidgetItem* picItem = new QTableWidgetItem;
    picItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    const QImage& scaledImage = image.scaledToHeight(31, Qt::SmoothTransformation);
    picItem->setData(Qt::DecorationRole, scaledImage);
    picItem->setData(Qt::DisplayRole, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    ui->tableWidget->setItem(N, 0, picItem);
    ui->tableWidget->setItem(N, 1, new QTableWidgetItem(QString("%1").arg(selected)));
    ui->tableWidget->item(N, 1)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->tableWidget->setItem(N, 2, new QTableWidgetItem(QString("%1").arg(generation)));
    ui->tableWidget->item(N, 2)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->tableWidget->setItem(N, 3, new QTableWidgetItem(QString("%1").arg(numPoints)));
    ui->tableWidget->item(N, 3)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->tableWidget->setItem(N, 4, new QTableWidgetItem(QString("%1").arg(numGenes)));
    ui->tableWidget->item(N, 4)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->tableWidget->setItem(N, 5, new QTableWidgetItem(QString("%1").arg(fitness)));
    ui->tableWidget->item(N, 5)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->tableWidget->scrollToBottom();
}


void LogViewerForm::cellDoubleClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    // TODO: show generated image
}


void LogViewerForm::clear(void)
{
    int row = ui->tableWidget->rowCount();
    while (row-- > 0)
        ui->tableWidget->removeRow(row);
}


void LogViewerForm::provideContextMenu(const QPoint& p)
{
    QTableWidgetItem* item = ui->tableWidget->itemAt(p);
    if (item != NULL) {
        qDebug() << item->row() << item->column();
        QAction* action = mMenu->exec(mapToGlobal(p));
        if (action != NULL) {
            const QString& cmd = action->data().toString();
            const int selected   = ui->tableWidget->item(item->row(), 1)->text().toInt();
            const int generation = ui->tableWidget->item(item->row(), 2)->text().toInt();
            if (cmd == "show") {
                emit showPicture(generation, selected);
            }
            else if (cmd == "copy") {
                emit copyPicture(generation, selected);
            }
            else if (cmd == "goto") {
                emit gotoPicture(generation, selected);
            }
            else
                qWarning() << "unknow command:" << cmd;
        }
    }
}
