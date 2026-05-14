#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <nlohmann/json_fwd.hpp>

namespace gpusims {

// Per-frame raw input snapshot. Filled by the caller from GLFW (or any other
// input source) before passing to Camera::update.
struct CameraInputState {
    bool key_w        = false;
    bool key_a        = false;
    bool key_s        = false;
    bool key_d        = false;
    bool key_q        = false;  // down (world space)
    bool key_e        = false;  // up (world space)
    bool shift_held   = false;  // boost / fine-control toggle

    bool mouse_left   = false;
    bool mouse_right  = false;
    bool mouse_middle = false;

    // Mouse delta in pixels since last update.
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;

    // Scroll wheel delta accumulated over the frame.
    float scroll_dy = 0.0f;
};

class Camera {
public:
    enum class Mode {
        FreeFly,  // FPS-like; WASD movement + RMB-drag look
        Arcball,  // orbit around target with LMB drag, zoom with scroll
        Orbit,    // animated orbit around target (no input needed)
    };

    Camera();

    // ----------------------------------------------------------------------
    // Mode and per-frame update
    // ----------------------------------------------------------------------
    void setMode(Mode m);
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    Mode mode() const { return mode_; }

    // Advance the camera by `dt` seconds, applying input.
    void update(float dt, const CameraInputState& input);

    // ----------------------------------------------------------------------
    // Output transforms
    // ----------------------------------------------------------------------
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::mat4 view()           const;
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::mat4 projection()     const;
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::mat4 viewProjection() const { return projection() * view(); }

// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::vec3 position() const { return position_; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::vec3 forward()  const;
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::vec3 right()    const;
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    glm::vec3 up()       const;

    // ----------------------------------------------------------------------
    // Lens parameters
    // ----------------------------------------------------------------------
    void setFovDeg(float deg)              { fov_deg_ = deg; }
    void setAspect(float aspect)           { aspect_  = aspect; }
    void setNearFar(float n, float f)      { near_    = n; far_ = f; }
    float fovDeg() const                   { return fov_deg_; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    float aspect() const                   { return aspect_; }

    // ----------------------------------------------------------------------
    // Free-fly tuning
    // ----------------------------------------------------------------------
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setMoveSpeed(float units_per_sec)  { move_speed_ = units_per_sec; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setLookSpeed(float deg_per_pixel)  { look_speed_ = deg_per_pixel; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setBoostMultiplier(float m)        { boost_mul_  = m; }

    // ----------------------------------------------------------------------
    // Arcball / Orbit tuning
    // ----------------------------------------------------------------------
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setTarget(glm::vec3 t)              { target_         = t; }
    void setOrbitDistance(float d)           { orbit_distance_ = d; }
    void setOrbitSpeed(float deg_per_sec)    { orbit_speed_    = deg_per_sec; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void resetArcball();  // reset arcball orientation to look at target

    // ----------------------------------------------------------------------
    // Direct positioning (for spec scripted shots, state restore)
    // ----------------------------------------------------------------------
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setPosition(glm::vec3 p)               { position_ = p; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void setOrientation(float yaw_deg, float pitch_deg);

    // ----------------------------------------------------------------------
    // ImGui inspector. Renders a collapsible window with all parameters.
    // ----------------------------------------------------------------------
    void drawImGui(const char* label = "Camera");

    // ----------------------------------------------------------------------
    // State capture serialization
    // ----------------------------------------------------------------------
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void toJson(nlohmann::json& j) const;
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void fromJson(const nlohmann::json& j);

private:
    Mode mode_ = Mode::FreeFly;

    // Free-fly state
    glm::vec3 position_ = glm::vec3(0.0f, 0.0f, 5.0f);
    float yaw_deg_      = -90.0f;  // looking down -Z
    float pitch_deg_    = 0.0f;

    // Arcball / Orbit state
    glm::vec3 target_         = glm::vec3(0.0f);
    float     orbit_distance_ = 5.0f;
    float     orbit_yaw_      = 0.0f;
    float     orbit_pitch_    = 20.0f;
    float     orbit_speed_    = 30.0f;  // deg/s for Mode::Orbit

    // Lens
    float fov_deg_ = 60.0f;
    float aspect_  = 16.0f / 9.0f;
    float near_    = 0.01f;
    float far_     = 1000.0f;

    // Tuning
    float move_speed_ = 5.0f;    // units/s
    float look_speed_ = 0.2f;    // deg/pixel
    float boost_mul_  = 5.0f;    // shift boost for free-fly
};

}  // namespace gpusims
