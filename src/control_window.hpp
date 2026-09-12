#pragma once

#include "analogtv.hpp"
#include "control.hpp"
#include "source.hpp"

#include <QtWidgets/QMainWindow>

#include <functional>

class QGroupBox;
class QVBoxLayout;
class QGridLayout;
class QLayout;

namespace atv
{

class ControlWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit ControlWindow(atv::Knobs& knobs, atv::ChanSetting& channel,
                          const std::vector<std::shared_ptr<atv::Source>>& sources,
                          int numChannels, QWidget* parent = nullptr);

  // rebuilds the Channel group box (noise level + reception tabs) to reflect a newly selected channel
  void setChannel(atv::ChanSetting& channel);

signals:
  void knobChanged(const QString& name, double value);
  void chanParamChanged(const QString& name, double value);
  void receptionParamChanged(int index, const QString& name, double value);
  void receptionSourceChanged(int index, int sourceIndex);
  void channelSwitchRequested(int channelIndex);
  void powerToggled(bool isOn);
  void quitRequested();

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void populateChannelSection(atv::ChanSetting& channel);
  void addRangedSliderKnob(const QString& title, double currentValue, double minV, double maxV,
                            QWidget* groupBox, QGridLayout* grid, int row, std::function<void(double)> onChange);
  static void clearLayout(QLayout* layout);

  QGroupBox* channelGroupBox = nullptr;
  QVBoxLayout* channelLayout = nullptr;
  const std::vector<std::shared_ptr<atv::Source>>* allSources = nullptr;
};

} // ::atv
