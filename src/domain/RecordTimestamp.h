#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

std::optional<QDateTime> parseRecordTimestamp(const QString &value);