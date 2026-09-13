#include "precomp.hpp"

#include "qt_output.hpp"
#include "utils.hpp"

#include <opencv2/imgproc.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

namespace atv
{

QtOutput::QtOutput(cv::Size imgSize)
{
    if (!QApplication::instance())
    {
        static int dummyArgc = 1;
        static char dummyAppName[] = "analogtv";
        static char* dummyArgv[] = { dummyAppName, nullptr };
        static auto fallbackApp = std::make_unique<QApplication>(dummyArgc, dummyArgv);
    }

    window = new QWidget();
    window->setWindowTitle("AnalogTV Output");
    window->resize(imgSize.width, imgSize.height);
    window->setFixedSize(imgSize.width, imgSize.height);

    label = new QLabel(window);
    label->setScaledContents(true);

    QVBoxLayout* layout = new QVBoxLayout(window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);

    window->show();
}

void QtOutput::send(const cv::Mat &m)
{
    if (!window || !label) return;

    cv::Mat bgra;
    if (m.channels() == 3)
    {
        cv::cvtColor(m, bgra, cv::COLOR_BGR2BGRA);
    }
    else
    {
        bgra = m;
    }

    QImage qimg(bgra.data, bgra.cols, bgra.rows, static_cast<int>(bgra.step), QImage::Format_RGB32);
    label->setPixmap(QPixmap::fromImage(qimg));

    QCoreApplication::processEvents();
}

QtOutput::~QtOutput()
{
    if (window)
    {
        window->close();
        delete window;
        window = nullptr;
        label = nullptr;
    }
}

namespace
{
struct QtOutputRegister
{
    QtOutputRegister()
    {
        Output::registerFactory("qt",
            [](const nlohmann::json&, cv::Size size) {
                return std::make_shared<QtOutput>(size);
            },
            [](const ParametricString&, cv::Size size) {
                return std::make_shared<QtOutput>(size);
            });
    }
} qtOutputRegister;
}

} // ::atv
