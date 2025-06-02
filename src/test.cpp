#include "test.hpp"

TestWidget::TestWidget(QWidget *parent)
    : QDockWidget("FU-test-plugin", parent)
{
    QWidget *innerWidget = new QWidget(this);
    button1 = new QPushButton("Click me", innerWidget);
    button2 = new QPushButton("Click me", innerWidget);

    QHBoxLayout *layout = new QHBoxLayout(innerWidget);
    layout->addWidget(button1);
    layout->addWidget(button2);
    innerWidget->setLayout(layout);

    setWidget(innerWidget);
    setVisible(false);
    setFloating(true);
    resize(300, 300);

    connect(button1, &QPushButton::clicked, this, &TestWidget::ButtonClicked);
    connect(button2, &QPushButton::clicked, this, &TestWidget::ButtonClicked);
}

TestWidget::~TestWidget() {}

void TestWidget::ButtonClicked() {
    QMessageBox::information(this, "FU-test-plugin", "Button clicked");
}
