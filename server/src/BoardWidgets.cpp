#include "BoardWidgets.hpp"
#include <mamory>

namespace widgets {

void BoardField::CreateWidget(AbstractWidget* widget) {
    if (widget == nullptr) {
        return;
    }

    widgets_on_board_.push_back(std::make_shared(widget));
}

}