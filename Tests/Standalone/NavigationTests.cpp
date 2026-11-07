#include "AVDungeonNavigation.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static TArray<uint8> Map(const std::vector<std::string>& Rows)
{
    TArray<uint8> Tiles;
    Tiles.Init(0, static_cast<int32>(Rows.size() * Rows[0].size()));
    for (int32 Y = 0; Y < static_cast<int32>(Rows.size()); ++Y)
        for (int32 X = 0; X < static_cast<int32>(Rows[0].size()); ++X)
            Tiles[Y * static_cast<int32>(Rows[0].size()) + X] =
                Rows[Y][X] == '.' ? 1 : 0;
    return Tiles;
}

int main()
{
    constexpr int32 W = 9, H = 6;
    const auto Tiles = Map({
        "#########",
        "#...#...#",
        "#...#...#",
        "#...#...#",
        "#.......#",
        "#########"
    });
    const FVector2D Hero(2.5f, 2.5f);
    const FVector2D OtherRoom(6.5f, 2.5f);

    assert(FAVDungeonNavigation::IsWalkable(Tiles, W, H, 2, 2));
    assert(!FAVDungeonNavigation::IsWalkable(Tiles, W, H, 4, 2));
    assert(!FAVDungeonNavigation::IsWalkable(Tiles, W, H, -1, 2));
    assert(!FAVDungeonNavigation::LineOfSight(Tiles, W, H, Hero, OtherRoom));
    assert(FAVDungeonNavigation::LineOfSight(Tiles, W, H, Hero, FVector2D(3.5f, 2.5f)));

    TArray<uint8> Visible, Explored;
    FAVDungeonNavigation::Reveal(Tiles, W, H, Hero, 12.f, Visible, Explored);
    assert(Visible[2 * W + 2] == 1);
    assert(Visible[2 * W + 6] == 0);
    assert(Explored[2 * W + 6] == 0);

    FAVDungeonNavigation::Reveal(Tiles, W, H, OtherRoom, 12.f, Visible, Explored);
    assert(Visible[2 * W + 6] == 1);
    assert(Explored[2 * W + 6] == 1);
    FAVDungeonNavigation::Reveal(Tiles, W, H, Hero, 12.f, Visible, Explored);
    assert(Visible[2 * W + 6] == 0 && Explored[2 * W + 6] == 1);

    TArray<int32> Distances;
    FAVDungeonNavigation::BuildFlow(Tiles, W, H, Hero, Distances);
    assert(Distances[2 * W + 2] == 0);
    assert(Distances[2 * W + 6] > 0);
    assert(Distances[2 * W + 4] == -1);
    const FVector2D Direction = FAVDungeonNavigation::FlowDirection(
        Tiles, Distances, W, H, OtherRoom, Hero);
    // Behind the dividing wall: approach through the lower corridor.
    assert(Direction.Y > 0.1f);

    // The diagonal destination is walkable but the two adjacent orthogonal
    // tiles are solid; no corner-peeking / attacks through touching walls.
    const auto Corner = Map({
        "#####",
        "#.#.#",
        "##..#",
        "#...#",
        "#####"
    });
    assert(!FAVDungeonNavigation::LineOfSight(
        Corner, 5, 5, FVector2D(1.5f, 1.5f), FVector2D(2.5f, 2.5f)));

    const auto Divided = Map({"#####", "#.#.#", "#####"});
    TArray<int32> Unreachable;
    FAVDungeonNavigation::BuildFlow(Divided, 5, 3, FVector2D(1.5f, 1.5f), Unreachable);
    assert(Unreachable[1 * 5 + 3] == -1);
    assert(Unreachable[1 * 5 + 1] == 0);
    std::cout << "PASS: navigation reachability, fog, line-of-sight and bounds\n";
    return 0;
}
