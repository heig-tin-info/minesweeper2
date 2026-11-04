// Tests for step 1. Every assert must pass: run `make test`.
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <vector>

#include "position.hpp"

static void positionsCompareByValue() {
    assert((Pos{2, 3} == Pos{2, 3}));
    assert(!(Pos{2, 3} == Pos{3, 2}));
}

static void sizeHasAnArea() {
    assert((Size{3, 4}.area() == 12));
    assert((Size{1, 1}.area() == 1));
}

static void sizeContainsOnlyCellsInsideTheGrid() {
    const Size size{2, 3};
    assert(size.contains({0, 0}));
    assert(size.contains({1, 2}));
    assert(!size.contains({2, 0}));   // one row too far
    assert(!size.contains({0, 3}));   // one column too far
    assert(!size.contains({-1, 0}));
    assert(!size.contains({0, -1}));
}

static void indexIsRowMajor() {
    const Size size{3, 4};   // 3 rows, 4 columns: not square on purpose
    assert(size.indexOf({0, 0}) == 0);
    assert(size.indexOf({0, 3}) == 3);
    assert(size.indexOf({1, 0}) == 4);
    assert(size.indexOf({2, 3}) == 11);
}

static void positionAndIndexAreInverse() {
    const Size size{3, 4};
    for (int i = 0; i < size.area(); ++i) assert(size.indexOf(size.posOf(i)) == i);
    assert((size.posOf(6) == Pos{1, 2}));
}

static void interiorCellHasEightNeighbors() {
    const auto around = neighbors({3, 3}, {1, 1});
    assert(around.size() == 8);
    for (Pos n : around) assert(!(n == Pos{1, 1}));   // not itself
}

static void cornerAndBorderCellsAreClipped() {
    assert(neighbors({3, 3}, {0, 0}).size() == 3);
    assert(neighbors({3, 3}, {2, 2}).size() == 3);
    assert(neighbors({3, 3}, {0, 1}).size() == 5);
    assert(neighbors({3, 3}, {1, 2}).size() == 5);
    assert((neighbors({3, 3}, {0, 0}) == std::vector<Pos>{{0, 1}, {1, 0}, {1, 1}}));
}

static void tinyGridsHaveFewNeighbors() {
    assert(neighbors({1, 1}, {0, 0}).empty());
    assert(neighbors({1, 3}, {0, 1}).size() == 2);
}

int main() {
    positionsCompareByValue();
    sizeHasAnArea();
    sizeContainsOnlyCellsInsideTheGrid();
    indexIsRowMajor();
    positionAndIndexAreInverse();
    interiorCellHasEightNeighbors();
    cornerAndBorderCellsAreClipped();
    tinyGridsHaveFewNeighbors();
    std::puts("Step 1: all tests passed.");
}
