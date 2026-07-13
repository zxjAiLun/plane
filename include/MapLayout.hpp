#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "Vector2.hpp"

struct MapObstacle {
    Vector2 center;
    Vector2 halfExtents;
};

struct MapLayoutDefinition {
    std::string id;
    std::vector<MapObstacle> obstacles;
    std::vector<Vector2> eventPositions;
};

class MapLayoutLibrary {
public:
    static constexpr int TemplateCount = 3;
    static constexpr int VariantCount = 3;

    static const std::vector<std::vector<MapLayoutDefinition>>& all() {
        static const std::vector<std::vector<MapLayoutDefinition>> layouts = {
            {
                {
                    "ashen-causeway-0",
                    {
                        {{720.0f, 1330.0f}, {135.0f, 70.0f}},
                        {{1040.0f, 1120.0f}, {95.0f, 170.0f}},
                        {{1250.0f, 730.0f}, {180.0f, 80.0f}},
                        {{1650.0f, 800.0f}, {100.0f, 145.0f}},
                    },
                    {{700.0f, 1090.0f}, {1315.0f, 950.0f}, {1365.0f, 555.0f}}
                },
                {
                    "ashen-causeway-1",
                    {
                        {{540.0f, 1260.0f}, {110.0f, 55.0f}},
                        {{900.0f, 1080.0f}, {75.0f, 145.0f}},
                        {{1270.0f, 900.0f}, {150.0f, 70.0f}},
                        {{1650.0f, 520.0f}, {90.0f, 100.0f}},
                        {{1750.0f, 1030.0f}, {85.0f, 60.0f}},
                    },
                    {{720.0f, 900.0f}, {1000.0f, 760.0f}, {1460.0f, 680.0f}}
                },
                {
                    "ashen-causeway-2",
                    {
                        {{620.0f, 1100.0f}, {80.0f, 180.0f}},
                        {{1020.0f, 780.0f}, {170.0f, 60.0f}},
                        {{1420.0f, 1160.0f}, {120.0f, 75.0f}},
                        {{1720.0f, 600.0f}, {75.0f, 130.0f}},
                    },
                    {{800.0f, 1330.0f}, {1250.0f, 600.0f}, {1450.0f, 850.0f}}
                },
            },
            {
                {
                    "stormscar-expanse-0",
                    {
                        {{650.0f, 1300.0f}, {90.0f, 130.0f}},
                        {{900.0f, 1020.0f}, {160.0f, 70.0f}},
                        {{1250.0f, 1200.0f}, {105.0f, 150.0f}},
                        {{1510.0f, 700.0f}, {160.0f, 85.0f}},
                        {{1770.0f, 620.0f}, {75.0f, 150.0f}},
                    },
                    {{600.0f, 1040.0f}, {1180.0f, 820.0f}, {1580.0f, 500.0f}}
                },
                {
                    "stormscar-expanse-1",
                    {
                        {{520.0f, 1320.0f}, {80.0f, 120.0f}},
                        {{900.0f, 1050.0f}, {140.0f, 60.0f}},
                        {{1280.0f, 1280.0f}, {120.0f, 80.0f}},
                        {{1540.0f, 760.0f}, {100.0f, 60.0f}},
                        {{1760.0f, 540.0f}, {70.0f, 110.0f}},
                    },
                    {{700.0f, 1180.0f}, {1100.0f, 900.0f}, {1500.0f, 1050.0f}}
                },
                {
                    "stormscar-expanse-2",
                    {
                        {{600.0f, 1000.0f}, {100.0f, 70.0f}},
                        {{1000.0f, 1300.0f}, {80.0f, 150.0f}},
                        {{1400.0f, 850.0f}, {180.0f, 55.0f}},
                        {{1750.0f, 1100.0f}, {90.0f, 80.0f}},
                    },
                    {{760.0f, 1250.0f}, {1190.0f, 1050.0f}, {1570.0f, 500.0f}}
                },
            },
            {
                {
                    "venom-hollow-0",
                    {
                        {{600.0f, 1420.0f}, {140.0f, 65.0f}},
                        {{970.0f, 1120.0f}, {90.0f, 180.0f}},
                        {{1410.0f, 1040.0f}, {150.0f, 65.0f}},
                        {{1650.0f, 700.0f}, {105.0f, 150.0f}},
                        {{1820.0f, 820.0f}, {80.0f, 115.0f}},
                    },
                    {{720.0f, 1150.0f}, {1260.0f, 850.0f}, {1450.0f, 510.0f}}
                },
                {
                    "venom-hollow-1",
                    {
                        {{560.0f, 1300.0f}, {120.0f, 70.0f}},
                        {{950.0f, 900.0f}, {100.0f, 160.0f}},
                        {{1300.0f, 1250.0f}, {180.0f, 65.0f}},
                        {{1650.0f, 620.0f}, {90.0f, 110.0f}},
                        {{1840.0f, 980.0f}, {70.0f, 100.0f}},
                    },
                    {{760.0f, 1140.0f}, {1160.0f, 1030.0f}, {1470.0f, 530.0f}}
                },
                {
                    "venom-hollow-2",
                    {
                        {{620.0f, 1080.0f}, {90.0f, 160.0f}},
                        {{1050.0f, 650.0f}, {150.0f, 60.0f}},
                        {{1370.0f, 950.0f}, {100.0f, 140.0f}},
                        {{1750.0f, 700.0f}, {100.0f, 60.0f}},
                        {{1830.0f, 1200.0f}, {60.0f, 120.0f}},
                    },
                    {{800.0f, 1320.0f}, {1200.0f, 1180.0f}, {1500.0f, 500.0f}}
                },
            },
        };
        return layouts;
    }

    static int normalizeTemplateIndex(int templateIndex) {
        const int count = static_cast<int>(all().size());
        return ((templateIndex % count) + count) % count;
    }

    static int normalizeVariantIndex(int layoutIndex) {
        return ((layoutIndex % VariantCount) + VariantCount) % VariantCount;
    }

    static const MapLayoutDefinition& forTemplate(int templateIndex, int layoutIndex) {
        const int normalizedTemplate = normalizeTemplateIndex(templateIndex);
        const int normalizedVariant = normalizeVariantIndex(layoutIndex);
        return all()[static_cast<std::size_t>(normalizedTemplate)]
            [static_cast<std::size_t>(normalizedVariant)];
    }

    static int variantForMapLevel(int mapLevel) {
        return normalizeVariantIndex(std::max(1, mapLevel) - 1);
    }
};
