#include <gpusims/camera.hpp>

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace gpusims {

namespace {
constexpr float kPitchLimit = 89.0f;  // degrees from horizontal
}

Camera::Camera() = default;

void Camera::setMode(Mode m) {
    mode_ = m;
}

void Camera::setOrientation(float yaw_deg, float pitch_deg) {
    yaw_deg_   = yaw_deg;
    pitch_deg_ = std::clamp(pitch_deg, -kPitchLimit, kPitchLimit);
}

void Camera::resetArcball() {
    orbit_yaw_   = 0.0f;
    orbit_pitch_ = 20.0f;
}

glm::vec3 Camera::forward() const {
    if (mode_ == Mode::FreeFly) {
        const float yaw   = glm::radians(yaw_deg_);
        const float pitch = glm::radians(pitch_deg_);
        return glm::normalize(glm::vec3(
            std::cos(pitch) * std::cos(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::sin(yaw)));
    }
    return glm::normalize(target_ - position_);
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

void Camera::update(float dt, const CameraInputState& input) {
    switch (mode_) {
    case Mode::FreeFly: {
        if (input.mouse_right) {
            yaw_deg_   += input.mouse_dx * look_speed_;
            pitch_deg_ -= input.mouse_dy * look_speed_;
            pitch_deg_  = std::clamp(pitch_deg_, -kPitchLimit, kPitchLimit);
        }
        const float speed = move_speed_ * (input.shift_held ? boost_mul_ : 1.0f);
        glm::vec3 vel(0.0f);
        const glm::vec3 fwd = forward();
        const glm::vec3 rt  = right();
        if (input.key_w) vel += fwd;
        if (input.key_s) vel -= fwd;
        if (input.key_d) vel += rt;
        if (input.key_a) vel -= rt;
        if (input.key_e) vel += glm::vec3(0.0f, 1.0f, 0.0f);
        if (input.key_q) vel -= glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::length(vel) > 0.0f) {
            position_ += glm::normalize(vel) * speed * dt;
        }
        break;
    }
    case Mode::Arcball: {
        if (input.mouse_left) {
            orbit_yaw_   += input.mouse_dx * look_speed_;
            orbit_pitch_ -= input.mouse_dy * look_speed_;
            orbit_pitch_  = std::clamp(orbit_pitch_, -kPitchLimit, kPitchLimit);
        }
        if (input.scroll_dy != 0.0f) {
            orbit_distance_ *= std::pow(1.1f, -input.scroll_dy);
            orbit_distance_  = std::max(orbit_distance_, 0.001f);
        }
        const float yaw   = glm::radians(orbit_yaw_);
        const float pitch = glm::radians(orbit_pitch_);
        position_ = target_ + orbit_distance_ * glm::vec3(
            std::cos(pitch) * std::cos(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::sin(yaw));
        break;
    }
    case Mode::Orbit: {
        orbit_yaw_ += orbit_speed_ * dt;
        if (orbit_yaw_ > 360.0f) orbit_yaw_ -= 360.0f;
        const float yaw   = glm::radians(orbit_yaw_);
        const float pitch = glm::radians(orbit_pitch_);
        position_ = target_ + orbit_distance_ * glm::vec3(
            std::cos(pitch) * std::cos(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::sin(yaw));
        break;
    }
    }
}

glm::mat4 Camera::view() const {
    if (mode_ == Mode::FreeFly) {
        return glm::lookAt(position_, position_ + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    return glm::lookAt(position_, target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projection() const {
    // Vulkan clip-space convention: y is flipped relative to OpenGL.
    glm::mat4 p = glm::perspective(glm::radians(fov_deg_), aspect_, near_, far_);
    p[1][1] *= -1.0f;
    return p;
}

void Camera::drawImGui(const char* label) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),    ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(label)) {
        ImGui::End();
        return;
    }
    static const char* const mode_names[] = {"Free-fly", "Arcball", "Orbit"};
    int mode_int = static_cast<int>(mode_);
    if (ImGui::Combo("Mode", &mode_int, mode_names, IM_ARRAYSIZE(mode_names))) {
        setMode(static_cast<Mode>(mode_int));
    }
    ImGui::SeparatorText("Lens");
    ImGui::SliderFloat("FOV (deg)", &fov_deg_, 10.0f, 120.0f);
    ImGui::DragFloat("Near", &near_, 0.001f, 0.0001f, far_ - 0.001f);
    ImGui::DragFloat("Far",  &far_,  0.1f,   near_ + 0.001f, 100000.0f);
    ImGui::SeparatorText("Position");
    ImGui::DragFloat3("Position", &position_.x, 0.05f);
    if (mode_ == Mode::FreeFly) {
        ImGui::SeparatorText("Free-fly");
        ImGui::SliderFloat("Yaw (deg)",   &yaw_deg_,   -180.0f, 180.0f);
        ImGui::SliderFloat("Pitch (deg)", &pitch_deg_, -kPitchLimit, kPitchLimit);
        ImGui::DragFloat("Move speed", &move_speed_, 0.05f, 0.0f, 1000.0f);
        ImGui::DragFloat("Look speed", &look_speed_, 0.005f, 0.0f, 5.0f);
        ImGui::DragFloat("Boost mul",  &boost_mul_,  0.1f,  1.0f, 100.0f);
    } else {
        ImGui::SeparatorText("Arcball / Orbit");
        ImGui::DragFloat3("Target", &target_.x, 0.05f);
        ImGui::DragFloat("Distance", &orbit_distance_, 0.01f, 0.001f, 10000.0f);
        ImGui::SliderFloat("Yaw (deg)",   &orbit_yaw_,   -180.0f, 180.0f);
        ImGui::SliderFloat("Pitch (deg)", &orbit_pitch_, -kPitchLimit, kPitchLimit);
        if (mode_ == Mode::Orbit) {
            ImGui::DragFloat("Orbit speed (deg/s)", &orbit_speed_, 0.5f, -360.0f, 360.0f);
        }
    }
    ImGui::End();
}

void Camera::toJson(nlohmann::json& j) const {
    j = nlohmann::json{
        {"mode",        static_cast<int>(mode_)},
        {"position",    {position_.x, position_.y, position_.z}},
        {"yaw_deg",     yaw_deg_},
        {"pitch_deg",   pitch_deg_},
        {"target",      {target_.x, target_.y, target_.z}},
        {"orbit", {
            {"distance", orbit_distance_},
            {"yaw",      orbit_yaw_},
            {"pitch",    orbit_pitch_},
            {"speed",    orbit_speed_},
        }},
        {"lens", {
            {"fov_deg", fov_deg_},
            {"aspect",  aspect_},
            {"near",    near_},
            {"far",     far_},
        }},
        {"freefly", {
            {"move_speed", move_speed_},
            {"look_speed", look_speed_},
            {"boost_mul",  boost_mul_},
        }},
    };
}

void Camera::fromJson(const nlohmann::json& j) {
    mode_       = static_cast<Mode>(j.value("mode", static_cast<int>(Mode::FreeFly)));
    auto pos    = j.value("position", std::vector<float>{0.0f, 0.0f, 5.0f});
    if (pos.size() == 3) position_ = glm::vec3(pos[0], pos[1], pos[2]);
    yaw_deg_    = j.value("yaw_deg",   yaw_deg_);
    pitch_deg_  = j.value("pitch_deg", pitch_deg_);
    auto tgt    = j.value("target", std::vector<float>{0.0f, 0.0f, 0.0f});
    if (tgt.size() == 3) target_ = glm::vec3(tgt[0], tgt[1], tgt[2]);
    if (j.contains("orbit")) {
        const auto& o = j["orbit"];
        orbit_distance_ = o.value("distance", orbit_distance_);
        orbit_yaw_      = o.value("yaw",      orbit_yaw_);
        orbit_pitch_    = o.value("pitch",    orbit_pitch_);
        orbit_speed_    = o.value("speed",    orbit_speed_);
    }
    if (j.contains("lens")) {
        const auto& l = j["lens"];
        fov_deg_ = l.value("fov_deg", fov_deg_);
        aspect_  = l.value("aspect",  aspect_);
        near_    = l.value("near",    near_);
        far_     = l.value("far",     far_);
    }
    if (j.contains("freefly")) {
        const auto& f = j["freefly"];
        move_speed_ = f.value("move_speed", move_speed_);
        look_speed_ = f.value("look_speed", look_speed_);
        boost_mul_  = f.value("boost_mul",  boost_mul_);
    }
}

}  // namespace gpusims
