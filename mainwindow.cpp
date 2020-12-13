#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCameraInfo>
#include <QThread>
#include <QTimer>

#include <iostream>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(640, 960);  // TODO
}

void MainWindow::init()
{
    std::cout << "The main thread is: " << QThread::currentThread() << std::endl;
    ui->videoInputStream_->init();    
}

void MainWindow::uninit()
{
    ui->videoInputStream_->uninit();
}

MainWindow::~MainWindow()
{
    delete ui;
}

