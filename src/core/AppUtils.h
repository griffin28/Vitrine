#pragma once

#include <QApplication>
#include <QFile>
#include <QTextStream>

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

namespace vitrine
{

/// @brief Per-vertex data fed into the ANARI triangle geometry path. Kept here
///        (rather than inside AnariRenderer) so the unit tests can construct
///        mesh data without dragging in the renderer's Qt threading.
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
};

/// @brief Utility class for application-wide helper functions (currently just
///        the stylesheet loaders).
class AppUtils
{
public:
    /// @brief Apply a dark mode stylesheet to the given QApplication.
    /// @param app the QApplication instance to apply the dark mode to
    static inline void applyDarkMode(QApplication &app)
    {
        QFile styleFile(":/qdarkstyle/dark/darkstyle.qss");

        if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    /// @brief Apply a light mode stylesheet to the given QApplication.
    /// @param app the QApplication instance to apply the light mode to
    static inline void applyLightMode(QApplication &app)
    {
        QFile styleFile(":/qdarkstyle/light/lightstyle.qss");

        if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }
};
} // namespace vitrine

template <>
struct std::hash<vitrine::Vertex>
{
    size_t operator()(const vitrine::Vertex& vertex) const noexcept
    {
        size_t h1 = std::hash<float>()(vertex.pos.x) ^ (std::hash<float>()(vertex.pos.y) << 1) ^ (std::hash<float>()(vertex.pos.z) << 2);
        size_t h2 = std::hash<float>()(vertex.color.x) ^ (std::hash<float>()(vertex.color.y) << 1) ^ (std::hash<float>()(vertex.color.z) << 2);
        size_t h3 = std::hash<float>()(vertex.texCoord.x) ^ (std::hash<float>()(vertex.texCoord.y) << 1);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
