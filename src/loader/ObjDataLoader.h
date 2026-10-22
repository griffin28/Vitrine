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
    explicit ObjDataLoader(const QString& filePath, QObject* parent = nullptr) 
        : DataLoader(filePath, parent) {}

    bool load(ANARIDevice device, AnariScene& anariScene) override;
};

} // namespace vitrine
