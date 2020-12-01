#pragma once

#include <QCheckBox>
#include <QLayout>
#include <QPointer>
#include "clickable-label.hpp"

class CollapseCheckBox : public QCheckBox {
	Q_OBJECT
};

class CollapsibleGroupBox : public QWidget {
	Q_OBJECT

	Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)
	Q_PROPERTY(bool collapsible READ isCollapsible WRITE setCollapsible)
	Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
	Q_PROPERTY(bool checked READ isChecked WRITE setChecked)

public:
	explicit CollapsibleGroupBox(const QString &title,
				     QWidget *parent = nullptr);
	explicit CollapsibleGroupBox(QWidget *parent = nullptr)
		: CollapsibleGroupBox("", parent){};
	~CollapsibleGroupBox();

	bool isCollapsed() const;
	void setCollapsed(bool col);

	bool isCollapsible() const;
	void setCollapsible(bool col);

	bool isCheckable() const;
	void setCheckable(bool check);

	bool isChecked() const;
	void setChecked(bool check);

	void setChildLayout(QLayout *childLayout);

signals:
	void collapseToggle(bool);
	void checkToggle(bool);

private:
	void expand();
	void collapse();

	static void loopVisibility(QLayout *list, bool visible);

	QPointer<QCheckBox> collapseExpandButton;
	QPointer<QCheckBox> enableCheckBox;
	ClickableLabel *label;

	QPointer<QVBoxLayout> mainLayout;
	QPointer<QHBoxLayout> headerLayout;
	QPointer<QWidget> childContainer;
	QPointer<QLayout> childLayout;

	bool collapsible = false;
	bool checkable = false;

	bool closed;
	bool enabled;

private slots:
	void toggleCollapse(bool = false);
	void toggleCheck(bool = false);
};
