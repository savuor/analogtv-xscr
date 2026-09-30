#pragma once

#include "precomp.hpp"

#include "analogtv.hpp"
#include "control.hpp"
#include "source.hpp"

#include <QtWidgets/QMainWindow>

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
                          int numChannels, bool autoOn = false, QWidget* parent = nullptr);

  // rebuilds the Channel group box (noise level + reception tabs) to reflect a newly selected channel
  void setChannel(atv::ChanSetting& channel);

signals:
  void knobChanged(const QString& name, double value);
  void chanParamChanged(const QString& name, double value);
  void receptionParamChanged(int index, const QString& name, double value);
  void receptionSourceChanged(int index, int sourceIndex);
  void sourceParamChanged(int index, const QString& name, double value);
  void channelSwitchRequested(int channelIndex);
  void powerToggled(bool isOn);
  void quitRequested();

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void populateChannelSection(atv::ChanSetting& channel);
  // creates the appropriate GUI control (dial, slider, checkbox, spin box...) for a property,
  // based on its ControlType, and wires it up to call onChange whenever the user edits it
  void addPropertyControl(const atv::Properties& properties, const std::string& paramName,
                           QWidget* parent, QGridLayout* grid, int row, std::function<void(double)> onChange);
  static void clearLayout(QLayout* layout);

  QGroupBox* channelGroupBox = nullptr;
  QVBoxLayout* channelLayout = nullptr;
  const std::vector<std::shared_ptr<atv::Source>>* allSources = nullptr;
};

} // ::atv
