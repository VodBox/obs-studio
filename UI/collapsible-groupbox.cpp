#include "collapsible-groupbox.hpp"
#include "obs-app.hpp"
#include <QLayout>
#include <QSizePolicy>

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);

	collapseExpandButton = new CollapseCheckBox();
	enableCheckBox = new QCheckBox();
	label = new ClickableLabel();
	label->setText(title);

	collapseExpandButton->setAccessibleName(QTStr("CollapseExpand"));
	enableCheckBox->setAccessibleName(QTStr("Enable"));

	collapseExpandButton->setVisible(false);
	enableCheckBox->setVisible(false);

	collapseExpandButton->setSizePolicy(QSizePolicy::Minimum,
					    QSizePolicy::Preferred);
	enableCheckBox->setSizePolicy(QSizePolicy::Minimum,
				      QSizePolicy::Preferred);
	label->setSizePolicy(QSizePolicy::MinimumExpanding,
			     QSizePolicy::Preferred);

	mainLayout = new QVBoxLayout();
	headerLayout = new QHBoxLayout();
	childContainer = new QWidget();

	headerLayout->addWidget(collapseExpandButton);
	headerLayout->addWidget(enableCheckBox);
	headerLayout->addWidget(label);

	mainLayout->addLayout(headerLayout);
	mainLayout->addWidget(childContainer);

	setLayout(mainLayout);
	setCollapsed(true);
	setEnabled(true);

	connect(collapseExpandButton, &QCheckBox::toggled, this,
		&CollapsibleGroupBox::toggleCollapse);
	connect(label, SIGNAL(clicked()), this, SLOT(toggleCollapse()));
	connect(enableCheckBox, &QCheckBox::toggled, this,
		&CollapsibleGroupBox::toggleCheck);
}

CollapsibleGroupBox::~CollapsibleGroupBox()
{
	collapseExpandButton->deleteLater();
	enableCheckBox->deleteLater();
	label->deleteLater();

	mainLayout->deleteLater();
	headerLayout->deleteLater();
	childContainer->deleteLater();
	if (childLayout)
		childLayout->deleteLater();
}

void CollapsibleGroupBox::loopVisibility(QLayout *layout, bool visible)
{
	for (int i = 0, l = layout->count(); i < l; i++) {
		QLayoutItem *item = layout->itemAt(i);

		QWidget *widget = item->widget();
		if (widget) {
			widget->setVisible(visible);
			continue;
		}

		QLayout *lay = item->layout();
		if (lay) {
			loopVisibility(lay, visible);
		}
	}
}

void CollapsibleGroupBox::expand()
{
	if (childLayout.isNull())
		return;

	childContainer->setVisible(true);
}

void CollapsibleGroupBox::collapse()
{
	if (childLayout.isNull())
		return;

	childContainer->setVisible(false);
}

bool CollapsibleGroupBox::isCollapsed() const
{
	return collapsible && closed;
}

void CollapsibleGroupBox::setCollapsed(bool col)
{
	bool old = collapseExpandButton->blockSignals(true);

	closed = col;
	collapseExpandButton->setChecked(col);
	col ? collapse() : expand();

	collapseExpandButton->blockSignals(old);
}

bool CollapsibleGroupBox::isCollapsible() const
{
	return collapsible;
}

void CollapsibleGroupBox::setCollapsible(bool col)
{
	collapsible = col;
	collapseExpandButton->setVisible(col);
}

bool CollapsibleGroupBox::isCheckable() const
{
	return checkable;
}

void CollapsibleGroupBox::setCheckable(bool check)
{
	checkable = check;
	enableCheckBox->setVisible(check);
}

bool CollapsibleGroupBox::isChecked() const
{
	return enabled;
}

void CollapsibleGroupBox::setChecked(bool check)
{
	bool old = enableCheckBox->blockSignals(true);

	enabled = check;
	enableCheckBox->setChecked(check);

	enableCheckBox->blockSignals(old);
}

void CollapsibleGroupBox::toggleCollapse(bool)
{
	setCollapsed(!closed);
	emit collapseToggle(closed);
}

void CollapsibleGroupBox::toggleCheck(bool)
{
	enabled = !enabled;
	childContainer->setEnabled(enabled);
	emit checkToggle(enabled);
}

void CollapsibleGroupBox::setChildLayout(QLayout *lay)
{
	childContainer->setLayout(lay);

	if (childLayout)
		childLayout->deleteLater();

	childLayout = lay;

	if (isCollapsible() && isCollapsed())
		collapse();

	if (!isCheckable() && !isChecked())
		setChecked(false);
}
