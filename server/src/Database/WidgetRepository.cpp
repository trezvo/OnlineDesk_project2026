#include "WidgetRepository.hpp"
#include "Models-odb.hxx"

namespace db {

using code_enum = WidgetRepository::CODE_ID;

code_enum WidgetRepository::create(Widget& widget) { 
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database(); 

        auto board = db.find<Board>(widget.board()->id());

        if (!board) {
            return code_enum::BOARD_NOT_FOUND;
        }

        try {
            db.persist(widget);
            return code_enum::OK;
        }
        catch (const odb::object_already_persistent&) {
            return code_enum::WIDGET_ALREADY_EXISTS;
        }
        catch (const odb::exception&) {
            return code_enum::INTERNAL_ERROR;
        }
    });
}

code_enum WidgetRepository::create(Widget& widget, uint64_t board_id) { 
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database(); 

        auto board = db.find<Board>(board_id);

        if (!board) {
            return code_enum::BOARD_NOT_FOUND;
        }

        widget.board() = std::shared_ptr<Board>(board);

        try {
            db.persist(widget);
            return code_enum::OK;
        }
        catch (const odb::object_already_persistent& e) {
            throw;
        }
        catch (const odb::exception& e) {
            throw;
        }
    });
}

code_enum WidgetRepository::update(Widget& widget) {
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database();

        try {
            db.update(widget);
            return code_enum::OK;
        }
        catch (const odb::object_not_persistent&) {
            return code_enum::WIDGET_NOT_FOUND;
        }
        catch (const odb::exception&) {
            return code_enum::INTERNAL_ERROR; 
        }
    });
}

code_enum WidgetRepository::update(Widget& widget, uint64_t board_id) {
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database();

        auto board = db.find<Board>(board_id);

        if (!board) {
            return code_enum::BOARD_NOT_FOUND;
        }

        widget.board() = std::shared_ptr<Board>(board);

        try {
            db.update(widget);
            return code_enum::OK;
        }
        catch (const odb::object_not_persistent&) {
            return code_enum::WIDGET_NOT_FOUND;
        }
        catch (const odb::exception&) {
            return code_enum::INTERNAL_ERROR; 
        }
    });
}

code_enum WidgetRepository::remove(Widget& widget) {
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database();
        try {
            db.erase<Widget>(widget.id());
            return code_enum::OK;
        }
        catch (const odb::object_not_persistent&) {
            return code_enum::WIDGET_NOT_FOUND;
        }
        catch (const odb::exception&) {
            return code_enum::INTERNAL_ERROR;
        }
    });
}

code_enum WidgetRepository::remove(uint64_t widget_id) {
    return transaction([&] (odb::transaction& t) -> code_enum {
        auto& db = t.database();
        try {
            db.erase<Widget>(widget_id);
            return code_enum::OK;
        }
        catch (const odb::object_not_persistent&) {
            return code_enum::WIDGET_NOT_FOUND;
        }
        catch (const odb::exception&) {
            return code_enum::INTERNAL_ERROR;
        }
    });
}

uint64_t WidgetRepository::createSnapshot(uint64_t old_board_id, uint64_t new_board_id) {
    return transaction([&] (odb::transaction& t) -> uint64_t {
        auto& db = t.database();

        auto old_board = db.find<Board>(old_board_id);
        auto new_board = db.find<Board>(new_board_id);

        if (!old_board || !new_board) {
            throw code_enum::BOARD_NOT_FOUND;
        }

        for (Widget& widget : findAllOnBoard(old_board_id)) {
            widget.id() = new_board_id;
            widget.board() = std::shared_ptr<Board>(old_board);
            switch (create(widget)) {
                case (code_enum::OK) : {
                    continue;
                } break;
                default : {
                    throw code_enum::INTERNAL_ERROR;
                } break;
            }
        }

        return new_board_id;
    }); 
}

std::optional<Widget> WidgetRepository::findById(uint64_t widget_id) {
    return transaction([&] (odb::transaction& t) -> std::optional<Widget> {
        auto& db = t.database();

        auto widget = db.find<Widget>(widget_id);

        if (!widget) {
            return std::nullopt;
        }

        return *widget;
    });
}

std::vector<Widget> WidgetRepository::findAllOnBoard(unsigned long long board_id) {
    return transaction([&] (odb::transaction& t) -> std::vector<Widget> {
        auto& db = t.database();

        auto board = db.find<Board>(board_id);

        if (!board) {
            return {};
        }

        using widget_query = odb::query<Widget>;
        auto result = db.query<Widget>(
            widget_query::board->id == board_id
        );

        std::vector<Widget> out;

        for (auto& widget : result) {
            out.push_back(std::move(widget));
        }

        return out;
    });
}

}