#pragma once

#include <anari/anari.h>

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

#include "LogLevel.h"

namespace vitrine
{

/// @brief Bridge that translates ANARI status callbacks into Qt-friendly log
///        messages. Because anariLoadLibrary's callback uses a C function
///        pointer + opaque user pointer, we keep the actual sink as a
///        std::function held by a heap-allocated trampoline whose address
///        we hand to ANARI. Ownership returns to the caller (typically
///        AppMainWindow) via take()/reset() so the lifetime tracks the
///        loaded library.
class AnariStatusSink
{
public:
    using Sink = std::function<void(LogLevel, QString)>;

    explicit AnariStatusSink(Sink sink);

    /// Returns the C-style callback to hand to anariLoadLibrary.
    ANARIStatusCallback callback() const { return &AnariStatusSink::dispatch; }

    /// Returns the userPtr to hand to anariLoadLibrary.
    const void* userData() const { return this; }

private:
    static void dispatch(const void* userPtr,
                         ANARIDevice device,
                         ANARIObject source,
                         ANARIDataType sourceType,
                         ANARIStatusSeverity severity,
                         ANARIStatusCode code,
                         const char* message);

    Sink m_sink;
};

/// @brief ANARI introspection helpers used by the backend picker UI.
namespace AnariUtils
{
    /// Enumerate ANARI backend libraries we should expose to the user.
    /// Combines a small built-in list with anything found via the
    /// ANARI_LIBRARY_PATH environment variable.
    QStringList enumerateBackendLibraries();

    /// Device subtypes the loaded library advertises (usually just "default").
    QStringList enumerateDeviceSubtypes(ANARILibrary library);

    /// Renderer subtypes the device advertises.
    QStringList enumerateRendererSubtypes(ANARIDevice device);

    /// A single parameter exposed by a renderer subtype, as returned by
    /// anariGetObjectInfo(... "parameter", ANARI_PARAMETER_LIST).
    struct RendererParameter
    {
        QString name;
        ANARIDataType type{};
        QString description;
    };

    std::vector<RendererParameter> enumerateRendererParameters(
        ANARIDevice device, const QString& rendererSubtype);
} // namespace AnariUtils

} // namespace vitrine
