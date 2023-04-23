#include "settingwindow.h"
#include "ui_settingwindow.h"
#include "collabroom.h"
#include "codec/frame_to_av.h"
#include "QSettings"
#include "QFile"
#include "QNetworkReply"
#include "QMessageBox"

SettingWindow::SettingWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::SettingWindow) {
    room = nullptr;
    init();
}

SettingWindow::SettingWindow(CollabRoom *parent) :
        QMainWindow(parent),
        ui(new Ui::SettingWindow) {
    room = parent;
    init();
}

SettingWindow::~SettingWindow() {
    delete ui;
}

void SettingWindow::updateDebug() {
    ui->debugInfo->setText(DebugCenter::debugInfo());
}

void SettingWindow::init() {
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    connect(&debugGather, &QTimer::timeout, this, &SettingWindow::updateDebug);
    debugGather.start(100);

    auto encoders = FrameToAv::getEncoders();
    for (auto &e: encoders) {
        ui->encoders->addItem(e.readable);
    }

    ui->shouldForceEncoder->setChecked(settings.value("forceEncoder").toBool());
    ui->encoders->setCurrentText(settings.value("forceEncoderName").toString());
    ui->useDxCapture->setChecked(settings.value("useDxCapture").toBool());
    ui->showDxgiWindow->setChecked(settings.value("showDxgiWindow").toBool());
    ui->forceShmem->setChecked(settings.value("forceShmem").toBool());
    ui->enableBuffering->setChecked(settings.value("enableBuffering").toBool());
    ui->disableIntraRefresh->setChecked(settings.value("disableIntraRefresh").toBool());

    connect(ui->disableIntraRefresh, &QCheckBox::clicked, this, [=, this](bool v) {
       settings.setValue("disableIntraRefresh", v);
       settings.sync();

        qDebug() << "disableIntraRefresh" << v;
    });

    connect(ui->encoders, &QComboBox::currentTextChanged, this, [=, this](const QString &s) {
        settings.setValue("forceEncoderName", s);
        settings.sync();
        qDebug() << "force encoder to" << s;
    });

    connect(ui->shouldForceEncoder, &QCheckBox::clicked, this, [=, this](bool v) {
        if (v) {
            settings.setValue("forceEncoderName", ui->encoders->currentText());
        }
        settings.setValue("forceEncoder", v);
        settings.sync();
        qDebug() << "force encoder" << v;
    });

    connect(ui->useDxCapture, &QCheckBox::clicked, this, [=, this](bool v) {
        settings.setValue("useDxCapture", v);
        settings.sync();
        qDebug() << "force use dx capture" << v;
    });

    connect(ui->showDxgiWindow, &QCheckBox::clicked, this, [=, this](bool v) {
        settings.setValue("showDxgiWindow", v);
        settings.sync();
        qDebug() << "show dxgi window" << v;
    });

    connect(ui->restoreIgnored, &QCheckBox::clicked, this, [=, this](bool v) {
        for (auto& i : settings.allKeys()) {
            if (i.startsWith("ignore"))
                settings.setValue(i, false);
        }
        settings.sync();
        qDebug() << "restore ignored";
    });

    connect(ui->actionSendLog, &QAction::triggered, this, [=, this]() {
        QNetworkRequest req;
        req.setUrl(QUrl("https://misc.reito.fun/report"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "plain/text");

        QFile outFile(VTSLINK_LOG_FILE);
        outFile.open(QIODevice::ReadOnly);
        auto arr = outFile.readAll();

        QNetworkReply* reply = networkAccessManager.post(req, arr);

        auto* box = new QMessageBox;
        box->setIcon(QMessageBox::Information);
        box->setWindowTitle(tr("正在上传"));
        box->setText(tr("请稍后"));
        auto ok = box->addButton(tr("确定"), QMessageBox::NoRole);
        auto cancel = box->addButton(tr("取消"), QMessageBox::NoRole);
        ok->setEnabled(false);

        connect(reply, &QNetworkReply::uploadProgress, this, [=](qint64 sent, qint64 total) {
            box->setText(tr("请稍后，已上传 %1/%2").arg(sent).arg(total));
        });

        connect(reply, &QNetworkReply::finished, this, [=]() {
            ok->setEnabled(true);
            cancel->setEnabled(false);
            box->setWindowTitle(tr("上传完成"));
            if (reply->error() != QNetworkReply::NetworkError::NoError) {
                box->setText(reply->errorString());
            } else {
                box->setText(tr("上传成功"));
            }
            reply->deleteLater();
        });

        connect(box, &QMessageBox::finished, this, [=]() {
            auto ret = dynamic_cast<QPushButton *>(box->clickedButton());
            if (ret == cancel) {
                reply->abort();
            }
            box->deleteLater();
        });

        box->show();
    });

    connect(ui->actionCrash, &QAction::triggered, this, [=]() {
       throw std::exception("active crashed");
    });

    connect(ui->enableBuffering, &QCheckBox::clicked, this, [=, this](bool v) {
        settings.setValue("enableBuffering", v);
        settings.sync();
        qDebug() << "enable buffering" << v;
    });

    connect(ui->forceShmem, &QCheckBox::clicked, this, [=, this](bool v) {
        settings.setValue("forceShmem", v);
        settings.sync();
        qDebug() << "force shmem" << v;
    });

    //TODO: 计算机\HKEY_CURRENT_USER\Software\Microsoft\DirectX\UserGpuPreferences
}
