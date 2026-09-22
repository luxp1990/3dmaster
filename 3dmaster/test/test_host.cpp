#include <QApplication>
#include <QPluginLoader>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include "seer/viewerbase.h"

void logMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(type);
    Q_UNUSED(context);
    QFile f("D:/seer/3dmaster/3dmaster/test_log.txt");
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << msg << "\n";
    }
}

class TestMonitor : public QObject {
    Q_OBJECT
public:
    bool loadingStarted = false;
    bool loadingSucceeded = false;
    bool loadingFailed = false;

public slots:
    void onSigCommand(int cmd, const QVariant& data) {
        qDebug() << "[Test Host] RECEIVED sigCommand: cmd=" << cmd << "data=" << data;
        if (cmd == VCT_StateChange) {
            int state = data.toInt();
            if (state == VCV_Loading) {
                loadingStarted = true;
                qDebug() << "[Test Host] -> State: VCV_Loading (0) - Loading in progress";
            } else if (state == VCV_Loaded) {
                loadingSucceeded = true;
                qDebug() << "[Test Host] -> State: VCV_Loaded (1) - MODEL LOADED SUCCESSFULLY!";
            } else if (state == VCV_Error) {
                loadingFailed = true;
                qDebug() << "[Test Host] -> State: VCV_Error (2) - Model load failed";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    QFile::remove("D:/seer/3dmaster/3dmaster/test_log.txt");
    qInstallMessageHandler(logMessage);

    QApplication app(argc, argv);

    QString pluginPath = "D:/seer/3dmaster/3dmaster/build/Release/3dmaster.dll";
    if (!QFile::exists(pluginPath)) {
        pluginPath = "D:/seer/3dmaster/3dmaster/3dmaster.dll";
    }

    QString testModel = "C:/Users/50350/Downloads/三腔接头-260416-2.stp";
    if (argc > 1) {
        testModel = QString::fromLocal8Bit(argv[1]);
    }
    QString screenshotPath = "D:/seer/3dmaster/3dmaster/test_render.png";
    if (argc > 2) {
        screenshotPath = QString::fromLocal8Bit(argv[2]);
    }

    qDebug() << "========================================";
    qDebug() << "[Test Host] Starting verification of 3dmaster plugin with official Seer-sdk";
    qDebug() << "[Test Host] Plugin:" << pluginPath;
    qDebug() << "[Test Host] Test Model:" << testModel;
    qDebug() << "========================================";

    QPluginLoader loader(pluginPath);
    QObject* instance = loader.instance();
    if (!instance) {
        qCritical() << "[Test Host] FAILED to load plugin:" << loader.errorString();
        return 1;
    }
    qDebug() << "[Test Host] SUCCESS: Plugin loaded into memory";

    auto* iface = qobject_cast<ViewerPluginInterface*>(instance);
    if (!iface) {
        qCritical() << "[Test Host] FAILED: Interface cast to ViewerPluginInterface failed";
        return 2;
    }
    qDebug() << "[Test Host] SUCCESS: ViewerPluginInterface queried";

    ViewerBase* viewer = iface->createViewer(nullptr);
    if (!viewer) {
        qCritical() << "[Test Host] FAILED: createViewer returned nullptr";
        return 3;
    }
    qDebug() << "[Test Host] SUCCESS: ViewerBase instance created, name:" << viewer->name();

    TestMonitor monitor;
    QObject::connect(viewer, &ViewerBase::sigCommand, &monitor, &TestMonitor::onSigCommand);

    // 构造官方标准的 ViewOptions 模拟真实 Seer 宿主环境
    ViewOptionsPrivate optPriv;
    optPriv.path = testModel;
    optPriv.dpr = 1.0;
    optPriv.theme = 1; // Dark
    optPriv.viewer_type = "3dmaster";

    ViewOptions opts;
    opts.d_ptr = &optPriv;

    qDebug() << "[Test Host] Calling official viewer->load(nullptr, &opts)...";
    viewer->load(nullptr, &opts);

    viewer->resize(viewer->getContentSize());
    viewer->show();

    QTimer* timeoutTimer = new QTimer(&app);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(8000);

    int exitCode = -1;

    bool screenshotCaptured = false;
    QTimer* checkTimer = new QTimer(&app);
    QObject::connect(checkTimer, &QTimer::timeout, [&]() {
        if (monitor.loadingSucceeded && !screenshotCaptured) {
            screenshotCaptured = true;
            checkTimer->stop();
            qDebug() << "[Test Host] PASS: Model successfully parsed and sigCommand(VCT_StateChange, VCV_Loaded) received!";
            qDebug() << "[Test Host] Waiting 300ms for OpenGL to complete render frames...";
            
            QTimer::singleShot(300, [&]() {
                if (argc > 3 && QString::fromLocal8Bit(argv[3]) == "section") {
                    const auto checkBoxes = viewer->findChildren<QCheckBox*>();
                    for (auto* cb : checkBoxes) {
                        if (cb->text().contains("截面剖切")) {
                            cb->setChecked(true);
                            break;
                        }
                    }
                    QTimer::singleShot(300, [&]() {
                        QImage screenshot = viewer->grab().toImage();
                        screenshot.save(screenshotPath);
                        qDebug() << "[Test Host] PASS: Section View Screenshot saved to" << screenshotPath;
                        exitCode = 0;
                        app.quit();
                    });
                    return;
                } else if (argc > 3 && QString::fromLocal8Bit(argv[3]) == "box") {
                    const auto checkBoxes = viewer->findChildren<QCheckBox*>();
                    for (auto* cb : checkBoxes) {
                        if (cb->text().contains("三维尺寸包围盒")) {
                            cb->setChecked(true);
                            break;
                        }
                    }
                    QTimer::singleShot(300, [&]() {
                        QImage screenshot = viewer->grab().toImage();
                        screenshot.save(screenshotPath);
                        qDebug() << "[Test Host] PASS: Bounding Box Screenshot saved to" << screenshotPath;
                        exitCode = 0;
                        app.quit();
                    });
                    return;
                } else if (argc > 3 && QString::fromLocal8Bit(argv[3]) == "anim") {
                    const auto buttons = viewer->findChildren<QPushButton*>();
                    for (auto* btn : buttons) {
                        if (btn->text() == "前视") {
                            btn->click();
                            break;
                        }
                    }
                    QTimer::singleShot(350, [&]() {
                        QImage screenshot = viewer->grab().toImage();
                        screenshot.save(screenshotPath);
                        qDebug() << "[Test Host] PASS: Front View after camera animation saved to" << screenshotPath;
                        exitCode = 0;
                        app.quit();
                    });
                    return;
                }

                QImage screenshot = viewer->grab().toImage();
                screenshot.save(screenshotPath);
                qDebug() << "[Test Host] PASS: Screenshot saved to" << screenshotPath 
                         << "(" << screenshot.width() << "x" << screenshot.height() << ")";

                exitCode = 0;
                app.quit();
            });
        } else if (monitor.loadingFailed) {
            checkTimer->stop();
            qCritical() << "[Test Host] FAIL: Plugin emitted VCV_Error";
            exitCode = 4;
            app.quit();
        }
    });
    checkTimer->start(50);

    QObject::connect(timeoutTimer, &QTimer::timeout, [&]() {
        if (exitCode == -1) {
            qCritical() << "[Test Host] FAIL: Timeout waiting for model to load!";
            exitCode = 5;
            app.quit();
        }
    });
    timeoutTimer->start();

    app.exec();

    qDebug() << "[Test Host] Verification complete with exit code:" << exitCode;
    exit(exitCode);
    return exitCode;
}

#include "test_host.moc"
