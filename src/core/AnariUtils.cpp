#include "AnariUtils.h"

#include <anari/anari.h>

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSet>

namespace vitrine
{

AnariStatusSink::AnariStatusSink(Sink sink)
    : m_sink(std::move(sink))
{
}

void AnariStatusSink::dispatch(const void* userPtr,
                               ANARIDevice /*device*/,
                               ANARIObject /*source*/,
                               ANARIDataType /*sourceType*/,
                               ANARIStatusSeverity severity,
                               ANARIStatusCode /*code*/,
                               const char* message)
{
    if (!userPtr || !message) {
        return;
    }
    auto* self = static_cast<const AnariStatusSink*>(userPtr);
    if (!self->m_sink) {
        return;
    }

    LogLevel level = LogLevel::Info;
    switch (severity) {
        case ANARI_SEVERITY_FATAL_ERROR:
        case ANARI_SEVERITY_ERROR:
            level = LogLevel::Error;
            break;
        case ANARI_SEVERITY_WARNING:
        case ANARI_SEVERITY_PERFORMANCE_WARNING:
            level = LogLevel::Warning;
            break;
        default:
            level = LogLevel::Info;
            break;
    }

    self->m_sink(level, QString::fromUtf8(message));
}

namespace AnariUtils
{

QStringList enumerateBackendLibraries()
{
    // Start with the libraries we know about; users running outside a
    // configured ANARI_LIBRARY_PATH still see Phenocryst as the first
    // choice.
    QStringList result{QStringLiteral("phenocryst"),
                       QStringLiteral("helide"),
                       QStringLiteral("visrtx")};

    const auto env = QProcessEnvironment::systemEnvironment();
    const QString path = env.value(QStringLiteral("ANARI_LIBRARY_PATH"));
    if (path.isEmpty()) {
        return result;
    }

    QSet<QString> seen{result.begin(), result.end()};
    const auto entries = path.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    for (const QString& dirPath : entries) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }
        const auto files = dir.entryInfoList(
            QStringList{QStringLiteral("libanari_library_*.so"),
                        QStringLiteral("anari_library_*.dll"),
                        QStringLiteral("libanari_library_*.dylib")},
            QDir::Files);
        for (const QFileInfo& fi : files) {
            QString name = fi.completeBaseName(); // libanari_library_<x> or anari_library_<x>
            if (name.startsWith(QStringLiteral("lib"))) {
                name.remove(0, 3);
            }
            const QString prefix = QStringLiteral("anari_library_");
            if (!name.startsWith(prefix)) {
                continue;
            }
            name.remove(0, prefix.size());
            if (!name.isEmpty() && !seen.contains(name)) {
                seen.insert(name);
                result << name;
            }
        }
    }
    return result;
}

QStringList enumerateDeviceSubtypes(ANARILibrary library)
{
    QStringList result;
    if (!library) {
        return result;
    }
    const char** subtypes = anariGetDeviceSubtypes(library);
    if (!subtypes) {
        return result;
    }
    for (int i = 0; subtypes[i] != nullptr; ++i) {
        result << QString::fromUtf8(subtypes[i]);
    }
    return result;
}

QStringList enumerateRendererSubtypes(ANARIDevice device)
{
    QStringList result;
    if (!device) {
        return result;
    }
    const char** subtypes = anariGetObjectSubtypes(device, ANARI_RENDERER);
    if (!subtypes) {
        return result;
    }
    for (int i = 0; subtypes[i] != nullptr; ++i) {
        result << QString::fromUtf8(subtypes[i]);
    }
    return result;
}

std::vector<RendererParameter> enumerateRendererParameters(
    ANARIDevice device, const QString& rendererSubtype)
{
    std::vector<RendererParameter> result;
    if (!device || rendererSubtype.isEmpty()) {
        return result;
    }
    const QByteArray subtypeUtf8 = rendererSubtype.toUtf8();
    const auto* params = static_cast<const ANARIParameter*>(
        anariGetObjectInfo(device,
                           ANARI_RENDERER,
                           subtypeUtf8.constData(),
                           "parameter",
                           ANARI_PARAMETER_LIST));
    if (!params) {
        return result;
    }
    for (int i = 0; params[i].name != nullptr; ++i) {
        RendererParameter p;
        p.name = QString::fromUtf8(params[i].name);
        p.type = params[i].type;

        const auto* desc = static_cast<const char*>(
            anariGetParameterInfo(device,
                                  ANARI_RENDERER,
                                  subtypeUtf8.constData(),
                                  params[i].name,
                                  params[i].type,
                                  "description",
                                  ANARI_STRING));
        if (desc) {
            p.description = QString::fromUtf8(desc);
        }
        result.push_back(std::move(p));
    }
    return result;
}

} // namespace AnariUtils
} // namespace vitrine
