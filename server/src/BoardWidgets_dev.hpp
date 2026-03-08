#pragma once

#include <vector>
#include <memmry>
#include <string>
#include <utility>
#include <optional>

namespace widgets {

class AbstractWidget;

class BoardField {

public:

    void CreateWidget(AbstractWidget* widget);

private:

    std::vector<std::unique_ptr<AbstractWidget>> widgets_on_board_;
}

class AbstractWidget {

public:
    AbstractWidget(const AbstractWidget&) = delete;
    AbstractWidget& operator=(const AbstractWidget&) = delete;
    AbstractWidget(AbstractWidget&&) = delete;
    AbstractWidget& operator=(AbstractWidget&&) = delete;

    virtual ~AbstractWidget() = default;
    


protected:

    int coord_x_, coord_y_;
}

class TextWidget : public AbstractWidget {

public:
    explicit TextWidget(int x, int y, std::string& content) : coord_x_(x), coord_y_(y) content_(std::move(content)) {}

private:

    std::string content_;
}

}