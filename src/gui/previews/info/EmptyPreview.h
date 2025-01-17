#pragma once

#include "AbstractInfoPreview.h"

class EmptyPreview : public AbstractInfoPreview {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS{};

    explicit EmptyPreview(QWidget* parent = nullptr);
};
