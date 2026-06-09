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

TEST(WidgetDataBaseTest, PostWithExistingIdReplacesPreviousWidgetData) {
    WidgetDataBase db;
    db.Post(42, WidgetsPost{7, 10, 20, "first note"});

    db.Post(42, WidgetsPost{8, 30, 40, "replacement note"});

    const WidgetsRead widget = db.Get(42);
    EXPECT_EQ(widget.widget_id, 42);
    EXPECT_EQ(widget.board_id, 8);
    EXPECT_EQ(widget.x, 30);
    EXPECT_EQ(widget.y, 40);
    EXPECT_EQ(widget.content, "replacement note");
}

TEST(WidgetDataBaseTest, StoresEmptyContentAndNegativeCoordinates) {
    WidgetDataBase db;

    db.Post(42, WidgetsPost{7, -10, -20, ""});

    const WidgetsRead widget = db.Get(42);
    EXPECT_EQ(widget.widget_id, 42);
    EXPECT_EQ(widget.board_id, 7);
    EXPECT_EQ(widget.x, -10);
    EXPECT_EQ(widget.y, -20);
    EXPECT_TRUE(widget.content.empty());
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

TEST(WidgetDataBaseTest, UpdateCanSetEmptyContent) {
    WidgetDataBase db;
    db.Post(42, WidgetsPost{7, 10, 20, "first note"});

    db.Update(42, WidgetsUpdate{30, 40, ""});

    const WidgetsRead widget = db.Get(42);
    EXPECT_EQ(widget.board_id, 7);
    EXPECT_EQ(widget.x, 30);
    EXPECT_EQ(widget.y, 40);
    EXPECT_TRUE(widget.content.empty());
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

TEST(WidgetDataBaseTest, SelectFromBoardReturnsEmptyVectorForBoardWithoutWidgets) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "board 100"});

    EXPECT_TRUE(db.SelectFromBoard(200).empty());
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

TEST(WidgetDataBaseTest, DeleteUnknownWidgetDoesNotChangeStoredWidgets) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "keep"});

    db.Delete(999);

    const std::vector<WidgetsRead> board_widgets = db.SelectFromBoard(100);
    ASSERT_EQ(board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(board_widgets, 1, 100, 0, 0, "keep"));
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

TEST(WidgetDataBaseTest, DeleteByUnknownBoardIdDoesNotChangeStoredWidgets) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "keep a"});
    db.Post(2, WidgetsPost{200, 1, 1, "keep b"});

    db.DeleteByBoardId(999);

    const std::vector<WidgetsRead> first_board_widgets = db.SelectFromBoard(100);
    const std::vector<WidgetsRead> second_board_widgets = db.SelectFromBoard(200);

    ASSERT_EQ(first_board_widgets.size(), 1);
    ASSERT_EQ(second_board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(first_board_widgets, 1, 100, 0, 0, "keep a"));
    EXPECT_TRUE(ContainsWidget(second_board_widgets, 2, 200, 1, 1, "keep b"));
}

TEST(WidgetDataBaseTest, UpdateOneWidgetDoesNotChangeAnotherWidgetOnSameBoard) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "first"});
    db.Post(2, WidgetsPost{100, 10, 10, "second"});

    db.Update(1, WidgetsUpdate{50, 60, "first updated"});

    const std::vector<WidgetsRead> board_widgets = db.SelectFromBoard(100);
    ASSERT_EQ(board_widgets.size(), 2);
    EXPECT_TRUE(ContainsWidget(board_widgets, 1, 100, 50, 60, "first updated"));
    EXPECT_TRUE(ContainsWidget(board_widgets, 2, 100, 10, 10, "second"));
}

TEST(WidgetDataBaseTest, RepostingExistingWidgetMovesItToAnotherBoard) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "on first board"});

    db.Post(1, WidgetsPost{200, 10, 20, "on second board"});

    EXPECT_TRUE(db.SelectFromBoard(100).empty());

    const std::vector<WidgetsRead> second_board_widgets = db.SelectFromBoard(200);
    ASSERT_EQ(second_board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(second_board_widgets, 1, 200, 10, 20, "on second board"));
}

TEST(WidgetDataBaseTest, DeletedWidgetIdCanBeAddedAgain) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "old"});
    db.Delete(1);

    db.Post(1, WidgetsPost{100, 7, 8, "new"});

    const WidgetsRead widget = db.Get(1);
    EXPECT_EQ(widget.widget_id, 1);
    EXPECT_EQ(widget.board_id, 100);
    EXPECT_EQ(widget.x, 7);
    EXPECT_EQ(widget.y, 8);
    EXPECT_EQ(widget.content, "new");
}

TEST(WidgetDataBaseTest, SelectFromBoardReturnsUpdatedWidgetData) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 1, 2, "draft"});

    db.Update(1, WidgetsUpdate{3, 4, "published"});

    const std::vector<WidgetsRead> board_widgets = db.SelectFromBoard(100);
    ASSERT_EQ(board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(board_widgets, 1, 100, 3, 4, "published"));
}

TEST(WidgetDataBaseTest, DeleteByBoardIdDoesNotRemoveRepostedWidgetFromNewBoard) {
    WidgetDataBase db;
    db.Post(1, WidgetsPost{100, 0, 0, "initial board"});
    db.Post(1, WidgetsPost{200, 5, 6, "moved board"});

    db.DeleteByBoardId(100);

    EXPECT_TRUE(db.SelectFromBoard(100).empty());

    const std::vector<WidgetsRead> second_board_widgets = db.SelectFromBoard(200);
    ASSERT_EQ(second_board_widgets.size(), 1);
    EXPECT_TRUE(ContainsWidget(second_board_widgets, 1, 200, 5, 6, "moved board"));
}
