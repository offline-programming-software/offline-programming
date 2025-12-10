#pragma once

#include <QSpinBox>

class PickSpinBox  : public QSpinBox
{
	Q_OBJECT

public:
	PickSpinBox(QWidget *parent);
	~PickSpinBox();

private:

protected:
	void focusInEvent(QFocusEvent *event) override {
		QSpinBox::focusInEvent(event);
		emit lineEditClicked();  // 自定义信号
	}

signals:
	void lineEditClicked();
};
