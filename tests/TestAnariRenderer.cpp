#include "AnariRenderer.h"

#include <anari/anari.h>

#include <QCoreApplication>
#include <QImage>
#include <QObject>
#include <QSignalSpy>
#include <QString>

#include <gtest/gtest.h>

#include <cstdint>

using vitrine::AnariRenderer;

namespace
{

// Provide a QCoreApplication for the entire test binary. QImage and QObject
// signal/slot connections need a running app for queued connections / event
// loops; the renderer also relies on QTimer which is QApplication-bound.
class QAppHolder
{
public:
    QAppHolder()
    {
        static int argc = 1;
        static char appName[] = "unit_tests";
        static char* argv[] = {appName, nullptr};
        if (!QCoreApplication::instance()) {
            static QCoreApplication app(argc, argv);
        }
    }
};

} // namespace

TEST(PhenocrystSmoke, LoadsPhenocrystLibrary)
{
    ANARILibrary lib = anariLoadLibrary("phenocryst", nullptr, nullptr);
    ASSERT_NE(lib, nullptr)
        << "anariLoadLibrary(\"phenocryst\") failed; is ANARI_LIBRARY_PATH set?";
    anariUnloadLibrary(lib);
}

TEST(PhenocrystSmoke, ClearFrameRoundTripsBackgroundColor)
{
    ANARILibrary lib = anariLoadLibrary("phenocryst", nullptr, nullptr);
    ASSERT_NE(lib, nullptr);
    ANARIDevice dev = anariNewDevice(lib, "default");
    ASSERT_NE(dev, nullptr);
    anariCommitParameters(dev, dev);

    ANARIRenderer renderer = anariNewRenderer(dev, "default");
    ASSERT_NE(renderer, nullptr);
    const float bg[4] = {0.2f, 0.4f, 0.8f, 1.0f};
    anariSetParameter(dev, renderer, "background", ANARI_FLOAT32_VEC4, bg);
    anariCommitParameters(dev, renderer);

    ANARICamera camera = anariNewCamera(dev, "perspective");
    anariCommitParameters(dev, camera);

    ANARIWorld world = anariNewWorld(dev);
    anariCommitParameters(dev, world);

    ANARIFrame frame = anariNewFrame(dev);
    const uint32_t size[2] = {32, 32};
    ANARIDataType colorType = ANARI_UFIXED8_VEC4;
    anariSetParameter(dev, frame, "size", ANARI_UINT32_VEC2, size);
    anariSetParameter(dev, frame, "channel.color", ANARI_DATA_TYPE, &colorType);
    anariSetParameter(dev, frame, "renderer", ANARI_RENDERER, &renderer);
    anariSetParameter(dev, frame, "camera", ANARI_CAMERA, &camera);
    anariSetParameter(dev, frame, "world", ANARI_WORLD, &world);
    anariCommitParameters(dev, frame);

    anariRenderFrame(dev, frame);
    EXPECT_EQ(anariFrameReady(dev, frame, ANARI_WAIT), 1);

    uint32_t w = 0;
    uint32_t h = 0;
    ANARIDataType pixelType = ANARI_UNKNOWN;
    const auto* pixels = static_cast<const unsigned char*>(
        anariMapFrame(dev, frame, "channel.color", &w, &h, &pixelType));
    ASSERT_NE(pixels, nullptr);
    EXPECT_EQ(w, 32u);
    EXPECT_EQ(h, 32u);
    EXPECT_EQ(pixelType, ANARI_UFIXED8_VEC4);
    EXPECT_NEAR(pixels[0], 51, 1);
    EXPECT_NEAR(pixels[1], 102, 1);
    EXPECT_NEAR(pixels[2], 204, 1);
    EXPECT_EQ(pixels[3], 255);

    anariUnmapFrame(dev, frame, "channel.color");
    anariRelease(dev, frame);
    anariRelease(dev, world);
    anariRelease(dev, camera);
    anariRelease(dev, renderer);
    anariRelease(dev, dev);
    anariUnloadLibrary(lib);
}

TEST(AnariRendererTest, EmitsFrameReadyOnMainThread)
{
    // Mirrors the production path: renderer on the main thread, driven by
    // its internal QTimer, NO_WAIT polled. QSignalSpy::wait runs a local
    // event loop which is what services the QTimer ticks.
    QAppHolder app;
    qRegisterMetaType<QImage>("QImage");

    AnariRenderer renderer;
    QSignalSpy spy(&renderer, &AnariRenderer::frameReady);
    ASSERT_TRUE(spy.isValid());

    renderer.resize(QSize(16, 16));
    renderer.loadBackend(QStringLiteral("phenocryst"), QStringLiteral("default"));
    renderer.start();

    const bool got = spy.wait(5000);
    renderer.stop();

    EXPECT_TRUE(got) << "AnariRenderer did not emit frameReady within timeout";
    if (got) {
        const QList<QVariant> args = spy.takeFirst();
        ASSERT_EQ(args.size(), 1);
        const QImage img = args[0].value<QImage>();
        EXPECT_EQ(img.width(), 16);
        EXPECT_EQ(img.height(), 16);
    }
}
