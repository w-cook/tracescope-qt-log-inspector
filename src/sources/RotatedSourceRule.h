#pragma once

enum class RotatedSourceNamingScheme
{
    Disabled,
    NumericSuffix,
    NumericBeforeExtension
};

struct RotatedSourceRule
{
    RotatedSourceNamingScheme namingScheme =
        RotatedSourceNamingScheme::Disabled;
};