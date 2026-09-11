#include "precomp.hpp"

#include "gui_control.hpp"

#include <thread>

#include <QtCore/QVariant>
#include <QtCore/QSignalMapper>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDial>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMessageBox>


#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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

class SliderSpinboxLabelWidget : public QWidget
{
  Q_OBJECT

public:
  explicit SliderSpinboxLabelWidget(const QString& title = { }, QWidget* parent = nullptr)
    : QWidget(parent)
  {
    v = 0.0;

    slider  = new QSlider(Qt::Horizontal, this);
    spinBox = new QDoubleSpinBox(this);
    label   = new QLabel(title, this);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(slider);
    layout->addWidget(spinBox);
    layout->addWidget(label);

    connect(slider, &QSlider::valueChanged, this, [this](int value)
    {
      this->setValueInternal(value);
      emit valueChanged(this->v);
    });
    //connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value)
    connect(spinBox, &QDoubleSpinBox::valueChanged, this, [this](double value)
    {
      this->setValueInternal(value);
      emit valueChanged(this->v);
    });

    this->setRange(0.0, 100.0);
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
      spinBox->setRange(min, max);
      slider->setRange(static_cast<int>(min), static_cast<int>(max));
  }

signals:
  void valueChanged(double value);

private:

  void setValueInternal(double value)
  {
    v = value;
    slider->blockSignals(true);
    spinBox->blockSignals(true);
    spinBox->setValue(v);
    slider->setValue(static_cast<int>(v));
    slider->blockSignals(false);
    spinBox->blockSignals(false);
  }

  QSlider* slider;
  QDoubleSpinBox* spinBox;
  QLabel* label;
  double v;
};


class QTintWidget : public QWidget
{
  Q_OBJECT

public:
  explicit QTintWidget(QWidget* parent = nullptr)
    : QWidget(parent)
  {
    v = 0.0;

    auto *layout = new QHBoxLayout(this);

    dial = new QDial(this);
    spinBox = new QDoubleSpinBox(this);
    label   = new QLabel("Tint", this);

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


// class MainWindow : public QMainWindow
// {
//   Q_OBJECT

// public:

//   Knobs& knobs;
//   std::vector<ChanSetting>& chanSettings;

//   MainWindow(Knobs& _knobs, std::vector<ChanSetting>& _chanSettings, QWidget *parent = nullptr):
//     QMainWindow(parent),
//     knobs(_knobs),
//     chanSettings(_chanSettings)
//   {
//     this->setWindowTitle("MainWindow");
//     this->resize(1061, 869);

//       QWidget* centralwidget = new QWidget(this);
//       QPushButton* pushButton = new QPushButton("\342\217\274 OFF", centralwidget);
//       pushButton->setGeometry(QRect(10, 10, 71, 32));
//       pushButton->setFont(QFont({QString::fromUtf8("Noto Serif")}, /*pointSize*/ 14));

//       createColorGroupBox(centralwidget);

//       createGeometryGroupBox(centralwidget);

//       createMiscelaneousGroupBox(centralwidget);

      

//       {
//         QGroupBox* groupBox = new QGroupBox("Channel", centralwidget);
//         groupBox->setGeometry(QRect(20, 490, 671, 331));
//         QVBoxLayout* vertLayout = new QVBoxLayout(groupBox);
        
//         SliderSpinboxLabelWidget* noiseLevelWidget = new SliderSpinboxLabelWidget("Noise Level", groupBox);
//         connect(noiseLevelWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//         {
//           emit MainWindow::valueChanged("noise_level", value);
//         });
//         vertLayout->addWidget(noiseLevelWidget);

//         QTabWidget* tabWidget = new QTabWidget(groupBox);

//         for (int i = 0; i < 2; ++i)
//         {
//           QWidget * tab = new QWidget();
//           QVBoxLayout * layout = new QVBoxLayout(tab);

//           QComboBox * sourcesCombo = new QComboBox(tab);
//           sourcesCombo->addItem("Source 1");
//           sourcesCombo->addItem("Source 2");
//           layout->addWidget(sourcesCombo);

//           QGridLayout * gridLayout = new QGridLayout();

//           SliderSpinboxLabelWidget* levelWidget          = new SliderSpinboxLabelWidget("Level", tab);
//           SliderSpinboxLabelWidget* multipathWidget      = new SliderSpinboxLabelWidget("Multipath", tab);
//           SliderSpinboxLabelWidget* offsetWidget         = new SliderSpinboxLabelWidget("Offset", tab);
//           SliderSpinboxLabelWidget* frequencyErrorWidget = new SliderSpinboxLabelWidget("Frequency Error", tab);

//           gridLayout->addWidget(levelWidget,          0, 0, 1, 1);
//           gridLayout->addWidget(multipathWidget,      0, 1, 1, 1);
//           gridLayout->addWidget(offsetWidget,         1, 0, 1, 1);
//           gridLayout->addWidget(frequencyErrorWidget, 1, 1, 1, 1);

//           layout->addLayout(gridLayout);

//           tabWidget->addTab(tab, "Reception " + QString::number(i));
//         }

//         //tabWidget->addTab(new QWidget(), "Add New...");

//         vertLayout->addWidget(tabWidget);

//         tabWidget->setCurrentIndex(0);

//       }
//     }

//   void createColorGroupBox(QWidget* centralwidget)
//   {
//     QGroupBox* groupBox = new QGroupBox("Color", centralwidget);
//     groupBox->setGeometry(QRect(20, 50, 351, 231));
//     QVBoxLayout* vertLayout = new QVBoxLayout(groupBox);

//     SliderSpinboxLabelWidget* colorKnobWidget = new SliderSpinboxLabelWidget("Color", groupBox);
//     connect(colorKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("color", value);
//     });
//     vertLayout->addWidget(colorKnobWidget);

//     SliderSpinboxLabelWidget* brightnessKnobWidget = new SliderSpinboxLabelWidget("Brightness", groupBox);
//     connect(brightnessKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("brightness", value);
//     });
//     vertLayout->addWidget(brightnessKnobWidget);

//     SliderSpinboxLabelWidget* contrastKnobWidget = new SliderSpinboxLabelWidget("Contrast", groupBox);
//     connect(contrastKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("contrast", value);
//     });
//     vertLayout->addWidget(contrastKnobWidget);

//     QTintWidget* tintWidget = new QTintWidget(groupBox);
//     connect(tintWidget, &QTintWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("tint", value);
//     });
//     vertLayout->addWidget(tintWidget);
//   }

//   void createGeometryGroupBox(QWidget* centralwidget)
//   {
//     QGroupBox* groupBox = new QGroupBox("Geometry", centralwidget);
//     groupBox->setGeometry(QRect(390, 20, 471, 251));

//     QVBoxLayout* vertLayout = new QVBoxLayout(groupBox);
//     SliderSpinboxLabelWidget* widthKnobWidget = new SliderSpinboxLabelWidget("Width", groupBox);
//     connect(widthKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("width", value);
//     });
//     vertLayout->addWidget(widthKnobWidget);
    
//     SliderSpinboxLabelWidget* heightKnobWidget = new SliderSpinboxLabelWidget("Height", groupBox);
//     connect(heightKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("height", value);
//     });
//     vertLayout->addWidget(heightKnobWidget);
    
//     SliderSpinboxLabelWidget* squishKnobWidget = new SliderSpinboxLabelWidget("Squish", groupBox);
//     connect(squishKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("squish", value);
//     });
//     vertLayout->addWidget(squishKnobWidget);

//     SliderSpinboxLabelWidget* squeezeBottomKnobWidget = new SliderSpinboxLabelWidget("Squeeze Bottom", groupBox);
//     connect(squeezeBottomKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("squeeze_bottom", value);
//     });
//     vertLayout->addWidget(squeezeBottomKnobWidget);
//   }

//   void createMiscelaneousGroupBox(QWidget* centralwidget)
//   {
//     QGroupBox* groupBox = new QGroupBox("Miscelaneous", centralwidget);
//     groupBox->setGeometry(QRect(420, 300, 551, 181));
//     QVBoxLayout* vertLayout = new QVBoxLayout(groupBox);

//     SliderSpinboxLabelWidget* powerupKnobWidget = new SliderSpinboxLabelWidget("Power Up", groupBox);
//     connect(powerupKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("powerup", value);
//     });
//     vertLayout->addWidget(powerupKnobWidget);

//     SliderSpinboxLabelWidget* horizontalDesyncKnobWidget = new SliderSpinboxLabelWidget("Horizontal Desync", groupBox);
//     connect(horizontalDesyncKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("horizontal_desync", value);
//     });
//     vertLayout->addWidget(horizontalDesyncKnobWidget);

//     QHBoxLayout* horizontalLayout = new QHBoxLayout();
//     QCheckBox* flutterBox      = new QCheckBox("Use Flutter Horizontal Desync", groupBox);
//     connect(flutterBox, &QCheckBox::stateChanged, [this](int state)
//     {
//       emit MainWindow::valueChanged("flutter_horizontal_desync", state == Qt::Checked ? 1.0 : 0.0);
//     });
//     horizontalLayout->addWidget(flutterBox);
//     QCheckBox* hashNoiseUseBox = new QCheckBox("Use Hash Noise", groupBox);
//     connect(hashNoiseUseBox, &QCheckBox::stateChanged, [this](int state)
//     {
//       emit MainWindow::valueChanged("hash_noise_use", state == Qt::Checked ? 1.0 : 0.0);
//     });
//     horizontalLayout->addWidget(hashNoiseUseBox);

//     QCheckBox* hashNoiseOnBox  = new QCheckBox("Enable Hash Noise", groupBox);
//     connect(hashNoiseOnBox, &QCheckBox::stateChanged, [this](int state)
//     {
//       emit MainWindow::valueChanged("hash_noise_on", state == Qt::Checked ? 1.0 : 0.0);
//     });
//     horizontalLayout->addWidget(hashNoiseOnBox);

//     vertLayout->addLayout(horizontalLayout);

//     SliderSpinboxLabelWidget* cyclesKnobWidget = new SliderSpinboxLabelWidget("Channel Change Cycles", groupBox);
//     connect(cyclesKnobWidget, &SliderSpinboxLabelWidget::valueChanged, [this](double value)
//     {
//       emit MainWindow::valueChanged("channel_change_cycles", value);
//     });
//     vertLayout->addWidget(cyclesKnobWidget);
//   }

//   ~MainWindow()
//   {  }

//   void updateKnobs(const QString& name, double value)
//   {
//     if (name == "tint") {
//       this->knobs.tint = value;
//     } else if (name == "color") {
//       this->knobs.color = value;
//     } else if (name == "brightness") {
//       this->knobs.brightness = value;
//     } else if (name == "contrast") {
//       this->knobs.contrast = value;
//     } else if (name == "width") {
//       this->knobs.width = value;
//     } else if (name == "height") {
//       this->knobs.height = value;
//     } else if (name == "squish") {
//       this->knobs.squish = value;
//     } else if (name == "squeeze_bottom") {
//       this->knobs.squeezeBottom = value;
//     } else if (name == "powerup") {
//       this->knobs.powerup = value;
//     } else if (name == "horizontal_desync") {
//       this->knobs.horizontalDesync = value;
//     } else if (name == "hash_noise_use") {
//       this->knobs.useHashNoise = static_cast<int>(value);
//     } else if (name == "hash_noise_on") {
//       this->knobs.enableHashNoise = static_cast<int>(value);
//     } else if (name == "flutter_horizontal_desync") {
//       this->knobs.useFlutterHorizontalDesync = static_cast<bool>(value);
//     } else if (name == "channel_change_cycles") {
//       this->knobs.channelChangeCycles = static_cast<int>(value);
//     }
//   }

//   void changeChannel(int newChannel)
//   {
//     if (newChannel < 0 || newChannel >= static_cast<int>(this->chanSettings.size()))
//     {
//       QMessageBox::warning(this, "Invalid Channel", "The channel number is out of range.");
//       return;
//     }

//     this->channel = newChannel;
//   }

//   void startPowerOff()
//   {

//   }
// };


class ControlWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit ControlWindow(GuiControl* _control, QWidget* parent = nullptr)
    : QMainWindow(parent), control(_control)
  {
    this->control->onParamChanged = [this]()
    {
      std::cerr << "ControlWindow: onParamChanged called" << std::endl;
      //QMetaObject::invokeMethod(this, [this]() { this->updateGui(); }, Qt::QueuedConnection);
    };

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
      std::lock_guard<std::mutex> lock(this->control->stateMutex);
      this->control->startFadeOut();
    });

    QTintWidget* tintWidget = new QTintWidget(centralWidget);
    tintWidget->setValue(this->control->knobs.tint);
    connect(tintWidget, &QTintWidget::valueChanged, [this](double value)
    {
      this->updateKnobs("tint", value);
    });
    layout->addWidget(tintWidget);
    registerWidget("tint", tintWidget);


    // QDial* dial = new QDial(this);
    // dial->setWrapping(true);
    // dial->setNotchesVisible(false);
    // dial->setRange(0, 360);
    // dial->setEnabled(true);
    // layout->addWidget(dial);
  }

private:
  GuiControl* control;
  std::map<std::string, QTintWidget*> widgets;

  void registerWidget(const std::string& name, QTintWidget* widget)
  {
    widgets[name] = widget;
  }

  void updateKnobs(const QString& name, double value)
  {
    // No mutex here: stateMutex may be held by getNext() which calls onParamChanged,
    // and locking here would cause a deadlock.
    // Writing a double is safe enough; the value will be picked up on the next getNext() call.
    this->control->knobs.setValue(name.toStdString(), value);
  }

  void updateGui()
  {
    std::cerr << "updateGui called, widgets.size()=" << widgets.size() << std::endl;
    for (const auto& pair : widgets)
    {
      const std::string& name = pair.first;
      QTintWidget* widget = pair.second;
      double value = *static_cast<double*>(this->control->knobs.paramInfos.at(name).value);
      std::cerr << "  " << name << " = " << value << std::endl;
      widget->setValue(value);
    }
  }
};


GuiControl::GuiControl(double _fps, bool _randomizeSettings)
{
  this->fps = _fps;
  this->randomizeSettings = _randomizeSettings;
}

const int MAX_MULTICHAN = 2;

// why const ref to sources does not work?
void GuiControl::createChannels(const std::vector<std::shared_ptr<atv::Source>> sources)
{
  size_t nChannels = std::max(sources.size() * 2, 6UL);

  this->chanSettings = { };
  for (size_t i = 0; i < nChannels; i++)
  {
    ChanSetting channelSetting;
    // noise: 0 to 0.2 or 0 to 5.0, default 0.04
    channelSetting.noise_level = 0.06;

    int last_station = 42;
    for (int stati = 0; stati < MAX_MULTICHAN; stati++)
    {
        int stationId;
        while (1)
        {
          stationId = this->rng() % (sources.size());
          // don't do ghost reception with the same station...
          if (stationId != last_station) break;
          // ...at least too often
          if (this->rng() % 10 == 0) break;
        }
        last_station = stationId;
        std::shared_ptr<atv::Source> source = sources[stationId];
        atv::AnalogReception rec;
        if (this->randomizeSettings)
        {
          rec.level = pow(this->rng.uniform(0.0, 1.0), 3.0) * 2.0 + 0.05;
          rec.ofs   = this->rng() % atv::ANALOGTV_SIGNAL_LEN;
          if (this->rng() % 3)
          {
            rec.multipath = this->rng.uniform(0.0, 1.0);
          }
          else
          {
            rec.multipath = 0.0;
          }
          if (stati > 0)
          {
            /* We only set a frequency error for ghosting stations,
              because it doesn't matter otherwise */
            rec.freqerr = this->rng.uniform(-1.0, 1.0) * 3.0;
          }
        }
        else
        {
          rec.level = 0.3;
          rec.ofs = 0;
          rec.multipath = 0.0;
          rec.freqerr = 0;
        }

        channelSetting.receptions.push_back(rec);
        channelSetting.sources.push_back(source);

        if (rec.level > 0.3) break;
        if (this->rng() % 4) break;
    }

    this->chanSettings.push_back(channelSetting);
  }
}


void GuiControl::rotateKnobsStart()
{
  // from analogtv_set_defaults()

  // values taken from analogtv-cli

  // tint: 0 to 360, default 5
  this->knobs.tint = 5;
  // color: 0 to 400, default 70
  // or 0 to +/- 500, need to check it
  this->knobs.color = 70 / 100.0;

  // brightness: -75 to 100, default 1.5 or 3.0
  this->knobs.brightness = 2 / 100.0;
  // contrast: 0 to 500, default 150
  this->knobs.contrast   = 150 / 100.0;
  this->knobs.height = 1.0;
  this->knobs.width  = 1.0;
  this->knobs.squish = 0.0;

  this->knobs.powerup = 1000.0;

  //tv.hashnoise_rpm = 0;
  //TODO: do we need both?
  this->knobs.useHashNoise = 0;
  this->knobs.enableHashNoise = 1;

  this->knobs.horizontalDesync = this->rng.uniform(-5.0, 5.0);
  this->knobs.squeezeBottom = this->rng.uniform(-1.0, 4.0);

  this->knobs.useFlutterHorizontalDesync = false;
  this->knobs.channelChangeCycles = 200000;

  if (this->randomizeSettings)
  {
    if (this->rng() % 4 == 0)
    {
      this->knobs.tint += pow(this->rng.uniform(-1.0, 1.0), 7) * 180.0;
    }
    if (1)
    {
      this->knobs.color += this->rng.uniform(0.0, 0.3) * ((this->rng() & 1) ? 1 : -1);
    }
    if (0) //if (darkp)
    {
      if (this->rng() % 4 == 0)
      {
        this->knobs.brightness += this->rng.uniform(0.0, 0.15);
      }
      if (this->rng() % 4 == 0)
      {
        this->knobs.contrast += this->rng.uniform(0.0, 0.2) * ((this->rng() & 1) ? 1 : -1);
      }
    }
  }
}


void GuiControl::rotateKnobsSwitch()
{
  if (this->randomizeSettings && !(this->rng() % 5))
  {
    if (this->rng() % 4 == 0) 
    {
      this->knobs.tint += pow(this->rng.uniform(-1.0, 1.0), 7) * 180.0 * ((this->rng() & 1) ? 1 : -1);
    }
    if (1)
    {
      this->knobs.color += this->rng.uniform(0.0, 0.3) * ((this->rng() & 1) ? 1 : -1);
    }
    if (0) //(darkp)
    {
      if (this->rng() % 4 == 0)
      {
        this->knobs.brightness += this->rng.uniform(0.0, 0.15);
      }
      if (this->rng() % 4 == 0)
      {
        this->knobs.contrast += this->rng.uniform(0.0, 0.2) * ((this->rng() & 1) ? 1 : -1);
      }
    }
  }
}


struct GuiControl::State
{
  enum class Type
  {
    POWER_UP, // n frames -> SHOW
    SHOW,
    FADE_OUT, // n frames -> QUIT
    SWITCH,   // 0 frames -> SHOW
    QUIT
  };

  virtual Type getType() const = 0;
  virtual Type nextType() const = 0;
  virtual Control::Operation::Type getOperationType() const
  {
    switch (getType())
    {
      case Type::POWER_UP:
        return Control::Operation::Type::NONE;
      case Type::SHOW:
        return Control::Operation::Type::NONE;
      case Type::FADE_OUT:
        return Control::Operation::Type::NONE;
      case Type::SWITCH:
        return Control::Operation::Type::SWITCH;
      case Type::QUIT:
        return Control::Operation::Type::QUIT;
    }
    return Control::Operation::Type::NONE; // Default case, should not happen
  }

  virtual ~State() {}
  State(int _startFrame = 0, int _lastFrame = 0) :
    startFrame(_startFrame), lastFrame(_lastFrame)
  { }

  static std::shared_ptr<State> create(Type type, int startFrame, double fps, double lastBrightness = 0.0, int newChannel = 0);

  int startFrame;
  int lastFrame;
};

const double POWERUP_DURATION = 6.0;  /* Hardcoded in analogtv.c */
struct PowerUpState : public GuiControl::State
{
  Type getType() const override { return Type::POWER_UP; }
  Type nextType() const override { return Type::SHOW; }
  int startFrame;
  PowerUpState(int _startFrame, double fps) :
    State(_startFrame, _startFrame + POWERUP_DURATION * fps)
  { }
};

struct ShowState : public GuiControl::State
{
  Type getType()  const override { return Type::SHOW; }
  Type nextType() const override { return Type::SHOW; }
  ShowState(int _startFrame) :
    State(_startFrame, std::numeric_limits<int>::max())
  { }
};


const double POWERDOWN_DURATION = 1.0;  /* Only used here */
struct FadeOutState : public GuiControl::State
{
  Type getType()  const override { return Type::FADE_OUT; }
  Type nextType() const override { return Type::QUIT; }
  FadeOutState(int _startFrame, double fps, double _lastBrightness) :
    State(_startFrame, _startFrame + POWERDOWN_DURATION * fps),
    lastBrightness(_lastBrightness)
  { }
  double lastBrightness;
};

struct SwitchState : public GuiControl::State
{
  Type getType() const override  { return Type::SWITCH; }
  Type nextType() const override { return Type::SHOW; }
  SwitchState(int _startFrame, int _newChannel) :
    State(_startFrame, _startFrame + 1),
    newChannel(_newChannel)
  { }
  int newChannel;
};

struct QuitState : public GuiControl::State
{
  Type getType() const override  { return Type::QUIT; }
  Type nextType() const override { return Type::QUIT; }
  QuitState(int _startFrame) :
    State(_startFrame, std::numeric_limits<int>::max())
  { }
};

std::shared_ptr<GuiControl::State> GuiControl::State::create(Type type, int startFrame, double fps, double lastBrightness, int newChannel)
{
  switch (type)
  {
    case Type::POWER_UP:
      return std::make_shared<PowerUpState>(startFrame, fps);
    case Type::SHOW:
      return std::make_shared<ShowState>(startFrame);
    case Type::FADE_OUT:
      return std::make_shared<FadeOutState>(startFrame, fps, lastBrightness);
    case Type::SWITCH:
      return std::make_shared<SwitchState>(startFrame, newChannel);
    case Type::QUIT:
      return std::make_shared<QuitState>(startFrame);
  }
  return nullptr;
}


void GuiControl::startFadeOut()
{
  this->currentState = std::make_shared<FadeOutState>(this->frameCounter, this->fps, this->knobs.brightness);
}

void GuiControl::startSwitchChannel(int newChannel)
{
  this->currentState = std::make_shared<SwitchState>(this->frameCounter, newChannel);
}

void GuiControl::run()
{
  this->frameCounter = 0;
  this->channel = this->rng() % this->chanSettings.size();
  this->currentState = std::make_shared<PowerUpState>(0, this->fps);
  this->rotateKnobsStart();

  //TODO: create GUI for channels
  std::thread thread([this]()
  {
    int argc = 0;
    char** argv = nullptr;
    QApplication app(argc, argv);
    ControlWindow window(this);
    window.show();
    app.exec();
  });
  thread.detach();
}


/* Usable range is something like -0.75 to 1.0 */
static const double minBrightness = -1.5;

Control::Operation GuiControl::getNext()
{
  std::lock_guard<std::mutex> lock(this->stateMutex);

  double curTime = this->frameCounter / this->fps;

  switch (currentState->getType())
  {
    case State::Type::POWER_UP:
    {
      this->knobs.powerup = curTime;
    }
    break;

    case State::Type::SWITCH:
    {
      this->channel = std::dynamic_pointer_cast<SwitchState>(currentState)->newChannel;
      this->rotateKnobsSwitch();
      atv::Log::write(2, std::to_string(curTime) + " sec: channel " + std::to_string(this->channel));
    }
    break;

    case State::Type::FADE_OUT:
    {
      /* Fade out, as there is no power-down animation. */
      double rate = (currentState->lastFrame - this->frameCounter) / this->fps / POWERDOWN_DURATION;
      double lastBrightness = std::dynamic_pointer_cast<FadeOutState>(currentState)->lastBrightness;
      this->knobs.brightness = minBrightness * (1.0 - rate) + lastBrightness * rate;
    }
    break;

    case State::Type::QUIT:
      break;

    case State::Type::SHOW:
      break;

    default:
      throw std::runtime_error("Unknown state type in GuiControl::getNext()");
      break;
  }

  if (this->onParamChanged)
  {
    std::cerr << "calling onParamChanged" << std::endl;
    this->onParamChanged();
  }
  else
  {
    std::cerr << "onParamChanged is null" << std::endl;
  }

  Operation op;
  op.type = currentState->getOperationType();
  op.channel = this->channel;

  if (this->frameCounter >= currentState->lastFrame)
  {
    this->currentState = State::create(currentState->nextType(), this->frameCounter, this->fps);
  }

  this->frameCounter++;

  return op;
}

} // ::atv

#include "gui_control.moc"