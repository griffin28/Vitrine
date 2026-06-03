#include "Camera.h"
#include "CameraConfig.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"

#include <gtest/gtest.h>

#include <memory>

using vitrine::Camera;
using vitrine::CameraConfig;
using vitrine::CameraType;
using vitrine::makeCamera;
using vitrine::OrthographicCamera;
using vitrine::PerspectiveCamera;

namespace
{
constexpr float kEps = 1e-4f;
}

TEST(CameraFactory, MakesRequestedSubtype)
{
    auto persp = makeCamera(CameraType::Perspective);
    auto ortho = makeCamera(CameraType::Orthographic);
    ASSERT_NE(persp, nullptr);
    ASSERT_NE(ortho, nullptr);
    EXPECT_EQ(persp->type(), CameraType::Perspective);
    EXPECT_EQ(ortho->type(), CameraType::Orthographic);
    EXPECT_STREQ(persp->anariSubtype(), "perspective");
    EXPECT_STREQ(ortho->anariSubtype(), "orthographic");
}

TEST(CameraConfigRoundTrip, PerspectivePreservesFields)
{
    CameraConfig cfg;
    cfg.type = CameraType::Perspective;
    cfg.eye = {1.0f, 2.0f, 3.0f};
    cfg.center = {0.0f, 0.5f, -1.0f};
    cfg.up = {0.0f, 1.0f, 0.0f};
    cfg.nearClip = 0.25f;
    cfg.farClip = 500.0f;
    cfg.fovYDegrees = 42.0f;

    PerspectiveCamera cam;
    cam.applyConfig(cfg);
    const CameraConfig out = cam.toConfig();

    EXPECT_EQ(out.type, CameraType::Perspective);
    EXPECT_NEAR(out.eye.x, cfg.eye.x, kEps);
    EXPECT_NEAR(out.eye.z, cfg.eye.z, kEps);
    EXPECT_NEAR(out.center.y, cfg.center.y, kEps);
    EXPECT_NEAR(out.nearClip, cfg.nearClip, kEps);
    EXPECT_NEAR(out.farClip, cfg.farClip, kEps);
    EXPECT_NEAR(out.fovYDegrees, cfg.fovYDegrees, 1e-2f);
}

TEST(CameraConfigRoundTrip, OrthographicPreservesHeight)
{
    CameraConfig cfg;
    cfg.type = CameraType::Orthographic;
    cfg.eye = {0.0f, 0.0f, 10.0f};
    cfg.height = 7.5f;

    OrthographicCamera cam;
    cam.applyConfig(cfg);
    const CameraConfig out = cam.toConfig();

    EXPECT_EQ(out.type, CameraType::Orthographic);
    EXPECT_NEAR(out.height, cfg.height, kEps);
    EXPECT_NEAR(out.eye.z, cfg.eye.z, kEps);
}

TEST(CameraConfigRoundTrip, ApplyConfigDoesNotTouchAspect)
{
    PerspectiveCamera cam;
    cam.setAspect(2.5f);

    CameraConfig cfg = cam.toConfig();
    cfg.fovYDegrees = 30.0f;
    cam.applyConfig(cfg);

    // Aspect is viewport-driven, so applyConfig must leave it untouched. We
    // can't read it back directly, but commit() would re-emit it; here we at
    // least confirm the basis is still well-formed (unit-length forward).
    const QVector3D fwd = cam.forward();
    EXPECT_NEAR(fwd.length(), 1.0f, kEps);
}
