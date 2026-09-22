#include "seer/viewerbase.h"
#include "master_viewer.h"
#include <QObject>
#include <QtPlugin>

/**
 * @brief 3dmaster 顶级 Qt 插件类导出入口
 */
class MasterPlugin : public QObject, public ViewerPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ViewerPluginInterface_iid FILE "../plugin.json")
    Q_INTERFACES(ViewerPluginInterface)

public:
    MasterPlugin() {
        qRegisterMetaType<ModelDataPtr>("ModelDataPtr");
    }
    virtual ~MasterPlugin() override = default;

    virtual ViewerBase* createViewer(QWidget* parent) override {
        return new MasterViewer(parent);
    }
};

#include "plugin_entry.moc"
