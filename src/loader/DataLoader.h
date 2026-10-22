#pragma once

#include "AnariRenderer.h" // AnariScene
#include "LogLevel.h"

#include <anari/anari.h>

#include <QObject>
#include <QString>

namespace vitrine
{

/// @brief Abstract base for file loaders that translate a scene file into
///        ANARI objects attached to an AnariScene. Concrete loaders are
///        created by DataLoaderFactory based on the file suffix. Loaders are
///        QObjects so progress / warning / error messages can be delivered to
///        the owner via the statusMessage signal.
class DataLoader : public QObject
{
    Q_OBJECT

public:
    explicit DataLoader(const QString& filePath, QObject* parent = nullptr) 
        : QObject(parent)
        , m_filePath(filePath) {}
    ~DataLoader() override = default;

    /// @brief Load the scene from the file into the ANARI scene.
    /// @param device The ANARI device to use.
    /// @param anariScene The ANARI scene to load the scene into.
    /// @return true if at least one renderable surface was produced.
    virtual bool load(ANARIDevice device, AnariScene& anariScene) = 0;

signals:
    /// @brief Emitted with a LogLevel (as int) and a human-readable message.
    ///        Mirrors AnariRenderer::statusMessage so it can be chained
    ///        signal-to-signal into the application log.
    void statusMessage(int level, const QString& message);

protected:
    void emitStatus(LogLevel level, const QString& message)
    {
        emit statusMessage(static_cast<int>(level), message);
    }

    QString m_filePath;
};

} // namespace vitrine
