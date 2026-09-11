#pragma once

#include "analogtv.hpp"

#include <QtWidgets/QMainWindow>

namespace atv
{

class ControlWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit ControlWindow(atv::Knobs& knobs, QWidget* parent = nullptr);

signals:
  void knobChanged(const QString& name, double value);
  void quitRequested();
};

} // ::atv
