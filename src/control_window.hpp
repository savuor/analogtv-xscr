#pragma once

#include "analogtv.hpp"
#include "control.hpp"
#include "source.hpp"

#include <QtWidgets/QMainWindow>

namespace atv
{

class ControlWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit ControlWindow(atv::Knobs& knobs, atv::ChanSetting& channel,
                          const std::vector<std::shared_ptr<atv::Source>>& allSources,
                          QWidget* parent = nullptr);

signals:
  void knobChanged(const QString& name, double value);
  void chanParamChanged(const QString& name, double value);
  void receptionParamChanged(int index, const QString& name, double value);
  void receptionSourceChanged(int index, int sourceIndex);
  void powerToggled(bool isOn);
  void quitRequested();

protected:
  void closeEvent(QCloseEvent* event) override;
};

} // ::atv
