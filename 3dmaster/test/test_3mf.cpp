#include <QtCore/private/qzipreader_p.h>
#include <QXmlStreamReader>
#include <QFile>
#include <QVector3D>
#include <vector>
#include <iostream>

int main(int argc, char* argv[]) {
    QString path = "D:/seer/3dmaster/test_models/prism.3mf";
    if (argc > 1) {
        path = QString::fromLocal8Bit(argv[1]);
    }
    std::cout << "Testing 3MF reader on: " << path.toStdString() << std::endl;
    QZipReader zip(path);
    if (!zip.isReadable()) {
        std::cerr << "Failed to open 3mf zip file: " << path.toStdString() << std::endl;
        return 1;
    }

    QByteArray modelXml = zip.fileData("3D/3dmodel.model");
    if (modelXml.isEmpty()) {
        for (const auto& entry : zip.fileInfoList()) {
            if (entry.filePath.endsWith(".model", Qt::CaseInsensitive)) {
                modelXml = zip.fileData(entry.filePath);
                break;
            }
        }
    }
    std::cout << "3dmodel.model size: " << modelXml.size() << " bytes" << std::endl;

    QXmlStreamReader xml(modelXml);
    int vertexCount = 0;
    int triangleCount = 0;
    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == QLatin1String("vertex")) {
                vertexCount++;
            } else if (xml.name() == QLatin1String("triangle")) {
                triangleCount++;
            }
        }
    }
    std::cout << "Parsed vertex count: " << vertexCount << ", triangle count: " << triangleCount << std::endl;
    return (vertexCount > 0 && triangleCount > 0) ? 0 : 1;
}
