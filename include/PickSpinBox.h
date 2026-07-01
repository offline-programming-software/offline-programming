#pragma once

#include <QSpinBox>

class PickSpinBox  : public QSpinBox
{
	Q_OBJECT

public:
	PickSpinBox(QWidget *parent);
	~PickSpinBox();

private:
	bool m_emitted = false;

protected:
	void focusInEvent(QFocusEvent *event) override {
		if (!m_emitted) {
			m_emitted = true;
			emit lineEditClicked();
		}
		QSpinBox::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent* event) override {
		m_emitted = false;
		QSpinBox::focusOutEvent(event);
	}

signals:
	void lineEditClicked();
};
