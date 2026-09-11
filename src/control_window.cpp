#include "precomp.hpp"

#include "control_window.hpp"

#include <algorithm>
#include <cmath>

#include <QtWidgets/QDial>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

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


ControlWindow::ControlWindow(atv::Knobs& knobs, QWidget* parent)
  : QMainWindow(parent)
{
  setWindowTitle("AnalogTV Control");
  resize(200, 100);

  QWidget* centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout* layout = new QVBoxLayout(centralWidget);

  QPushButton* offButton = new QPushButton("\342\217\274 OFF", centralWidget);
  offButton->setFont(QFont({QString::fromUtf8("Noto Serif")}, 14));
  layout->addWidget(offButton);

  connect(offButton, &QPushButton::clicked, [this]()
  {
    emit quitRequested();
  });

  QGroupBox* colorGroupBox = new QGroupBox("Color", centralWidget);
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
  QVBoxLayout* geometryLayout = new QVBoxLayout(geometryGroupBox);

  QGridLayout* geometryGrid = new QGridLayout();
  geometryGrid->setColumnStretch(1, 1); // slider column fills remaining space, keeping all sliders the same width
  geometryLayout->addLayout(geometryGrid);

  addSliderKnob("Width",          "width",         knobs.width,         geometryGroupBox, geometryGrid, 0);
  addSliderKnob("Height",         "height",        knobs.height,        geometryGroupBox, geometryGrid, 1);
  addSliderKnob("Squish",         "squish",        knobs.squish,        geometryGroupBox, geometryGrid, 2);
  addSliderKnob("Squeeze Bottom", "squeezeBottom", knobs.squeezeBottom, geometryGroupBox, geometryGrid, 3);

  layout->addWidget(geometryGroupBox);
}

} // ::atv

#include "control_window.moc"
