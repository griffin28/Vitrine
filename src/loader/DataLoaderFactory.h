#pragma once

#include "DataLoader.h"

#include <QString>
#include <QStringList>

#include <memory>

namespace vitrine
{

/// @brief Picks a concrete DataLoader for a file based on its suffix and
///        advertises the file-type filters that the open dialog should offer.
class DataLoaderFactory
{
public:
    /// @brief Create a loader for `filePath` based on its (case-insensitive)
    ///        suffix. Returns nullptr if no loader handles that suffix.
    static std::unique_ptr<DataLoader> createLoader(const QString& filePath);

    /// @brief Name filters for QFileDialog, one entry per supported type plus
    ///        a trailing "All Files (*)". Suitable for QFileDialog
    ///        ::setNameFilters or, joined with ";;", getOpenFileName's filter.
    static QStringList fileFilters();
};

} // namespace vitrine
