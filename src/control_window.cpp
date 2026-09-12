#include "precomp.hpp"

#include "control_window.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDial>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QtGui/QCloseEvent>

#include <QtCore/qmetatype.h>
#include <QtCore/qtmochelpers.h>
#include <QtCore/qxptype_traits.h>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif


namespace atv
{

class QTintWidget : public QWidget
{
  Q_OBJECT

public:
  explicit QTintWidget(QWidget* parent = nullptr)
    : QWidget(parent)
  {
    v = 0.0;

    auto *layout = new QHBoxLayout(this);

    label   = new QLabel("Tint", this);
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
  SliderSpinboxKnob(const QString& title, QWidget* parent, QGridLayout* grid, int row)
    : QObject(parent)
  {
    v = 0.0;
    minV = 0.0;
    maxV = 100.0;

    label   = new QLabel(title, parent);
    slider  = new QSlider(Qt::Horizontal, parent);
    spinBox = new QDoubleSpinBox(parent);

    slider->setRange(0, sliderSteps);

    grid->addWidget(label,   row, 0);
    grid->addWidget(slider,  row, 1);
    grid->addWidget(spinBox, row, 2);

    // slider is always integer-valued, so its value is rescaled to/from the [minV, maxV] range
    connect(slider, &QSlider::valueChanged, this, [this](int value)
    {
      double newValue = minV + (maxV - minV) * value / sliderSteps;
      this->setValueInternal(newValue);
      emit valueChanged(this->v);
    });

    connect(spinBox, &QDoubleSpinBox::valueChanged, this, [this](double value)
    {
      this->setValueInternal(value);
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
    spinBox->setRange(min, max);
    spinBox->setSingleStep((max - min) / 100.0);
    setValueInternal(std::clamp(v, minV, maxV));
  }

signals:
  void valueChanged(double value);

private:

  void setValueInternal(double value)
  {
    v = std::clamp(value, minV, maxV);
    slider->blockSignals(true);
    spinBox->blockSignals(true);
    spinBox->setValue(v);
    int sliderValue = (maxV > minV) ? static_cast<int>(std::round((v - minV) / (maxV - minV) * sliderSteps)) : 0;
    slider->setValue(sliderValue);
    slider->blockSignals(false);
    spinBox->blockSignals(false);
  }

  static constexpr int sliderSteps = 1000;

  QSlider* slider;
  QDoubleSpinBox* spinBox;
  QLabel* label;
  double v, minV, maxV;
};


// makes a QGroupBox checkable and collapses/expands its contents (but keeps the title bar) on toggle
static void makeCollapsible(QGroupBox* box)
{
  box->setCheckable(true);
  box->setChecked(true);
  QObject::connect(box, &QGroupBox::toggled, [box](bool expanded)
  {
    for (QObject* child : box->children())
    {
      if (QWidget* widget = qobject_cast<QWidget*>(child))
        widget->setVisible(expanded);
    }
  });
}


ControlWindow::ControlWindow(atv::Knobs& knobs, atv::ChanSetting& channel,
                             const std::vector<std::shared_ptr<atv::Source>>& sources,
                             int numChannels, QWidget* parent)
  : QMainWindow(parent)
{
  setWindowTitle("AnalogTV Control");
  resize(200, 100);

  QWidget* centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout* layout = new QVBoxLayout(centralWidget);

  QPushButton* powerButton = new QPushButton("\342\217\274 OFF", centralWidget);
  powerButton->setFont(QFont({QString::fromUtf8("Noto Serif")}, 14));
  powerButton->setCheckable(true);
  powerButton->setChecked(false); // default state is off
  layout->addWidget(powerButton);

  connect(powerButton, &QPushButton::toggled, [powerButton, this](bool isOn)
  {
    powerButton->setText(isOn ? "\342\217\274 ON" : "\342\217\274 OFF");
    emit powerToggled(isOn);
  });

  QGroupBox* colorGroupBox = new QGroupBox("Color", centralWidget);
  makeCollapsible(colorGroupBox);
  QVBoxLayout* colorLayout = new QVBoxLayout(colorGroupBox);

  QTintWidget* tintWidget = new QTintWidget(colorGroupBox);
  tintWidget->setValue(knobs.tint);
  connect(tintWidget, &QTintWidget::valueChanged, [this](double value)
  {
    emit knobChanged("tint", value);
  });
  colorLayout->addWidget(tintWidget);

  QGridLayout* colorGrid = new QGridLayout();
  colorGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  colorLayout->addLayout(colorGrid);

  auto addSliderKnob = [this, &knobs](const QString& title, const std::string& paramName, double currentValue,
                                       QWidget* groupBox, QGridLayout* grid, int row)
  {
    SliderSpinboxKnob* knob = new SliderSpinboxKnob(title, groupBox, grid, row);
    auto [minV, maxV] = knobs.getRange(paramName);
    knob->setRange(minV, maxV);
    knob->setValue(currentValue);
    connect(knob, &SliderSpinboxKnob::valueChanged, [this, paramName](double value)
    {
      emit knobChanged(QString::fromStdString(paramName), value);
    });
  };

  addSliderKnob("Color",      "color",      knobs.color,      colorGroupBox, colorGrid, 0);
  addSliderKnob("Brightness", "brightness", knobs.brightness, colorGroupBox, colorGrid, 1);
  addSliderKnob("Contrast",   "contrast",   knobs.contrast,   colorGroupBox, colorGrid, 2);

  layout->addWidget(colorGroupBox);

  QGroupBox* geometryGroupBox = new QGroupBox("Geometry", centralWidget);
  makeCollapsible(geometryGroupBox);
  QVBoxLayout* geometryLayout = new QVBoxLayout(geometryGroupBox);

  QGridLayout* geometryGrid = new QGridLayout();
  geometryGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  geometryLayout->addLayout(geometryGrid);

  addSliderKnob("Width",          "width",         knobs.width,         geometryGroupBox, geometryGrid, 0);
  addSliderKnob("Height",         "height",        knobs.height,        geometryGroupBox, geometryGrid, 1);
  addSliderKnob("Squish",         "squish",        knobs.squish,        geometryGroupBox, geometryGrid, 2);
  addSliderKnob("Squeeze Bottom", "squeezeBottom", knobs.squeezeBottom, geometryGroupBox, geometryGrid, 3);

  layout->addWidget(geometryGroupBox);

  QGroupBox* miscGroupBox = new QGroupBox("Miscelaneous", centralWidget);
  makeCollapsible(miscGroupBox);
  QVBoxLayout* miscLayout = new QVBoxLayout(miscGroupBox);

  QGridLayout* miscGrid = new QGridLayout();
  miscGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  miscLayout->addLayout(miscGrid);

  addSliderKnob("Power Up",           "powerup",          knobs.powerup,          miscGroupBox, miscGrid, 0);
  addSliderKnob("Horizontal Desync",  "horizontalDesync", knobs.horizontalDesync, miscGroupBox, miscGrid, 1);

  auto addCheckBox = [this, miscGroupBox, miscLayout](const QString& title, const std::string& paramName, bool currentValue)
  {
    QCheckBox* checkBox = new QCheckBox(title, miscGroupBox);
    checkBox->setChecked(currentValue);
    connect(checkBox, &QCheckBox::checkStateChanged, [this, paramName](Qt::CheckState state)
    {
      emit knobChanged(QString::fromStdString(paramName), state == Qt::Checked ? 1.0 : 0.0);
    });
    miscLayout->addWidget(checkBox);
  };

  addCheckBox("Use Flutter Horizontal Desync", "useFlutterHorizontalDesync", knobs.useFlutterHorizontalDesync);
  addCheckBox("Use Hash Noise",                "useHashNoise",               knobs.useHashNoise);
  addCheckBox("Enable Hash Noise",             "enableHashNoise",            knobs.enableHashNoise);

  QHBoxLayout* cyclesLayout = new QHBoxLayout();
  QLabel* cyclesLabel = new QLabel("Channel Change Cycles", miscGroupBox);
  QSpinBox* cyclesSpinBox = new QSpinBox(miscGroupBox);
  auto [cyclesMin, cyclesMax] = knobs.getRange("channelChangeCycles");
  cyclesSpinBox->setRange(static_cast<int>(cyclesMin), static_cast<int>(cyclesMax));
  cyclesSpinBox->setValue(knobs.channelChangeCycles);
  connect(cyclesSpinBox, &QSpinBox::valueChanged, [this](int value)
  {
    emit knobChanged("channelChangeCycles", value);
  });
  cyclesLayout->addWidget(cyclesLabel);
  cyclesLayout->addWidget(cyclesSpinBox);
  miscLayout->addLayout(cyclesLayout);

  layout->addWidget(miscGroupBox);

  channelGroupBox = new QGroupBox("Channel", centralWidget);
  makeCollapsible(channelGroupBox);
  channelLayout = new QVBoxLayout(channelGroupBox);
  this->allSources = &sources;

  populateChannelSection(channel);

  layout->addWidget(channelGroupBox);

  QGroupBox* channelsGroupBox = new QGroupBox("Channels", centralWidget);
  makeCollapsible(channelsGroupBox);
  QGridLayout* channelsGrid = new QGridLayout(channelsGroupBox);

  const int channelButtonColumns = 4;
  for (int i = 0; i < numChannels; ++i)
  {
    QPushButton* channelButton = new QPushButton(QString::number(i + 1), channelsGroupBox);
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

void ControlWindow::addRangedSliderKnob(const QString& title, double currentValue, double minV, double maxV,
                                         QWidget* groupBox, QGridLayout* grid, int row, std::function<void(double)> onChange)
{
  SliderSpinboxKnob* knob = new SliderSpinboxKnob(title, groupBox, grid, row);
  knob->setRange(minV, maxV);
  knob->setValue(currentValue);
  connect(knob, &SliderSpinboxKnob::valueChanged, onChange);
}

void ControlWindow::populateChannelSection(atv::ChanSetting& channel)
{
  clearLayout(channelLayout);

  QGridLayout* channelGrid = new QGridLayout();
  channelGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  channelLayout->addLayout(channelGrid);

  auto [noiseMin, noiseMax] = channel.getRange("noise_level");
  addRangedSliderKnob("Noise Level", channel.noise_level, noiseMin, noiseMax, channelGroupBox, channelGrid, 0,
    [this](double value)
    {
      emit chanParamChanged("noise_level", value);
    });

  QTabWidget* receptionsTabs = new QTabWidget(channelGroupBox);
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

    auto addReceptionSlider = [this, tab, recGrid, index](const QString& title, const std::string& paramName,
                                                           double currentValue, double minV, double maxV, int row)
    {
      addRangedSliderKnob(title, currentValue, minV, maxV, tab, recGrid, row, [this, index, paramName](double value)
      {
        emit receptionParamChanged(index, QString::fromStdString(paramName), value);
      });
    };

    auto [levelMin, levelMax] = rec.getRange("level");
    addReceptionSlider("Level", "level", rec.level, levelMin, levelMax, 0);

    auto [multipathMin, multipathMax] = rec.getRange("multipath");
    addReceptionSlider("Multipath", "multipath", rec.multipath, multipathMin, multipathMax, 1);

    auto [ofsMin, ofsMax] = rec.getRange("ofs");
    addReceptionSlider("Offset", "ofs", rec.ofs, ofsMin, ofsMax, 2);

    auto [freqMin, freqMax] = rec.getRange("freqerr");
    addReceptionSlider("Frequency Error", "freqerr", rec.freqerr, freqMin, freqMax, 3);

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
