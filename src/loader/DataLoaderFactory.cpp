#include "DataLoaderFactory.h"
#include "ObjDataLoader.h"

#include <QFileInfo>

namespace vitrine
{

std::unique_ptr<DataLoader> DataLoaderFactory::createLoader(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == QStringLiteral("obj")) {
        return std::make_unique<ObjDataLoader>();
    }

    return nullptr;
}

QStringList DataLoaderFactory::fileFilters()
{
    return {
        QStringLiteral("Wavefront OBJ (*.obj)"),
        QStringLiteral("All Files (*)"),
    };
}

} // namespace vitrine
