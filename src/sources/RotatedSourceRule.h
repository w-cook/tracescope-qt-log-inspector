#pragma once

#include <QString>

enum class RotatedSourceNamingScheme
{
    Disabled,
    NumericSuffix,
    NumericBeforeExtension,
    IsoDateTimeBeforeExtension,
    CustomRegex
};

enum class RotatedSourceOrderValueType
{
    Numeric,
    Lexicographic,
    DateTime
};

enum class RotatedSourceOrderDirection
{
    Ascending,
    Descending
};

struct RotatedSourceRule
{
    RotatedSourceNamingScheme namingScheme =
        RotatedSourceNamingScheme::Disabled;

    /*
     * Numeric rotation schemes occur in both common
     * directions.
     *
     * Descending:
     *     service.log.3 -> service.log.2 ->
     *     service.log.1 -> service.log
     *
     * Ascending:
     *     service.log.1 -> service.log.2 ->
     *     service.log.3 -> service.log
     *
     * Descending remains the default for conventional
     * rollover naming.
     */
    RotatedSourceOrderDirection numericOrderDirection =
        RotatedSourceOrderDirection::Descending;

    /*
     * CustomRegex rules operate on the complete file
     * name, not the full path.
     *
     * The complete regular-expression match must span
     * the entire file name.
     */
    QString customRegularExpression;

    int customOrderCaptureGroup = 1;

    RotatedSourceOrderValueType customOrderValueType =
        RotatedSourceOrderValueType::Lexicographic;

    RotatedSourceOrderDirection customOrderDirection =
        RotatedSourceOrderDirection::Ascending;

    /*
     * Used only when customOrderValueType is DateTime.
     *
     * Example:
     *     MM-dd-yyyy
     *     yyyyMMdd-HHmmss
     */
    QString customDateTimeFormat;
};