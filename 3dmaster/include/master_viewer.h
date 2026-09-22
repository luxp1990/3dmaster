#pragma once

#include "seer/viewerbase.h"
#include "master_widget.h"
#include "master_sidebar.h"
#include "model_loader_worker.h"
#include <QThread>
#include <QPointer>

/**
 * @brief 3dmaster 顶级插件视窗管理类 (继承自 Seer 官方 SDK ViewerBase)
 */
class MasterViewer : public ViewerBase {
    Q_OBJECT
public:
    explicit MasterViewer(QWidget* parent = nullptr);
    ~MasterViewer() override;

    QString name() const override { return "3dmaster"; }
    QSize getContentSize() const override;
    void updateDPR(qreal dpr) override;
    void updateTheme(int theme) override;

protected:
    void loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar) override;

private slots:
    void onModelLoaded(ModelDataPtr model);
    void onModelLoadFailed(const QString& err);
    void onCancelRequested();

private:
    void startAsyncLoad(const QString& filePath);

private:
    MasterWidget* m_widget = nullptr;
    MasterSidebar* m_sidebar = nullptr;

    // 多线程模型加载与去重状态
    QPointer<QThread> m_workerThread;
    QPointer<ModelLoaderWorker> m_worker;
    QString m_currentLoadingPath;
    QString m_loadedPath;
    ModelDataPtr m_model;
    uint64_t m_loadRequestId = 0;
};
