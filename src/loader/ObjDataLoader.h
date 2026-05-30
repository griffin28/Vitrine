#pragma once

#include "DataLoader.h"

namespace vitrine
{

/// @brief Loads Wavefront OBJ files. Each OBJ shape becomes its own ANARI
///        triangle geometry / matte material / surface; all surfaces are
///        gathered into a single group + instance attached to the world.
class ObjDataLoader : public DataLoader
{
    Q_OBJECT

public:
    explicit ObjDataLoader(QObject* parent = nullptr) : DataLoader(parent) {}

    bool loadSceneFromFile(ANARIDevice device,
                           AnariScene& anariScene,
                           const QString& filePath) override;
};

} // namespace vitrine
