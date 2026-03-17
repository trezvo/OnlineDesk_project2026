#pragma once

#include "BoardScreenFwd.hpp"
#include <QDialog> 
#include <QString>
#include <QLineEdit>
#include <string>
#include <utility>

struct WidgetUpdate {
    uint64_t widget_id;
    int new_x;
    int new_y;
};

class Widget : public QDialog {

    Q_OBJECT

    int x_;
    int y_;

    uint64_t widget_id_;
    QLineEdit* x_setup_line_;
    QLineEdit* y_setup_line_;
    QPushButton* update_button_;
    QPushButton* delete_button_;

    void SetupUI();

public slots:
    void onUpdateClicled();
    void onDeleteClicked();

signals:

    void requestUpdate(WidgetUpdate request);
    void requestDelete(uint64_t widget_id);

public:

    explicit Widget(uint64_t widget_id, QWidget* parent = nullptr);
    void UpdateCoords(int new_x, int new_y);
    std::pair<int, int> GetCoords();
};
