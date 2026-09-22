#include "master_viewer.h"
#include <QHBoxLayout>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>

static QString getSettingsFilePath() {
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(appDataDir);
    return QDir(appDataDir).filePath("3dmaster.ini");
}

static void traceLog(const QString& msg) {
    qDebug() << "[3dmaster Viewer]" << msg;
    const QString logPath = QDir(QDir::tempPath()).filePath("3dmaster.log");
    QFile f(logPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " [Viewer] " << msg << "\n";
    }
}

MasterViewer::MasterViewer(QWidget* parent) : ViewerBase(parent) {
    qRegisterMetaType<ModelDataPtr>("ModelDataPtr");
    traceLog("MasterViewer created");
}

MasterViewer::~MasterViewer() {
    traceLog("~MasterViewer destroyed");
    onCancelRequested();
}

QSize MasterViewer::getContentSize() const {
    if (options()) {
        return options()->dpr() * QSize(1000, 680);
    }
    return QSize(1000, 680);
}

void MasterViewer::loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar) {
    Q_UNUSED(lay_ctrlbar);
    traceLog("loadImpl called");

    if (m_widget && m_sidebar) {
        traceLog("loadImpl: UI layout already initialized, skipping duplicate layout rebuild");
    } else {
        if (!m_widget) {
            m_widget = new MasterWidget(this);
        }
        if (!m_sidebar) {
            m_sidebar = new MasterSidebar(this);
        }

        auto containerLayout = new QHBoxLayout();
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);
        containerLayout->addWidget(m_widget, 1);
        containerLayout->addWidget(m_sidebar, 0);

        if (lay_content) {
            lay_content->addLayout(containerLayout);
        }

        // 信号槽连接：侧边栏控制视口
        connect(m_sidebar, &MasterSidebar::sigCameraPreset, m_widget, &MasterWidget::switchCamera);
        connect(m_sidebar, &MasterSidebar::sigFitViewRequested, m_widget, &MasterWidget::fitView);
        connect(m_sidebar, &MasterSidebar::sigShadingModeChanged, m_widget, &MasterWidget::setShadingMode);
        connect(m_sidebar, &MasterSidebar::sigShowGrid, m_widget, &MasterWidget::setShowGrid);
        connect(m_sidebar, &MasterSidebar::sigShowAxis, m_widget, &MasterWidget::setShowAxis);
        connect(m_sidebar, &MasterSidebar::sigShowWireframe, m_widget, &MasterWidget::setShowWireframe);
        connect(m_sidebar, &MasterSidebar::sigShowFeatureEdges, m_widget, &MasterWidget::setShowFeatureEdges);
        connect(m_sidebar, &MasterSidebar::sigShowBoundingBox, m_widget, &MasterWidget::setShowBoundingBox);
        connect(m_sidebar, &MasterSidebar::sigOrthographic, m_widget, &MasterWidget::setOrthographic);

        // 动态剖切截面信号
        connect(m_sidebar, &MasterSidebar::sigSectionEnabled, m_widget, &MasterWidget::setSectionEnabled);
        connect(m_sidebar, &MasterSidebar::sigSectionAxisChanged, m_widget, &MasterWidget::setSectionAxis);
        connect(m_sidebar, &MasterSidebar::sigSectionDepthChanged, m_widget, &MasterWidget::setSectionDepth);

        // 零件装配体独立显隐联动
        connect(m_sidebar, &MasterSidebar::sigPartVisibleChanged, m_widget, &MasterWidget::setPartVisible);
        connect(m_sidebar, &MasterSidebar::sigAllPartsVisibleChanged, m_widget, &MasterWidget::setAllPartsVisible);

        // 交互操作手感模式持久化 (3dmaster.ini)
        connect(m_sidebar, &MasterSidebar::sigNavigationPresetChanged, this, [this](MasterWidget::NavigationPreset preset) {
            if (m_widget) {
                m_widget->setNavigationPreset(preset);
            }
            QSettings settings(getSettingsFilePath(), QSettings::IniFormat);
            settings.setValue("Navigation/Preset", static_cast<int>(preset));
            settings.sync();
        });

        // 读取持久化交互手感配置 (默认西门子 UG NX)
        QSettings settings(getSettingsFilePath(), QSettings::IniFormat);
        int savedPresetInt = settings.value("Navigation/Preset", static_cast<int>(MasterWidget::NavigationPreset::UG_NX)).toInt();
        auto savedPreset = static_cast<MasterWidget::NavigationPreset>(savedPresetInt);
        m_sidebar->setNavigationPreset(savedPreset);
        m_widget->setNavigationPreset(savedPreset);

        connect(m_widget, &MasterWidget::sigCancelRequested, this, &MasterViewer::onCancelRequested);
    }

    // 主题与高分屏自适应
    if (options()) {
        updateTheme(options()->theme());
        updateDPR(options()->dpr());
    }

    // 官方契约：唯一合法的路径获取来源是 options()->path()，直接在此启动异步加载
    QString targetPath;
    if (options()) {
        targetPath = options()->path();
    }
    traceLog(QString("loadImpl targetPath = '%1'").arg(targetPath));

    if (targetPath.isEmpty()) {
        m_widget->setErrorMessage("无法获取待预览的文件路径 (options->path() 为空)");
        emit sigCommand(VCT_StateChange, VCV_Error);
        return;
    }

    startAsyncLoad(targetPath);
}

void MasterViewer::startAsyncLoad(const QString& filePath) {
    traceLog(QString("startAsyncLoad entered: '%1'").arg(filePath));

    // 如果该模型文件已经成功载入并处于当前激活展示状态，坚决不重新加载
    if (m_model && m_loadedPath == filePath) {
        traceLog("startAsyncLoad: File ALREADY LOADED and active, ignoring redundant load request");
        if (m_widget) {
            m_widget->setLoading(false);
        }
        // 关键补发：由于 Seer 每次调用 load() 都会先将宿主状态置为 Loading，去重命中时必须补发 Loaded！
        emit sigCommand(VCT_StateChange, VCV_Loaded);
        return;
    }

    if (m_currentLoadingPath == filePath && m_workerThread && m_workerThread->isRunning()) {
        traceLog("startAsyncLoad: Already loading this file, skipping duplicate");
        return;
    }

    onCancelRequested(); // 中止旧任务并断开旧信号

    m_currentLoadingPath = filePath;
    const uint64_t currentReqId = ++m_loadRequestId;

    if (!m_widget) {
        m_widget = new MasterWidget(this);
    }

    m_widget->setLoading(true, "正在准备后台解析线程...");
    // 触发官方状态变更：Loading 中
    emit sigCommand(VCT_StateChange, VCV_Loading);

    // 线程生命周期管理：不设 parent 为 this，由 finished 信号安全触发 deleteLater
    m_workerThread = new QThread();
    m_worker = new ModelLoaderWorker(filePath);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &ModelLoaderWorker::startLoading);

    connect(m_worker, &ModelLoaderWorker::sigProgress, this, [this, currentReqId](int pct, const QString& status) {
        if (currentReqId != m_loadRequestId) return;
        if (m_widget) {
            m_widget->setLoading(true, QString("%1 (%2%)").arg(status).arg(pct));
        }
    });

    connect(m_worker, &ModelLoaderWorker::sigFinished, this, [this, currentReqId](ModelDataPtr model) {
        if (currentReqId != m_loadRequestId) return;
        onModelLoaded(model);
    });

    connect(m_worker, &ModelLoaderWorker::sigFailed, this, [this, currentReqId](const QString& err) {
        if (currentReqId != m_loadRequestId) return;
        onModelLoadFailed(err);
    });

    // 线程安全退出与对象析构
    connect(m_worker, &ModelLoaderWorker::sigFinished, m_workerThread, &QThread::quit);
    connect(m_worker, &ModelLoaderWorker::sigFailed, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
}

void MasterViewer::onModelLoaded(ModelDataPtr model) {
    traceLog("onModelLoaded CALLED! Emitting sigCommand(VCT_StateChange, VCV_Loaded)...");
    m_model = model;
    m_loadedPath = m_currentLoadingPath;

    if (m_widget) {
        m_widget->setModel(model);
        m_widget->setLoading(false);
    }
    if (m_sidebar) {
        m_sidebar->updateModelStats(model);
    }

    // 官方契约：通知宿主 Seer 加载完毕 (VCV_Loaded)
    emit sigCommand(VCT_StateChange, VCV_Loaded);
    traceLog("sigCommand(VCT_StateChange, VCV_Loaded) EMITTED!");
}

void MasterViewer::onModelLoadFailed(const QString& err) {
    traceLog("onModelLoadFailed CALLED with err: " + err);
    m_model.reset();
    m_loadedPath.clear();
    m_currentLoadingPath.clear();

    if (m_widget) {
        m_widget->setModel(nullptr);
        m_widget->setErrorMessage(err);
    }
    if (m_sidebar) {
        m_sidebar->updateModelStats(nullptr);
    }

    // 官方契约：通知宿主 Seer 加载失败 (VCV_Error)
    emit sigCommand(VCT_StateChange, VCV_Error);
    traceLog("sigCommand(VCT_StateChange, VCV_Error) EMITTED from failed handler!");
}

void MasterViewer::onCancelRequested() {
    ++m_loadRequestId; // 使旧请求的回调栅栏全部失效，杜绝任何迟到回调
    if (m_worker) {
        m_worker->disconnect(this); // 立即切断旧 worker 与本窗口的所有信号连接
        m_worker->requestCancel();  // 触发原子标记与 OcctCancelIndicator 中断
    }
    if (QThread* th = m_workerThread) {
        if (th->isRunning()) {
            th->quit();
            // 宿主卸载 DLL 或快速切换文件时优雅等待至多 1500ms，杜绝 OCCT 单例后台线程与 Guest DLL 卸载竞态
            if (!th->wait(1500)) {
                // 超时后切断 QPointer 指针引用，避免野指针，剩余生命周期由 finished -> deleteLater 自回收
                m_worker.clear();
                m_workerThread.clear();
            }
        }
    }
}

void MasterViewer::updateDPR(qreal dpr) {
    if (m_sidebar) {
        m_sidebar->setFixedWidth(qRound(240 * dpr));
    }
    if (m_widget) {
        m_widget->update();
    }
}

void MasterViewer::updateTheme(int theme) {
    // 0: Light, 1: Dark
    bool isDark = (theme != 0);
    if (m_sidebar) {
        m_sidebar->setStyleSheet(isDark ? "background-color: #111827; color: #E5E7EB;"
                                        : "background-color: #F3F4F6; color: #1F2937;");
    }
}
