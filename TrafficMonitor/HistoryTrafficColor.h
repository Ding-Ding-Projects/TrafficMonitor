#pragma once

// Colors that mark the size of one traffic total in the history views (list figure bars, calendar day marks).
// The thresholds are the ones the history views have always used: 1 GB, 10 GB, 100 GB and 1 TB.
inline COLORREF HistoryTrafficColor(unsigned __int64 kBytes)
{
    if (kBytes < 1024ull * 1024)                   // below 1 GB
        return TRAFFIC_COLOR_BLUE;
    if (kBytes < 10ull * 1024 * 1024)              // below 10 GB
        return TRAFFIC_COLOR_GREEN;
    if (kBytes < 100ull * 1024 * 1024)             // below 100 GB
        return TRAFFIC_COLOR_YELLOE;
    if (kBytes < 1024ull * 1024 * 1024)            // below 1 TB
        return TRAFFIC_COLOR_RED;
    return TRAFFIC_COLOR_DARK_RED;                 // 1 TB and above
}
