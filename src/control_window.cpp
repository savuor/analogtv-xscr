#include "precomp.hpp"

#include "control_window.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDial>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QtGui/QCloseEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>
#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>

namespace atv
{

class QTintWidget : public QWidget
{
  Q_OBJECT

public:
  explicit QTintWidget(const QString& title, QWidget* parent = nullptr)
    : QWidget(parent)
  {
    v = 0.0;

    auto *layout = new QHBoxLayout(this);

    label   = new QLabel(title, this);
    dial    = new QDial(this);
    spinBox = new QDoubleSpinBox(this);

    layout->addWidget(label);
    layout->addWidget(dial);
    layout->addWidget(spinBox);

    connect(dial, &QDial::valueChanged, this, [this](int value)
    {
      this->setValueInternal(value);
      emit valueChanged(this->v);
    });

    connect(spinBox, &QDoubleSpinBox::valueChanged, this, [this](double value)
    {
      this->setValueInternal(value);
      emit valueChanged(this->v);
    });

    dial->setWrapping(true);
    dial->setNotchesVisible(false);
    dial->setRange(0, 360);
    spinBox->setRange(0.0, 360.0);
    spinBox->setWrapping(true);
  }

  void setValue(double value)
  {
    setValueInternal(value);
  }

  double value() const
  {
    return this->v;
  }

signals:
  void valueChanged(double value);

private:

  void setValueInternal(double value)
  {
    v = value;
    dial->blockSignals(true);
    spinBox->blockSignals(true);
    spinBox->setValue(v);
    dial->setValue(static_cast<int>(v));
    dial->blockSignals(false);
    spinBox->blockSignals(false);
  }

  QDial* dial;
  QDoubleSpinBox* spinBox;
  QLabel* label;
  double v;
};


class SliderSpinboxKnob : public QObject
{
  Q_OBJECT

public:
  // adds label/slider/spinbox to row `row` of `grid` so all knobs in the grid share column widths
  // in integer mode the slider maps 1:1 to the [min, max] range and an int spin box is used
  SliderSpinboxKnob(const QString& title, QWidget* parent, QGridLayout* grid, int row, bool isInteger = false)
    : QObject(parent), integer(isInteger)
  {
    v = 0.0;
    minV = 0.0;
    maxV = 100.0;

    label  = new QLabel(title, parent);
    slider = new QSlider(Qt::Horizontal, parent);

    // typed pointers are only needed here, to connect the type-specific valueChanged signals
    if (integer)
    {
      auto* intSpin = new QSpinBox(parent);
      spinBox = intSpin;
      connect(intSpin, &QSpinBox::valueChanged, this, [this](int value)
      {
        this->setValueInternal(value);
        emit valueChanged(this->v);
      });
    }
    else
    {
      auto* doubleSpin = new QDoubleSpinBox(parent);
      spinBox = doubleSpin;
      connect(doubleSpin, &QDoubleSpinBox::valueChanged, this, [this](double value)
      {
        this->setValueInternal(value);
        emit valueChanged(this->v);
      });
    }

    grid->addWidget(label,   row, 0);
    grid->addWidget(slider,  row, 1);
    grid->addWidget(spinBox, row, 2);

    // slider is always integer-valued, so in double mode its value is rescaled to/from the [minV, maxV] range
    connect(slider, &QSlider::valueChanged, this, [this](int value)
    {
      double newValue = integer ? value : minV + (maxV - minV) * value / sliderSteps;
      this->setValueInternal(newValue);
      emit valueChanged(this->v);
    });

    setRange(0.0, 100.0);
  }

  void setValue(double value)
  {
    setValueInternal(value);
  }

  double value() const
  {
    return this->v;
  }

  void setRange(double min, double max)
  {
    minV = min;
    maxV = max;
    slider->blockSignals(true);
    spinBox->blockSignals(true);
    if (integer)
    {
      spinBox->setProperty("minimum", static_cast<int>(min));
      spinBox->setProperty("maximum", static_cast<int>(max));
      slider->setRange(static_cast<int>(min), static_cast<int>(max));
    }
    else
    {
      spinBox->setProperty("minimum", min);
      spinBox->setProperty("maximum", max);
      spinBox->setProperty("singleStep", (max - min) / 100.0);
      slider->setRange(0, sliderSteps);
    }
    spinBox->blockSignals(false);
    slider->blockSignals(false);
    setValueInternal(std::clamp(v, minV, maxV));
  }

signals:
  void valueChanged(double value);

private:

  void setValueInternal(double value)
  {
    v = std::clamp(value, minV, maxV);
    if (integer)
      v = std::round(v);

    slider->blockSignals(true);
    spinBox->blockSignals(true);
    if (integer)
    {
      spinBox->setProperty("value", static_cast<int>(v));
      slider->setValue(static_cast<int>(v));
    }
    else
    {
      spinBox->setProperty("value", v);
      int sliderValue = (maxV > minV) ? static_cast<int>(std::round((v - minV) / (maxV - minV) * sliderSteps)) : 0;
      slider->setValue(sliderValue);
    }
    spinBox->blockSignals(false);
    slider->blockSignals(false);
  }

  static constexpr int sliderSteps = 1000;

  bool integer;
  QSlider* slider;
  QAbstractSpinBox* spinBox;
  QLabel* label;
  double v, minV, maxV;
};


// A group box whose title toggles the visibility of its contents.
class CollapsibleGroupBox : public QGroupBox
{
  Q_OBJECT

public:
  explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr)
    : QGroupBox(title, parent), originalTitle(title)
  {
    auto* groupLayout = new QVBoxLayout(this);
    content = new QWidget(this);
    groupLayout->addWidget(content);
    updateTitle();
  }

  QWidget* contentWidget() const
  {
    return content;
  }

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    titlePressed = titleRect().contains(event->pos());
    QGroupBox::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    if (titlePressed && titleRect().contains(event->pos()))
    {
      expanded = !expanded;
      updateTitle();
      content->setVisible(expanded);
    }

    titlePressed = false;
    QGroupBox::mouseReleaseEvent(event);
  }

private:
  QRect titleRect() const
  {
    QStyleOptionGroupBox option;
    option.initFrom(this);
    option.text = title();
    return style()->subControlRect(QStyle::CC_GroupBox, &option,
                                   QStyle::SC_GroupBoxLabel, this);
  }

  void updateTitle()
  {
    setTitle(QString(expanded ? "[-] " : "[+] ") + originalTitle);
  }

  QString originalTitle;
  QWidget* content = nullptr;
  bool expanded = true;
  bool titlePressed = false;
};


ControlWindow::ControlWindow(atv::Knobs& knobs, atv::ChanSetting& channel,
                             const std::vector<std::shared_ptr<atv::Source>>& sources,
                             int numChannels, bool autoOn, QWidget* parent)
  : QMainWindow(parent)
{
  setWindowTitle("AnalogTV Control");
  resize(450, 400);

  QScrollArea* scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  setCentralWidget(scrollArea);

  QWidget* centralWidget = new QWidget(scrollArea);
  scrollArea->setWidget(centralWidget);

  QVBoxLayout* layout = new QVBoxLayout(centralWidget);

  QPushButton* powerButton = new QPushButton("\342\217\274 OFF", centralWidget);
  powerButton->setFont(QFont({QString::fromUtf8("Noto Serif")}, 14));
  powerButton->setCheckable(true);
  layout->addWidget(powerButton);

  connect(powerButton, &QPushButton::toggled, [powerButton, this](bool isOn)
  {
    powerButton->setText(isOn ? "\342\217\274 ON" : "\342\217\274 OFF");
    emit powerToggled(isOn);
  });

  powerButton->setChecked(autoOn);

  QGroupBox* colorGroupBox = new CollapsibleGroupBox("Color", centralWidget);
  QWidget* colorContent = static_cast<CollapsibleGroupBox*>(colorGroupBox)->contentWidget();
  QVBoxLayout* colorLayout = new QVBoxLayout(colorContent);

  QGridLayout* colorGrid = new QGridLayout();
  colorGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  colorLayout->addLayout(colorGrid);

  auto addKnobControl = [this, &knobs](const std::string& paramName,
                                        QWidget* groupBox, QGridLayout* grid, int row)
  {
    addPropertyControl(knobs.properties, paramName, groupBox, grid, row, [this, paramName](double value)
    {
      emit knobChanged(QString::fromStdString(paramName), value);
    });
  };

  int colorRow = 0;
  addKnobControl("tint",       colorContent, colorGrid, colorRow++);
  addKnobControl("color",      colorContent, colorGrid, colorRow++);
  addKnobControl("brightness", colorContent, colorGrid, colorRow++);
  addKnobControl("contrast",   colorContent, colorGrid, colorRow++);

  layout->addWidget(colorGroupBox);

  QGroupBox* geometryGroupBox = new CollapsibleGroupBox("Geometry", centralWidget);
  QWidget* geometryContent = static_cast<CollapsibleGroupBox*>(geometryGroupBox)->contentWidget();
  QVBoxLayout* geometryLayout = new QVBoxLayout(geometryContent);

  QGridLayout* geometryGrid = new QGridLayout();
  geometryGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  geometryLayout->addLayout(geometryGrid);

  int geometryRow = 0;
  addKnobControl("width",         geometryContent, geometryGrid, geometryRow++);
  addKnobControl("height",        geometryContent, geometryGrid, geometryRow++);
  addKnobControl("squish",        geometryContent, geometryGrid, geometryRow++);
  addKnobControl("squeezeBottom", geometryContent, geometryGrid, geometryRow++);

  layout->addWidget(geometryGroupBox);

  QGroupBox* miscGroupBox = new CollapsibleGroupBox("Miscelaneous", centralWidget);
  QWidget* miscContent = static_cast<CollapsibleGroupBox*>(miscGroupBox)->contentWidget();
  QVBoxLayout* miscLayout = new QVBoxLayout(miscContent);

  QGridLayout* miscGrid = new QGridLayout();
  miscGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  miscLayout->addLayout(miscGrid);

  int miscRow = 0;
  addKnobControl("useFlutterHorizontalDesync", miscContent, miscGrid, miscRow++);
  addKnobControl("horizontalDesync",           miscContent, miscGrid, miscRow++);
  addKnobControl("shrinkpulseProbability",     miscContent, miscGrid, miscRow++);
  addKnobControl("channelChangeCycles",        miscContent, miscGrid, miscRow++);

  layout->addWidget(miscGroupBox);

  channelGroupBox = new CollapsibleGroupBox("Channel", centralWidget);
  channelContentWidget = static_cast<CollapsibleGroupBox*>(channelGroupBox)->contentWidget();
  channelLayout = new QVBoxLayout(channelContentWidget);
  this->allSources = &sources;

  populateChannelSection(channel);

  layout->addWidget(channelGroupBox);

  QGroupBox* channelsGroupBox = new CollapsibleGroupBox("Channels", centralWidget);
  QWidget* channelsContent = static_cast<CollapsibleGroupBox*>(channelsGroupBox)->contentWidget();
  QVBoxLayout* channelsLayout = new QVBoxLayout(channelsContent);
  QGridLayout* channelsGrid = new QGridLayout();
  channelsLayout->addLayout(channelsGrid);

  const int channelButtonColumns = 4;
  for (int i = 0; i < numChannels; ++i)
  {
    QPushButton* channelButton = new QPushButton(QString::number(i), channelsContent);
    connect(channelButton, &QPushButton::clicked, [this, i]()
    {
      emit channelSwitchRequested(i);
    });
    channelsGrid->addWidget(channelButton, i / channelButtonColumns, i % channelButtonColumns);
  }

  layout->addWidget(channelsGroupBox);
}

void ControlWindow::setChannel(atv::ChanSetting& channel)
{
  populateChannelSection(channel);
}

void ControlWindow::clearLayout(QLayout* layoutToClear)
{
  while (QLayoutItem* item = layoutToClear->takeAt(0))
  {
    if (QWidget* widget = item->widget())
    {
      widget->deleteLater();
      delete item;
    }
    else if (QLayout* childLayout = item->layout())
    {
      // a QLayout is itself a QLayoutItem, so item and childLayout are the same
      // object here: deleting childLayout already deletes item, don't double-delete
      clearLayout(childLayout);
      delete childLayout;
    }
    else
    {
      delete item;
    }
  }
}

void ControlWindow::addPropertyControl(const atv::Properties& properties, const std::string& paramName,
                                        QWidget* parent, QGridLayout* grid, int row, std::function<void(double)> onChange)
{
  const QString title = QString::fromStdString(properties.getDescription(paramName));
  double currentValue = properties.getValue(paramName);
  auto [minV, maxV] = properties.getRange(paramName);

  switch (properties.getControlType(paramName))
  {
    case atv::ControlType::Dial:
    {
      QTintWidget* dial = new QTintWidget(title, parent);
      dial->setValue(currentValue);
      connect(dial, &QTintWidget::valueChanged, onChange);
      grid->addWidget(dial, row, 0, 1, 3);
      break;
    }

    case atv::ControlType::CheckBox:
    {
      QCheckBox* checkBox = new QCheckBox(title, parent);
      checkBox->setChecked(currentValue > std::numeric_limits<double>::epsilon());
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
      connect(checkBox, &QCheckBox::checkStateChanged, [onChange](Qt::CheckState state)
      {
        onChange(state == Qt::Checked ? 1.0 : 0.0);
      });
#else
      connect(checkBox, &QCheckBox::stateChanged, [onChange](int state)
      {
        onChange(state == Qt::Checked ? 1.0 : 0.0);
      });
#endif
      grid->addWidget(checkBox, row, 0, 1, 3);
      break;
    }

    case atv::ControlType::IntSpin:
    case atv::ControlType::DoubleSpin:
    {
      const bool isInteger = properties.getControlType(paramName) == atv::ControlType::IntSpin;
      SliderSpinboxKnob* knob = new SliderSpinboxKnob(title, parent, grid, row, isInteger);
      knob->setRange(minV, maxV);
      knob->setValue(currentValue);
      connect(knob, &SliderSpinboxKnob::valueChanged, onChange);
      break;
    }
    default:
    {
      throw std::runtime_error("Unsupported control type");
    }
      break;
  }
}

void ControlWindow::populateChannelSection(atv::ChanSetting& channel)
{
  clearLayout(channelLayout);

  QGridLayout* channelGrid = new QGridLayout();
  channelGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  channelLayout->addLayout(channelGrid);

  addPropertyControl(channel.properties, "noise_level", channelContentWidget, channelGrid, 0,
    [this](double value)
    {
      emit chanParamChanged("noise_level", value);
    });

  QTabWidget* receptionsTabs = new QTabWidget(channelContentWidget);
  channelLayout->addWidget(receptionsTabs);

  for (size_t i = 0; i < channel.receptions.size(); ++i)
  {
    atv::AnalogReception& rec = channel.receptions[i];
    int index = static_cast<int>(i);

    QWidget* tab = new QWidget();
    QVBoxLayout* tabLayout = new QVBoxLayout(tab);

    QComboBox* sourceCombo = new QComboBox(tab);
    int currentSourceIdx = -1;
    for (size_t s = 0; s < allSources->size(); ++s)
    {
      sourceCombo->addItem(QString::fromStdString((*allSources)[s]->getName()));
      if ((*allSources)[s] == channel.sources[i]) currentSourceIdx = static_cast<int>(s);
    }
    sourceCombo->setCurrentIndex(currentSourceIdx);
    connect(sourceCombo, &QComboBox::currentIndexChanged, [this, index](int sourceIdx)
    {
      emit receptionSourceChanged(index, sourceIdx);
    });
    tabLayout->addWidget(sourceCombo);

    QGridLayout* recGrid = new QGridLayout();
    recGrid->setColumnStretch(1, 1);
    tabLayout->addLayout(recGrid);

    auto addReceptionControl = [this, &rec, tab, recGrid, index](const std::string& paramName, int row)
    {
      addPropertyControl(rec.properties, paramName, tab, recGrid, row, [this, index, paramName](double value)
      {
        emit receptionParamChanged(index, QString::fromStdString(paramName), value);
      });
    };

    int row = 0;
    addReceptionControl("level", row++);
    addReceptionControl("multipath", row++);
    addReceptionControl("ofs", row++);
    addReceptionControl("freqerr", row++);
    addReceptionControl("hfloss_enable", row++);

    recGrid->addWidget(new QLabel("Source properties:", tab), row++, 0, 1, 2);

    // these belong to the currently selected source, not to the reception itself
    std::shared_ptr<atv::Source> source = channel.sources[i];
    auto addSourceControl = [this, source, tab, recGrid, index](const std::string& paramName, int controlRow)
    {
      addPropertyControl(source->properties, paramName, tab, recGrid, controlRow, [this, index, paramName](double value)
      {
        emit sourceParamChanged(index, QString::fromStdString(paramName), value);
      });
    };

    addSourceControl("do_ssavi", row++);
    addSourceControl("do_cb", row++);
    addSourceControl("displayTimestamp", row++);
    addSourceControl("vertUnderscan", row++);
    addSourceControl("horizUnderscan", row++);
    addSourceControl("lineOverscan", row++);
    addSourceControl("filter_enabled", row++);
    addSourceControl("vertical_smoothing", row++);

    receptionsTabs->addTab(tab, QString("Reception %1").arg(index));
  }
}

void ControlWindow::closeEvent(QCloseEvent* event)
{
  emit quitRequested();
  QMainWindow::closeEvent(event);
}

} // ::atv

#include "control_window.moc"
