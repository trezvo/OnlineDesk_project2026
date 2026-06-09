#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "WidgetsDataBase/WidgetsDB.hpp"

namespace {

bool ContainsWidget(
    const std::vector<WidgetsRead>& widgets,
    uint64_t widget_id,
    uint64_t board_id,
    int x,
    int y,
    const std::string& content
) {
    return std::any_of(widgets.begin(), widgets.end(), [&](const WidgetsRead& widget) {
        return widget.widget_id == widget_id &&
               widget.board_id == board_id &&
               widget.x == x &&
               widget.y == y &&
               widget.content == content;
    });
}

}  // namespace

TEST(WidgetDataBaseTest, PostAndGetReturnsStoredWidgetData) {
    WidgetDataBase db;

    db.Post(42, WidgetsPost{7, 10, 20, "first note"});

    const WidgetsRead widget = db.Get(42);
    EXPECT_EQ(widget.widget_id, 42);
    EXPECT_EQ(widget.board_id, 7);
    EXPECT_EQ(widget.x, 10);
    EXPECT_EQ(widget.y, 20);
    EXPECT_EQ(widget.content, "first note");
}

TEST(WidgetDataBaseTest, UpdateChangesCoordinatesButKeepsBoard) {
    WidgetDataBase db;
    db.Post(42, WidgetsPost{7, 10, 20, "first note"});

    db.Update(42, WidgetsUpdate{30, 40, "updated note"});

    const WidgetsRead widget = db.Get(42);
    EXPECT_EQ(widget.widget_id, 42);
    EXPECT_EQ(widget.board_id, 7);
    EXPECT_EQ(widget.x, 30);
    EXPECT_EQ(widget.y, 40);
    EXPECT_EQ(widget.content, "updated note");
}

TEST(WidgetDataBaseTest, SelectFromBoardReturnsOnlyWidgetsFromRequestedBoard) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "board 100 a"});
    db.Post(2, WidgetsPost{200, 1, 1, "board 200"});
    db.Post(3, WidgetsPost{100, 2, 2, "board 100 b"});

    const std::vector<WidgetsRead> board_widgets = db.SelectFromBoard(100);

    ASSERT_EQ(board_widgets.size(), 2);
    EXPECT_TRUE(ContainsWidget(board_widgets, 1, 100, 0, 0, "board 100 a"));
    EXPECT_TRUE(ContainsWidget(board_widgets, 3, 100, 2, 2, "board 100 b"));
    EXPECT_FALSE(ContainsWidget(board_widgets, 2, 200, 1, 1, "board 200"));
}

TEST(WidgetDataBaseTest, DeleteRemovesSingleWidget) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "keep"});
    db.Post(2, WidgetsPost{100, 1, 1, "remove"});

    db.Delete(2);

    const std::vector<WidgetsRead> board_widgets = db.SelectFromBoard(100);
    ASSERT_EQ(board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(board_widgets, 1, 100, 0, 0, "keep"));
    EXPECT_FALSE(ContainsWidget(board_widgets, 2, 100, 1, 1, "remove"));
}

TEST(WidgetDataBaseTest, DeleteByBoardIdRemovesOnlyWidgetsFromThatBoard) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "delete a"});
    db.Post(2, WidgetsPost{200, 1, 1, "keep"});
    db.Post(3, WidgetsPost{100, 2, 2, "delete b"});

    db.DeleteByBoardId(100);

    EXPECT_TRUE(db.SelectFromBoard(100).empty());

    const std::vector<WidgetsRead> remaining_widgets = db.SelectFromBoard(200);
    ASSERT_EQ(remaining_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(remaining_widgets, 2, 200, 1, 1, "keep"));
}
